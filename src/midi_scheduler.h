#pragma once

#include <adlmidi.h>

#include <deque>

// Sample-accurate scheduler for real-time libADLMIDI events: note_on/
// note_off/... calls are queued by target frame, then mix() generates audio
// up to each due message and dispatches it in chronological order.
//
// Deliberately has no dependency on Godot or godot-cpp so it can be unit
// tested directly (see tests/runtime_scheduling_test.cpp).
// AudioStreamPlaybackMIDISequencer (src/audio_stream_midi_sequencer.h) is a thin
// GDExtension wrapper around it, handling player lifecycle and binding.
//
// Does not own the ADL_MIDIPlayer it operates on -- the caller creates and
// destroys it, and must call set_player() again (nullptr, then the new
// pointer) whenever it does.
class MidiScheduler {
public:
	enum class MessageType {
		NOTE_ON,
		NOTE_OFF,
		NOTE_AFTER_TOUCH,
		CHANNEL_AFTER_TOUCH,
		CONTROLLER_CHANGE,
		PATCH_CHANGE,
		PITCH_BEND,
		PANIC,
		RESET_STATE,
	};

	struct NoteOnParams {
		ADL_UInt8 channel;
		ADL_UInt8 note;
		ADL_UInt8 velocity;
	};

	struct NoteOffParams {
		ADL_UInt8 channel;
		ADL_UInt8 note;
	};

	struct NoteAfterTouchParams {
		ADL_UInt8 channel;
		ADL_UInt8 note;
		ADL_UInt8 value;
	};

	struct ChannelAfterTouchParams {
		ADL_UInt8 channel;
		ADL_UInt8 value;
	};

	struct ControllerChangeParams {
		ADL_UInt8 channel;
		ADL_UInt8 controller;
		ADL_UInt8 value;
	};

	struct PatchChangeParams {
		ADL_UInt8 channel;
		ADL_UInt8 patch;
	};

	struct PitchBendParams {
		ADL_UInt8 channel;
		ADL_UInt16 value;
	};

	// PANIC and RESET_STATE carry no parameters; they need no union member.
	union MessageParams {
		NoteOnParams note_on;
		NoteOffParams note_off;
		NoteAfterTouchParams note_after_touch;
		ChannelAfterTouchParams channel_after_touch;
		ControllerChangeParams controller_change;
		PatchChangeParams patch_change;
		PitchBendParams pitch_bend;
	};

	struct QueuedMessage {
		MessageType type;
		int time;
		MessageParams params;
	};

	struct AudioFrame {
		float left;
		float right;
	};

	// Invoked right after a message is dispatched to the player, with the
	// frame it was dispatched at. Purely an observability hook for tests;
	// production code can leave it unset. Never called for a message
	// queue_message() rejected for being scheduled in the past.
	using DispatchCallback = void (*)(void *userdata, const QueuedMessage &message, int frame);

private:
	ADL_MIDIPlayer *player = nullptr;
	int current_frame = 0;
	std::deque<QueuedMessage> message_queue;
	DispatchCallback dispatch_callback = nullptr;
	void *dispatch_callback_userdata = nullptr;

	// Appends a message to the queue, unless p_time is already in the past
	// (before current_frame), in which case it's discarded. Returns whether
	// the message was queued.
	bool queue_message(int p_time, MessageType p_type, const MessageParams &p_params);

	// Applies a due message to the player via the matching adl_rt_* call.
	void dispatch_message(const QueuedMessage &p_message);

public:
	void set_player(ADL_MIDIPlayer *p_player) { player = p_player; }

	void set_dispatch_callback(DispatchCallback p_callback, void *p_userdata = nullptr) {
		dispatch_callback = p_callback;
		dispatch_callback_userdata = p_userdata;
	}

	// Resets current_frame to 0 and drops any pending messages. Call when
	// (re)starting playback.
	void reset();

	int get_current_frame() const { return current_frame; }
	size_t get_queue_size() const { return message_queue.size(); }

	// Sorts the queue by time, then fills p_dst_buffer with p_frame_count
	// frames of interleaved float stereo audio, alternating between
	// generating audio up to the next due message and dispatching every
	// message that's become due, so messages take effect at the right
	// frame within the buffer. Returns false (without touching the buffer)
	// if no player has been set.
	bool mix(AudioFrame *p_dst_buffer, int p_frame_count);

	bool note_on(int p_time, int p_channel, int p_note, int p_velocity);
	bool note_off(int p_time, int p_channel, int p_note);
	bool note_after_touch(int p_time, int p_channel, int p_note, int p_value);
	bool channel_after_touch(int p_time, int p_channel, int p_value);
	bool controller_change(int p_time, int p_channel, int p_controller, int p_value);
	bool patch_change(int p_time, int p_channel, int p_patch);
	bool pitch_bend(int p_time, int p_channel, int p_value);
	bool panic(int p_time);
	bool reset_state(int p_time);
};
