#pragma once

#include "audio_stream_midi_base.h"

#include <godot_cpp/classes/audio_stream_playback_resampled.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <adlmidi.h>

namespace godot {

class AudioStreamPlaybackMIDI;

// A playable MIDI song rendered through libADLMIDI's OPL3 FM emulation.
// Holds the raw MIDI bytes plus the file-specific playback settings; the
// chip/bank/emulator configuration lives in AudioStreamMIDIBase. Each
// playback instance gets its own ADL_MIDIPlayer so the same resource can be
// played concurrently by multiple AudioStreamPlayers.
class AudioStreamMIDI : public AudioStreamMIDIBase {
	GDCLASS(AudioStreamMIDI, AudioStreamMIDIBase)

	friend class AudioStreamPlaybackMIDI;

	PackedByteArray midi_data;
	int song_number = -1;
	bool loop_enabled = true;
	int loop_count = -1;

	mutable ADL_MIDIPlayer *info_player = nullptr;

	ADL_MIDIPlayer *ensure_info_player() const;
	void invalidate_info_player() const;

protected:
	static void _bind_methods();
	virtual void _on_config_changed() override;

public:
	AudioStreamMIDI();
	~AudioStreamMIDI();

	// Creates and fully configures a new player instance from this resource's
	// settings, opening the custom/embedded bank and the MIDI data (if any).
	// Returns nullptr if libADLMIDI failed to initialize or load the bank.
	ADL_MIDIPlayer *create_player(long sample_rate) const;

	virtual Ref<AudioStreamPlayback> _instantiate_playback() const override;
	virtual String _get_stream_name() const override;
	virtual double _get_length() const override;
	virtual bool _has_loop() const override;

	void set_midi_data(const PackedByteArray &p_data);
	PackedByteArray get_midi_data() const { return midi_data; }

	void set_song_number(int p_song);
	int get_song_number() const { return song_number; }

	void set_loop_enabled(bool p_enabled);
	bool is_loop_enabled() const { return loop_enabled; }

	void set_loop_count(int p_count);
	int get_loop_count() const { return loop_count; }

	String get_title() const;
	String get_copyright() const;
	int get_track_count() const;
	String get_track_title(int p_index) const;
	int get_songs_count() const;
	double get_loop_start_time() const;
	double get_loop_end_time() const;

	static int get_bank_count();
	static String get_bank_name(int p_index);
};

// Per-playback instance: owns its own ADL_MIDIPlayer so several
// AudioStreamPlayers can play the same AudioStreamMIDI independently,
// auto-playing through libADLMIDI's built-in sequencer. For driving the
// synth live instead of playing a fixed song, see AudioStreamMIDISequencer.
class AudioStreamPlaybackMIDI : public AudioStreamPlaybackResampled {
	GDCLASS(AudioStreamPlaybackMIDI, AudioStreamPlaybackResampled)

	friend class AudioStreamMIDI;

	Ref<AudioStreamMIDI> stream;
	ADL_MIDIPlayer *player = nullptr;
	bool active = false;
	float mix_rate = 44100.0f;

protected:
	static void _bind_methods();

public:
	AudioStreamPlaybackMIDI();
	~AudioStreamPlaybackMIDI();

	virtual void _start(double p_from_pos) override;
	virtual void _stop() override;
	virtual bool _is_playing() const override;
	virtual double _get_playback_position() const override;
	virtual void _seek(double p_position) override;
	virtual int32_t _mix_resampled(AudioFrame *p_dst_buffer, int32_t p_frame_count) override;
	virtual float _get_stream_sampling_rate() const override;

	double get_song_length() const;
	bool is_at_end() const;
};

} // namespace godot
