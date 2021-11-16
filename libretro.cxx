#ifndef _MSC_VER
#include <sched.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#include <boolean.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <libretro.h>
#include <streams/file_stream.h>
#include "libretro_core_options.h"

#include "Console.hxx"
#include "Cart.hxx"
#include "Props.hxx"
#include "MD5.hxx"
#include "Sound.hxx"
#include "SerialPort.hxx"
#include "TIA.hxx"
#include "Switches.hxx"
#include "StateManager.hxx"
#include "PropsSet.hxx"
#include "Paddles.hxx"
#include "Sound.hxx"
#include "M6532.hxx"
#include "Version.hxx"

#include "Stubs.hxx"

#ifdef _3DS
extern "C" void* linearMemAlign(size_t size, size_t alignment);
extern "C" void linearFree(void* mem);
#endif

static Console *console = 0;
static Cartridge *cartridge = 0;
static Settings *settings = 0;
static OSystem osystem;
static StateManager stateManager(&osystem);

static int videoWidth, videoHeight;

#define FRAME_BUFFER_SIZE (256 * 160 * 4)
static uint8_t *frameBuffer = NULL;
static uint8_t *frameBufferPrev = NULL;
static uint8_t framePixelBytes = 2;
static const uint32_t *currentPalette32 = NULL;
static uint16_t currentPalette16[256] = {0};

#define MAX_RETROPAD_DEVICES 2

#define RETROPAD_STELLA_GAMEPAD RETRO_DEVICE_JOYPAD
#define RETROPAD_STELLA_PADDLES RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 1)

static unsigned retropad_devices[MAX_RETROPAD_DEVICES] = {
   RETROPAD_STELLA_GAMEPAD,
   RETROPAD_STELLA_GAMEPAD,
};

static const struct retro_controller_description retropad_desc[] = {
   { "Gamepad",                RETROPAD_STELLA_GAMEPAD },
   { "Paddles (Stelladaptor)", RETROPAD_STELLA_PADDLES },
   { NULL, 0 },
};

static const struct retro_controller_info retropad_port_info[] = {
   { retropad_desc, 2 },
   { retropad_desc, 2 },
   { NULL, 0 },
};

static struct retro_input_descriptor retropad_inputs_gamepad0_gamepad1[] = {
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Fire" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Left Difficulty A" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Left Difficulty B" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Color" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Right Difficulty A" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Right Difficulty B" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Black/White" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Reset" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Paddle Fire" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Paddle Analog" },

   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Fire" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Paddle Fire" },
   { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Paddle Analog" },

   { 0 },
};

static struct retro_input_descriptor retropad_inputs_gamepad0_paddles1[] = {
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Fire" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Left Difficulty A" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Left Difficulty B" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Color" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Right Difficulty A" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Right Difficulty B" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Black/White" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Reset" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Paddle Fire" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Paddle Analog" },

   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "P3 Fire" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "P4 Fire" },
   { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "P3 Wheel" },
   { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P4 Wheel" },

   { 0 },
};

static struct retro_input_descriptor retropad_inputs_paddles0_gamepad1[] = {
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "P1 Fire" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "P2 Fire" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Left Difficulty A" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Left Difficulty B" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Color" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Right Difficulty A" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Right Difficulty B" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Black/White" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Reset" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "P1 Wheel" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P2 Wheel" },

   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Fire" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Paddle Fire" },
   { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Paddle Analog" },

   { 0 },
};

static struct retro_input_descriptor retropad_inputs_paddles0_paddles1[] = {
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "P1 Fire" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "P2 Fire" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Left Difficulty A" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Left Difficulty B" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "Color" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Right Difficulty A" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Right Difficulty B" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "Black/White" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Reset" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "P1 Wheel" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P2 Wheel" },

   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "P3 Fire" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "P4 Fire" },
   { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "P3 Wheel" },
   { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "P4 Wheel" },

   { 0 },
};

/* Regular gamepad-related parameters */
static Controller::Type left_controller_type = Controller::Joystick;
static int paddle_digital_sensitivity = 50;

#define PADDLE_ANALOG_RANGE 0x8000
static float paddle_analog_sensitivity = 50.0f;
static bool paddle_analog_is_quadratic = false;
static int paddle_analog_deadzone = (int)(0.15f * (float)PADDLE_ANALOG_RANGE);

static Event::Type MouseAxisValue0   = Event::MouseAxisXValue;
static Event::Type MouseButtonValue0 = Event::MouseButtonLeftValue;
static Event::Type MouseAxisValue1   = Event::MouseAxisYValue;
static Event::Type MouseButtonValue1 = Event::MouseButtonRightValue;

/* Stelladaptor-related parameters
 * > This type of paddle control bears no resemblance
 *   to regular gamepads, thus independent sensitivity
 *   and offset options are required */
#define STELLADAPTOR_ANALOG_SENSE_DEFAULT  20
#define STELLADAPTOR_ANALOG_SENSE_FACTOR   0.148643628f
#define STELLADAPTOR_ANALOG_SENSE_BASE     1.1f
#define STELLADAPTOR_ANALOG_SENSE_MIN      0
#define STELLADAPTOR_ANALOG_SENSE_MAX      30

#define STELLADAPTOR_ANALOG_CENTER_DEFAULT 0
#define STELLADAPTOR_ANALOG_CENTER_FACTOR  860.0f
#define STELLADAPTOR_ANALOG_CENTER_MIN     -10
#define STELLADAPTOR_ANALOG_CENTER_MAX     30

static float stelladaptor_analog_sensitivity = 1.0f;
static float stelladaptor_analog_center      = 0.0f;

/* Low pass audio filter */
static bool low_pass_enabled       = false;
static int32_t low_pass_range      = 0;
static int32_t low_pass_left_prev  = 0;
static int32_t low_pass_right_prev = 0;

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static struct retro_system_av_info g_av_info;

static bool libretro_supports_bitmasks = false;

/************************************
 * Interframe blending
 ************************************/

enum frame_blend_method
{
   FRAME_BLEND_NONE = 0,
   FRAME_BLEND_MIX,
   FRAME_BLEND_GHOST_65,
   FRAME_BLEND_GHOST_75,
   FRAME_BLEND_GHOST_85,
   FRAME_BLEND_GHOST_95
};

/* It would be more flexible to have 'persistence'
 * as a core option, but using a variable parameter
 * reduces performance by ~15%. We therefore offer
 * fixed values, and use macros to avoid excessive
 * duplication of code...
 * Note: persistence fraction is (persistence/128),
 * using a power of 2 like this further increases
 * performance by ~15% */
#define BLEND_FRAMES_GHOST_16(persistence)                                                         \
{                                                                                                  \
   const uint32_t *palette32 = console->getPalette(0);                                             \
   uint16_t *palette16       = currentPalette16;                                                   \
   uInt8 *in                 = stella_fb;                                                          \
   uint16_t *prev            = (uint16_t*)frameBufferPrev;                                         \
   uint16_t *out             = (uint16_t*)frameBuffer;                                             \
   int i;                                                                                          \
                                                                                                   \
   /* If palette has changed, re-cache converted                                                   \
    * RGB565 values */                                                                             \
   if (palette32 != currentPalette32)                                                              \
   {                                                                                               \
      currentPalette32 = palette32;                                                                \
      convert_palette(palette32, palette16);                                                       \
   }                                                                                               \
                                                                                                   \
   for (i = 0; i < width * height; i++)                                                            \
   {                                                                                               \
      /* Get colours from current + previous frames */                                             \
      uint16_t color_curr = *(palette16 + *(in + i));                                              \
      uint16_t color_prev = *(prev + i);                                                           \
                                                                                                   \
      /* Unpack colours */                                                                         \
      uint16_t r_curr     = (color_curr >> 11) & 0x1F;                                             \
      uint16_t g_curr     = (color_curr >>  6) & 0x1F;                                             \
      uint16_t b_curr     = (color_curr      ) & 0x1F;                                             \
                                                                                                   \
      uint16_t r_prev     = (color_prev >> 11) & 0x1F;                                             \
      uint16_t g_prev     = (color_prev >>  6) & 0x1F;                                             \
      uint16_t b_prev     = (color_prev      ) & 0x1F;                                             \
                                                                                                   \
      /* Mix colors */                                                                             \
      uint16_t r_mix      = ((r_curr * (128 - persistence)) >> 7) + ((r_prev * persistence) >> 7); \
      uint16_t g_mix      = ((g_curr * (128 - persistence)) >> 7) + ((g_prev * persistence) >> 7); \
      uint16_t b_mix      = ((b_curr * (128 - persistence)) >> 7) + ((b_prev * persistence) >> 7); \
                                                                                                   \
      /* Output colour is the maximum of the input                                                 \
       * and decayed values */                                                                     \
      uint16_t r_out      = (r_mix > r_curr) ? r_mix : r_curr;                                     \
      uint16_t g_out      = (g_mix > g_curr) ? g_mix : g_curr;                                     \
      uint16_t b_out      = (b_mix > b_curr) ? b_mix : b_curr;                                     \
      uint16_t color_out  = r_out << 11 | g_out << 6 | b_out;                                      \
                                                                                                   \
      /* Assign colour and store for next frame */                                                 \
      *(out++)            = color_out;                                                             \
      *(prev + i)         = color_out;                                                             \
   }                                                                                               \
}

#define BLEND_FRAMES_GHOST_32(persistence)                                                         \
{                                                                                                  \
   const uint32_t *palette = console->getPalette(0);                                               \
   uInt8 *in               = stella_fb;                                                            \
   uint32_t *prev          = (uint32_t*)frameBufferPrev;                                           \
   uint32_t *out           = (uint32_t*)frameBuffer;                                               \
   int i;                                                                                          \
                                                                                                   \
   for (i = 0; i < width * height; i++)                                                            \
   {                                                                                               \
      /* Get colours from current + previous frames */                                             \
      uint32_t color_curr = *(palette + *(in + i));                                                \
      uint32_t color_prev = *(prev + i);                                                           \
                                                                                                   \
      /* Unpack colours */                                                                         \
      uint32_t r_curr     = (color_curr >> 16) & 0xFF;                                             \
      uint32_t g_curr     = (color_curr >>  8) & 0xFF;                                             \
      uint32_t b_curr     = (color_curr      ) & 0xFF;                                             \
                                                                                                   \
      uint32_t r_prev     = (color_prev >> 16) & 0xFF;                                             \
      uint32_t g_prev     = (color_prev >>  8) & 0xFF;                                             \
      uint32_t b_prev     = (color_prev      ) & 0xFF;                                             \
                                                                                                   \
      /* Mix colors */                                                                             \
      uint32_t r_mix      = ((r_curr * (128 - persistence)) >> 7) + ((r_prev * persistence) >> 7); \
      uint32_t g_mix      = ((g_curr * (128 - persistence)) >> 7) + ((g_prev * persistence) >> 7); \
      uint32_t b_mix      = ((b_curr * (128 - persistence)) >> 7) + ((b_prev * persistence) >> 7); \
                                                                                                   \
      /* Output colour is the maximum of the input                                                 \
       * and decayed values */                                                                     \
      uint32_t r_out      = (r_mix > r_curr) ? r_mix : r_curr;                                     \
      uint32_t g_out      = (g_mix > g_curr) ? g_mix : g_curr;                                     \
      uint32_t b_out      = (b_mix > b_curr) ? b_mix : b_curr;                                     \
      uint32_t color_out  = r_out << 16 | g_out << 8 | b_out;                                      \
                                                                                                   \
      /* Assign colour and store for next frame */                                                 \
      *(out++)            = color_out;                                                             \
      *(prev + i)         = color_out;                                                             \
   }                                                                                               \
}

static void convert_palette(const uint32_t *palette32, uint16_t *palette16)
{
   size_t i;
   for (i = 0; i < 256; i++)
   {
      uint32_t color32 = *(palette32 + i);
      *(palette16 + i) = ((color32 & 0xF80000) >> 8) |
                         ((color32 & 0x00F800) >> 5) |
                         ((color32 & 0x0000F8) >> 3);
   }
}

static void blend_frames_null_16(uInt8 *stella_fb, int width, int height)
{
   const uint32_t *palette32 = console->getPalette(0);
   uint16_t *palette16       = currentPalette16;
   uInt8 *in                 = stella_fb;
   uint16_t *out             = (uint16_t*)frameBuffer;
   int i;

   /* If palette has changed, re-cache converted
    * RGB565 values */
   if (palette32 != currentPalette32)
   {
      currentPalette32 = palette32;
      convert_palette(palette32, palette16);
   }

   for (i = 0; i < width * height; i++)
      *(out++) = *(palette16 + *(in++));
}

static void blend_frames_null_32(uInt8 *stella_fb, int width, int height)
{
   const uint32_t *palette = console->getPalette(0);
   uInt8 *in               = stella_fb;
   uint32_t *out           = (uint32_t*)frameBuffer;
   int i;

   for (i = 0; i < width * height; i++)
      *(out++) = *(palette + *(in++));
}

static void blend_frames_mix_16(uInt8 *stella_fb, int width, int height)
{
   const uint32_t *palette32 = console->getPalette(0);
   uint16_t *palette16       = currentPalette16;
   uInt8 *in                 = stella_fb;
   uint16_t *prev            = (uint16_t*)frameBufferPrev;
   uint16_t *out             = (uint16_t*)frameBuffer;
   int i;

   /* If palette has changed, re-cache converted
    * RGB565 values */
   if (palette32 != currentPalette32)
   {
      currentPalette32 = palette32;
      convert_palette(palette32, palette16);
   }

   for (i = 0; i < width * height; i++)
   {
      /* Get colours from current + previous frames */
      uint16_t color_curr = *(palette16 + *(in + i));
      uint16_t color_prev = *(prev + i);

      /* Store colours for next frame */
      *(prev + i) = color_curr;

      /* Mix colours */
      *(out++) = (color_curr + color_prev + ((color_curr ^ color_prev) & 0x821)) >> 1;
   }
}

static void blend_frames_mix_32(uInt8 *stella_fb, int width, int height)
{
   const uint32_t *palette = console->getPalette(0);
   uInt8 *in               = stella_fb;
   uint32_t *prev          = (uint32_t*)frameBufferPrev;
   uint32_t *out           = (uint32_t*)frameBuffer;
   int i;

   for (i = 0; i < width * height; i++)
   {
      /* Get colours from current + previous frames */
      uint32_t color_curr = *(palette + *(in + i));
      uint32_t color_prev = *(prev + i);

      /* Store colours for next frame */
      *(prev + i) = color_curr;

      /* Mix colours */
      *(out++) = (color_curr + color_prev + ((color_curr ^ color_prev) & 0x1010101)) >> 1;
   }
}

static void blend_frames_ghost65_16(uInt8 *stella_fb, int width, int height)
{
   /* 65% = 83 / 128 */
   BLEND_FRAMES_GHOST_16(83);
}

static void blend_frames_ghost65_32(uInt8 *stella_fb, int width, int height)
{
   BLEND_FRAMES_GHOST_32(83);
}

static void blend_frames_ghost75_16(uInt8 *stella_fb, int width, int height)
{
   /* 75% = 95 / 128 */
   BLEND_FRAMES_GHOST_16(95);
}

static void blend_frames_ghost75_32(uInt8 *stella_fb, int width, int height)
{
   BLEND_FRAMES_GHOST_32(95);
}

static void blend_frames_ghost85_16(uInt8 *stella_fb, int width, int height)
{
   /* 85% ~= 109 / 128 */
   BLEND_FRAMES_GHOST_16(109);
}

static void blend_frames_ghost85_32(uInt8 *stella_fb, int width, int height)
{
   BLEND_FRAMES_GHOST_32(109);
}

static void blend_frames_ghost95_16(uInt8 *stella_fb, int width, int height)
{
   /* 95% ~= 122 / 128 */
   BLEND_FRAMES_GHOST_16(122);
}

static void blend_frames_ghost95_32(uInt8 *stella_fb, int width, int height)
{
   BLEND_FRAMES_GHOST_32(122);
}

static void (*blend_frames_16)(uInt8 *stella_fb, int width, int height) = blend_frames_null_16;
static void (*blend_frames_32)(uInt8 *stella_fb, int width, int height) = blend_frames_null_32;

static void init_frame_blending(enum frame_blend_method blend_method)
{
   /* Allocate/zero out buffer, if required */
   if (blend_method != FRAME_BLEND_NONE)
   {
      if (!frameBufferPrev)
#ifdef _3DS
         frameBufferPrev = (uint8_t*)linearMemAlign(FRAME_BUFFER_SIZE, 128);
#else
         frameBufferPrev = (uint8_t*)malloc(FRAME_BUFFER_SIZE);
#endif
      memset(frameBufferPrev, 0, FRAME_BUFFER_SIZE);
   }

   /* Assign function pointers */
   switch (blend_method)
   {
      case FRAME_BLEND_MIX:
         blend_frames_16 = blend_frames_mix_16;
         blend_frames_32 = blend_frames_mix_32;
         break;
      case FRAME_BLEND_GHOST_65:
         blend_frames_16 = blend_frames_ghost65_16;
         blend_frames_32 = blend_frames_ghost65_32;
         break;
      case FRAME_BLEND_GHOST_75:
         blend_frames_16 = blend_frames_ghost75_16;
         blend_frames_32 = blend_frames_ghost75_32;
         break;
      case FRAME_BLEND_GHOST_85:
         blend_frames_16 = blend_frames_ghost85_16;
         blend_frames_32 = blend_frames_ghost85_32;
         break;
      case FRAME_BLEND_GHOST_95:
         blend_frames_16 = blend_frames_ghost95_16;
         blend_frames_32 = blend_frames_ghost95_32;
         break;
      default:
         blend_frames_16 = blend_frames_null_16;
         blend_frames_32 = blend_frames_null_32;
         break;
   }
}

/************************************
 * Low pass audio filter
 ************************************/

static void apply_low_pass_filter_mono(int16_t *buf, int length)
{
   int samples      = length;
   int16_t *out     = buf;

   /* Restore previous sample */
   int32_t low_pass = low_pass_left_prev;

   /* Single-pole low-pass filter (6 dB/octave) */
   int32_t factor_a = low_pass_range;
   int32_t factor_b = 0x10000 - factor_a;

   do
   {
      /* Apply low-pass filter */
      low_pass = (low_pass * factor_a) + (*out * factor_b);

      /* 16.16 fixed point */
      low_pass >>= 16;

      /* Update sound buffer
       * > Converted to stereo by duplicating
       *   the left/right channels */
      *out++   = (int16_t)low_pass;
      *out++   = (int16_t)low_pass;
   }
   while (--samples);

   /* Save last sample for next frame */
   low_pass_left_prev = low_pass;
}

static void apply_low_pass_filter_stereo(int16_t *buf, int length)
{
   int samples            = length;
   int16_t *out           = buf;

   /* Restore previous samples */
   int32_t low_pass_left  = low_pass_left_prev;
   int32_t low_pass_right = low_pass_right_prev;

   /* Single-pole low-pass filter (6 dB/octave) */
   int32_t factor_a       = low_pass_range;
   int32_t factor_b       = 0x10000 - factor_a;

   do
   {
      /* Apply low-pass filter */
      low_pass_left  = (low_pass_left  * factor_a) + (*out       * factor_b);
      low_pass_right = (low_pass_right * factor_a) + (*(out + 1) * factor_b);

      /* 16.16 fixed point */
      low_pass_left  >>= 16;
      low_pass_right >>= 16;

      /* Update sound buffer */
      *out++ = (int16_t)low_pass_left;
      *out++ = (int16_t)low_pass_right;
   }
   while (--samples);

   /* Save last samples for next frame */
   low_pass_left_prev  = low_pass_left;
   low_pass_right_prev = low_pass_right;
}

static void (*apply_low_pass_filter)(int16_t *buf, int length) = apply_low_pass_filter_mono;

/************************************
 * Auxiliary functions
 ************************************/

static float get_stelladaptor_analog_sensitivity(int sensitivity)
{
   /* STELLADAPTOR_ANALOG_SENSE_FACTOR *
    *       (STELLADAPTOR_ANALOG_SENSE_BASE ^
    *             STELLADAPTOR_ANALOG_SENSE_DEFAULT) = 1.0 */

   int sense = (sensitivity > STELLADAPTOR_ANALOG_SENSE_MAX) ?
         STELLADAPTOR_ANALOG_SENSE_MAX : sensitivity;

   sense = (sense < STELLADAPTOR_ANALOG_SENSE_MIN) ?
         STELLADAPTOR_ANALOG_SENSE_MIN : sense;

   return STELLADAPTOR_ANALOG_SENSE_FACTOR * (float)pow(
         (double)STELLADAPTOR_ANALOG_SENSE_BASE, (double)sense);
}

static float get_stelladaptor_analog_center(int center)
{
   /* Convert into ~5 pixel steps */

   int offset = (center > STELLADAPTOR_ANALOG_CENTER_MAX) ?
         STELLADAPTOR_ANALOG_CENTER_MAX : center;

   offset = (offset < STELLADAPTOR_ANALOG_CENTER_MIN) ?
         STELLADAPTOR_ANALOG_CENTER_MIN : offset;

   return STELLADAPTOR_ANALOG_CENTER_FACTOR * (float)offset;
}

static void init_paddles(void)
{
   /* Check whether paddles are active */
   left_controller_type = console->controller(Controller::Left).type();

   if (left_controller_type == Controller::Paddles)
   {
      /* Set initial digital sensitivity */
      Paddles::setDigitalSensitivity(paddle_digital_sensitivity);

      /* Configure mouse control (mapped to
       * gamepad analog sticks) */
      console->controller(Controller::Left).setMouseControl(
            Controller::Paddles, 0, Controller::Paddles, 1);

      /* Stella internal mouse sensitivity is hard coded
       * to a value of 1 - we handle 'actual' sensitivity
       * via the libretro interface */
      Paddles::setMouseSensitivity(1);

      /* Check whether port 0/1 paddles should be swapped */
      if (console->properties().get(Controller_SwapPaddles) == "YES")
      {
         MouseAxisValue1   = Event::MouseAxisXValue;
         MouseButtonValue1 = Event::MouseButtonLeftValue;
         MouseAxisValue0   = Event::MouseAxisYValue;
         MouseButtonValue0 = Event::MouseButtonRightValue;
      }
      else
      {
         MouseAxisValue0   = Event::MouseAxisXValue;
         MouseButtonValue0 = Event::MouseButtonLeftValue;
         MouseAxisValue1   = Event::MouseAxisYValue;
         MouseButtonValue1 = Event::MouseButtonRightValue;
      }
   }
}

static void update_input()
{
   unsigned i;

   if (!input_poll_cb)
      return;

   input_poll_cb();
   Event &ev = osystem.eventHandler().event();

   /* Loop over input devices */
   for (i = 0; i < MAX_RETROPAD_DEVICES; i++)
   {
      int16_t joy_bits = 0;

      /* Read button input (required in all cases) */
      if (libretro_supports_bitmasks)
         joy_bits = input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      else
      {
         unsigned j;
         for (j = 0; j < (RETRO_DEVICE_ID_JOYPAD_R3+1); j++)
            joy_bits |= input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, j) ? (1 << j) : 0;
      }

      if (retropad_devices[i] == RETROPAD_STELLA_PADDLES)
      {
         /* Handle paddle devices */

         /* Read analog input */
         int paddle_a = input_state_cb(i, RETRO_DEVICE_ANALOG,
               RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
         int paddle_b = input_state_cb(i, RETRO_DEVICE_ANALOG,
               RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);

         /* Apply sensitivity/offset factors */
         paddle_a = (int)(((float)paddle_a * stelladaptor_analog_sensitivity) +
               stelladaptor_analog_center);
         paddle_a = (paddle_a >  0x7FFF) ?  0x7FFF : paddle_a;
         paddle_a = (paddle_a < -0x7FFF) ? -0x7FFF : paddle_a;

         paddle_b = (int)(((float)paddle_b * stelladaptor_analog_sensitivity) +
               stelladaptor_analog_center);
         paddle_b = (paddle_b >  0x7FFF) ?  0x7FFF : paddle_b;
         paddle_b = (paddle_b < -0x7FFF) ? -0x7FFF : paddle_b;

         if (i == 0)
         {
            /* Events for left player's paddles */

            /* Paddle 0 */
            ev.set(Event::Type(Event::SALeftAxis0Value), paddle_a);
            ev.set(Event::Type(Event::PaddleZeroFire), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_A));

            /* Paddle 1 */
            ev.set(Event::Type(Event::SALeftAxis1Value), paddle_b);
            ev.set(Event::Type(Event::PaddleOneFire), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B));

            /* Generic inputs */
            ev.set(Event::Type(Event::ConsoleLeftDiffA),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_L));
            ev.set(Event::Type(Event::ConsoleLeftDiffB),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_L2));
            ev.set(Event::Type(Event::ConsoleColor),      joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_L3));
            ev.set(Event::Type(Event::ConsoleRightDiffA), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_R));
            ev.set(Event::Type(Event::ConsoleRightDiffB), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_R2));
            ev.set(Event::Type(Event::ConsoleBlackWhite), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_R3));
            ev.set(Event::Type(Event::ConsoleSelect),     joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT));
            ev.set(Event::Type(Event::ConsoleReset),      joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_START));
         }
         else
         {
            /* Events for right player's paddles */

            /* Paddle 2 */
            ev.set(Event::Type(Event::SARightAxis0Value), paddle_a);
            ev.set(Event::Type(Event::PaddleTwoFire), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_A));

            /* Paddle 3 */
            ev.set(Event::Type(Event::SARightAxis1Value), paddle_b);
            ev.set(Event::Type(Event::PaddleThreeFire), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B));
         }
      }
      else
      {
         /* Handle regular gamepad devices */
         if (i == 0)
         {
            /* Events for left player's joystick */
            ev.set(Event::Type(Event::JoystickZeroUp),    joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_UP));
            ev.set(Event::Type(Event::JoystickZeroDown),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN));
            ev.set(Event::Type(Event::JoystickZeroLeft),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT));
            ev.set(Event::Type(Event::JoystickZeroRight), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT));
            ev.set(Event::Type(Event::JoystickZeroFire),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B));
            ev.set(Event::Type(Event::ConsoleLeftDiffA),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_L));
            ev.set(Event::Type(Event::ConsoleLeftDiffB),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_L2));
            ev.set(Event::Type(Event::ConsoleColor),      joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_L3));
            ev.set(Event::Type(Event::ConsoleRightDiffA), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_R));
            ev.set(Event::Type(Event::ConsoleRightDiffB), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_R2));
            ev.set(Event::Type(Event::ConsoleBlackWhite), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_R3));
            ev.set(Event::Type(Event::ConsoleSelect),     joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT));
            ev.set(Event::Type(Event::ConsoleReset),      joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_START));
         }
         else
         {
            /* Events for right player's joystick */
            ev.set(Event::Type(Event::JoystickOneUp),    joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_UP));
            ev.set(Event::Type(Event::JoystickOneDown),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN));
            ev.set(Event::Type(Event::JoystickOneLeft),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT));
            ev.set(Event::Type(Event::JoystickOneRight), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT));
            ev.set(Event::Type(Event::JoystickOneFire),  joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B));
         }

         /* Read analog paddle input, if required */
         if (left_controller_type == Controller::Paddles)
         {
            float paddle_amp = 0.0f;
            int paddle       = input_state_cb(i, RETRO_DEVICE_ANALOG,
                  RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);

            /* Account for paddle deadzone, and convert
             * analog stick input to an 'amplitude' value */
            if ((paddle < -paddle_analog_deadzone) ||
                (paddle > paddle_analog_deadzone))
            {
               paddle_amp = (float)((paddle > paddle_analog_deadzone) ?
                     (paddle - paddle_analog_deadzone) :
                           (paddle + paddle_analog_deadzone)) /
                                 (float)(PADDLE_ANALOG_RANGE - paddle_analog_deadzone);

               /* Check whether paddle response is quadratic */
               if (paddle_analog_is_quadratic)
               {
                  if (paddle_amp < 0.0)
                     paddle_amp = -(paddle_amp * paddle_amp);
                  else
                     paddle_amp = paddle_amp * paddle_amp;
               }
            }

            /* Convert paddle amplitude back to an integer,
             * scaling by current analog sensitivity value
             * > Note: Stella internally divides paddle value
             *   by 2 - counter this by premultiplying */
            paddle = (int)(paddle_amp * paddle_analog_sensitivity) << 1;

            if (i == 0)
            {
               /* Events for left player's paddle */
               ev.set(Event::Type(MouseAxisValue0), paddle);
               ev.set(Event::Type(MouseButtonValue0), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_Y));
            }
            else
            {
               /* Events for right player's paddle */
               ev.set(Event::Type(MouseAxisValue1), paddle);
               ev.set(Event::Type(MouseButtonValue1), joy_bits & (1 << RETRO_DEVICE_ID_JOYPAD_Y));
            }
         }
      }
   }

   /* Tell all input devices to read their state from the event structure */
   console->controller(Controller::Left).update();
   console->controller(Controller::Right).update();
   console->switches().update();
}

static void check_variables(bool first_run)
{
   struct retro_variable var            = {0};
   enum frame_blend_method blend_method = FRAME_BLEND_NONE;
   int last_paddle_sensitivity;
   int stelladaptor_sensitivity;
   int stelladaptor_center;

   /* Only read colour depth option on first run */
   if (first_run)
   {
      var.key   = "stella2014_color_depth";
      var.value = NULL;

      /* Set 16bpp by default */
      framePixelBytes = 2;

      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (strcmp(var.value, "24bit") == 0)
            framePixelBytes = 4;
   }

   /* Read interframe blending option */
   var.key   = "stella2014_mix_frames";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (!strcmp(var.value, "mix"))
         blend_method = FRAME_BLEND_MIX;
      else if (!strcmp(var.value, "ghost_65"))
         blend_method = FRAME_BLEND_GHOST_65;
      else if (!strcmp(var.value, "ghost_75"))
         blend_method = FRAME_BLEND_GHOST_75;
      else if (!strcmp(var.value, "ghost_85"))
         blend_method = FRAME_BLEND_GHOST_85;
      else if (!strcmp(var.value, "ghost_95"))
         blend_method = FRAME_BLEND_GHOST_95;
   }

   init_frame_blending(blend_method);

   /* Read low pass audio filter settings */
   var.key   = "stella2014_low_pass_filter";
   var.value = NULL;

   low_pass_enabled = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (strcmp(var.value, "enabled") == 0)
         low_pass_enabled = true;

   var.key   = "stella2014_low_pass_range";
   var.value = NULL;

   low_pass_range = (60 * 0x10000) / 100;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      low_pass_range = (strtol(var.value, NULL, 10) * 0x10000) / 100;

   /* Read paddle digital sensitivity option */
   var.key   = "stella2014_paddle_digital_sensitivity";
   var.value = NULL;

   last_paddle_sensitivity    = paddle_digital_sensitivity;
   paddle_digital_sensitivity = 50;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      paddle_digital_sensitivity = atoi(var.value);
      paddle_digital_sensitivity = (paddle_digital_sensitivity > 100) ?
            100 : paddle_digital_sensitivity;
      paddle_digital_sensitivity = (paddle_digital_sensitivity <  10) ?
            10  : paddle_digital_sensitivity;
   }

   /* Only apply paddle sensitivity update if
    * this is *not* the first run */
   if (!first_run &&
       (left_controller_type == Controller::Paddles) &&
       (paddle_digital_sensitivity != last_paddle_sensitivity))
      Paddles::setDigitalSensitivity(paddle_digital_sensitivity);

   /* Read paddle analog sensitivity option */
   var.key   = "stella2014_paddle_analog_sensitivity";
   var.value = NULL;

   paddle_analog_sensitivity = 50.0f;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      int analog_sensitivity = atoi(var.value);
      analog_sensitivity = (analog_sensitivity > 150) ?
            150 : analog_sensitivity;
      analog_sensitivity = (analog_sensitivity <  10) ?
            10  : analog_sensitivity;
      paddle_analog_sensitivity = (float)analog_sensitivity;
   }

   /* Read paddle analog response option */
   var.key   = "stella2014_paddle_analog_response";
   var.value = NULL;

   paddle_analog_is_quadratic = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      if (strcmp(var.value, "quadratic") == 0)
         paddle_analog_is_quadratic = true;

   /* Read paddle analog deadzone option */
   var.key   = "stella2014_paddle_analog_deadzone";
   var.value = NULL;

   paddle_analog_deadzone = (int)(0.15f * (float)PADDLE_ANALOG_RANGE);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      paddle_analog_deadzone = (int)((float)atoi(var.value) * 0.01f * (float)PADDLE_ANALOG_RANGE);

   /* Read Stelladaptor analog sensitivity option */
   var.key   = "stella2014_stelladaptor_analog_sensitivity";
   var.value = NULL;

   stelladaptor_sensitivity = STELLADAPTOR_ANALOG_SENSE_DEFAULT;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      stelladaptor_sensitivity = atoi(var.value);

   stelladaptor_analog_sensitivity =
         get_stelladaptor_analog_sensitivity(
               stelladaptor_sensitivity);

   /* Read Stelladaptor analog centre option */
   var.key   = "stella2014_stelladaptor_analog_center";
   var.value = NULL;

   stelladaptor_center = STELLADAPTOR_ANALOG_CENTER_DEFAULT;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      stelladaptor_center = atoi(var.value);

   stelladaptor_analog_center =
         get_stelladaptor_analog_center(
               stelladaptor_center);
}

/************************************
 * libretro implementation
 ************************************/

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;
   environ_cb = cb;
   libretro_set_core_options(environ_cb);
   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)retropad_port_info);

   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
	   filestream_vfs_init(&vfs_iface_info);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name = "Stella 2014";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version = STELLA_VERSION GIT_VERSION;
   info->need_fullpath = false;
   info->valid_extensions = "a26|bin";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->timing.fps            = console->getFramerate();
   info->timing.sample_rate    = 31400;
   info->geometry.base_width   = 160 * 2;
   info->geometry.base_height  = videoHeight;
   info->geometry.max_width    = 320;
   info->geometry.max_height   = 256;
   info->geometry.aspect_ratio = 4.0f / 3.0f;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   struct retro_core_option_display option_display;
   bool show_gamepad_options;
   bool show_stelladaptor_options;

   if (port >= MAX_RETROPAD_DEVICES)
      return;

   switch (device)
   {
      case RETROPAD_STELLA_GAMEPAD:
         retropad_devices[port] = RETROPAD_STELLA_GAMEPAD;
         break;
      case RETROPAD_STELLA_PADDLES:
         retropad_devices[port] = RETROPAD_STELLA_PADDLES;
         break;
      default:
         if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                  "[Stella]: Invalid libretro controller device, using default: RETROPAD_STELLA_GAMEPAD\n");
         retropad_devices[port] = RETROPAD_STELLA_GAMEPAD;
         break;
   }

   /* Ugly workaround to support different input
    * descriptors on different ports... */
   if (retropad_devices[0] == RETROPAD_STELLA_PADDLES)
   {
      if (retropad_devices[1] == RETROPAD_STELLA_PADDLES)
         environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, retropad_inputs_paddles0_paddles1);
      else
         environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, retropad_inputs_paddles0_gamepad1);
   }
   else
   {
      if (retropad_devices[1] == RETROPAD_STELLA_PADDLES)
         environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, retropad_inputs_gamepad0_paddles1);
      else
         environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, retropad_inputs_gamepad0_gamepad1);
   }

   /* Show/hide relevant controller-related core options */
   show_gamepad_options =
         (retropad_devices[0] == RETROPAD_STELLA_GAMEPAD) ||
         (retropad_devices[1] == RETROPAD_STELLA_GAMEPAD);

   show_stelladaptor_options =
         (retropad_devices[0] == RETROPAD_STELLA_PADDLES) ||
         (retropad_devices[1] == RETROPAD_STELLA_PADDLES);

   /* > Gamepad */
   option_display.visible = show_gamepad_options;

   option_display.key = "stella2014_paddle_digital_sensitivity";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   option_display.key = "stella2014_paddle_analog_sensitivity";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   option_display.key = "stella2014_paddle_analog_response";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   option_display.key = "stella2014_paddle_analog_deadzone";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   /* > Stelladaptor */
   option_display.visible = show_stelladaptor_options;

   option_display.key = "stella2014_stelladaptor_analog_sensitivity";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

   option_display.key = "stella2014_stelladaptor_analog_center";
   environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
}

size_t retro_serialize_size(void) 
{
   Serializer state;
   if(!stateManager.saveState(state))
      return 0;
   return state.get().size();
}

bool retro_serialize(void *data, size_t size)
{
    Serializer state;
    if(!stateManager.saveState(state))
        return false;
    std::string s = state.get();
    memcpy(data, s.data(), s.size());
    return true;
}

bool retro_unserialize(const void *data, size_t size)
{
    std::string s((const char*)data, size);
    Serializer state;
    state.set(s);
   if(stateManager.loadState(state))
      return true;
   return false;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

bool retro_load_game(const struct retro_game_info *info)
{
   enum retro_pixel_format fmt;

   if (!info || info->size >= 96*1024)
      return false;

   // Set color depth
   check_variables(true);

   if (framePixelBytes == 4)
   {
      fmt = RETRO_PIXEL_FORMAT_XRGB8888;
      if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      {
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[Stella]: XRGB8888 is not supported - trying RGB565...\n");

         /* Fallback to RETRO_PIXEL_FORMAT_RGB565 */
         framePixelBytes = 2;
      }
   }

   if (framePixelBytes == 2)
   {
      fmt = RETRO_PIXEL_FORMAT_RGB565;
      if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      {
         if (log_cb)
            log_cb(RETRO_LOG_INFO, "[Stella]: RGB565 is not supported.\n");
         return false;
      }
   }

   // Get the game properties
   string cartMD5 = MD5((const uInt8*)info->data, (uInt32)info->size);
   Properties props;
   osystem.propSet().getMD5(cartMD5, props);

   // Load the cart
   string cartType = props.get(Cartridge_Type);
   string cartId;//, romType("AUTO-DETECT");
   settings = new Settings(&osystem);
   settings->setValue("romloadcount", false);
   cartridge = Cartridge::create((const uInt8*)info->data, (uInt32)info->size, cartMD5, cartType, cartId, osystem, *settings);

   if(cartridge == 0)
   {
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, "Stella: Failed to load cartridge.\n");
      return false;
   }

   // Create the console
   console = new Console(&osystem, cartridge, props);
   osystem.myConsole = console;

   // Init sound and video
   console->initializeVideo();
   console->initializeAudio();

   // Check number of audio channels
   if (console->properties().get(Cartridge_Sound) == "STEREO")
      apply_low_pass_filter = apply_low_pass_filter_stereo;
   else
      apply_low_pass_filter = apply_low_pass_filter_mono;

   // Init paddle controls
   init_paddles();

   // Get the ROM's width and height
   TIA& tia = console->tia();
   videoWidth = tia.width();
   videoHeight = tia.height();

   return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   (void)game_type;
   (void)info;
   (void)num_info;
   return false;
}

void retro_unload_game(void) 
{
   if (console)
   {
      delete console;
      console = 0;
   }
   else if (cartridge)
   {
      delete cartridge;
      cartridge = 0;
   }
   if (settings)
   {
      delete settings;
      settings = 0;
   }
}

unsigned retro_get_region(void)
{
   //console->getFramerate();
   return RETRO_REGION_NTSC;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
   switch (id)
   {
      case RETRO_MEMORY_SYSTEM_RAM: return console->system().m6532().getRAM();
      default: return NULL;
   }
}

size_t retro_get_memory_size(unsigned id)
{
   switch (id)
   {
      case RETRO_MEMORY_SYSTEM_RAM: return 128;
      default: return 0;
   }
}

void retro_init(void)
{
   struct retro_log_callback log;
   unsigned level = 4;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

#ifdef _3DS
   frameBuffer = (uint8_t*)linearMemAlign(FRAME_BUFFER_SIZE, 128);
#else
   frameBuffer = (uint8_t*)malloc(FRAME_BUFFER_SIZE);
#endif
}

void retro_deinit(void)
{
   libretro_supports_bitmasks = false;
   left_controller_type       = Controller::Joystick;
   MouseAxisValue0            = Event::MouseAxisXValue;
   MouseButtonValue0          = Event::MouseButtonLeftValue;
   MouseAxisValue1            = Event::MouseAxisYValue;
   MouseButtonValue1          = Event::MouseButtonRightValue;
   low_pass_enabled           = false;
   low_pass_left_prev         = 0;
   low_pass_right_prev        = 0;
   currentPalette32           = NULL;

   if (frameBuffer)
   {
#ifdef _3DS
      linearFree(frameBuffer);
#else
      free(frameBuffer);
#endif
      frameBuffer = NULL;
   }

   if (frameBufferPrev)
   {
#ifdef _3DS
      linearFree(frameBufferPrev);
#else
      free(frameBufferPrev);
#endif
      frameBufferPrev = NULL;
   }
}

void retro_reset(void)
{
   console->system().reset();
}

void retro_run(void)
{
   static int16_t sampleBuffer[2048];
   //Get the number of samples in a frame
   static uint32_t tiaSamplesPerFrame = (uint32_t)(31400.0f/console->getFramerate());

   //CORE OPTIONS
   bool updated = false;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables(false);

   //INPUT
   update_input();

   //EMULATE
   TIA& tia = console->tia();
   tia.update();

   //VIDEO
   //Get the frame info from stella
   videoWidth = tia.width();
   videoHeight = tia.height();

   //Copy the frame from stella to libretro
   if (framePixelBytes == 2)
      blend_frames_16(tia.currentFrameBuffer(), videoWidth, videoHeight);
   else
      blend_frames_32(tia.currentFrameBuffer(), videoWidth, videoHeight);

   video_cb(frameBuffer, videoWidth, videoHeight, videoWidth * framePixelBytes);

   osystem.sound().processFragment(sampleBuffer, tiaSamplesPerFrame);

   if (low_pass_enabled)
      apply_low_pass_filter(sampleBuffer, tiaSamplesPerFrame);

   audio_batch_cb(sampleBuffer, tiaSamplesPerFrame);
}
