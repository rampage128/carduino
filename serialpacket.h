#ifndef SERIALPACKET_H
#define SERIALPACKET_H

#include "binarydata.h"

template<typename T>
class SerialDataPacket {
public:
	SerialDataPacket(uint8_t type, uint8_t id) {
		_id = id;
		_type = type;
	}

	void serialize(Stream * serial) {
		serial->write("{");
		serial->write(_type);
		serial->write(_id);
		if (sizeof(_payload) > 0) {
			serial->write(sizeof(_payload));
			serial->write((byte*) &_payload, sizeof(_payload));
		}
		serial->write("}");
	}

	T *payload() {
		return &_payload;
	}

	void payload(T newPayload) {
		_payload = newPayload;
	}
private:
	uint8_t _id;
	uint8_t _type;
	T _payload;
};

typedef SerialDataPacket<int[0]> SerialPacket;

class SerialReader {
public:
	SerialReader(uint8_t size) {
		_serialBuffer = new BinaryBuffer(size);
	}
	~SerialReader() {
		delete _serialBuffer;
	}
	void read(Stream &serial,
			void (*callback)(uint8_t type, uint8_t id, BinaryBuffer *payloadBuffer)) {
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
					if (payloadBuffer->available() > 0 && packetEndIndex - _serialBuffer->getPosition() > 0) {
						payloadBuffer->write(_serialBuffer);
						payloadBuffer->goTo(0);
					}
				}
				else {
					payloadBuffer = new BinaryBuffer(0);
				}

				callback(type, id, payloadBuffer);

				delete payloadBuffer;

				_serialBuffer->goTo(0);
				packetEndIndex = -1;
				_packetStartIndex = -1;
			}
		}
	}
private:
	int _packetStartIndex = -1;
	BinaryBuffer *_serialBuffer;
};

#endif
