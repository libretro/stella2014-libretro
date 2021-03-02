#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 1.3
 ********************************
 *
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

struct retro_core_option_definition option_defs_us[] = {
   {
      "stella2014_color_depth",
      "Color Depth (Restart)",
      "Specifies number of colors to display on-screen. 24-bit may increase performance overheads on some platforms.",
      {
         { "16bit", "Thousands (16-bit)" },
         { "24bit", "Millions (24-bit)" },
         { NULL, NULL },
      },
      "16bit"
   },
   {
      "stella2014_mix_frames",
      "Interframe Blending",
      "Simulates CRT phosphor ghosting effects. 'Simple' performs a 50:50 mix of the current and previous frames. 'Ghosting' accumulates pixels from multiple successive frames. May be used to alleviate screen flicker when games display large numbers of simultaneous objects by toggling sprites on alternate frames.",
      {
         { "disabled", NULL },
         { "mix",      "Simple" },
         { "ghost_65", "Ghosting (65%)" },
         { "ghost_75", "Ghosting (75%)" },
         { "ghost_85", "Ghosting (85%)" },
         { "ghost_95", "Ghosting (95%)" },
         { NULL,    NULL },
      },
      "disabled"
   },
   {
      "stella2014_paddle_digital_sensitivity",
      "Paddle Sensitivity (Digital)",
      "Sets the sensitivity for digital emulation of paddle movement. Higher values increase paddle speed.",
      {
         { "10",  "10%" },
         { "15",  "15%" },
         { "20",  "20%" },
         { "25",  "25%" },
         { "30",  "30%" },
         { "35",  "35%" },
         { "40",  "40%" },
         { "45",  "45%" },
         { "50",  "50%" },
         { "55",  "55%" },
         { "60",  "60%" },
         { "65",  "65%" },
         { "70",  "70%" },
         { "75",  "75%" },
         { "80",  "80%" },
         { "85",  "85%" },
         { "90",  "90%" },
         { "95",  "95%" },
         { "100", "100%" },
         { NULL, NULL },
      },
      "50"
   },
   {
      "stella2014_paddle_analog_sensitivity",
      "Paddle Sensitivity (Analog)",
      "Sets the sensitivity for analog emulation of paddle movement. Higher values increase paddle speed.",
      {
         { "10",  "10%" },
         { "15",  "15%" },
         { "20",  "20%" },
         { "25",  "25%" },
         { "30",  "30%" },
         { "35",  "35%" },
         { "40",  "40%" },
         { "45",  "45%" },
         { "50",  "50%" },
         { "55",  "55%" },
         { "60",  "60%" },
         { "65",  "65%" },
         { "70",  "70%" },
         { "75",  "75%" },
         { "80",  "80%" },
         { "85",  "85%" },
         { "90",  "90%" },
         { "95",  "95%" },
         { "100", "100%" },
         { "105", "105%" },
         { "110", "110%" },
         { "115", "115%" },
         { "120", "120%" },
         { "125", "125%" },
         { "130", "130%" },
         { "135", "135%" },
         { "140", "140%" },
         { "145", "145%" },
         { "150", "150%" },
         { NULL, NULL },
      },
      "50"
   },
   {
      "stella2014_paddle_analog_response",
      "Paddle Response (Analog)",
      "Determines how the speed of the paddle varies when tilting the RetroPad analog stick. 'Linear': Speed is directly proportional to analog stick displacement. 'Quadratic': Speed increases quadratically with analog stick displacement; this allows for greater precision when making small movements without sacrificing maximum speed at full analog range.",
      {
         { "linear",    "Linear" },
         { "quadratic", "Quadratic" },
         { NULL, NULL },
      },
      "linear"
   },
   {
      "stella2014_paddle_analog_deadzone",
      "Paddle Deadzone (Analog)",
      "Sets the deadzone of the RetroPad analog sticks. Used to eliminate drift/unwanted input when emulating analog paddle movement.",
      {
         { "0",  "0%" },
         { "3",  "3%" },
         { "6",  "6%" },
         { "9",  "9%" },
         { "12", "12%" },
         { "15", "15%" },
         { "18", "18%" },
         { "21", "21%" },
         { "24", "24%" },
         { "27", "37%" },
         { "30", "30%" },
         { NULL, NULL },
      },
      "15"
   },
   { NULL, NULL, NULL, {{0}}, NULL },
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_option_definition *option_defs_intl[RETRO_LANGUAGE_LAST] = {
   option_defs_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   NULL,           /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   NULL,           /* RETRO_LANGUAGE_TURKISH */
   NULL,           /* RETRO_LANGUAGE_SLOVAK */
   NULL,           /* RETRO_LANGUAGE_PERSIAN */
   NULL,           /* RETRO_LANGUAGE_HEBREW */
   NULL,           /* RETRO_LANGUAGE_ASTURIAN */
   NULL,           /* RETRO_LANGUAGE_FINNISH */

};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb)
{
   unsigned version = 0;

   if (!environ_cb)
      return;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && (version >= 1))
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_intl core_options_intl;
      unsigned language = 0;

      core_options_intl.us    = option_defs_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = option_defs_intl[language];

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_intl);
#else
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, &option_defs_us);
#endif
   }
   else
   {
      size_t i;
      size_t num_options               = 0;
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine number of options */
      for (;;)
      {
         if (!option_defs_us[num_options].key)
            break;
         num_options++;
      }

      /* Allocate arrays */
      variables  = (struct retro_variable *)calloc(num_options + 1, sizeof(struct retro_variable));
      values_buf = (char **)calloc(num_options, sizeof(char *));

      if (!variables || !values_buf)
         goto error;

      /* Copy parameters from option_defs_us array */
      for (i = 0; i < num_options; i++)
      {
         const char *key                        = option_defs_us[i].key;
         const char *desc                       = option_defs_us[i].desc;
         const char *default_value              = option_defs_us[i].default_value;
         struct retro_core_option_value *values = option_defs_us[i].values;
         size_t buf_len                         = 3;
         size_t default_index                   = 0;

         values_buf[i] = NULL;

         if (desc)
         {
            size_t num_values = 0;

            /* Determine number of values */
            for (;;)
            {
               if (!values[num_values].value)
                  break;

               /* Check if this is the default value */
               if (default_value)
                  if (strcmp(values[num_values].value, default_value) == 0)
                     default_index = num_values;

               buf_len += strlen(values[num_values].value);
               num_values++;
            }

            /* Build values string */
            if (num_values > 0)
            {
               size_t j;

               buf_len += num_values - 1;
               buf_len += strlen(desc);

               values_buf[i] = (char *)calloc(buf_len, sizeof(char));
               if (!values_buf[i])
                  goto error;

               strcpy(values_buf[i], desc);
               strcat(values_buf[i], "; ");

               /* Default value goes first */
               strcat(values_buf[i], values[default_index].value);

               /* Add remaining values */
               for (j = 0; j < num_values; j++)
               {
                  if (j != default_index)
                  {
                     strcat(values_buf[i], "|");
                     strcat(values_buf[i], values[j].value);
                  }
               }
            }
         }

         variables[i].key   = key;
         variables[i].value = values_buf[i];
      }

      /* Set variables */
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);

error:

      /* Clean up */
      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
