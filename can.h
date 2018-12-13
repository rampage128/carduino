#ifndef CAN_H_
#define CAN_H_

#include <mcp_can.h>
#include "bitfield.h"
#include "serialpacket.h"

static SerialPacket statusInitSuccess(0x61, 0x30);
static SerialPacket statusInitError(0x65, 0x30);

static SerialPacket canNotInitializedError(0x65, 0x31);
static SerialPacket canTransactionError(0x65, 0x32);

static SerialPacket canSendBufferFull(0x65, 0x33);
static SerialPacket canSendTimeout(0x65, 0x34);

static SerialPacket canControlError(0x65, 0x35);

struct CanData {
	union {
		unsigned char data[4];
		BitFieldMember<0, 32> canId;
	} header;
	uint8_t data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
};
static SerialDataPacket<CanData> snifferPacket(0x62, 0x6d);

class Can {
public:
	Can(Stream * serial, uint8_t canInterruptPin, uint8_t canCsPin) {
		this->serial = serial;
		this->can = new MCP_CAN(canCsPin);
		this->canInterruptPin = canInterruptPin;
	}
	~Can() {
		delete this->can;
	}
	boolean setup(uint8_t mode, uint8_t speed, uint8_t clock) {
		uint8_t canStatus = this->can->begin(mode, speed, clock);
		if (canStatus == CAN_OK) {
			this->can->setMode(MCP_NORMAL);
			pinMode(this->canInterruptPin, INPUT);
			this->isInitialized = true;
			statusInitSuccess.serialize(&Serial);
		} else {
			statusInitError.serialize(&Serial);
		}
		return this->isInitialized;
	}

	void beginTransaction() {
		if (!this->isInitialized) {
			canNotInitializedError.serialize(this->serial);
			return;
		}

		if (this->inTransaction) {
			canTransactionError.serialize(this->serial);
			return;
		}

		if (!digitalRead(canInterruptPin)) {
			this->inTransaction = true;

			this->can->readMsgBuf(&this->currentCanPacketId,
					&this->currentCanPacketLength, this->currentCanPacketData);
			if (this->isSniffing) {
				this->sniff();
			}
		}
	}

	void endTransaction() {
		if (this->can->checkError() == CAN_CTRLERROR) {
			canControlError.serialize(this->serial);
		}
		this->inTransaction = false;
	}

	void startSniffer() {
		this->isSniffing = true;
	}

	void stopSniffer() {
		this->isSniffing = false;
	}

	template<typename T>
	void updateFromCan(long unsigned int canId, SerialDataPacket<T> * packet,
			void (*callback)(long unsigned int id, unsigned char len,
					unsigned char data[8], T * carSystem)) {
		if (!this->inTransaction) {
			/*
			 * Intentionally return silently.
			 * beginTransaction only starts a transaction if a can-packet was
			 * received.
			 */
			return;
		}

		if (canId == this->currentCanPacketId) {
			int dataLength = sizeof(T);
			T * carSystem = new T();
			memcpy(carSystem, packet->payload(), dataLength);

			callback(this->currentCanPacketId, this->currentCanPacketLength,
					this->currentCanPacketData, carSystem);

			if (memcmp(carSystem, packet->payload(), dataLength) != 0) {
				memcpy(packet->payload(), carSystem, dataLength);
				packet->serialize(this->serial);
			}

			delete carSystem;
		}
	}

	template <uint8_t BYTE_INDEX, uint8_t BIT_MASK, uint8_t COMPARE_VALUE>
	static bool readFlag(uint8_t * data) {
		return (data[BYTE_INDEX] & BIT_MASK) == COMPARE_VALUE;
	}
	template <uint8_t BYTE_INDEX, uint8_t BIT_MASK>
	static inline bool readFlag(uint8_t * data) {
		return Can::readFlag<BYTE_INDEX, BIT_MASK, BIT_MASK>(data);
	}

	void write(INT32U id, INT8U ext, INT8U len, INT8U *buf) {
		uint8_t result = this->can->sendMsgBuf(id, ext, len, buf);
		if (result == CAN_GETTXBFTIMEOUT) {
			canSendBufferFull.serialize(serial);
		} else if (result == CAN_SENDMSGTIMEOUT) {
			canSendTimeout.serialize(serial);
		}
	}
private:
	MCP_CAN * can;
	Stream * serial;

	uint8_t canInterruptPin = 2;
	boolean isInitialized = false;
	boolean isSniffing = false;

	bool inTransaction = false;
	long unsigned int currentCanPacketId = 0;
	uint8_t currentCanPacketLength = 0;
	unsigned char currentCanPacketData[8];

	void sniff() {
		snifferPacket.payload()->header.canId = this->currentCanPacketId;
		for (uint8_t i = 0; i < 8; i++) {
			uint8_t data = 0x00;
			if (i < this->currentCanPacketLength) {
				data = this->currentCanPacketData[i];
			}
			snifferPacket.payload()->data[i] = data;
		}
		snifferPacket.serialize(this->serial);
	}
};

#endif /* CAN_H_ */
