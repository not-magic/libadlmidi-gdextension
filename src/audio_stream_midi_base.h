#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <adlmidi.h>

namespace godot {

// Shared functionality between AudioStreamMIDI (plays a fixed song) and
// AudioStreamMIDISequencer (a live synth driven with no song loaded): the
// chip/bank/emulator configuration, and the libADLMIDI player setup that
// only depends on it. Registered as an abstract class — not meant to be
// instantiated directly.
class AudioStreamMIDIBase : public AudioStream {
	GDCLASS(AudioStreamMIDIBase, AudioStream)

	PackedByteArray bank_data;
	int embedded_bank = 0;
	int num_chips = 4;
	int four_op_channels = -1;
	int volume_model = ADLMIDI_VolumeModel_AUTO;
	int emulator = ADLMIDI_EMU_NUKED;
	bool full_range_brightness = false;

protected:
	static void _bind_methods();

	// Called after any setter below changes the configuration. No-op here;
	// AudioStreamMIDI overrides it to invalidate its cached metadata player.
	virtual void _on_config_changed() {}

	// Creates and configures a new ADL_MIDIPlayer from the chip/bank/emulator
	// settings above. Does not load any MIDI sequence data — that's up to
	// the derived class. Returns nullptr (and logs an error) if libADLMIDI
	// failed to initialize or load the given bank.
	ADL_MIDIPlayer *create_base_player(long p_sample_rate) const;

public:
	virtual bool _is_monophonic() const override;

	void set_bank_data(const PackedByteArray &p_data);
	PackedByteArray get_bank_data() const { return bank_data; }

	void set_embedded_bank(int p_bank);
	int get_embedded_bank() const { return embedded_bank; }

	void set_num_chips(int p_chips);
	int get_num_chips() const { return num_chips; }

	void set_four_op_channels(int p_channels);
	int get_four_op_channels() const { return four_op_channels; }

	void set_volume_model(int p_model);
	int get_volume_model() const { return volume_model; }

	void set_emulator(int p_emulator);
	int get_emulator() const { return emulator; }

	void set_full_range_brightness(bool p_enabled);
	bool is_full_range_brightness() const { return full_range_brightness; }
};

} // namespace godot
