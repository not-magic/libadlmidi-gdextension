// Regression test for MidiScheduler (src/midi_scheduler.h), the sample-
// accurate message queue/dispatch engine behind AudioStreamPlaybackMIDISequencer.
// MidiScheduler has no Godot dependency, so it's exercised directly here
// against a real libADLMIDI player.
//
// Verifies: the queue gets sorted regardless of insertion order, every mix()
// call is fully filled with real generated audio (no leftover uninitialized
// frames), current_frame tracks exactly with total frames generated (no
// drift), messages dispatch in chronological order at their exact requested
// frame, and the loop never hangs.
#include "midi_scheduler.h"

#include <adlmidi.h>

#include <cstdio>
#include <vector>

struct DispatchRecord {
	int frame;
	int note;
};

static std::vector<DispatchRecord> dispatch_log;

static void on_dispatch(void *userdata, const MidiScheduler::QueuedMessage &message, int frame) {
	int note = -1;
	if (message.type == MidiScheduler::MessageType::NOTE_ON) {
		note = message.params.note_on.note;
	} else if (message.type == MidiScheduler::MessageType::NOTE_OFF) {
		note = message.params.note_off.note;
	}
	dispatch_log.push_back({ frame, note });
}

int main() {
	ADL_MIDIPlayer *player = adl_init(44100);
	if (!player) {
		printf("FAIL: adl_init failed: %s\n", adl_errorString());
		return 1;
	}
	adl_setNumChips(player, 4);
	adl_setBank(player, 0);

	MidiScheduler scheduler;
	scheduler.set_player(player);
	scheduler.set_dispatch_callback(on_dispatch);

	// Deliberately out of order, to exercise the sort.
	scheduler.note_on(50000, 0, 64, 100); // far future
	scheduler.note_on(0, 0, 60, 100); // immediate
	scheduler.note_off(2049, 0, 60); // arbitrary mid-buffer frame
	scheduler.note_on(1024, 0, 62, 100); // exact batch boundary

	const int frame_count = 1024;
	std::vector<MidiScheduler::AudioFrame> buf(frame_count);
	bool ok = true;
	int total_frames = 0;

	// ~3 seconds of audio in typical-sized batches.
	for (int i = 0; i < 130 && ok; i++) {
		for (auto &f : buf) {
			f.left = -999.0f;
			f.right = -999.0f;
		}

		ok = scheduler.mix(buf.data(), frame_count);

		if (ok) {
			for (auto &f : buf) {
				if (f.left == -999.0f && f.right == -999.0f) {
					printf("FAIL: under-filled buffer at call %d\n", i);
					ok = false;
					break;
				}
			}
		}

		total_frames += frame_count;
	}

	adl_close(player);

	printf("ok=%d total_frames=%d current_frame=%d queue_remaining=%zu\n",
			ok, total_frames, scheduler.get_current_frame(), scheduler.get_queue_size());

	if (scheduler.get_current_frame() != total_frames) {
		printf("FAIL: current_frame (%d) drifted from total frames generated (%d)\n", scheduler.get_current_frame(), total_frames);
		ok = false;
	}

	if (scheduler.get_queue_size() != 0) {
		printf("FAIL: %zu message(s) never dispatched\n", scheduler.get_queue_size());
		ok = false;
	}

	bool ordered = true;
	int last_frame = -1;
	printf("dispatch order:\n");
	for (auto &d : dispatch_log) {
		printf("  at frame=%d note=%d\n", d.frame, d.note);
		if (d.frame < last_frame) {
			ordered = false;
		}
		last_frame = d.frame;
	}
	if (!ordered) {
		printf("FAIL: messages dispatched out of chronological order\n");
		ok = false;
	}
	if (dispatch_log.size() != 4) {
		printf("FAIL: expected 4 dispatched messages, got %zu\n", dispatch_log.size());
		ok = false;
	}

	// Every message here lands on an exact frame boundary, so dispatch
	// should happen at exactly the requested frame, never early or late.
	int expected_frames[4] = { 0, 1024, 2049, 50000 };
	for (int i = 0; i < (int)dispatch_log.size() && i < 4; i++) {
		if (dispatch_log[i].frame != expected_frames[i]) {
			printf("FAIL: message %d dispatched at frame %d, expected %d\n", i, dispatch_log[i].frame, expected_frames[i]);
			ok = false;
		}
	}

	printf(ok ? "PASS\n" : "FAIL\n");
	return ok ? 0 : 1;
}
