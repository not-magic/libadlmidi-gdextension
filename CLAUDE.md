# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

A Godot 4 GDExtension that adds `.mid`/`.midi` playback and real-time MIDI-driven music generation to Godot, rendered as OPL3 FM synthesis audio via **libADLMIDI**. It exposes:

- `AudioStreamMIDI` (`src/audio_stream_midi.h/.cpp`) — a `Resource`/`AudioStream` holding raw MIDI bytes (`midi_data`) plus synth settings (chip count, bank, emulator, volume model, looping, etc). Metadata (title, length, track list, loop points) is queried lazily via a cached "info" `ADL_MIDIPlayer` instance that is invalidated whenever a setting changes.
- `AudioStreamPlaybackMIDI` (same files) — one `ADL_MIDIPlayer` per playback instance, so the same `AudioStreamMIDI` resource can be played concurrently by multiple `AudioStreamPlayer`s. If the stream has `midi_data`, it auto-plays through libADLMIDI's built-in sequencer (`adl_play`/`adl_playFormat`). If `midi_data` is empty, it runs as a live real-time OPL3 synth instead (`adl_generateFormat`), driven entirely through the bound `note_on`/`note_off`/`controller_change`/... methods — this is the "generate MIDI music" path.
- `ResourceFormatLoaderMIDI` (`src/resource_format_loader_midi.h/.cpp`) — recognizes `.mid`/`.midi`/`.rmi`/`.xmi`/`.kar`, reads the file via Godot's `FileAccess`, and wraps the bytes in an `AudioStreamMIDI`.

Audio is generated directly in Godot's native float `AudioFrame` layout: libADLMIDI's `ADLMIDI_AudioFormat` (F32, `containerSize=4`, `sampleOffset=sizeof(AudioFrame)=8`) writes straight into the interleaved `left`/`right` buffer with zero conversion — see `AudioStreamPlaybackMIDI::_mix_resampled`. This mapping (and the `sampleCount = frame_count * 2` convention libADLMIDI uses) was verified against libADLMIDI's own source (`libADLMIDI/src/adlmidi.cpp`), not just its header docs, since the docs are ambiguous about whether "samples" means per-channel or interleaved-total.

Playback classes derive from godot-cpp's **`AudioStreamPlaybackResampled`** (not raw `AudioStreamPlayback`), matching the pattern in `~/Documents/gh/obfuscated-godot/modules/fluidsynth` (a Fluidsynth-based Godot audio module used as the reference implementation for this project) — libADLMIDI is initialized at the engine's actual mix rate, so resampling is a no-op in practice but this still gets `AudioStreamPlayer.pitch_scale` support for free.

### IMPORTANT: libADLMIDI contribution policy

`libADLMIDI/AGENTS.md` states that upstream **does not accept AI-generated code contributions** (license-provenance concerns). This repo only *consumes* libADLMIDI as a submodule/dependency, so writing GDExtension binding code in `src/` is fine — but never author, edit, or "fix" code *inside* the `libADLMIDI/` submodule itself; if a change there is genuinely needed, flag it for the user to handle upstream rather than committing it here.

## Repository layout

- `src/` — the GDExtension C++ source (`register_types.*`, `audio_stream_midi.*`, `resource_format_loader_midi.*`).
- `godot-cpp/` — git submodule (branch `4.5`), the official C++ bindings for Godot's GDExtension API.
- `libADLMIDI/` — git submodule, the MIDI/OPL3 synth library being wrapped. Public C API: `libADLMIDI/include/adlmidi.h` (`extern "C"`, `adl_*`/`ADL_*`).
- `extension_api.json` (repo root) — GDExtension API dump from Godot v4.7.2. The root `SConstruct` defaults `custom_api_file` to this so bindings match the target Godot version (overridable with `scons custom_api_file=...`).
- `demo/` — a minimal Godot project for manually exercising the extension. `demo/bin/adlmidi.gdextension` is the extension descriptor; `demo/bin/*.so|*.dll|*.dylib` are build output (gitignored).
- `build/libADLMIDI/` — libADLMIDI's compiled object files (gitignored). The root `SConstruct` builds them here via `VariantDir` instead of in-place, specifically so the `libADLMIDI/` submodule's working tree never picks up untracked build artifacts.

## Building

SCons, following the [official GDExtension C++ example](https://docs.godotengine.org/en/4.4/tutorials/scripting/gdextension/gdextension_cpp_example.html)'s directory/`SConstruct` layout. libADLMIDI has no SCons build of its own, so the root `SConstruct` compiles its sources directly (see "libADLMIDI source list" below) instead of shelling out to its `CMakeLists.txt`.

```bash
git submodule update --init --recursive   # first time only
scons platform=linux target=template_debug -j$(nproc)
```

Swap `platform=linux` for `windows`/`macos` as needed, and `target=template_debug` for `target=template_release` for a release/export build. Output lands in `demo/bin/libadlmidi<suffix><SHLIBSUFFIX>` (e.g. `demo/bin/libadlmidi.linux.template_debug.x86_64.so`), matching the filenames already referenced in `demo/bin/adlmidi.gdextension`. The first build compiles godot-cpp's full binding set from scratch (~2100 files) plus libADLMIDI with every default OPL3 emulator backend — this takes a while but is a one-time cost; re-running `scons` after touching only `src/` is fast and incremental (SCons correctly no-ops on files it hasn't seen change).

### libADLMIDI source list

`SConstruct` hardcodes the list of libADLMIDI `.c`/`.cpp` files to compile (`adlmidi_relative_sources`) plus one required define (`ENABLE_END_SILENCE_SKIPPING`) and a `-fexceptions` override (godot-cpp disables exceptions by default; libADLMIDI's `pl_list` container throws `std::bad_alloc`). This list was extracted from a real `cmake --build` of `libADLMIDI/CMakeLists.txt` with default options (`WITH_MIDI_SEQUENCER`, `WITH_EMBEDDED_BANKS`, `WITH_XMI_SUPPORT`, and all non-restricted-platform emulator backends ON) — i.e. it mirrors what libADLMIDI itself considers its default, full-featured build. If libADLMIDI is updated upstream and adds/removes/renames source files, this list needs to be updated by hand; there's no automated way to keep it in sync short of re-diffing against libADLMIDI's `CMakeLists.txt`.

There is no in-repo Godot editor binary or automated test suite — verification so far has been: (1) `scons` builds cleanly and links a self-contained `.so` (only depends on libc/libm — libADLMIDI is statically linked in), and (2) a standalone smoke test compiled directly against the object files SCons produced (bypassing the GDExtension layer) confirmed the `ADLMIDI_AudioFormat`/`AudioFrame` interleaving produces correct, bounded, non-NaN float audio for both the file-playback path (`adl_playFormat`) and the real-time note-driven path (`adl_generateFormat` + `adl_rt_noteOn`). Actually loading the extension in a Godot editor/project has not been verified in this environment (no `godot`/`godot4` binary available) — do that before relying on it end-to-end.

## Architecture notes for future work

- godot-cpp virtual method overrides in extension classes are **underscore-prefixed** (`_start`, `_mix_resampled`, `_instantiate_playback`, ...) — this differs from the non-prefixed names used by core-engine C++ modules (like the fluidsynth reference module), so don't copy method names verbatim from a core module; check the relevant `godot-cpp/gen/include/godot_cpp/classes/*.hpp` for the exact GDExtension virtual signature instead.
- `AudioStreamMIDI::create_player()` is the single place that turns the resource's settings into a configured `ADL_MIDIPlayer*`; both the lazy metadata "info player" and each `AudioStreamPlaybackMIDI`'s real playback player go through it, so keep new settings wired through there rather than duplicating setup logic.
- Enum-like settings (`volume_model`, `emulator`) are exposed as plain `int` properties with `PROPERTY_HINT_ENUM` hint strings rather than registered C++ enum types — simpler to wire up in godot-cpp and still gives an Inspector dropdown.
