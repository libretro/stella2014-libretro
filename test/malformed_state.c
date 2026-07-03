/* Robustness test for retro_unserialize against malformed savestates.
 *
 * A frontend can hand the core a savestate that is truncated (partial
 * write, interrupted transfer) or otherwise inconsistent (version skew,
 * a stale buffer). retro_unserialize must always either load a state or
 * cleanly reject it with a false return -- never abort the process --
 * and the core must remain usable afterwards, with a known-good state
 * still loading correctly.
 *
 * This exercises three families of malformed buffers built from a valid
 * state: every truncation length, buffers of random bytes, and valid
 * states with a handful of bytes corrupted. After each, the core is run
 * a few frames; at the end a known-good state is reloaded to confirm
 * recovery. Exit code is non-zero only on a genuine crash or a failed
 * recovery load.
 */
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
    uint8_t *bad = malloc(sz > 4096 ? sz : 4096);
    srand(999);

    int ok = 0, total = 0;

    /* 1. truncations at many lengths */
    for (size_t n = 0; n < sz; n += (sz/97)+1)
    {
        memcpy(bad, good, n);
        total++;
        p_retro_unserialize(bad, n);        /* return value may be false */
        for (int i = 0; i < 3; i++) p_retro_run();
        ok++;
    }
    printf("truncation: %d/%d ok\n", ok, total);

    /* 2. pure garbage of assorted sizes */
    ok = total = 0;
    for (int t = 0; t < 200; t++)
    {
        size_t n = 1 + (rand() % sz);
        for (size_t i = 0; i < n; i++) bad[i] = rand();
        total++;
        p_retro_unserialize(bad, n);
        for (int i = 0; i < 3; i++) p_retro_run();
        ok++;
    }
    printf("garbage:    %d/%d ok\n", ok, total);

    /* 3. single-bit and byte corruptions of a valid state */
    ok = total = 0;
    for (int t = 0; t < 500; t++)
    {
        memcpy(bad, good, sz);
        int flips = 1 + rand()%8;
        for (int k = 0; k < flips; k++)
            bad[rand()%sz] ^= 1 << (rand()%8);
        total++;
        p_retro_unserialize(bad, sz);
        for (int i = 0; i < 3; i++) p_retro_run();
        ok++;
    }
    printf("bitflips:   %d/%d ok\n", ok, total);

    /* recovery: a good state must still load and run */
    if (!p_retro_unserialize(good, sz)) { printf("recovery load FAILED\n"); return 1; }
    for (int i = 0; i < 30; i++) p_retro_run();
    printf("recovery:   OK\n");

    p_retro_unload_game();
    p_retro_deinit();
    free(good); free(bad);
    return 0;
}
