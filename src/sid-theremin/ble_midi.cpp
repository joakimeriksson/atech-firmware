#include "ble_midi.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace {
// the MIDI over Bluetooth LE specification: one service, one characteristic
const char* SERVICE_UUID = "03b80e5a-ede8-4b33-a751-6ce34ec4c700";
const char* CHARACTERISTIC_UUID = "7772e5db-3868-4112-a1a9-f2669d106bf3";

BleMidiPacket packetHandler = nullptr;
volatile bool connected = false;

class ServerEvents : public BLEServerCallbacks {
    void onConnect(BLEServer*) override { connected = true; }
    void onDisconnect(BLEServer*) override {
        connected = false;
        BLEDevice::startAdvertising();           // the stack stops advertising on a connection and does not resume
    }
};

class MidiEvents : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* characteristic) override {
        std::string value = characteristic->getValue();
        if (packetHandler && !value.empty()) { packetHandler((const uint8_t*) value.data(), value.size()); }
    }
};
}

bool bleMidiBegin(const char* name, BleMidiPacket onPacket) {
    packetHandler = onPacket;
    BLEDevice::init(name);
    BLEServer* server = BLEDevice::createServer();
    if (!server) { return false; }
    server->setCallbacks(new ServerEvents());
    BLEService* service = server->createService(BLEUUID(SERVICE_UUID));
    BLECharacteristic* midi = service->createCharacteristic(
        BLEUUID(CHARACTERISTIC_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY);
    midi->addDescriptor(new BLE2902());          // a host subscribes before it will use the port, though nothing is sent
    midi->setCallbacks(new MidiEvents());
    service->start();

    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);   // 128 bits: it fills the advertisement, so the name goes in the scan response
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);          // a 7.5 to 15 ms connection interval, which is what Apple's hosts give MIDI
    advertising->setMaxPreferred(0x0C);
    BLEDevice::startAdvertising();
    return true;
}

bool bleMidiConnected() { return connected; }
