#ifndef CARDUINO_H_
#define CARDUINO_H_

#include "serial.h"
#include "can.h"

union CarduinoEvent {
	unsigned char data[1] = { 0x00 };
	BitFieldMember<0, 8> eventNum;
};
static SerialDataPacket<CarduinoEvent> carduinoEvent (0x73, 0x72);

class Carduino: public SerialListener {
private:
	SerialReader * serialReader;
	Stream * serial;
	Can * can = NULL;
	void (*serialEvent)(uint8_t eventId, BinaryBuffer *payloadBuffer) = NULL;
public:
	Carduino(Stream * serial,
			void (*userEvent)(uint8_t eventId, BinaryBuffer *payloadBuffer)) {
		this->serialReader = new SerialReader(128, serial);
		this->serialEvent = userEvent;
		this->serial = serial;
	}
	~Carduino() {
		delete this->serialReader;
		delete this->can;
	}
	void readSerial() {
		this->serialReader->read(this);
	}
	void triggerEvent(uint8_t eventNum) {
		carduinoEvent.payload()->eventNum = eventNum;
		carduinoEvent.serialize(this->serial);
	}
	void addCan(Can * can) {
		this->can = can;
	}
	virtual void onSerialPacket(uint8_t type, uint8_t id,
			BinaryBuffer *payloadBuffer) {

		switch (type) {
		case 0x61:
			switch (id) {
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
			case 0x72: { // set baud rate
				BinaryData::LongResult result = payloadBuffer->readLong();
				if (result.state == BinaryData::OK) {
					baudRatePacket.payload(htonl(result.data));
					baudRatePacket.serialize(&Serial);
					Serial.flush();
					Serial.end();
					Serial.begin(result.data);
					while (Serial.available()) {
						Serial.read();
					}
				}
				break;
			}
			}
			break;
		case 0x63:
			if (this->serialEvent) {
				this->serialEvent(id, payloadBuffer);
			}
			break;
		}
	}
};

#endif /* CARDUINO_H_ */
