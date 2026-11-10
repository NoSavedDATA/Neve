// include.h — audio lib (Neve FFI pattern, see neve-lang.dev/cpp_docs).
// Declarations mirroring src/audio.cpp for the lib scanning phase.

#pragma once

#include "../../src/nsk_cpp.h"

extern "C" void *nsk_audio_fopen(Scope_Struct *scope_struct, DT_str path, DT_str mode);
extern "C" long nsk_audio_fread(Scope_Struct *scope_struct, DT_str dst, long size, long nmemb, void *f);
extern "C" long nsk_audio_fwrite(Scope_Struct *scope_struct, DT_str src, long size, long nmemb, void *f);
extern "C" int nsk_audio_fseek(Scope_Struct *scope_struct, void *f, long offset, int whence);
extern "C" long nsk_audio_ftell(Scope_Struct *scope_struct, void *f);
extern "C" int nsk_audio_fclose(Scope_Struct *scope_struct, void *f);

extern "C" float nsk_audio_sin(Scope_Struct *scope_struct, float x);
extern "C" float nsk_audio_cos(Scope_Struct *scope_struct, float x);
extern "C" float nsk_audio_exp(Scope_Struct *scope_struct, float x);
extern "C" float nsk_audio_log(Scope_Struct *scope_struct, float x);
extern "C" float nsk_audio_log10(Scope_Struct *scope_struct, float x);
extern "C" float nsk_audio_fabs(Scope_Struct *scope_struct, float x);
extern "C" float nsk_audio_atan(Scope_Struct *scope_struct, float x);
extern "C" int nsk_audio_f2i(Scope_Struct *scope_struct, float x);
extern "C" int nsk_audio_tic(Scope_Struct *scope_struct);
extern "C" float nsk_audio_toc(Scope_Struct *scope_struct);
extern "C" bool nsk_audio_isnull(Scope_Struct *scope_struct, void *p);
extern "C" void nsk_audio_free(Scope_Struct *scope_struct, void *p);
extern "C" int nsk_audio_zero(Scope_Struct *scope_struct, float *data, int n);

extern "C" int nsk_audio_wav_info_c(Scope_Struct *scope_struct, DT_str path, int *out);
extern "C" int nsk_audio_read_decode(Scope_Struct *scope_struct, DT_str path, float *out,
                                     int C, int T, long data_offset, int format, int bits);
extern "C" int nsk_audio_save_wav(Scope_Struct *scope_struct, DT_str path, float *data,
                                  int C, int T, int sample_rate, DT_str fmt);

extern "C" int nsk_audio_fft(Scope_Struct *scope_struct, float *re, float *im, int n, int inverse);
extern "C" int nsk_audio_hann(Scope_Struct *scope_struct, float *win, int n);
extern "C" int nsk_audio_stft_bins(Scope_Struct *scope_struct, int n_fft);
extern "C" int nsk_audio_stft_frames(Scope_Struct *scope_struct, int T, int hop);
extern "C" int nsk_audio_stft(Scope_Struct *scope_struct, float *wf, int T,
                              int n_fft, int hop, int win,
                              float *re, float *im, int bins, int frames);
extern "C" int nsk_audio_spectrogram(Scope_Struct *scope_struct, float *wf, int T,
                                     int n_fft, int hop, int win, float power,
                                     float *out, int bins, int frames);
extern "C" int nsk_audio_amplitude_to_db(Scope_Struct *scope_struct, float *in, float *out, int n);
extern "C" int nsk_audio_db_to_amplitude(Scope_Struct *scope_struct, float *in, float *out, int n);

extern "C" int nsk_audio_resample_len(Scope_Struct *scope_struct, int T, int orig_sr, int new_sr);
extern "C" int nsk_audio_resample_linear(Scope_Struct *scope_struct, float *in, int C, int T,
                                         int orig_sr, int new_sr, float *out, int T_out);
extern "C" int nsk_audio_resample_sinc(Scope_Struct *scope_struct, float *in, int C, int T,
                                       int orig_sr, int new_sr, float *out, int T_out);
extern "C" int nsk_audio_melspectrogram(Scope_Struct *scope_struct, float *wf, int T, int sr,
                                        int n_fft, int hop, int n_mels,
                                        float fmin, float fmax,
                                        float *out, int bins, int frames);
extern "C" int nsk_audio_mfcc(Scope_Struct *scope_struct, float *wf, int T, int sr,
                              int n_mfcc, int n_fft, int hop, int n_mels,
                              float *out, int frames);
extern "C" int nsk_audio_mulaw_encode(Scope_Struct *scope_struct, float *in, float *out, int n);
extern "C" int nsk_audio_mulaw_decode(Scope_Struct *scope_struct, float *in, float *out, int n);
extern "C" int nsk_audio_fade(Scope_Struct *scope_struct, float *data, int C, int T,
                              int fade_in, int fade_out);
extern "C" int nsk_audio_vol(Scope_Struct *scope_struct, float *data, int n, float gain, int gain_type);
extern "C" int nsk_audio_time_mask(Scope_Struct *scope_struct, float *spec, int bins, int frames,
                                   int width, int t0);
extern "C" int nsk_audio_freq_mask(Scope_Struct *scope_struct, float *spec, int bins, int frames,
                                   int width, int f0);
extern "C" int nsk_audio_time_stretch(Scope_Struct *scope_struct, float *wf, int T,
                                      float rate, float *out, int T_out);
