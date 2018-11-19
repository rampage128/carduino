#ifndef SERIAL_H_
#define SERIAL_H_

#include "serialpacket.h"
#include "can.h"

static SerialDataPacket<unsigned long> baudRatePacket(0x61, 0x02);

class SerialReader {
public:
	SerialReader(uint8_t size, Can * can) {
		_serialBuffer = new BinaryBuffer(size);
		this->can = can;
	}
	~SerialReader() {
		delete _serialBuffer;
	}
	void read(Stream &serial,
			void (*eventCallback)(uint8_t id, BinaryBuffer *payloadBuffer)) {
		int packetEndIndex = -1;
		while (serial.available() && _serialBuffer->available() > 0) {
			uint8_t data = serial.read();
			_serialBuffer->write(data);

			if (_packetStartIndex == -1 && data == 0x7b) {
				_packetStartIndex = _serialBuffer->getPosition();
				continue;
			}

			if (data == 0x7d) {
				packetEndIndex = _serialBuffer->getPosition();

				_serialBuffer->goTo(_packetStartIndex);
				uint8_t type = _serialBuffer->readByte().data;
				uint8_t id = _serialBuffer->readByte().data;

				BinaryBuffer *payloadBuffer;
				if (_serialBuffer->getPosition() < packetEndIndex) {
					uint8_t payloadLength = _serialBuffer->readByte().data;
					payloadBuffer = new BinaryBuffer(payloadLength);
					if (payloadBuffer->available() > 0
							&& packetEndIndex - _serialBuffer->getPosition()
									> 0) {
						payloadBuffer->write(_serialBuffer);
						payloadBuffer->goTo(0);
					}
				} else {
					payloadBuffer = new BinaryBuffer(0);
				}

				processSerialPacket(type, id, payloadBuffer, eventCallback);

				delete payloadBuffer;

				_serialBuffer->goTo(0);
				packetEndIndex = -1;
				_packetStartIndex = -1;
			}
		}
	}
private:
	Can * can;
	int _packetStartIndex = -1;
	BinaryBuffer *_serialBuffer;


	void processSerialPacket(uint8_t type, uint8_t id,
			BinaryBuffer *payloadBuffer,
			void (*eventCallback)(uint8_t id, BinaryBuffer *payloadBuffer)) {
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
			eventCallback(id, payloadBuffer);
			break;
		}
	}
};

#endif /* SERIAL_H_ */
