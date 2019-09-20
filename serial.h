#ifndef SERIAL_H_
#define SERIAL_H_

#include "serialpacket.h"

class SerialListener {
public:
    virtual ~SerialListener() {
    }
    virtual void onSerialPacket(uint8_t type, uint8_t id,
            BinaryBuffer *payloadBuffer) = 0;
};

class SerialReader {
private:
    int packetStartIndex = -1;
    BinaryBuffer *serialBuffer;
    Stream * serial;
public:
    SerialReader(uint8_t size, Stream * serial) {
        this->serialBuffer = new BinaryBuffer(size);
        this->serial = serial;
    }
    ~SerialReader() {
        delete this->serialBuffer;
    }
    void read(SerialListener * listener) {
        int packetEndIndex = -1;
        while (this->serial->available() && this->serialBuffer->available() > 0) {
            uint8_t data = this->serial->read();
            this->serialBuffer->write(data);

            if (this->packetStartIndex == -1 && data == 0x7b) {
                this->packetStartIndex = this->serialBuffer->getPosition();
                continue;
            }

            if (data == 0x7d) {
                packetEndIndex = this->serialBuffer->getPosition();

                this->serialBuffer->goTo(this->packetStartIndex);
                uint8_t type = this->serialBuffer->readByte().data;
                uint8_t id = this->serialBuffer->readByte().data;

                BinaryBuffer *payloadBuffer;
                if (this->serialBuffer->getPosition() < packetEndIndex) {
                    uint8_t payloadLength = this->serialBuffer->readByte().data;
                    payloadBuffer = new BinaryBuffer(payloadLength);
                    if (payloadBuffer->available() > 0
                            && packetEndIndex
                                    - this->serialBuffer->getPosition() > 0) {
                        payloadBuffer->write(this->serialBuffer);
                        payloadBuffer->goTo(0);
                    }
                } else {
                    payloadBuffer = new BinaryBuffer(0);
                }

                listener->onSerialPacket(type, id, payloadBuffer);

                delete payloadBuffer;

                this->serialBuffer->goTo(0);
                packetEndIndex = -1;
                this->packetStartIndex = -1;
            }
        }
    }
};

#endif /* SERIAL_H_ */
