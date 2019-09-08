#ifndef CARDUINO_H_
#define CARDUINO_H_

#include "serial.h"
#include "can.h"
#include "power.h"

static SerialPacket baudRateReadError(0x65, 0x01);

union CarduinoProtocolVersion {
	unsigned char data[3] = { 0x01, 0x00, 0x00 };
	BitFieldMember<0, 8> major;
	BitFieldMember<8, 16> minor;
	BitFieldMember<16, 24> revision;
};
static SerialDataPacket<CarduinoProtocolVersion> ping(0x61, 0x00);

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
	void (*serialEvent)(uint8_t eventId, BinaryBuffer *payloadBuffer) = NULL;
public:
	Carduino(HardwareSerial * serial,
			void (*userEvent)(uint8_t eventId, BinaryBuffer *payloadBuffer)) {
		this->serialReader = new SerialReader(128, serial);
		this->serialEvent = userEvent;
		this->serial = serial;
	}
	~Carduino() {
		delete this->serialReader;
		delete this->can;
	}
	bool isConnected() {
		if (!this->isConnectedFlag) {
			ping.serialize(this->serial, 500);
		}
		return this->isConnectedFlag;
	}
	void readSerial() {
		this->serialReader->read(this);
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
			case 0x00: // Connection request
				this->isConnectedFlag = true;
				startup.serialize(this->serial);
				break;
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
		case 0x63:
			if (this->serialEvent) {
				this->serialEvent(id, payloadBuffer);
			}
			break;
		}
	}
};

#endif /* CARDUINO_H_ */
