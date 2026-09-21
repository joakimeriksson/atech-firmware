#pragma once
#include <Arduino.h>

// A warm reset of the ESP32 — a reflash, the reset button, a watchdog — leaves the I2C modules
// powered, and one that was mid-transfer is still in that transfer when the new firmware makes
// its first START. Measured with the VL53L5CX in port 13, which is read in long bursts 15 times
// a second: 5 of 8 warm resets left it not answering, 0 of 10 with this in front of Wire.begin().
// SDA was never found held low in those ten, so it was the STOP that ended the transfer; the
// clock pulses are the I2C specification's remedy for the other case, a module part-way through
// sending a byte and holding SDA for the bit it has reached, which nine pulses always clear.
// Call it before the bus is started. Returns the pulses it took, 0 when SDA was free.
static inline int atechI2cRecover(int sda, int scl) {
    const int HALF_US = 10;                                  // 50 kHz: slow enough for any module
    pinMode(sda, INPUT_PULLUP);
    pinMode(scl, OUTPUT_OPEN_DRAIN);
    digitalWrite(scl, HIGH);
    delayMicroseconds(HALF_US);
    int pulses = 0;
    while (digitalRead(sda) == LOW && pulses < 18) {
        digitalWrite(scl, LOW);  delayMicroseconds(HALF_US);
        digitalWrite(scl, HIGH); delayMicroseconds(HALF_US);
        pulses++;
    }
    digitalWrite(scl, LOW);  delayMicroseconds(HALF_US);     // STOP: SDA rises while SCL is high
    pinMode(sda, OUTPUT_OPEN_DRAIN);
    digitalWrite(sda, LOW);  delayMicroseconds(HALF_US);
    digitalWrite(scl, HIGH); delayMicroseconds(HALF_US);
    digitalWrite(sda, HIGH); delayMicroseconds(HALF_US);
    pinMode(sda, INPUT); pinMode(scl, INPUT);                // hand the lines to the Wire driver
    if (pulses) { Serial.printf("[i2c] SDA on GPIO %d was held low after the reset: freed with %d clock pulses\n", sda, pulses); }
    return pulses;
}
