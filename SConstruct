#!/usr/bin/env python
import os

# Pin godot-cpp's bindings to the Godot version this extension was written
# against, unless the caller already asked for a specific api_version or
# custom_api_file on the command line (e.g. `scons custom_api_file=...`).
ARGUMENTS.setdefault("custom_api_file", "extension_api.json")

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

env.Append(CPPPATH=["src/", "libADLMIDI/include/"])
sources = Glob("src/*.cpp")

# libADLMIDI doesn't ship its own SCons build, so its sources are compiled
# directly here instead. This list (and the ENABLE_END_SILENCE_SKIPPING
# define below) mirrors libADLMIDI's default CMake configuration
# (WITH_MIDI_SEQUENCER, WITH_EMBEDDED_BANKS, WITH_XMI_SUPPORT, and every
# emulator backend it enables by default) and was extracted from a working
# `cmake --build` of libADLMIDI/CMakeLists.txt, so it should be kept in sync
# by hand if libADLMIDI adds or removes default source files upstream.
adlmidi_env = env.Clone()
adlmidi_env.Append(CPPDEFINES=["ENABLE_END_SILENCE_SKIPPING"])

# godot-cpp builds with exceptions disabled by default, but libADLMIDI's
# internal containers use them (e.g. pl_list's std::bad_alloc on OOM).
if "-fno-exceptions" in adlmidi_env["CXXFLAGS"]:
    adlmidi_env["CXXFLAGS"].remove("-fno-exceptions")
    adlmidi_env.Append(CXXFLAGS=["-fexceptions"])

# libADLMIDI is a git submodule (a separate upstream repo); build its object
# files into build/libADLMIDI/ instead of alongside its sources so `scons`
# never leaves untracked build artifacts inside the submodule's working tree.
libadlmidi_src_dir = "libADLMIDI/src"
libadlmidi_build_dir = "build/libADLMIDI"
VariantDir(libadlmidi_build_dir, libadlmidi_src_dir, duplicate=0)

adlmidi_relative_sources = [
    "adlmidi.cpp",
    "adlmidi_load.cpp",
    "adlmidi_midiplay.cpp",
    "adlmidi_opl3.cpp",
    "adlmidi_private.cpp",
    "adlmidi_sequencer.cpp",
    "inst_db.cpp",
    "wopl/wopl_file.c",
    "models/model_ail.c",
    "models/model_apogee.c",
    "models/model_dmx.c",
    "models/model_generic.c",
    "models/model_hmi_sos.c",
    "models/model_msadlib.c",
    "models/model_oconnell.c",
    "models/model_win9x.c",
    "chips/dosbox_opl2.cpp",
    "chips/dosbox_opl3.cpp",
    "chips/dosbox/dbopl.cpp",
    "chips/nuked_opl2.cpp",
    "chips/nuked_opl3.cpp",
    "chips/nuked_opl3_fast.cpp",
    "chips/nuked_cqm.cpp",
    "chips/nuked/nukedopl2.c",
    "chips/nuked/nukedopl3.c",
    "chips/nuked_fast/nukedopl3_fast.c",
    "chips/nuked_cqm/cqm.c",
    "chips/opal_opl3.cpp",
    "chips/opal/opal.c",
    "chips/java_opl3.cpp",
    "chips/esfmu_opl3.cpp",
    "chips/esfmu/esfm.c",
    "chips/esfmu/esfm_registers.c",
    "chips/mame_opl2.cpp",
    "chips/mame/mame_fmopl.cpp",
    "chips/ymfm_opl2.cpp",
    "chips/ymfm_opl3.cpp",
    "chips/ymfm/ymfm_adpcm.cpp",
    "chips/ymfm/ymfm_misc.cpp",
    "chips/ymfm/ymfm_opl.cpp",
    "chips/ymfm/ymfm_pcm.cpp",
    "chips/ymfm/ymfm_ssg.cpp",
]

sources += [adlmidi_env.SharedObject(os.path.join(libadlmidi_build_dir, f)) for f in adlmidi_relative_sources]

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libadlmidi.{}.{}.framework/libadlmidi.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/bin/libadlmidi.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/bin/libadlmidi.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "demo/bin/libadlmidi{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
