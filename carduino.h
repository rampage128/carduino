#ifndef CARDUINO_H_
#define CARDUINO_H_

#include <EEPROM.h>
#include "serial.h"
#include "can.h"
#include "power.h"

static SerialPacket baudRateReadError(0x65, 0x01);
static SerialPacket carDataReadError(0x65, 0x02);
static SerialPacket carDataFullError(0x65, 0x03);
static SerialPacket tooManyFeaturesError(0x65, 0x04);
static SerialPacket idChangeError(0x65, 0x05);

union CarduinoPing {
    unsigned char data[6] = { 0x01, 0x00, 0x00, 0x41, 0x41, 0x41 };
    BitFieldMember<0, 8> major;
    BitFieldMember<8, 8> minor;
    BitFieldMember<16, 8> revision;
    BitFieldMember<24, 8> type1;
    BitFieldMember<32, 8> type2;
    BitFieldMember<40, 8> type3;
};
static SerialDataPacket<CarduinoPing> ping(0x61, 0x00);

union CarduinoIdChange {
    unsigned char data[3] = { 0x00, 0x00, 0x00 };
    BitFieldMember<0, 8> type1;
    BitFieldMember<8, 8> type2;
    BitFieldMember<16, 8> type3;
};
static SerialDataPacket<CarduinoIdChange> idChange(0x61, 0x49);

static SerialPacket startup(0x61, 0x01);
static SerialDataPacket<unsigned long> baudRatePacket(0x61, 0x02);
static SerialPacket shutdown(0x61, 0x03);

class Carduino: public SerialListener {
private:
    SerialReader * serialReader;
    HardwareSerial * serial;
    Can * can = NULL;
    PowerManager * powerManager = NULL;
    bool isConnectedFlag = false;
    uint32_t lastSerialEvent = 0;
    void (*serialEvent)(uint8_t type, uint8_t id, BinaryBuffer *payloadBuffer) = NULL;
    void (*timeoutCallback)(void) = NULL;
public:
    Carduino(HardwareSerial * serial,
            void (*userEvent)(uint8_t type, uint8_t id, BinaryBuffer *payloadBuffer),
            void (*timeoutCallback)(void)) {
        this->serialReader = new SerialReader(128, serial);
        this->serialEvent = userEvent;
        this->timeoutCallback = timeoutCallback;
        this->serial = serial;

        ping.payload()->type1 = EEPROM.read(0);
        ping.payload()->type2 = EEPROM.read(1);
        ping.payload()->type3 = EEPROM.read(2);
    }
    ~Carduino() {
        delete this->serialReader;
        delete this->can;
    }
    bool update() {
        if (this->serial->available()) {
            this->lastSerialEvent = millis();
            this->serialReader->read(this);
        }

        if (!this->isConnectedFlag) {
            ping.serialize(this->serial, 500);
        } else {
            if (millis() - this->lastSerialEvent >= 1000) {
                this->isConnectedFlag = false;
                this->timeoutCallback();
            }
        }
        return this->isConnectedFlag;
    }
    void triggerEvent(uint8_t eventNum) {
        SerialPacket carduinoEvent(0x63, eventNum);
        carduinoEvent.serialize(this->serial);
    }
    void addCan(Can * can) {
        this->can = can;
    }
    void addPowerManager(PowerManager * powerManager) {
        this->powerManager = powerManager;
    }
    void begin() {
        this->serial->begin(115200);
        delay(1000);
    }
    void end() {
        this->triggerEvent(2);
        this->serial->flush();
        delay(500);
        shutdown.serialize(this->serial);
        this->serial->flush();
        this->serial->end();
        this->isConnectedFlag = false;
    }
    virtual void onSerialPacket(uint8_t type, uint8_t id,
            BinaryBuffer *payloadBuffer) {

        switch (type) {
        case 0x61:
            switch (id) {
            case 0x00: { // Connection request
                BinaryData::ByteResult majorVersionResult =
                        payloadBuffer->readByte();
                if (majorVersionResult.state == BinaryData::OK
                        && majorVersionResult.data == ping.payload()->major) {
                    this->isConnectedFlag = true;
                    startup.serialize(this->serial);
                    this->triggerEvent(1);
                }
                break;
            }
            case 0x0a: // start sniffer
                if (this->can) {
                    this->can->startSniffer();
                }
                break;
            case 0x0b: // stop sniffer
                if (this->can) {
                    this->can->stopSniffer();
                }
                break;
            case 0x49: {
                BinaryData::ByteResult type1 = payloadBuffer->readByte();
                BinaryData::ByteResult type2 = payloadBuffer->readByte();
                BinaryData::ByteResult type3 = payloadBuffer->readByte();
                if (type1.state == BinaryData::OK
                        && type2.state == BinaryData::OK
                        && type3.state == BinaryData::OK) {
                    EEPROM.update(0, type1.data);
                    EEPROM.update(1, type2.data);
                    EEPROM.update(2, type3.data);

                    ping.payload()->type1 = type1.data;
                    ping.payload()->type2 = type2.data;
                    ping.payload()->type3 = type3.data;

                    idChange.payload()->type1 = type1.data;
                    idChange.payload()->type2 = type2.data;
                    idChange.payload()->type3 = type3.data;
                    idChange.serialize(this->serial, 0);
                } else {
                    idChangeError.serialize(this->serial, 0);
                }
                break;
            }
            case 0x63: {
                BinaryData::LongResult canIdResult = payloadBuffer->readLong();
                BinaryData::ByteResult maskResult = payloadBuffer->readByte();
                if (canIdResult.state == BinaryData::OK
                        && maskResult.state == BinaryData::OK) {
                    if (this->can) {
                        if (!this->can->addCanPacket(canIdResult.data,
                                maskResult.data)) {
                            carDataFullError.serialize(this->serial);
                            ;
                        }
                    }
                } else {
                    carDataReadError.serialize(this->serial);
                }
                break;
            }
            case 0x72: { // set baud rate
                BinaryData::LongResult result = payloadBuffer->readLong();
                if (result.state == BinaryData::OK) {
                    baudRatePacket.payload(htonl(result.data));
                    baudRatePacket.serialize(&Serial);
                    this->serial->flush();
                    this->serial->end();
                    this->serial->begin(result.data);
                } else {
                    baudRateReadError.serialize(this->serial);
                }
                break;
            }
            }
            break;
        default:
            if (this->serialEvent) {
                this->serialEvent(type, id, payloadBuffer);
            }
            break;
        }
    }
};

#endif /* CARDUINO_H_ */
