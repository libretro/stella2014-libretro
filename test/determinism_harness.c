/* Determinism regression test for the stella2014 libretro core.
 *
 * Loads the core with dlopen, runs an embedded 4K test ROM (a VSYNC
 * kernel that sweeps AUDF0 and COLUBK every frame so both the audio
 * and video streams vary over time), and verifies:
 *
 *   1. Savestate round-trip determinism: run 120 warm-up frames,
 *      serialize, hash 60 frames of audio+video output, unserialize,
 *      hash 60 frames again -- the hashes must be identical.
 *   2. Lifecycle: retro_load_game called again without an intervening
 *      retro_unload_game must not crash or leak (run under valgrind
 *      to verify the leak half).
 *   3. Cross-run reproducibility: the whole procedure twice from the
 *      same retro_init must produce identical hashes.
 *
 * Usage: determinism_harness <path/to/stella2014_libretro.so> [rom]
 * Exit code 0 on success, 1 on any mismatch or failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>
#include "libretro.h"

/* Embedded 4K test ROM: 6507 kernel at $F000.
 *   SEI/CLD, set stack, then per frame: VSYNC pulse (3 WSYNCs),
 *   AUDC0=4, AUDV0=15, AUDF0=X, COLUBK=X (X increments per frame),
 *   256 WSYNC scanlines, loop. Reset/NMI/IRQ vectors -> $F000. */
static const uint8_t rom_code[] = {
    0x78,             /* SEI            */
    0xD8,             /* CLD            */
    0xA2, 0xFF,       /* LDX #$FF       */
    0x9A,             /* TXS            */
    /* frame loop @ $F005 */
    0xE8,             /* INX            */
    0xA9, 0x02,       /* LDA #$02       */
    0x85, 0x00,       /* STA VSYNC (on) */
    0x85, 0x02,       /* STA WSYNC      */
    0x85, 0x02,       /* STA WSYNC      */
    0x85, 0x02,       /* STA WSYNC      */
    0xA9, 0x00,       /* LDA #$00       */
    0x85, 0x00,       /* STA VSYNC (off)*/
    0xA9, 0x04,       /* LDA #$04       */
    0x85, 0x15,       /* STA AUDC0      */
    0xA9, 0x0F,       /* LDA #$0F       */
    0x85, 0x19,       /* STA AUDV0      */
    0x8A,             /* TXA            */
    0x85, 0x17,       /* STA AUDF0      */
    0x85, 0x09,       /* STA COLUBK     */
    0xA0, 0x00,       /* LDY #$00       */
    /* scanline loop @ $F023 */
    0x85, 0x02,       /* STA WSYNC      */
    0x88,             /* DEY            */
    0xD0, 0xFB,       /* BNE $F023      */
    0x4C, 0x05, 0xF0, /* JMP $F005      */
};

static void build_rom(uint8_t rom[4096])
{
    memset(rom, 0xFF, 4096);
    memcpy(rom, rom_code, sizeof(rom_code));
    rom[0xFFA] = 0x00; rom[0xFFB] = 0xF0;  /* NMI   */
    rom[0xFFC] = 0x00; rom[0xFFD] = 0xF0;  /* RESET */
    rom[0xFFE] = 0x00; rom[0xFFF] = 0xF0;  /* IRQ   */
}

static uint64_t g_hash;
static void hash_bytes(const void *p, size_t n)
{
    const uint8_t *b = (const uint8_t*)p;
    size_t i;
    for (i = 0; i < n; i++)
        g_hash = (g_hash ^ b[i]) * 1099511628211ull;  /* FNV-1a */
}

static bool env_cb(unsigned cmd, void *data)
{
    if (cmd == RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)
        return *(enum retro_pixel_format*)data == RETRO_PIXEL_FORMAT_RGB565;
    return false;
}
static void video_cb(const void *data, unsigned w, unsigned h, size_t pitch)
{
    unsigned y;
    if (!data) return;
    for (y = 0; y < h; y++)
        hash_bytes((const uint8_t*)data + y*pitch, w*2);
}
static size_t audio_batch_cb(const int16_t *data, size_t frames)
{
    hash_bytes(data, frames*2*sizeof(int16_t));
    return frames;
}
static void audio_cb(int16_t l, int16_t r) { (void)l; (void)r; }
static void input_poll_cb(void) {}
static int16_t input_state_cb(unsigned a, unsigned b, unsigned c, unsigned d)
{ (void)a; (void)b; (void)c; (void)d; return 0; }

int main(int argc, char **argv)
{
    static uint8_t rom[4096];
    void *so;
    struct retro_game_info gi;
    uint64_t coldrun[2] = {0, 0};
    int pass;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <core.so> [rom.a26]\n", argv[0]);
        return 1;
    }
    so = dlopen(argv[1], RTLD_NOW);
    if (!so) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }

#define SYM(name) __typeof__(name) *p_##name = (__typeof__(name)*)dlsym(so, #name); \
    if (!p_##name) { fprintf(stderr, "missing symbol " #name "\n"); return 1; }
    SYM(retro_set_environment); SYM(retro_set_video_refresh);
    SYM(retro_set_audio_sample); SYM(retro_set_audio_sample_batch);
    SYM(retro_set_input_poll); SYM(retro_set_input_state);
    SYM(retro_init); SYM(retro_deinit);
    SYM(retro_load_game); SYM(retro_unload_game); SYM(retro_run);
    SYM(retro_serialize_size); SYM(retro_serialize); SYM(retro_unserialize);
#undef SYM

    if (argc >= 3)
    {
        FILE *f = fopen(argv[2], "rb");
        if (!f || fread(rom, 1, 4096, f) != 4096)
        { fprintf(stderr, "bad rom file\n"); return 1; }
        fclose(f);
    }
    else
        build_rom(rom);

    gi.path = "embedded.a26";
    gi.data = rom;
    gi.size = 4096;
    gi.meta = NULL;

    p_retro_set_environment(env_cb);
    p_retro_set_video_refresh(video_cb);
    p_retro_set_audio_sample(audio_cb);
    p_retro_set_audio_sample_batch(audio_batch_cb);
    p_retro_set_input_poll(input_poll_cb);
    p_retro_set_input_state(input_state_cb);
    p_retro_init();

    for (pass = 0; pass < 2; pass++)
    {
        size_t sz;
        uint8_t *st;
        uint64_t h1, h2;
        int i;

        if (!p_retro_load_game(&gi))
        { fprintf(stderr, "load failed\n"); return 1; }

        for (i = 0; i < 120; i++) p_retro_run();

        sz = p_retro_serialize_size();
        st = (uint8_t*)malloc(sz);
        if (!st || !p_retro_serialize(st, sz))
        { fprintf(stderr, "serialize failed\n"); return 1; }

        g_hash = 1469598103934665603ull;
        for (i = 0; i < 60; i++) p_retro_run();
        h1 = g_hash;

        if (!p_retro_unserialize(st, sz))
        { fprintf(stderr, "unserialize failed\n"); return 1; }
        free(st);

        g_hash = 1469598103934665603ull;
        for (i = 0; i < 60; i++) p_retro_run();
        h2 = g_hash;

        printf("pass %d: post-save %016llx / post-load %016llx  %s\n",
               pass, (unsigned long long)h1, (unsigned long long)h2,
               h1 == h2 ? "DETERMINISTIC" : "MISMATCH");
        if (h1 != h2) return 1;
        coldrun[pass] = h1;

        /* lifecycle: repeat load without unload, then unload */
        if (!p_retro_load_game(&gi))
        { fprintf(stderr, "re-load failed\n"); return 1; }
        for (i = 0; i < 10; i++) p_retro_run();
        p_retro_unload_game();
    }

    printf("cross-run: %016llx vs %016llx  %s\n",
           (unsigned long long)coldrun[0], (unsigned long long)coldrun[1],
           coldrun[0] == coldrun[1] ? "REPRODUCIBLE" : "MISMATCH");
    p_retro_deinit();
    dlclose(so);
    return coldrun[0] == coldrun[1] ? 0 : 1;
}
