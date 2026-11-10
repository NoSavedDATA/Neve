# Compressed codecs and playback — design for a future phase

This lib (`import audio`) covers **WAV PCM (U8/S16/S24/S32/F32)** and the full
torchaudio-style DSP. This document describes how to add, without changing any
existing code:

1. **Compressed codecs** (MP3, FLAC, OGG, AAC) — the equivalent of the
   torchaudio `sox_io`/`soundfile` backend.
2. **Playback/capture** — the equivalent of `sounddevice`/`torchaudio.play`.

## 1. Compressed codecs — external `audio_codecs` lib (frost pattern)

An installable lib in `$NEVE_LIBS/audio_codecs/` (same mechanism as `frost`,
`networks`, `readers`):

```
audio_codecs/
├── include.nv        # protos + audio_codecs_load/save/info wrappers
├── codecs.cpp        # C++ with miniaudio (single header, MIT)
├── lib.so            # clang++-21 -O3 -fPIC -shared codecs.cpp -o lib.so
└── Makefile          # based on lib/std_lib/Makefile_llvm
```

### Proposed API (Neve)

```neve
import audio_codecs

main
    AudioData a = audio_codecs_load("/neve/song.mp3")   # or .flac/.ogg
    printl(a.sample_rate, " ", a.wf.shape[1])
    audio_codecs_save("/neve/out.flac", a.wf, a.sample_rate)
```

### C++ (sketch)

```cpp
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

extern "C" int nsk_codecs_load(Scope_Struct*, DT_str path, float* out, int* out_info) {
    ma_decoder dec;
    // ma_decoder_init_file + ma_decoder_read_pcm_frames -> float interleaved
    // out_info: [0]=sr [1]=channels [2]=frames
    ...
}
```

`lib.so` is resolved by the existing FFI mechanism: `import audio_codecs`
dlopens `$NEVE_LIBS/audio_codecs/lib.so` and binds the symbols automatically
(`src/compiler_frontend/libs_parser.cpp`). No compiler changes needed.

### Building lib.so

Copy `lib/std_lib/Makefile_llvm` and adjust: `clang++-21 -O3 -fPIC -shared
-mavx src/*.cpp -o lib.so` with rpath `$ORIGIN/../../sys_lib`. Install via
`nsm i <profile>/audio_codecs` or `git clone` into `$NEVE_LIBS/audio_codecs`.

## 2. Playback/capture (ALSA)

`libasound.so.2` is not linked into the `neve` binary, but it can be loaded at
runtime via `dlopen` + `dlsym` — just declare protos for the resolved
symbols. Since the JIT resolves symbols from the process, the path is a small
C wrapper in this same folder (`lib/audio/src/audio.cpp` — the same place as
the WAV/DSP kernels, built into `lib.so` by the lib Makefile):

```cpp
extern "C" int nsk_audio_play_start(Scope_Struct*, float* data, int frames, int channels, int sr);
extern "C" int nsk_audio_play_stop(Scope_Struct*);
```

Implementation: `snd_pcm_open(SND_PCM_STREAM_PLAYBACK)`, a consumer thread with
Neve `finish asyncs`/`asyncs` feeding a ring buffer — do **not** call ALSA's
callback directly from inside the JIT (reentrancy); the Neve thread should
pull samples from a channel.

Suggested design:

```neve
import audio_play
main
    AudioData a = audio_load("/neve/x.wav")
    audio_play(a.wf, a.sample_rate)   # blocks until finished
```

## Why not in this delivery

- WAV covers the full ML audio pipeline (ML datasets are usually WAV:
  SpeechCommands, LibriSpeech, ESC-50).
- Compressed codecs require their own `lib.so` release (build CI), outside
  the "zero changes, new files only" scope of this phase.
- Playback requires threading/GC design decisions that deserve their own
  phase.
