#include "audio_stream_midi_base.h"

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

bool AudioStreamMIDIBase::_is_monophonic() const {
	return false;
}

ADL_MIDIPlayer *AudioStreamMIDIBase::create_base_player(long p_sample_rate) const {
	ADL_MIDIPlayer *p = adl_init(p_sample_rate);
	if (!p) {
		UtilityFunctions::push_error("libADLMIDI: failed to initialize: ", String(adl_errorString()));
		return nullptr;
	}

	adl_setNumChips(p, num_chips);
	adl_setVolumeRangeModel(p, volume_model);
	adl_switchEmulator(p, emulator);
	adl_setNumFourOpsChn(p, four_op_channels);
	adl_setFullRangeBrightness(p, full_range_brightness ? 1 : 0);

	if (!bank_data.is_empty()) {
		if (adl_openBankData(p, bank_data.ptr(), (unsigned long)bank_data.size()) < 0) {
			UtilityFunctions::push_error("libADLMIDI: failed to load custom bank: ", String(adl_errorInfo(p)));
			adl_close(p);
			return nullptr;
		}
	} else {
		adl_setBank(p, embedded_bank);
	}

	return p;
}

void AudioStreamMIDIBase::set_bank_data(const PackedByteArray &p_data) {
	bank_data = p_data;
	_on_config_changed();
	emit_changed();
}

void AudioStreamMIDIBase::set_embedded_bank(int p_bank) {
	embedded_bank = p_bank;
	_on_config_changed();
}

void AudioStreamMIDIBase::set_num_chips(int p_chips) {
	num_chips = p_chips;
	_on_config_changed();
}

void AudioStreamMIDIBase::set_four_op_channels(int p_channels) {
	four_op_channels = p_channels;
	_on_config_changed();
}

void AudioStreamMIDIBase::set_volume_model(int p_model) {
	volume_model = p_model;
	_on_config_changed();
}

void AudioStreamMIDIBase::set_emulator(int p_emulator) {
	emulator = p_emulator;
	_on_config_changed();
}

void AudioStreamMIDIBase::set_full_range_brightness(bool p_enabled) {
	full_range_brightness = p_enabled;
	_on_config_changed();
}

void AudioStreamMIDIBase::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_bank_data", "data"), &AudioStreamMIDIBase::set_bank_data);
	ClassDB::bind_method(D_METHOD("get_bank_data"), &AudioStreamMIDIBase::get_bank_data);
	ClassDB::bind_method(D_METHOD("set_embedded_bank", "bank"), &AudioStreamMIDIBase::set_embedded_bank);
	ClassDB::bind_method(D_METHOD("get_embedded_bank"), &AudioStreamMIDIBase::get_embedded_bank);
	ClassDB::bind_method(D_METHOD("set_num_chips", "chips"), &AudioStreamMIDIBase::set_num_chips);
	ClassDB::bind_method(D_METHOD("get_num_chips"), &AudioStreamMIDIBase::get_num_chips);
	ClassDB::bind_method(D_METHOD("set_four_op_channels", "channels"), &AudioStreamMIDIBase::set_four_op_channels);
	ClassDB::bind_method(D_METHOD("get_four_op_channels"), &AudioStreamMIDIBase::get_four_op_channels);
	ClassDB::bind_method(D_METHOD("set_volume_model", "model"), &AudioStreamMIDIBase::set_volume_model);
	ClassDB::bind_method(D_METHOD("get_volume_model"), &AudioStreamMIDIBase::get_volume_model);
	ClassDB::bind_method(D_METHOD("set_emulator", "emulator"), &AudioStreamMIDIBase::set_emulator);
	ClassDB::bind_method(D_METHOD("get_emulator"), &AudioStreamMIDIBase::get_emulator);
	ClassDB::bind_method(D_METHOD("set_full_range_brightness", "enabled"), &AudioStreamMIDIBase::set_full_range_brightness);
	ClassDB::bind_method(D_METHOD("is_full_range_brightness"), &AudioStreamMIDIBase::is_full_range_brightness);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "bank_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_bank_data", "get_bank_data");

	ADD_GROUP("Synthesizer", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "embedded_bank"), "set_embedded_bank", "get_embedded_bank");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_chips", PROPERTY_HINT_RANGE, "1,100,1"), "set_num_chips", "get_num_chips");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "four_op_channels", PROPERTY_HINT_RANGE, "-1,128,1"), "set_four_op_channels", "get_four_op_channels");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "volume_model", PROPERTY_HINT_ENUM, "Auto,Generic,Native OPL3,DMX,Apogee,9X,DMX Fixed,Apogee Fixed,AIL,9X Generic FM,HMI,HMI Old,MS AdLib,IMF Creator,O'Connell"), "set_volume_model", "get_volume_model");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emulator", PROPERTY_HINT_ENUM, "Nuked,Nuked Fast,DosBox,Opal,Java,ESFMu,MAME OPL2,YMFM OPL2,YMFM OPL3,Nuked OPL2 LLE,Nuked OPL3 LLE,Nuked OPL2 Lite,Nuked CQM,DosBox OPL2"), "set_emulator", "get_emulator");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "full_range_brightness"), "set_full_range_brightness", "is_full_range_brightness");
}
