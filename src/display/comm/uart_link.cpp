#include "uart_link.h"

#include <gui_protocol.h>
#include <edrum_config.h>

#include "../ui/ui_manager.h"
#include "../drivers/ring_led_controller.h"
#include "link_state.h"

namespace display::comm {
namespace {
HardwareSerial* linkSerial = nullptr;
uint8_t rxBuffer[UART_MAX_PAYLOAD];

void handleConfigJsonPayload(const uint8_t* payload, uint16_t length) {
    if (!payload || length == 0) {
        return;
    }

    String json;
    json.reserve(length + 1);
    for (uint16_t i = 0; i < length; ++i) {
        json += static_cast<char>(payload[i]);
    }

    LinkState::updateConfigJSON(json.c_str());
}
}  // namespace

void UARTLink::begin(HardwareSerial& serial, uint32_t baudrate) {
    linkSerial = &serial;
    linkSerial->end();
    linkSerial->setRxBufferSize(4096);
    linkSerial->begin(baudrate, SERIAL_8N1, UART_RX_DISPLAY, UART_TX_DISPLAY);
    LinkState::init();
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

bool UARTLink::sendMessage(uint8_t msgType, const uint8_t* payload, uint16_t length) {
    if (!linkSerial || length > UART_MAX_PAYLOAD) {
        return false;
    }

    uint8_t header[4];
    header[0] = UART_START_BYTE;
    header[1] = msgType;
    header[2] = (length >> 8) & 0xFF;
    header[3] = length & 0xFF;

    uint16_t crc = calculateCRC16(header, 4);
    if (length > 0 && payload) {
        crc = calculateCRC16(payload, length, crc);
    }

    linkSerial->write(header, 4);
    if (length > 0 && payload) {
        linkSerial->write(payload, length);
    }
    linkSerial->write((crc >> 8) & 0xFF);
    linkSerial->write(crc & 0xFF);
    return true;
}

bool UARTLink::sendCommand(uint8_t cmdType, const void* payload, uint16_t length) {
    const uint8_t* rawPayload = static_cast<const uint8_t*>(payload);
    if (length > 0 && payload == nullptr) {
        return false;
    }
    return sendMessage(cmdType, rawPayload, length);
}

void UARTLink::requestConfigDump() {
    if (sendCommand(CMD_GET_CONFIG)) {
        Serial.println("[UART] Requested configuration dump from main brain");
    }
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

        case MSG_PAD_STATE:
            if (length == sizeof(PadStateMsg)) {
                const PadStateMsg* state = reinterpret_cast<const PadStateMsg*>(payload);
                LinkState::updatePadState(*state);
            }
            break;

        case MSG_SYSTEM_STATUS:
            if (length == sizeof(SystemStatusMsg)) {
                const SystemStatusMsg* status = reinterpret_cast<const SystemStatusMsg*>(payload);
                LinkState::updateSystemStatus(*status);
            }
            break;

        case MSG_CONFIG_UPDATE:
        case MSG_CONFIG_DUMP:
            handleConfigJsonPayload(payload, length);
            break;

        case MSG_CALIBRATION_DATA:
            if (length == sizeof(CalibrationDataMsg)) {
                const CalibrationDataMsg* data = reinterpret_cast<const CalibrationDataMsg*>(payload);
                Serial.printf("[UART] Calibration pad %u: baseline=%u noise=%u suggested=%u\n",
                              data->padId,
                              data->baseline,
                              data->noiseFloor,
                              data->suggestedThreshold);
            }
            break;

        case MSG_ACK:
            if (length >= 1) {
                Serial.printf("[UART] ACK for command 0x%02X\n", payload[0]);
            } else {
                Serial.println("[UART] ACK received");
            }
            break;

        case MSG_NACK:
            if (length >= 1) {
                const char* reason = (length > 1) ? reinterpret_cast<const char*>(&payload[1]) : "";
                Serial.printf("[UART] NACK for command 0x%02X: %s\n", payload[0], reason);
            } else {
                Serial.println("[UART] NACK received");
            }
            break;

        case MSG_MENU_STATE:
            if (length == sizeof(MenuStateMsg)) {
                const MenuStateMsg* menu = reinterpret_cast<const MenuStateMsg*>(payload);
                LinkState::updateMenuState(*menu);
                ui::UIManager::instance().onMenuState(*menu);
                Serial.printf("[UART] Menu state: %d, pad: %s, opt: %s\n",
                              menu->state, menu->padName, menu->optionName);
            }
            break;

        case MSG_MENU_SAMPLES:
            if (length == sizeof(SampleListMsg)) {
                const SampleListMsg* samples = reinterpret_cast<const SampleListMsg*>(payload);
                LinkState::updateSampleList(*samples);
                ui::UIManager::instance().onSampleList(*samples);
                Serial.printf("[UART] Sample list: %d samples, showing %d-%d\n",
                              samples->totalCount, samples->startIndex,
                              samples->startIndex + samples->count - 1);
            }
            break;

        default:
            Serial.printf("[UART] Unhandled message 0x%02X (len=%u)\n", msgType, length);
            break;
    }
}

}  // namespace display::comm
