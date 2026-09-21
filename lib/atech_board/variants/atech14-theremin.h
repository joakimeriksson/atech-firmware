// Generated from atech14-theremin.yaml by tools/gen-board-header.py — do not edit above the
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
//   port 13    Distance Sensor (VL53L5CX 8x8 ToF) GPIO 13:39/38
//   port 14    6-Axis IMU (ICM-40608 / LSM6DSOX)  GPIO 14:36/35

#include "modules/audio/speaker.h"
#include "modules/display/st7735_tft.h"
#include "modules/input/rotary_encoder.h"
#include "modules/input/button.h"
#include "modules/led/neopixel.h"
#include "modules/sensor/icm40608.h"
#include "modules/sensor/vl53l5cx.h"
#include "modules/shared/i2c_recover.h"

RotaryEncoder rotary_encoder_1(5, 4, 9, 8);
static const int rotary_encoder_1_pin_clk = 5, rotary_encoder_1_pin_dt = 4;
ButtonModule button_1(17);
ButtonModule button_2(16);
Speaker speaker_1(12, 13, 10);
NeoPixelGrid light_grid_7(6, 7);
ST7735_TFT st7735_tft_1(2, 41, 1, 40);
NeoPixelGrid light_grid_11(43, 44);
VL53L5CX_Sensor distance_sensor_1(Wire);
WireI2C imu_1_bus(Wire1, 36, 35);
ICM40608 imu_1(&imu_1_bus);

/// Start distance_sensor_1 on Wire: free the bus, then what the SDK's setup template does.
static inline void distance_sensor_1_setup() {
    atechI2cRecover(39, 38);
    Wire.begin(39, 38);
    Wire.setClock(400000);
    distance_sensor_1.begin();
}
/// Start imu_1 on Wire1: free the bus, then what the SDK's setup template does.
static inline void imu_1_setup() {
    atechI2cRecover(36, 35);
    imu_1_bus.begin(400000);
    imu_1.begin();
}
// ---- measured, not generated ---------------------------------------------------
// The Light Grids in ports 7 and 11: chain-to-glass maps, measured with grid-selftest.
#include "measured/light-grid-v11-ports-7-11.h"

// ---- The IMU in port 14 -------------------------------------------------------------------
// SDK 1.0.0a6 codegen does not call setMountRotation(), so imu_1 reports chip-frame axes, and
// which way those point on this slot is a fact about the board, like the grid maps above. With
// the board flat in the hand and USB-C towards the player, an app wants two numbers: how far
// the far edge is tilted away (down), and how far the board is tilted to the right.
//
//   IMU_SWAP_PITCH_ROLL   1 if tilting away moves the driver's getRoll() rather than getPitch()
//   IMU_AWAY_SIGN         the sign that makes tilting away positive
//   IMU_RIGHT_SIGN        the sign that makes tilting right positive
//
// Not measured yet: these are the chip-frame defaults. The sid-theremin build shows the tilt as
// a dot on the screen and the grids, so a wrong value is visible at once — fix it here.
#define IMU_SWAP_PITCH_ROLL 0
#define IMU_AWAY_SIGN  (+1.0f)
#define IMU_RIGHT_SIGN (+1.0f)
