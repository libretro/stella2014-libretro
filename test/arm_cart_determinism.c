/* ARM-cartridge (CDF / BUS) determinism regression test for the
 * stella2014 libretro core.
 *
 * The base determinism_harness exercises a plain 4K ROM. This harness
 * covers the ARM coprocessor mappers instead: it builds synthetic 32K
 * CDF0 and BUS1 images (each with the driver signature the detector
 * looks for and a BX LR at the ARM entry so the driver returns cleanly),
 * loads each through the real libretro path, and verifies:
 *
 *   1. Savestate round-trip: run warm-up frames, serialize, hash frames
 *      of output, unserialize, hash again -- must match. This is what
 *      exercises serialization of the Thumbulator/ARM state, the music
 *      counters and the datastream pointers in RAM.
 *   2. Cross-run reproducibility: the whole procedure twice must produce
 *      identical hashes (the integer music clock + ARM execution must be
 *      deterministic).
 *
 * These mappers cannot be exercised with real commercial ROMs here, but
 * the synthetic images drive detection -> construction -> ARM run ->
 * savestate, which is exactly the machinery a core change is most likely
 * to break.
 *
 * Usage: arm_cart_determinism <path/to/stella2014_libretro.so>
 * Exit code 0 on success, 1 on any mismatch or failure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>
#include "libretro.h"

/* - - - synthetic ARM ROM builders - - - */

static void put_u32le(uint8_t *p, uint32_t v)
{
    p[0] = v & 0xff; p[1] = (v >> 8) & 0xff;
    p[2] = (v >> 16) & 0xff; p[3] = (v >> 24) & 0xff;
}

/* CDF0: three "CDFx" records (subversion 0) in the first 2K so
 * setupVersion picks CDF0; BX LR at the CDF entry 0x808. */
static void build_cdf(uint8_t rom[32768])
{
    int k;
    memset(rom, 0x00, 32768);
    for (k = 0; k < 3; k++)
    {
        rom[0x40 + k*4 + 0] = 0x43; /* C */
        rom[0x40 + k*4 + 1] = 0x44; /* D */
        rom[0x40 + k*4 + 2] = 0x46; /* F */
        rom[0x40 + k*4 + 3] = 0x00; /* subversion 0 -> CDF0 */
    }
    rom[0x808] = 0x70; rom[0x809] = 0x47;   /* BX LR */
    rom[0x7FFC] = 0x00; rom[0x7FFD] = 0xF0;
    rom[0x7FFE] = 0x00; rom[0x7FFF] = 0xF0;
}

/* BUS1: "BUS\0" signature word at 0x7f4 (draconian layout) so
 * setupVersion picks BUS1, plus enough "BUS" occurrences for the
 * detector (>=2); BX LR at the BUS entry 0x808. */
static void build_bus(uint8_t rom[32768])
{
    memset(rom, 0x00, 32768);
    put_u32le(rom + 0x7f4, 0x00535542);     /* 'B','U','S',0 */
    memcpy(rom + 0x100, "BUS", 3);
    memcpy(rom + 0x200, "BUS", 3);
    rom[0x808] = 0x70; rom[0x809] = 0x47;   /* BX LR */
    rom[0x7FFC] = 0x00; rom[0x7FFD] = 0xF0;
    rom[0x7FFE] = 0x00; rom[0x7FFF] = 0xF0;
}

/* - - - libretro plumbing (mirrors determinism_harness) - - - */

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

/* function pointers resolved from the core */
static struct {
    void (*set_environment)(retro_environment_t);
    void (*set_video_refresh)(retro_video_refresh_t);
    void (*set_audio_sample)(retro_audio_sample_t);
    void (*set_audio_sample_batch)(retro_audio_sample_batch_t);
    void (*set_input_poll)(retro_input_poll_t);
    void (*set_input_state)(retro_input_state_t);
    void (*init)(void);
    void (*deinit)(void);
    bool (*load_game)(const struct retro_game_info*);
    void (*unload_game)(void);
    void (*run)(void);
    size_t (*serialize_size)(void);
    bool (*serialize)(void*, size_t);
    bool (*unserialize)(const void*, size_t);
} c;

/* Run the determinism procedure for one 32K ARM image. Returns 0 on
 * success. 'coldhash' receives the warm output hash for cross-run
 * comparison. */
static int run_one(const char *label, uint8_t *rom, uint64_t *coldhash)
{
    struct retro_game_info gi;
    size_t sz;
    uint8_t *st;
    uint64_t h1, h2;
    int i;

    gi.path = "embedded.a26";
    gi.data = rom;
    gi.size = 32768;
    gi.meta = NULL;

    if (!c.load_game(&gi))
    {
        fprintf(stderr, "%s: load failed (not detected as ARM cart?)\n", label);
        return 1;
    }

    for (i = 0; i < 120; i++) c.run();

    sz = c.serialize_size();
    st = (uint8_t*)malloc(sz);
    if (!st || !c.serialize(st, sz))
    { fprintf(stderr, "%s: serialize failed\n", label); free(st); return 1; }

    g_hash = 1469598103934665603ull;
    for (i = 0; i < 60; i++) c.run();
    h1 = g_hash;

    if (!c.unserialize(st, sz))
    { fprintf(stderr, "%s: unserialize failed\n", label); free(st); return 1; }
    free(st);

    g_hash = 1469598103934665603ull;
    for (i = 0; i < 60; i++) c.run();
    h2 = g_hash;

    printf("%s: post-save %016llx / post-load %016llx  %s\n",
           label, (unsigned long long)h1, (unsigned long long)h2,
           h1 == h2 ? "DETERMINISTIC" : "MISMATCH");

    c.unload_game();

    *coldhash = h1;
    return h1 == h2 ? 0 : 1;
}

int main(int argc, char **argv)
{
    static uint8_t cdf[32768], bus[32768];
    void *so;
    uint64_t cold_cdf[2], cold_bus[2];
    int pass, rc = 0;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <core.so>\n", argv[0]);
        return 1;
    }
    so = dlopen(argv[1], RTLD_NOW);
    if (!so) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }

#define SYM(field, name) \
    c.field = (__typeof__(c.field))dlsym(so, name); \
    if (!c.field) { fprintf(stderr, "missing symbol " name "\n"); return 1; }
    SYM(set_environment,        "retro_set_environment");
    SYM(set_video_refresh,      "retro_set_video_refresh");
    SYM(set_audio_sample,       "retro_set_audio_sample");
    SYM(set_audio_sample_batch, "retro_set_audio_sample_batch");
    SYM(set_input_poll,         "retro_set_input_poll");
    SYM(set_input_state,        "retro_set_input_state");
    SYM(init,                   "retro_init");
    SYM(deinit,                 "retro_deinit");
    SYM(load_game,              "retro_load_game");
    SYM(unload_game,            "retro_unload_game");
    SYM(run,                    "retro_run");
    SYM(serialize_size,         "retro_serialize_size");
    SYM(serialize,              "retro_serialize");
    SYM(unserialize,            "retro_unserialize");
#undef SYM

    build_cdf(cdf);
    build_bus(bus);

    c.set_environment(env_cb);
    c.set_video_refresh(video_cb);
    c.set_audio_sample(audio_cb);
    c.set_audio_sample_batch(audio_batch_cb);
    c.set_input_poll(input_poll_cb);
    c.set_input_state(input_state_cb);
    c.init();

    for (pass = 0; pass < 2; pass++)
    {
        if (run_one("CDF", cdf, &cold_cdf[pass])) rc = 1;
        if (run_one("BUS", bus, &cold_bus[pass])) rc = 1;
    }

    printf("cross-run CDF: %016llx vs %016llx  %s\n",
           (unsigned long long)cold_cdf[0], (unsigned long long)cold_cdf[1],
           cold_cdf[0] == cold_cdf[1] ? "REPRODUCIBLE" : "MISMATCH");
    printf("cross-run BUS: %016llx vs %016llx  %s\n",
           (unsigned long long)cold_bus[0], (unsigned long long)cold_bus[1],
           cold_bus[0] == cold_bus[1] ? "REPRODUCIBLE" : "MISMATCH");
    if (cold_cdf[0] != cold_cdf[1] || cold_bus[0] != cold_bus[1]) rc = 1;

    c.deinit();
    dlclose(so);

    if (rc == 0) printf("arm cart determinism: ALL PASS\n");
    return rc;
}
