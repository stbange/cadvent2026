/* SPDX-License-Identifier: MIT */
#include "audio/audio.h"

#include <string.h>

#include "core/mathx.h"

void cadv_audio_init(cadv_audio *a)
{
    memset(a, 0, sizeof *a);
    a->master = 0.8f;
    a->rng = 0x2026u;
}

float cadv_note_hz(int midi_note)
{
    /* 440 * 2^((n-69)/12) without libm: multiply octaves, then semitones */
    float f = 440.0f;
    int d = midi_note - 69;
    static const float semis[12] = {1.0f, 1.0594631f, 1.1224620f, 1.1892071f, 1.2599210f, 1.3348398f,
                                    1.4142135f, 1.4983071f, 1.5874011f, 1.6817929f, 1.7817974f, 1.8877486f};
    while (d >= 12) {
        f *= 2.0f;
        d -= 12;
    }
    while (d < 0) {
        f *= 0.5f;
        d += 12;
    }
    return f * semis[d];
}

float cadv_audio_envelope(const cadv_tone *t, double time)
{
    double rel_start = t->duration;
    double level;
    if (time < 0.0) {
        return 0.0f;
    }
    if (t->attack > 0.0f && time < t->attack) {
        level = time / t->attack;
    } else if (t->decay > 0.0f && time < t->attack + t->decay) {
        level = 1.0 - (1.0 - t->sustain) * ((time - t->attack) / t->decay);
    } else {
        level = t->sustain;
    }
    if (time >= rel_start) {
        double r = t->release > 0.0f ? (time - rel_start) / t->release : 1.0;
        level = r >= 1.0 ? 0.0 : level * (1.0 - r);
    }
    return (float)level;
}

int cadv_audio_play(cadv_audio *a, const cadv_tone *t)
{
    int i;
    int best = 0;
    double oldest = -1.0;
    for (i = 0; i < CADV_AUDIO_VOICES; ++i) {
        if (!a->v[i].active) {
            best = i;
            break;
        }
        if (a->v[i].time > oldest) {
            oldest = a->v[i].time;
            best = i;
        }
    }
    memset(&a->v[best], 0, sizeof a->v[best]);
    a->v[best].t = *t;
    a->v[best].active = true;
    a->rng = a->rng * 1664525u + 1013904223u;
    a->v[best].noise = a->rng | 1u;
    return best;
}

static cadv_tone tone(cadv_wave w, float f0, float f1, float vol, float a, float d, float s,
                      float r, float dur)
{
    cadv_tone t;
    t.wave = w;
    t.freq = f0;
    t.freq_end = f1;
    t.volume = vol;
    t.pan = 0.0f;
    t.attack = a;
    t.decay = d;
    t.sustain = s;
    t.release = r;
    t.duration = dur;
    return t;
}

void cadv_audio_sfx(cadv_audio *a, cadv_sfx s, float volume, float pan)
{
    cadv_tone t;
    switch (s) {
    case CADV_SFX_STEP: t = tone(CADV_WAVE_NOISE, 900, 300, 0.25f, 0.001f, 0.04f, 0.0f, 0.02f, 0.05f); break;
    case CADV_SFX_JUMP: t = tone(CADV_WAVE_SQUARE, 220, 440, 0.2f, 0.005f, 0.1f, 0.3f, 0.05f, 0.12f); break;
    case CADV_SFX_LAND: t = tone(CADV_WAVE_NOISE, 300, 80, 0.35f, 0.001f, 0.08f, 0.0f, 0.03f, 0.08f); break;
    case CADV_SFX_PICKUP: t = tone(CADV_WAVE_TRIANGLE, 660, 990, 0.3f, 0.005f, 0.05f, 0.6f, 0.1f, 0.12f); break;
    case CADV_SFX_STAR: t = tone(CADV_WAVE_TRIANGLE, 880, 1760, 0.35f, 0.01f, 0.2f, 0.5f, 0.6f, 0.4f); break;
    case CADV_SFX_SOCKET: t = tone(CADV_WAVE_SINE, 1320, 1320, 0.4f, 0.002f, 0.3f, 0.3f, 1.2f, 0.3f); break;
    case CADV_SFX_DENIED: t = tone(CADV_WAVE_SQUARE, 160, 110, 0.25f, 0.005f, 0.1f, 0.5f, 0.1f, 0.2f); break;
    case CADV_SFX_PORTAL: t = tone(CADV_WAVE_SAW, 110, 880, 0.25f, 0.2f, 0.5f, 0.6f, 0.6f, 1.2f); break;
    case CADV_SFX_SHOT: t = tone(CADV_WAVE_NOISE, 2000, 200, 0.4f, 0.001f, 0.12f, 0.0f, 0.05f, 0.12f); break;
    case CADV_SFX_HIT: t = tone(CADV_WAVE_SQUARE, 300, 120, 0.3f, 0.001f, 0.08f, 0.0f, 0.04f, 0.08f); break;
    case CADV_SFX_EXPLOSION: t = tone(CADV_WAVE_NOISE, 400, 40, 0.6f, 0.002f, 0.6f, 0.2f, 0.6f, 0.6f); break;
    case CADV_SFX_DOOR: t = tone(CADV_WAVE_SAW, 70, 60, 0.3f, 0.05f, 0.4f, 0.5f, 0.3f, 0.6f); break;
    default: t = tone(CADV_WAVE_SINE, 523.25f, 523.25f, 0.35f, 0.002f, 1.5f, 0.0f, 0.5f, 1.5f); break;
    }
    t.volume *= volume;
    t.pan = pan;
    cadv_audio_play(a, &t);
}

void cadv_audio_music(cadv_audio *a, bool on, float volume)
{
    a->music.playing = on;
    a->music.volume = volume;
    a->music.step = 0;
    a->music.step_time = 0.0;
}

/* Original slow ambient progression for the hall: one chord per 4 s, a
 * sparse bell melody on top. Notes are MIDI numbers, 0 = rest. */
#define MUSIC_STEP 0.5
static const int k_chords[8][3] = {
    {50, 57, 62}, {48, 55, 64}, {46, 53, 62}, {45, 52, 60},
    {50, 57, 65}, {43, 50, 62}, {45, 52, 61}, {50, 57, 62},
};
static const int k_bells[64] = {
    74, 0, 0, 0, 72, 0, 0, 0, 0, 0, 69, 0, 0, 0, 0, 0, 77, 0, 0, 0, 76, 0, 74, 0, 0, 0, 0, 0, 72, 0, 0, 0,
    74, 0, 0, 0, 79, 0, 0, 0, 0, 0, 77, 0, 76, 0, 0, 0, 74, 0, 0, 0, 73, 0, 0, 0, 0, 0, 0, 0, 74, 0, 0, 0,
};

static void music_step(cadv_audio *a)
{
    int step = a->music.step;
    float vol = a->music.volume;
    if (step % 8 == 0) {
        int c = (step / 8) % 8;
        int i;
        for (i = 0; i < 3; ++i) {
            cadv_tone t = tone(CADV_WAVE_TRIANGLE, cadv_note_hz(k_chords[c][i]), 0, 0.08f * vol, 1.2f,
                               1.0f, 0.7f, 1.5f, 3.6f);
            t.pan = (float)(i - 1) * 0.5f;
            cadv_audio_play(a, &t);
        }
    }
    if (k_bells[step % 64]) {
        cadv_tone t = tone(CADV_WAVE_SINE, cadv_note_hz(k_bells[step % 64]), 0, 0.10f * vol, 0.003f,
                           1.8f, 0.0f, 0.3f, 1.8f);
        t.pan = 0.3f;
        cadv_audio_play(a, &t);
    }
    a->music.step = (step + 1) % 64;
}

static float sample(cadv_voice *v, float freq)
{
    double ph = v->phase;
    float s;
    switch (v->t.wave) {
    case CADV_WAVE_SINE: s = cadv_sinf((float)(ph * CADV_TAU)); break;
    case CADV_WAVE_TRIANGLE: s = (float)(ph < 0.5 ? 4.0 * ph - 1.0 : 3.0 - 4.0 * ph); break;
    case CADV_WAVE_SQUARE: s = ph < 0.5 ? 0.6f : -0.6f; break;
    case CADV_WAVE_SAW: s = (float)(2.0 * ph - 1.0) * 0.6f; break;
    default: s = v->noise_val; break;
    }
    v->phase += (double)freq / CADV_AUDIO_RATE;
    if (v->phase >= 1.0) {
        v->phase -= (double)(int)v->phase;
        /* new noise value once per cycle: 'freq' sets the noise colour */
        v->noise ^= v->noise << 13;
        v->noise ^= v->noise >> 17;
        v->noise ^= v->noise << 5;
        v->noise_val = (float)(v->noise & 0xffffu) / 32767.5f - 1.0f;
    }
    return s;
}

void cadv_audio_mix(cadv_audio *a, float *out, int frames)
{
    int f;
    int i;
    const double dt = 1.0 / CADV_AUDIO_RATE;
    memset(out, 0, sizeof(float) * 2u * (size_t)frames);
    for (f = 0; f < frames; ++f) {
        float l = 0.0f;
        float r = 0.0f;
        if (a->music.playing) {
            a->music.step_time += dt;
            if (a->music.step_time >= MUSIC_STEP || (a->music.step == 0 && a->music.step_time <= dt)) {
                if (a->music.step_time >= MUSIC_STEP) {
                    a->music.step_time -= MUSIC_STEP;
                }
                music_step(a);
            }
        }
        for (i = 0; i < CADV_AUDIO_VOICES; ++i) {
            cadv_voice *v = &a->v[i];
            float env;
            float freq;
            float s;
            if (!v->active) {
                continue;
            }
            env = cadv_audio_envelope(&v->t, v->time);
            if (v->time >= v->t.duration + v->t.release) {
                v->active = false;
                continue;
            }
            freq = v->t.freq;
            if (v->t.freq_end > 0.0f && v->t.duration > 0.0f) {
                float k = cadv_clampf((float)(v->time / v->t.duration), 0.0f, 1.0f);
                freq = cadv_lerpf(v->t.freq, v->t.freq_end, k);
            }
            s = sample(v, freq) * env * v->t.volume;
            l += s * (0.5f - 0.5f * v->t.pan) * 2.0f * 0.5f;
            r += s * (0.5f + 0.5f * v->t.pan) * 2.0f * 0.5f;
            v->time += dt;
        }
        out[f * 2] = cadv_clampf(l * a->master, -1.0f, 1.0f);
        out[f * 2 + 1] = cadv_clampf(r * a->master, -1.0f, 1.0f);
    }
}

int cadv_audio_active_voices(const cadv_audio *a)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_AUDIO_VOICES; ++i) {
        n += a->v[i].active;
    }
    return n;
}
