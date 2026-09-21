#include "audio_stream_midi.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstring>

using namespace godot;

// ============================== AudioStreamMIDI ==============================

AudioStreamMIDI::AudioStreamMIDI() {
}

AudioStreamMIDI::~AudioStreamMIDI() {
	invalidate_info_player();
}

ADL_MIDIPlayer *AudioStreamMIDI::create_player(long p_sample_rate) const {
	ADL_MIDIPlayer *p = adl_init(p_sample_rate);
	if (!p) {
		UtilityFunctions::push_error("AudioStreamMIDI: failed to initialize libADLMIDI: ", String(adl_errorString()));
		return nullptr;
	}

	adl_setNumChips(p, num_chips);
	adl_setVolumeRangeModel(p, volume_model);
	adl_switchEmulator(p, emulator);
	adl_setNumFourOpsChn(p, four_op_channels);
	adl_setFullRangeBrightness(p, full_range_brightness ? 1 : 0);
	adl_setLoopEnabled(p, loop_enabled ? 1 : 0);
	adl_setLoopCount(p, loop_count);

	if (!bank_data.is_empty()) {
		if (adl_openBankData(p, bank_data.ptr(), (unsigned long)bank_data.size()) < 0) {
			UtilityFunctions::push_error("AudioStreamMIDI: failed to load custom bank: ", String(adl_errorInfo(p)));
			adl_close(p);
			return nullptr;
		}
	} else {
		adl_setBank(p, embedded_bank);
	}

	if (!midi_data.is_empty()) {
		if (song_number >= 0) {
			adl_selectSongNum(p, song_number);
		}

		if (adl_openData(p, midi_data.ptr(), (unsigned long)midi_data.size()) < 0) {
			UtilityFunctions::push_error("AudioStreamMIDI: failed to load MIDI data: ", String(adl_errorInfo(p)));
			adl_close(p);
			return nullptr;
		}
	}

	return p;
}

ADL_MIDIPlayer *AudioStreamMIDI::ensure_info_player() const {
	if (!info_player) {
		info_player = create_player(ADL_CHIP_SAMPLE_RATE);
	}
	return info_player;
}

void AudioStreamMIDI::invalidate_info_player() const {
	if (info_player) {
		adl_close(info_player);
		info_player = nullptr;
	}
}

Ref<AudioStreamPlayback> AudioStreamMIDI::_instantiate_playback() const {
	Ref<AudioStreamPlaybackMIDI> playback;
	playback.instantiate();
	playback->stream = Ref<AudioStreamMIDI>(const_cast<AudioStreamMIDI *>(this));
	return playback;
}

String AudioStreamMIDI::_get_stream_name() const {
	return "MIDI";
}

double AudioStreamMIDI::_get_length() const {
	ADL_MIDIPlayer *p = ensure_info_player();
	return p ? adl_totalTimeLength(p) : 0.0;
}

bool AudioStreamMIDI::_is_monophonic() const {
	return false;
}

bool AudioStreamMIDI::_has_loop() const {
	return loop_enabled;
}

void AudioStreamMIDI::set_midi_data(const PackedByteArray &p_data) {
	midi_data = p_data;
	invalidate_info_player();
	emit_changed();
}

void AudioStreamMIDI::set_bank_data(const PackedByteArray &p_data) {
	bank_data = p_data;
	invalidate_info_player();
	emit_changed();
}

void AudioStreamMIDI::set_embedded_bank(int p_bank) {
	embedded_bank = p_bank;
	invalidate_info_player();
}

void AudioStreamMIDI::set_num_chips(int p_chips) {
	num_chips = p_chips;
	invalidate_info_player();
}

void AudioStreamMIDI::set_four_op_channels(int p_channels) {
	four_op_channels = p_channels;
	invalidate_info_player();
}

void AudioStreamMIDI::set_volume_model(int p_model) {
	volume_model = p_model;
	invalidate_info_player();
}

void AudioStreamMIDI::set_emulator(int p_emulator) {
	emulator = p_emulator;
	invalidate_info_player();
}

void AudioStreamMIDI::set_song_number(int p_song) {
	song_number = p_song;
	invalidate_info_player();
}

void AudioStreamMIDI::set_loop_enabled(bool p_enabled) {
	loop_enabled = p_enabled;
	invalidate_info_player();
}

void AudioStreamMIDI::set_loop_count(int p_count) {
	loop_count = p_count;
	invalidate_info_player();
}

void AudioStreamMIDI::set_full_range_brightness(bool p_enabled) {
	full_range_brightness = p_enabled;
	invalidate_info_player();
}

String AudioStreamMIDI::get_title() const {
	ADL_MIDIPlayer *p = ensure_info_player();
	if (!p) {
		return String();
	}
	const char *title = adl_metaMusicTitle(p);
	return title ? String(title) : String();
}

String AudioStreamMIDI::get_copyright() const {
	ADL_MIDIPlayer *p = ensure_info_player();
	if (!p) {
		return String();
	}
	const char *copyright = adl_metaMusicCopyright(p);
	return copyright ? String(copyright) : String();
}

int AudioStreamMIDI::get_track_count() const {
	ADL_MIDIPlayer *p = ensure_info_player();
	return p ? (int)adl_metaTrackTitleCount(p) : 0;
}

String AudioStreamMIDI::get_track_title(int p_index) const {
	ADL_MIDIPlayer *p = ensure_info_player();
	if (!p) {
		return String();
	}
	const char *title = adl_metaTrackTitle(p, (size_t)p_index);
	return title ? String(title) : String();
}

int AudioStreamMIDI::get_songs_count() const {
	ADL_MIDIPlayer *p = ensure_info_player();
	return p ? adl_getSongsCount(p) : 0;
}

double AudioStreamMIDI::get_loop_start_time() const {
	ADL_MIDIPlayer *p = ensure_info_player();
	return p ? adl_loopStartTime(p) : -1.0;
}

double AudioStreamMIDI::get_loop_end_time() const {
	ADL_MIDIPlayer *p = ensure_info_player();
	return p ? adl_loopEndTime(p) : -1.0;
}

int AudioStreamMIDI::get_bank_count() {
	return adl_getBanksCount();
}

String AudioStreamMIDI::get_bank_name(int p_index) {
	const char *const *names = adl_getBankNames();
	int count = adl_getBanksCount();
	if (!names || p_index < 0 || p_index >= count) {
		return String();
	}
	return String(names[p_index]);
}

void AudioStreamMIDI::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_midi_data", "data"), &AudioStreamMIDI::set_midi_data);
	ClassDB::bind_method(D_METHOD("get_midi_data"), &AudioStreamMIDI::get_midi_data);
	ClassDB::bind_method(D_METHOD("set_bank_data", "data"), &AudioStreamMIDI::set_bank_data);
	ClassDB::bind_method(D_METHOD("get_bank_data"), &AudioStreamMIDI::get_bank_data);

	ClassDB::bind_method(D_METHOD("set_embedded_bank", "bank"), &AudioStreamMIDI::set_embedded_bank);
	ClassDB::bind_method(D_METHOD("get_embedded_bank"), &AudioStreamMIDI::get_embedded_bank);
	ClassDB::bind_method(D_METHOD("set_num_chips", "chips"), &AudioStreamMIDI::set_num_chips);
	ClassDB::bind_method(D_METHOD("get_num_chips"), &AudioStreamMIDI::get_num_chips);
	ClassDB::bind_method(D_METHOD("set_four_op_channels", "channels"), &AudioStreamMIDI::set_four_op_channels);
	ClassDB::bind_method(D_METHOD("get_four_op_channels"), &AudioStreamMIDI::get_four_op_channels);
	ClassDB::bind_method(D_METHOD("set_volume_model", "model"), &AudioStreamMIDI::set_volume_model);
	ClassDB::bind_method(D_METHOD("get_volume_model"), &AudioStreamMIDI::get_volume_model);
	ClassDB::bind_method(D_METHOD("set_emulator", "emulator"), &AudioStreamMIDI::set_emulator);
	ClassDB::bind_method(D_METHOD("get_emulator"), &AudioStreamMIDI::get_emulator);
	ClassDB::bind_method(D_METHOD("set_song_number", "song"), &AudioStreamMIDI::set_song_number);
	ClassDB::bind_method(D_METHOD("get_song_number"), &AudioStreamMIDI::get_song_number);
	ClassDB::bind_method(D_METHOD("set_loop_enabled", "enabled"), &AudioStreamMIDI::set_loop_enabled);
	ClassDB::bind_method(D_METHOD("is_loop_enabled"), &AudioStreamMIDI::is_loop_enabled);
	ClassDB::bind_method(D_METHOD("set_loop_count", "count"), &AudioStreamMIDI::set_loop_count);
	ClassDB::bind_method(D_METHOD("get_loop_count"), &AudioStreamMIDI::get_loop_count);
	ClassDB::bind_method(D_METHOD("set_full_range_brightness", "enabled"), &AudioStreamMIDI::set_full_range_brightness);
	ClassDB::bind_method(D_METHOD("is_full_range_brightness"), &AudioStreamMIDI::is_full_range_brightness);

	ClassDB::bind_method(D_METHOD("get_title"), &AudioStreamMIDI::get_title);
	ClassDB::bind_method(D_METHOD("get_copyright"), &AudioStreamMIDI::get_copyright);
	ClassDB::bind_method(D_METHOD("get_track_count"), &AudioStreamMIDI::get_track_count);
	ClassDB::bind_method(D_METHOD("get_track_title", "index"), &AudioStreamMIDI::get_track_title);
	ClassDB::bind_method(D_METHOD("get_songs_count"), &AudioStreamMIDI::get_songs_count);
	ClassDB::bind_method(D_METHOD("get_loop_start_time"), &AudioStreamMIDI::get_loop_start_time);
	ClassDB::bind_method(D_METHOD("get_loop_end_time"), &AudioStreamMIDI::get_loop_end_time);

	ClassDB::bind_static_method("AudioStreamMIDI", D_METHOD("get_bank_count"), &AudioStreamMIDI::get_bank_count);
	ClassDB::bind_static_method("AudioStreamMIDI", D_METHOD("get_bank_name", "index"), &AudioStreamMIDI::get_bank_name);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "midi_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_midi_data", "get_midi_data");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "bank_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_bank_data", "get_bank_data");

	ADD_GROUP("Synthesizer", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "embedded_bank"), "set_embedded_bank", "get_embedded_bank");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "num_chips", PROPERTY_HINT_RANGE, "1,100,1"), "set_num_chips", "get_num_chips");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "four_op_channels", PROPERTY_HINT_RANGE, "-1,128,1"), "set_four_op_channels", "get_four_op_channels");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "volume_model", PROPERTY_HINT_ENUM, "Auto,Generic,Native OPL3,DMX,Apogee,9X,DMX Fixed,Apogee Fixed,AIL,9X Generic FM,HMI,HMI Old,MS AdLib,IMF Creator,O'Connell"), "set_volume_model", "get_volume_model");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emulator", PROPERTY_HINT_ENUM, "Nuked,Nuked Fast,DosBox,Opal,Java,ESFMu,MAME OPL2,YMFM OPL2,YMFM OPL3,Nuked OPL2 LLE,Nuked OPL3 LLE,Nuked OPL2 Lite,Nuked CQM,DosBox OPL2"), "set_emulator", "get_emulator");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "full_range_brightness"), "set_full_range_brightness", "is_full_range_brightness");

	ADD_GROUP("Playback", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "song_number", PROPERTY_HINT_RANGE, "-1,255,1"), "set_song_number", "get_song_number");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop_enabled"), "set_loop_enabled", "is_loop_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "loop_count", PROPERTY_HINT_RANGE, "-1,100,1"), "set_loop_count", "get_loop_count");
}

// =========================== AudioStreamPlaybackMIDI ===========================

AudioStreamPlaybackMIDI::AudioStreamPlaybackMIDI() {
}

AudioStreamPlaybackMIDI::~AudioStreamPlaybackMIDI() {
	if (player) {
		adl_close(player);
	}
}

void AudioStreamPlaybackMIDI::_start(double p_from_pos) {
	if (player) {
		adl_close(player);
		player = nullptr;
	}

	active = false;
	has_sequence = false;

	if (stream.is_valid()) {
		mix_rate = AudioServer::get_singleton()->get_mix_rate();
		player = stream->create_player((long)mix_rate);
		has_sequence = !stream->get_midi_data().is_empty();
	}

	if (player) {
		active = true;
		if (has_sequence && p_from_pos > 0.0) {
			adl_positionSeek(player, p_from_pos);
		}
	}

	begin_resample();
}

void AudioStreamPlaybackMIDI::_stop() {
	if (player) {
		adl_close(player);
		player = nullptr;
	}
	active = false;
}

bool AudioStreamPlaybackMIDI::_is_playing() const {
	return active;
}

double AudioStreamPlaybackMIDI::_get_playback_position() const {
	return (player && has_sequence) ? adl_positionTell(player) : 0.0;
}

void AudioStreamPlaybackMIDI::_seek(double p_position) {
	if (player && has_sequence) {
		adl_positionSeek(player, p_position);
	}
}

int32_t AudioStreamPlaybackMIDI::_mix_resampled(AudioFrame *p_dst_buffer, int32_t p_frame_count) {
	if (!active || !player) {
		std::memset(p_dst_buffer, 0, sizeof(AudioFrame) * (size_t)p_frame_count);
		return p_frame_count;
	}

	ADLMIDI_AudioFormat format;
	format.type = ADLMIDI_SampleType_F32;
	format.containerSize = sizeof(float);
	format.sampleOffset = sizeof(AudioFrame);

	ADL_UInt8 *left = reinterpret_cast<ADL_UInt8 *>(&p_dst_buffer[0].left);
	ADL_UInt8 *right = reinterpret_cast<ADL_UInt8 *>(&p_dst_buffer[0].right);
	int sample_count = p_frame_count * 2;

	int got = has_sequence ? adl_playFormat(player, sample_count, left, right, &format) : adl_generateFormat(player, sample_count, left, right, &format);

	int frames_got = got / 2;
	if (frames_got < p_frame_count) {
		std::memset(p_dst_buffer + frames_got, 0, sizeof(AudioFrame) * (size_t)(p_frame_count - frames_got));
		if (has_sequence) {
			// Reached the end of the song with looping disabled (or loop count exhausted).
			active = false;
		}
	}

	return p_frame_count;
}

float AudioStreamPlaybackMIDI::_get_stream_sampling_rate() const {
	return mix_rate;
}

bool AudioStreamPlaybackMIDI::note_on(int p_channel, int p_note, int p_velocity) {
	return player ? adl_rt_noteOn(player, (ADL_UInt8)p_channel, (ADL_UInt8)p_note, (ADL_UInt8)p_velocity) != 0 : false;
}

void AudioStreamPlaybackMIDI::note_off(int p_channel, int p_note) {
	if (player) {
		adl_rt_noteOff(player, (ADL_UInt8)p_channel, (ADL_UInt8)p_note);
	}
}

void AudioStreamPlaybackMIDI::note_after_touch(int p_channel, int p_note, int p_value) {
	if (player) {
		adl_rt_noteAfterTouch(player, (ADL_UInt8)p_channel, (ADL_UInt8)p_note, (ADL_UInt8)p_value);
	}
}

void AudioStreamPlaybackMIDI::channel_after_touch(int p_channel, int p_value) {
	if (player) {
		adl_rt_channelAfterTouch(player, (ADL_UInt8)p_channel, (ADL_UInt8)p_value);
	}
}

void AudioStreamPlaybackMIDI::controller_change(int p_channel, int p_controller, int p_value) {
	if (player) {
		adl_rt_controllerChange(player, (ADL_UInt8)p_channel, (ADL_UInt8)p_controller, (ADL_UInt8)p_value);
	}
}

void AudioStreamPlaybackMIDI::patch_change(int p_channel, int p_patch) {
	if (player) {
		adl_rt_patchChange(player, (ADL_UInt8)p_channel, (ADL_UInt8)p_patch);
	}
}

void AudioStreamPlaybackMIDI::pitch_bend(int p_channel, int p_value) {
	if (player) {
		adl_rt_pitchBend(player, (ADL_UInt8)p_channel, (ADL_UInt16)p_value);
	}
}

void AudioStreamPlaybackMIDI::panic() {
	if (player) {
		adl_panic(player);
	}
}

void AudioStreamPlaybackMIDI::reset_state() {
	if (player) {
		adl_rt_resetState(player);
	}
}

double AudioStreamPlaybackMIDI::get_song_length() const {
	return (player && has_sequence) ? adl_totalTimeLength(player) : 0.0;
}

bool AudioStreamPlaybackMIDI::is_at_end() const {
	return (player && has_sequence) ? adl_atEnd(player) == 1 : false;
}

void AudioStreamPlaybackMIDI::_bind_methods() {
	ClassDB::bind_method(D_METHOD("note_on", "channel", "note", "velocity"), &AudioStreamPlaybackMIDI::note_on);
	ClassDB::bind_method(D_METHOD("note_off", "channel", "note"), &AudioStreamPlaybackMIDI::note_off);
	ClassDB::bind_method(D_METHOD("note_after_touch", "channel", "note", "value"), &AudioStreamPlaybackMIDI::note_after_touch);
	ClassDB::bind_method(D_METHOD("channel_after_touch", "channel", "value"), &AudioStreamPlaybackMIDI::channel_after_touch);
	ClassDB::bind_method(D_METHOD("controller_change", "channel", "controller", "value"), &AudioStreamPlaybackMIDI::controller_change);
	ClassDB::bind_method(D_METHOD("patch_change", "channel", "patch"), &AudioStreamPlaybackMIDI::patch_change);
	ClassDB::bind_method(D_METHOD("pitch_bend", "channel", "value"), &AudioStreamPlaybackMIDI::pitch_bend);
	ClassDB::bind_method(D_METHOD("panic"), &AudioStreamPlaybackMIDI::panic);
	ClassDB::bind_method(D_METHOD("reset_state"), &AudioStreamPlaybackMIDI::reset_state);
	ClassDB::bind_method(D_METHOD("get_song_length"), &AudioStreamPlaybackMIDI::get_song_length);
	ClassDB::bind_method(D_METHOD("is_at_end"), &AudioStreamPlaybackMIDI::is_at_end);
}
