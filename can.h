#ifndef CAN_H_
#define CAN_H_

#include <mcp_can.h>
#include "bitfield.h"

struct CanData {
	union {
		unsigned char data[4];
		BitFieldMember<0, 32> canId;
	} metaData;
	uint8_t rxBuf[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
};

class Can {
private:
	uint8_t _canInterruptPin = 2;
	boolean _isInitialized = false;
	MCP_CAN* _can;
	boolean _sniff = false;
	SerialDataPacket<CanData> *_snifferPacket;
public:
	Can(uint8_t canInterruptPin, uint8_t canCsPin) {
		_can = new MCP_CAN(canCsPin);
		_canInterruptPin = canInterruptPin;
		_snifferPacket = new SerialDataPacket<CanData>(0x62, 0x6d);
	}
	~Can() {
		delete _can;
	}
	boolean setup(uint8_t mode, uint8_t speed, uint8_t clock) {
		uint8_t canStatus = _can->begin(mode, speed, clock);
		if (canStatus == CAN_OK) {
			_can->setMode(MCP_NORMAL);
			pinMode(_canInterruptPin, INPUT);
			_isInitialized = true;
		}
		return _isInitialized;
	}
	void update(Stream &serial,
			void (*callback)(long unsigned int id, unsigned char len,
					unsigned char rxBuf[8])) {
		if (!_isInitialized) {
			return;
		}
		if (!digitalRead(_canInterruptPin)) {
			long unsigned int id = 0;
			unsigned char len = 0;
			unsigned char rxBuf[8];

			_can->readMsgBuf(&id, &len, rxBuf);

			if (_sniff) {
				_snifferPacket->payload()->metaData.canId = id;
				for (uint8_t i = 0; i < 8; i++) {
					uint8_t data = 0x00;
					if (i < len) {
						 data = rxBuf[i];
					}
					_snifferPacket->payload()->rxBuf[i] = data;
				}
				_snifferPacket->serialize(serial);
			}
			else {
				callback(id, len, rxBuf);
			}
		}
	}
	void write(INT32U id, INT8U ext, INT8U len, INT8U *buf) {
		_can->sendMsgBuf(id, ext, len, buf);
	}
	void startSniffer() {
		_sniff = true;
	}
	void stopSniffer() {
		_sniff = false;
	}
};

#endif /* CAN_H_ */
