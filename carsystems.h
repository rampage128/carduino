#ifndef CARSYSTEMS_H_
#define CARSYSTEMS_H_

#include "network.h"

class CarData {
private:
    uint32_t canId;
    uint8_t * data;
    uint8_t mask;
    uint8_t length = 0;
public:
    CarData(uint32_t canId, uint8_t mask) {
        this->mask = mask;
        this->canId = canId;
        for (uint8_t i = 0; i < 8; i++) {
            if (mask & 1 << i) {
                this->length++;
            }
        }
        this->data = (uint8_t*) calloc(sizeof(char), this->length);
    }
    ~CarData() {
        free(this->data);
    }
    uint32_t getCanId() {
        return this->canId;
    }
    void setMask(uint8_t mask) {
        this->mask = mask;
    }
    boolean serialize(uint32_t canId, uint8_t canData[8], Stream * serial) {
        if (this->canId != canId) {
            return false;
        }

        bool dataChanged = false;
        uint8_t index = 0;
        for (uint8_t i = 0; i < 8; i++) {
            if (mask & 1 << (7 - i)) {
                uint8_t dataByte = canData[i];
                if (this->data[index] != dataByte) {
                    this->data[index] = dataByte;
                    dataChanged = true;
                }
                index++;
            }
        }

        if (dataChanged) {
            uint32_t flippedCanId = htonl(this->canId);
            serial->write("{");
            serial->write(0x62);
            serial->write(0x01);
            serial->write(this->length + 0x04);
            serial->write((byte*)&flippedCanId, sizeof(this->canId));
            for (uint8_t i = 0; i < this->length; i++) {
                serial->write(this->data[i]);
            }
            serial->write("}");
        }

        return dataChanged;
    }
};

#endif /* CARSYSTEMS_H_ */
