#pragma once
// The board a build runs on: which modules are plugged into which ports, and the physical facts
// about those modules that the code cannot infer. One variant per board build; the environment
// picks it with a -D flag, so an app never names a board itself.
//
//   variants/<name>.h   the module instances the Atech Hardware Platform generates for a layout
//
// Adding a board build: drop a variant here, add -DATECH_BOARD_<NAME> to a new [env:] section.
#include "modules/audio/speaker.h"
#include "modules/display/st7735_tft.h"
#include "modules/input/rotary_encoder.h"
#include "modules/input/button.h"
#include "modules/led/neopixel.h"

#if defined(ATECH_BOARD_ATECH14_SYNTH)
#  include "variants/atech14-synth.h"
#else
#  error "no board selected: define ATECH_BOARD_<NAME> in the platformio.ini environment"
#endif
