#ifndef SERIALPACKET_H_
#define SERIALPACKET_H_

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

#endif /* SERIALPACKET_H_ */
