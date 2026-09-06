// Light Grid V1.1 bring-up. The 3x3 modules are addressed as a chain, and the chain is not
// row-major on the glass, so the only way to learn the layout is to light one LED at a time and
// look at the board. This build does that: it walks the chain, naming each index on the console,
// and takes serial commands to hold or place a single pixel.
//
//   {"action":"grid_hold","value":"4"}                     light chain index 4 and stop sweeping
//   {"action":"grid_pixel","value":{"index":0,"r":51}}     one pixel, explicit colour
//   {"action":"grid_cell","value":{"row":0,"col":2}}       one cell of the glass, via GRID_GLASS
//   {"action":"grid_sweep","value":"on"}                   resume the walk
//
// The map the sweep confirms lives in the board header, next to the wiring it belongs to.
#include <Arduino.h>
#include <ArduinoJson.h>

#include "modules/shared/atech_helpers.h"
#include "modules/connectivity/serial_templates.h"
#include <atech_board.h>

AtechSerial serialLink(115200);

static const uint32_t STEP_MS = 1500;
static bool sweeping = true;
static uint8_t chain = 0;
static uint32_t lastStep = 0;

static const char* CELL_NAME[9] = { "top-left", "top-centre", "top-right",
                                    "middle-left", "centre", "middle-right",
                                    "bottom-left", "bottom-centre", "bottom-right" };

/// Where the board header says chain index `i` shows up on each grid's glass, board upright.
/// The two differ: the slots hold the module 90 degrees apart.
static void glassNamesOfChain(uint8_t i, const char** on7, const char** on11) {
    *on7 = *on11 = "?";
    for (uint8_t cell = 0; cell < 9; cell++) {
        if (GRID_GLASS_PORT7[cell] == i)  *on7  = CELL_NAME[cell];
        if (GRID_GLASS_PORT11[cell] == i) *on11 = CELL_NAME[cell];
    }
}

static void showOnly(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    NeoPixelGrid* grids[2] = { &light_grid_7, &light_grid_11 };
    for (NeoPixelGrid* grid : grids) {
        grid->clear();
        if (index < NeoPixelGrid::NUM_LEDS) { grid->setPixel(index, r, g, b); }
        grid->show();
    }
}

static void handleMessage(const char* action, const char* value) {
    if (strcmp(action, "grid_sweep") == 0) {
        sweeping = strcmp(value, "off") != 0;
        Serial.printf("[grid] sweep %s\n", sweeping ? "on" : "off");
        return;
    }
    if (strcmp(action, "grid_hold") == 0) {
        sweeping = false; chain = (uint8_t) atoi(value);
        showOnly(chain, 51, 51, 51);
        const char *a, *b; glassNamesOfChain(chain, &a, &b);
        Serial.printf("[grid] chain %u held, expected port7 %s / port11 %s\n", chain, a, b);
        return;
    }
    if (strcmp(action, "grid_pixel") == 0 || strcmp(action, "grid_cell") == 0) {
        JsonDocument doc;
        if (deserializeJson(doc, value)) { Serial.println("[grid] value is not an object"); return; }
        uint8_t r = doc["r"] | 51, g = doc["g"] | 51, b = doc["b"] | 51;
        uint8_t index;
        if (strcmp(action, "grid_cell") == 0) {
            uint8_t row = doc["row"] | 0, col = doc["col"] | 0;
            index = GRID_GLASS_PORT7[(row % 3) * 3 + (col % 3)];
            Serial.printf("[grid] port7 glass (%u,%u) is chain %u\n", row, col, index);
        } else {
            index = doc["index"] | 0;
            const char *a, *b; glassNamesOfChain(index, &a, &b);
            Serial.printf("[grid] chain %u, expected port7 %s / port11 %s\n", index, a, b);
        }
        sweeping = false; chain = index;
        showOnly(index, r, g, b);
        return;
    }
    Serial.printf("[grid] unknown action %s\n", action);
}

void setup() {
    // AtechSerial::connect() is a no-op — the generated firmware starts Serial itself, so
    // an app must too. setTxTimeoutMs(0) keeps prints from blocking when no host is attached.
    Serial.setRxBufferSize(8192);
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    serialLink.connect();
    serialLink.onMessage(handleMessage);
    light_grid_7.begin(); light_grid_11.begin();
    light_grid_7.clear(); light_grid_11.clear();
    light_grid_7.show(); light_grid_11.show();
    Serial.println("[grid] Light Grid self-test: one chain LED at a time, both modules");
    Serial.println("[grid] hold the motherboard upright, USB-C at the bottom, and read both grids");
}

void loop() {
    serialLink.maintain();
    if (sweeping && millis() - lastStep >= STEP_MS) {
        lastStep = millis();
        showOnly(chain, 51, 51, 51);
        const char *a, *b; glassNamesOfChain(chain, &a, &b);
        Serial.printf("[grid] chain %u lit, expected port7 %s / port11 %s\n", chain, a, b);
        chain = (chain + 1) % NeoPixelGrid::NUM_LEDS;
    }
    delay(5);
}
