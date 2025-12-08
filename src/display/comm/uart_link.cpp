#include "uart_link.h"

#include <gui_protocol.h>
#include <edrum_config.h>

#include "../ui/ui_manager.h"
#include "../drivers/ring_led_controller.h"

namespace display::comm {
namespace {
HardwareSerial* linkSerial = nullptr;
uint8_t rxBuffer[UART_MAX_PAYLOAD];
}  // namespace

void UARTLink::begin(HardwareSerial& serial, uint32_t baudrate) {
    linkSerial = &serial;
    linkSerial->end();
    linkSerial->setRxBufferSize(1024);
    linkSerial->begin(baudrate, SERIAL_8N1, UART_RX_DISPLAY, UART_TX_DISPLAY);
    Serial.printf("[UART] Display link up @ %lu baud\n", static_cast<unsigned long>(baudrate));
}

void UARTLink::process() {
    if (!linkSerial) {
        return;
    }

    while (linkSerial->available() >= 4) {
        uint8_t msgType = 0;
        uint16_t length = 0;

        if (!receiveMessage(msgType, rxBuffer, length)) {
            break;
        }

        handleMessage(msgType, rxBuffer, length);
    }
}

bool UARTLink::receiveMessage(uint8_t& msgType, uint8_t* payload, uint16_t& length) {
    if (!linkSerial) {
        return false;
    }

    uint32_t start = millis();
    while (linkSerial->available() > 0 && linkSerial->peek() != UART_START_BYTE) {
        linkSerial->read();
        if (millis() - start > UART_TIMEOUT_MS) {
            return false;
        }
    }

    if (linkSerial->available() < 4) {
        return false;
    }

    uint8_t header[4];
    linkSerial->readBytes(header, 4);

    if (header[0] != UART_START_BYTE) {
        return false;
    }

    msgType = header[1];
    length = static_cast<uint16_t>(header[2] << 8) | header[3];
    if (length > UART_MAX_PAYLOAD) {
        return false;
    }

    start = millis();
    while (linkSerial->available() < length + 2) {
        if (millis() - start > UART_TIMEOUT_MS) {
            return false;
        }
    }

    linkSerial->readBytes(payload, length);
    uint8_t crcHi = linkSerial->read();
    uint8_t crcLo = linkSerial->read();
    uint16_t frameCrc = (static_cast<uint16_t>(crcHi) << 8) | crcLo;

    uint16_t calcCrc = calculateCRC16(header, 4);
    calcCrc = calculateCRC16(payload, length, calcCrc);

    if (frameCrc != calcCrc) {
        Serial.println("[UART] CRC mismatch from main brain");
        return false;
    }

    return true;
}

uint16_t UARTLink::calculateCRC16(const uint8_t* data, uint16_t length, uint16_t crc) {
    while (length--) {
        crc ^= *data++ << 8;
        for (uint8_t i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void UARTLink::handleMessage(uint8_t msgType, const uint8_t* payload, uint16_t length) {
    switch (msgType) {
        case MSG_HIT_EVENT:
            if (length == sizeof(HitEventMsg)) {
                const HitEventMsg* msg = reinterpret_cast<const HitEventMsg*>(payload);
                ui::UIManager::instance().onPadHit(msg->padId, msg->velocity);
                RingLEDController::pulsePad(msg->padId, msg->velocity);
            }
            break;

        default:
            break;
    }
}

}  // namespace display::comm
