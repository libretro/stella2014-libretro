/* Feed retro_unserialize hostile input: truncated, garbage, bit-flipped.
 * The core must return false or recover -- never crash or corrupt memory.
 * After each attack, verify the core still runs and a good state loads. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>
#include "libretro.h"

static bool env_cb(unsigned cmd, void *data)
{
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)
        return *(enum retro_pixel_format*)data == RETRO_PIXEL_FORMAT_RGB565;
    return false;
}
static void video_cb(const void *d, unsigned w, unsigned h, size_t p) {}
static size_t audio_batch_cb(const int16_t *d, size_t f) { return f; }
static void audio_cb(int16_t l, int16_t r) {}
static void input_poll_cb(void) {}
static int16_t input_state_cb(unsigned a,unsigned b,unsigned c,unsigned d){return 0;}

static const uint8_t rom_code[] = {
    0x78,0xD8,0xA2,0xFF,0x9A,0xE8,0xA9,0x02,0x85,0x00,0x85,0x02,0x85,0x02,
    0x85,0x02,0xA9,0x00,0x85,0x00,0xA9,0x04,0x85,0x15,0xA9,0x0F,0x85,0x19,
    0x8A,0x85,0x17,0x85,0x09,0xA0,0x00,0x85,0x02,0x88,0xD0,0xFB,0x4C,0x05,0xF0,
};

int main(int argc, char **argv)
{
    static uint8_t rom[4096];
    memset(rom, 0xFF, 4096);
    memcpy(rom, rom_code, sizeof(rom_code));
    rom[0xFFA]=0; rom[0xFFB]=0xF0; rom[0xFFC]=0; rom[0xFFD]=0xF0;
    rom[0xFFE]=0; rom[0xFFF]=0xF0;

    void *so = dlopen(argv[1], RTLD_NOW);
    if (!so) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
#define SYM(n) __typeof__(n) *p_##n = (__typeof__(n)*)dlsym(so, #n)
    SYM(retro_set_environment); SYM(retro_set_video_refresh);
    SYM(retro_set_audio_sample); SYM(retro_set_audio_sample_batch);
    SYM(retro_set_input_poll); SYM(retro_set_input_state);
    SYM(retro_init); SYM(retro_deinit);
    SYM(retro_load_game); SYM(retro_unload_game); SYM(retro_run);
    SYM(retro_serialize_size); SYM(retro_serialize); SYM(retro_unserialize);

    struct retro_game_info gi = { "t.a26", rom, 4096, NULL };
    p_retro_set_environment(env_cb);
    p_retro_set_video_refresh(video_cb);
    p_retro_set_audio_sample(audio_cb);
    p_retro_set_audio_sample_batch(audio_batch_cb);
    p_retro_set_input_poll(input_poll_cb);
    p_retro_set_input_state(input_state_cb);
    p_retro_init();
    if (!p_retro_load_game(&gi)) return 1;
    for (int i = 0; i < 30; i++) p_retro_run();

    size_t sz = p_retro_serialize_size();
    uint8_t *good = malloc(sz);
    p_retro_serialize(good, sz);
    uint8_t *evil = malloc(sz > 4096 ? sz : 4096);
    srand(999);

    int survived = 0, total = 0;

    /* 1. truncations at many lengths */
    for (size_t n = 0; n < sz; n += (sz/97)+1)
    {
        memcpy(evil, good, n);
        total++;
        p_retro_unserialize(evil, n);        /* return value may be false */
        for (int i = 0; i < 3; i++) p_retro_run();
        survived++;
    }
    printf("truncation: %d/%d survived\n", survived, total);

    /* 2. pure garbage of assorted sizes */
    survived = total = 0;
    for (int t = 0; t < 200; t++)
    {
        size_t n = 1 + (rand() % sz);
        for (size_t i = 0; i < n; i++) evil[i] = rand();
        total++;
        p_retro_unserialize(evil, n);
        for (int i = 0; i < 3; i++) p_retro_run();
        survived++;
    }
    printf("garbage:    %d/%d survived\n", survived, total);

    /* 3. single-bit and byte corruptions of a valid state */
    survived = total = 0;
    for (int t = 0; t < 500; t++)
    {
        memcpy(evil, good, sz);
        int flips = 1 + rand()%8;
        for (int k = 0; k < flips; k++)
            evil[rand()%sz] ^= 1 << (rand()%8);
        total++;
        p_retro_unserialize(evil, sz);
        for (int i = 0; i < 3; i++) p_retro_run();
        survived++;
    }
    printf("bitflips:   %d/%d survived\n", survived, total);

    /* recovery: a good state must still load and run */
    if (!p_retro_unserialize(good, sz)) { printf("recovery load FAILED\n"); return 1; }
    for (int i = 0; i < 30; i++) p_retro_run();
    printf("recovery:   OK\n");

    p_retro_unload_game();
    p_retro_deinit();
    free(good); free(evil);
    return 0;
}
