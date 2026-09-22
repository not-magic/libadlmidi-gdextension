#include "midi_scheduler.h"

#include <algorithm>

void MidiScheduler::reset() {
	current_frame = 0;
	message_queue.clear();
	message_queue_dirty = false;
}

bool MidiScheduler::queue_message(int p_time, MessageType p_type, const MessageParams &p_params) {
	if (p_time < current_frame) {
		return false;
	}

	QueuedMessage message;
	message.type = p_type;
	message.time = p_time;
	message.params = p_params;
	message_queue.push_back(message);
	message_queue_dirty = true;
	return true;
}

void MidiScheduler::dispatch_message(const QueuedMessage &p_message) {
	switch (p_message.type) {
		case MessageType::NOTE_ON: {
			const NoteOnParams &p = p_message.params.note_on;
			adl_rt_noteOn(player, p.channel, p.note, p.velocity);
		} break;
		case MessageType::NOTE_OFF: {
			const NoteOffParams &p = p_message.params.note_off;
			adl_rt_noteOff(player, p.channel, p.note);
		} break;
		case MessageType::NOTE_AFTER_TOUCH: {
			const NoteAfterTouchParams &p = p_message.params.note_after_touch;
			adl_rt_noteAfterTouch(player, p.channel, p.note, p.value);
		} break;
		case MessageType::CHANNEL_AFTER_TOUCH: {
			const ChannelAfterTouchParams &p = p_message.params.channel_after_touch;
			adl_rt_channelAfterTouch(player, p.channel, p.value);
		} break;
		case MessageType::CONTROLLER_CHANGE: {
			const ControllerChangeParams &p = p_message.params.controller_change;
			adl_rt_controllerChange(player, p.channel, p.controller, p.value);
		} break;
		case MessageType::PATCH_CHANGE: {
			const PatchChangeParams &p = p_message.params.patch_change;
			adl_rt_patchChange(player, p.channel, p.patch);
		} break;
		case MessageType::PITCH_BEND: {
			const PitchBendParams &p = p_message.params.pitch_bend;
			adl_rt_pitchBend(player, p.channel, p.value);
		} break;
		case MessageType::PANIC:
			adl_panic(player);
			break;
		case MessageType::RESET_STATE:
			adl_rt_resetState(player);
			break;
	}

	if (dispatch_callback) {
		dispatch_callback(dispatch_callback_userdata, p_message, current_frame);
	}
}

bool MidiScheduler::mix(AudioFrame *p_dst_buffer, int p_frame_count) {
	if (!player) {
		return false;
	}

	if (message_queue_dirty) {
		std::sort(message_queue.begin(), message_queue.end(), [](const QueuedMessage &a, const QueuedMessage &b) {
			return a.time < b.time;
		});
		message_queue_dirty = false;
	}

	ADLMIDI_AudioFormat format;
	format.type = ADLMIDI_SampleType_F32;
	format.containerSize = sizeof(float);
	format.sampleOffset = sizeof(AudioFrame);

	int batch_end_frame = current_frame + p_frame_count;
	int frames_filled = 0;

	auto process_until = [&](int end_frame) {
		const int frames_to_process = end_frame - current_frame;
		if (frames_to_process <= 0) {
			return;
		}

		AudioFrame *segment_dst = p_dst_buffer + frames_filled;
		ADL_UInt8 *left = reinterpret_cast<ADL_UInt8 *>(&segment_dst[0].left);
		ADL_UInt8 *right = reinterpret_cast<ADL_UInt8 *>(&segment_dst[0].right);

		int frames_generated = adl_generateFormat(player, frames_to_process * 2, left, right, &format) / 2;
		frames_filled += frames_generated;
		current_frame += frames_generated;
	};

	while (!message_queue.empty() && message_queue.front().time < batch_end_frame) {
		process_until(message_queue.front().time);
		dispatch_message(message_queue.front());
		message_queue.pop_front();
	}

	process_until(batch_end_frame);

	return true;
}

bool MidiScheduler::note_on(int p_time, int p_channel, int p_note, int p_velocity) {
	MessageParams params{};
	params.note_on = { (ADL_UInt8)p_channel, (ADL_UInt8)p_note, (ADL_UInt8)p_velocity };
	return queue_message(p_time, MessageType::NOTE_ON, params);
}

bool MidiScheduler::note_off(int p_time, int p_channel, int p_note) {
	MessageParams params{};
	params.note_off = { (ADL_UInt8)p_channel, (ADL_UInt8)p_note };
	return queue_message(p_time, MessageType::NOTE_OFF, params);
}

bool MidiScheduler::note_after_touch(int p_time, int p_channel, int p_note, int p_value) {
	MessageParams params{};
	params.note_after_touch = { (ADL_UInt8)p_channel, (ADL_UInt8)p_note, (ADL_UInt8)p_value };
	return queue_message(p_time, MessageType::NOTE_AFTER_TOUCH, params);
}

bool MidiScheduler::channel_after_touch(int p_time, int p_channel, int p_value) {
	MessageParams params{};
	params.channel_after_touch = { (ADL_UInt8)p_channel, (ADL_UInt8)p_value };
	return queue_message(p_time, MessageType::CHANNEL_AFTER_TOUCH, params);
}

bool MidiScheduler::controller_change(int p_time, int p_channel, int p_controller, int p_value) {
	MessageParams params{};
	params.controller_change = { (ADL_UInt8)p_channel, (ADL_UInt8)p_controller, (ADL_UInt8)p_value };
	return queue_message(p_time, MessageType::CONTROLLER_CHANGE, params);
}

bool MidiScheduler::patch_change(int p_time, int p_channel, int p_patch) {
	MessageParams params{};
	params.patch_change = { (ADL_UInt8)p_channel, (ADL_UInt8)p_patch };
	return queue_message(p_time, MessageType::PATCH_CHANGE, params);
}

bool MidiScheduler::pitch_bend(int p_time, int p_channel, int p_value) {
	MessageParams params{};
	params.pitch_bend = { (ADL_UInt8)p_channel, (ADL_UInt16)p_value };
	return queue_message(p_time, MessageType::PITCH_BEND, params);
}

bool MidiScheduler::panic(int p_time) {
	MessageParams params{};
	return queue_message(p_time, MessageType::PANIC, params);
}

bool MidiScheduler::reset_state(int p_time) {
	MessageParams params{};
	return queue_message(p_time, MessageType::RESET_STATE, params);
}
