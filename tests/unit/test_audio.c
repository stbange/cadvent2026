/* SPDX-License-Identifier: MIT */
#include <math.h>

#include "audio/audio.h"
#include "unity.h"

static cadv_audio a;
static float buf[2 * 4800];

void setUp(void) { cadv_audio_init(&a); }
void tearDown(void) {}

static float peak(int frames)
{
    float p = 0.0f;
    int i;
    for (i = 0; i < frames * 2; ++i) {
        p = fmaxf(p, fabsf(buf[i]));
    }
    return p;
}

static void test_note_frequencies(void)
{
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 440.0f, cadv_note_hz(69));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 880.0f, cadv_note_hz(81));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 261.63f, cadv_note_hz(60));
}

static void test_envelope_shape(void)
{
    cadv_tone t = {CADV_WAVE_SINE, 440, 0, 1, 0, 0.1f, 0.1f, 0.5f, 0.2f, 0.5f};
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_audio_envelope(&t, -1.0));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.5f, cadv_audio_envelope(&t, 0.05));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.75f, cadv_audio_envelope(&t, 0.15));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.5f, cadv_audio_envelope(&t, 0.3));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 0.25f, cadv_audio_envelope(&t, 0.6));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_audio_envelope(&t, 0.8));
    t.attack = 0.0f;
    t.decay = 0.0f;
    t.release = 0.0f;
    TEST_ASSERT_EQUAL_FLOAT(0.5f, cadv_audio_envelope(&t, 0.0));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, cadv_audio_envelope(&t, 0.5));
}

static void test_every_sfx_makes_sound_and_ends(void)
{
    int s;
    for (s = 0; s < CADV_SFX_COUNT; ++s) {
        cadv_audio_init(&a);
        cadv_audio_sfx(&a, (cadv_sfx)s, 1.0f, (float)(s % 3) - 1.0f);
        cadv_audio_mix(&a, buf, 2400);
        TEST_ASSERT_TRUE_MESSAGE(peak(2400) > 0.001f, "silent sfx");
    }
    for (s = 0; s < 40; ++s) {
        cadv_audio_mix(&a, buf, 4800);
    }
    TEST_ASSERT_EQUAL_INT(0, cadv_audio_active_voices(&a));
}

static void test_voice_stealing(void)
{
    int i;
    cadv_tone t = {CADV_WAVE_SQUARE, 200, 0, 0.1f, 0.0f, 0.0f, 0.0f, 1.0f, 0.1f, 10.0f};
    for (i = 0; i < CADV_AUDIO_VOICES; ++i) {
        cadv_audio_play(&a, &t);
        cadv_audio_mix(&a, buf, 10);
    }
    TEST_ASSERT_EQUAL_INT(CADV_AUDIO_VOICES, cadv_audio_active_voices(&a));
    TEST_ASSERT_EQUAL_INT(0, cadv_audio_play(&a, &t)); /* oldest voice reused */
}

static void test_waveforms_and_clipping(void)
{
    cadv_wave w;
    for (w = CADV_WAVE_SINE; w <= CADV_WAVE_NOISE; ++w) {
        cadv_tone t = {w, 300, 600, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f};
        int i;
        cadv_audio_init(&a);
        a.master = 10.0f;
        for (i = 0; i < 8; ++i) {
            cadv_audio_play(&a, &t);
        }
        cadv_audio_mix(&a, buf, 4800);
        TEST_ASSERT_TRUE(peak(4800) <= 1.0f);
        TEST_ASSERT_TRUE(peak(4800) > 0.1f);
    }
}

static void test_music_plays_chords_and_bells(void)
{
    int i;
    cadv_audio_music(&a, true, 1.0f);
    cadv_audio_mix(&a, buf, 100);
    TEST_ASSERT_TRUE(cadv_audio_active_voices(&a) >= 3);
    for (i = 0; i < 70; ++i) {
        cadv_audio_mix(&a, buf, 4800); /* 70 x 0.1 s */
    }
    TEST_ASSERT_TRUE(a.music.step > 8);
    cadv_audio_music(&a, false, 0.0f);
    TEST_ASSERT_FALSE(a.music.playing);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_note_frequencies);
    RUN_TEST(test_envelope_shape);
    RUN_TEST(test_every_sfx_makes_sound_and_ends);
    RUN_TEST(test_voice_stealing);
    RUN_TEST(test_waveforms_and_clipping);
    RUN_TEST(test_music_plays_chords_and_bells);
    return UNITY_END();
}
