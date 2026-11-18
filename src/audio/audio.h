/* SPDX-License-Identifier: MIT */
#ifndef CADV_AUDIO_AUDIO_H
#define CADV_AUDIO_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

/* Tiny software synthesizer + mixer. Everything audible in the game is
 * generated here (original sounds, CC BY 4.0), so no sample files are needed.
 * The platform layer pulls interleaved stereo float frames from it. */

#define CADV_AUDIO_RATE 48000
#define CADV_AUDIO_VOICES 32

typedef enum cadv_wave {
    CADV_WAVE_SINE = 0,
    CADV_WAVE_TRIANGLE,
    CADV_WAVE_SQUARE,
    CADV_WAVE_SAW,
    CADV_WAVE_NOISE
} cadv_wave;

typedef struct cadv_tone {
    cadv_wave wave;
    float freq;        /* Hz */
    float freq_end;    /* Hz at the end of the note (pitch sweep), 0 = same */
    float volume;      /* 0..1 */
    float pan;         /* -1 left .. 1 right */
    float attack, decay, sustain, release; /* seconds, sustain level 0..1 */
    float duration;    /* seconds before release starts */
} cadv_tone;

typedef struct cadv_voice {
    cadv_tone t;
    bool active;
    double phase;
    uint32_t noise;
    float noise_val;
    double time;       /* seconds since start */
} cadv_voice;

/* Named sound effects used by the hub and portals. */
typedef enum cadv_sfx {
    CADV_SFX_STEP = 0,
    CADV_SFX_JUMP,
    CADV_SFX_LAND,
    CADV_SFX_PICKUP,
    CADV_SFX_STAR,
    CADV_SFX_SOCKET,
    CADV_SFX_DENIED,
    CADV_SFX_PORTAL,
    CADV_SFX_SHOT,
    CADV_SFX_HIT,
    CADV_SFX_EXPLOSION,
    CADV_SFX_DOOR,
    CADV_SFX_BELL,
    CADV_SFX_COUNT
} cadv_sfx;

typedef struct cadv_music {
    bool playing;
    int step;          /* current step in the pattern */
    double step_time;  /* seconds into the current step */
    float volume;
} cadv_music;

typedef struct cadv_audio {
    cadv_voice v[CADV_AUDIO_VOICES];
    float master;
    cadv_music music;
    uint32_t rng;
} cadv_audio;

void cadv_audio_init(cadv_audio *a);
/* Starts a tone on a free (or the oldest) voice; returns the voice index. */
int cadv_audio_play(cadv_audio *a, const cadv_tone *t);
void cadv_audio_sfx(cadv_audio *a, cadv_sfx s, float volume, float pan);
void cadv_audio_music(cadv_audio *a, bool on, float volume);
/* Mixes 'frames' stereo frames into out (interleaved L R), overwriting it. */
void cadv_audio_mix(cadv_audio *a, float *out, int frames);
int cadv_audio_active_voices(const cadv_audio *a);
/* Envelope level of a tone at time t (seconds). Exposed for tests. */
float cadv_audio_envelope(const cadv_tone *t, double time);
float cadv_note_hz(int midi_note);

#endif
