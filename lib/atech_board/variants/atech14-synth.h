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
static const int rotary_encoder_1_pin_clk = 5, rotary_encoder_1_pin_dt = 4;
ButtonModule button_1(17);
ButtonModule button_2(16);
Speaker speaker_1(12, 13, 10);
NeoPixelGrid light_grid_7(6, 7);
ST7735_TFT st7735_tft_1(2, 41, 1, 40);
NeoPixelGrid light_grid_11(43, 44);
// ---- measured, not generated ---------------------------------------------------
// The Light Grids in ports 7 and 11: chain-to-glass maps, measured with grid-selftest.
#include "measured/light-grid-v11-ports-7-11.h"
