#pragma once

#include <Arduino.h>

namespace display::comm {

class UARTLink {
public:
    static void begin(HardwareSerial& serial, uint32_t baudrate);
    static void process();
    static bool sendCommand(uint8_t cmdType, const void* payload = nullptr, uint16_t length = 0);
    static void requestConfigDump();

private:
    static bool receiveMessage(uint8_t& msgType, uint8_t* payload, uint16_t& length);
    static bool sendMessage(uint8_t msgType, const uint8_t* payload, uint16_t length);
    static uint16_t calculateCRC16(const uint8_t* data, uint16_t length, uint16_t crc = 0xFFFF);
    static void handleMessage(uint8_t msgType, const uint8_t* payload, uint16_t length);
};

}  // namespace display::comm
