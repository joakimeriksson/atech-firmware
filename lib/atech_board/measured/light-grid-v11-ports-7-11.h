#pragma once
// Measured on the board, not generated: how a Light Grid V1.1 sits in port 7 and in port 11.
// A fact about those two slots, so every board variant with grids there includes this.
//
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
