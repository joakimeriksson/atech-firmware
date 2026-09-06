// Included by <atech_board.h> when the build defines ATECH_BOARD_ATECH14_SYNTH.
// The Atech 14-port motherboard as this build populates it. One header per board build: the
// module instances the Atech Hardware Platform generates for a port layout, plus the physical
// facts about those modules that the code above cannot infer.
//
//   port 1+2   Knob V1.1        rotary encoder CLK 5, DT 4, SW 9, 12-LED ring 8
//   port 3     Button           GPIO 17, active low
//   port 4     Button           GPIO 16, active low
//   port 5+6   Speaker          MAX98357A: BCLK 12, LRCLK 13, DIN 10
//   port 7     Light Grid V1.1  lines A 6 / B 7
//   port 9+10  ST7735 TFT       SCLK 2, CS 41, MOSI 1, DC 40
//   port 11    Light Grid V1.1  lines A 43 / B 44
//
// ========== MODULE INSTANCES ==========
// I2S Speaker (MAX98357A)
// Pins swapped for left-side placement: port_6 pins first, port_5 pins second
Speaker speaker_1(12, 13, 10);
// ST7735 TFT Color Display (160x80)
ST7735_TFT st7735_tft_1(2, 41, 1, 40);
// Rotary Encoder Knob
// Pins swapped for left-side placement: port_2 pins first, port_1 pins second
RotaryEncoder rotary_encoder_1(5, 4, 9, 8);
// Button
ButtonModule button_1(17, true);
// Button
ButtonModule button_2(16, true);
// Light Grid V1.1 (NeoPixel 3x3): port 7 is the isolated top-middle slot, port 11 the right
// column. Each takes two data lines — Line A (WS2812B, RGB) and Line B (SK6812, RGBW) — and the
// SDK driver writes both every frame, so one instance covers either module revision.
NeoPixelGrid light_grid_7(6, 7);
NeoPixelGrid light_grid_11(43, 44);

// ---- Physical layout of the Light Grid V1.1 ----------------------------------------------
// The 3x3 chain is not row-major on the glass. With the module's ESP32 connector edge down it
// runs down the left column, up the middle, then down the right. Measured on the board with
// the grid-selftest build, one chain LED at a time. GRID_GLASS[row * 3 + col] is the chain
// index that lights that cell, so a bar drawn by glass geometry stays a bar on the module.
static const uint8_t GRID_GLASS[9] = { 0, 5, 6, 1, 4, 7, 2, 3, 8 };

/// Light the cell (row, col) of the glass, rows counted from the top with the connector down.
static inline void gridSetGlassXY(NeoPixelGrid& g, uint8_t row, uint8_t col, uint8_t r, uint8_t gr, uint8_t b) {
    g.setPixel(GRID_GLASS[row * 3 + col], r, gr, b);
}
