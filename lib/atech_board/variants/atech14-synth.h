// Generated from atech14-synth.yaml by tools/gen-board-header.py — do not edit above the
// marker; run `make board-headers` instead. Everything below the marker is hand-written.
//
// Board: Atech 14-Port Board (ESP32-S3, 8 MB flash, 512 KB RAM)
//
//   port 1, 2  Rotary Encoder Knob                GPIO 1:9/8 2:5/4
//   port 3     Button                             GPIO 3:17/18
//   port 4     Button                             GPIO 4:16/15
//   port 5, 6  I2S Speaker (MAX98357A)            GPIO 5:11/10 6:13/12
//   port 7     NeoPixel 3x3 Grid                  GPIO 7:6/7
//   port 9, 10 ST7735 TFT Color Display (160x80)  GPIO 9:40/41 10:1/2
//   port 11    NeoPixel 3x3 Grid                  GPIO 11:43/44

#include "modules/audio/speaker.h"
#include "modules/display/st7735_tft.h"
#include "modules/input/rotary_encoder.h"
#include "modules/input/button.h"
#include "modules/led/neopixel.h"

RotaryEncoder rotary_encoder_1(5, 4, 9, 8);
ButtonModule button_1(17);
ButtonModule button_2(16);
Speaker speaker_1(12, 13, 10);
NeoPixelGrid light_grid_7(6, 7);
ST7735_TFT st7735_tft_1(2, 41, 1, 40);
NeoPixelGrid light_grid_11(43, 44);
// ---- measured, not generated ---------------------------------------------------
// ---- Physical layout of the Light Grid V1.1 ----------------------------------------------
// The 3x3 chain is not row-major on the glass: with a module's own ESP32 connector edge down it
// runs down the left column, up the middle, then down the right. And the two modules are not
// mounted alike — a slot's side decides how a module sits, so with the motherboard upright
// (USB-C at the bottom) the port-7 module is turned 180 degrees and the port-11 one 90 degrees.
// Measured with the grid-selftest build, one chain LED at a time on both grids at once: chain 0
// is top-left on port 7 and top-right on port 11.
//
// GRID_GLASS_*[row * 3 + col] is the chain index lighting that cell, board upright, so a bar
// drawn by glass geometry stays a bar on the module.
static const uint8_t GRID_GLASS_PORT7[9]  = { 0, 5, 6, 1, 4, 7, 2, 3, 8 };
static const uint8_t GRID_GLASS_PORT11[9] = { 2, 1, 0, 3, 4, 5, 8, 7, 6 };

/// Light the cell (row, col) of one grid's glass, rows counted from the top, board upright.
static inline void gridSetGlassXY(NeoPixelGrid& grid, const uint8_t map[9],
                                  uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b) {
    grid.setPixel(map[row * 3 + col], r, g, b);
}
