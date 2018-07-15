#include "Arduino.h"
#include "network.h"
#include "carsystems.h"
#include "serialpacket.h"
#include "can.h"
#include "370z.h"
#include <everytime.h>

SerialPacket statusInitSuccess(0x61, 0x01);
SerialPacket statusInitError(0x65, 0x01);
SerialDataPacket<unsigned long> baudRateChange(0x65, 0x01);

SerialDataPacket<ClimateControl> climateControl(0x73, 0x63);
SerialDataPacket<DriveTrain> gearBox(0x73, 0x67);

Can can(2, 10);
bool canSniffing = false;
SerialReader serialReader(128);

NissanClimateControl nissanClimateControl;
NissanSteeringControl nissanSteeringControl(A6, A7);

//The setup function is called once at startup of the sketch
void setup() {
	Serial.begin(115200);

	if (can.setup(MCP_ANY, CAN_500KBPS, MCP_8MHZ)) {
		statusInitSuccess.serialize(Serial);
	}
	// Notify serial on error
	else {
		statusInitError.serialize(Serial);
	}
}

// The loop function is called in an endless loop
void loop() {
	can.update(Serial, readCan);
	nissanSteeringControl.check(Serial);

	every(250) {
		climateControl.serialize(Serial);
		nissanClimateControl.broadcast(can);
	}

	every(1000) {
		gearBox.serialize(Serial);
	}
}

void serialEvent() {
	serialReader.read(Serial, readSerial);
}

void readSerial(uint8_t type, uint8_t id, BinaryBuffer *payloadBuffer) {
	switch (type) {
	case 0x61:
		switch (id) {
			case 0x0a: // start sniffer
				can.startSniffer();
				break;
			case 0x0b: // stop sniffer
				can.stopSniffer();
				break;
			case 0x72: { // set baud rate
				BinaryData::LongResult result = payloadBuffer->readLong();
				if (result.state == BinaryData::OK) {
					baudRateChange.payload(htonl(result.data));
					baudRateChange.serialize(Serial);
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
		nissanClimateControl.push(id, payloadBuffer);
		break;
	}
}

void readCan(long unsigned int id, unsigned char len, unsigned char rxBuf[8]) {
	nissanClimateControl.read(climateControl, id, len, rxBuf);
	switch(id) {
			// GEARBOX
		case (0x421): {
			uint8_t gear = rxBuf[0];

			// gear is N (0) or R (-1)
			if (gear < 0x80) {
				gearBox.payload()->gearNum = ((int8_t) gear / 8) - 3;
			}
			// gear is 1-6
			else {
				gearBox.payload()->gearNum = ((int8_t) gear - 120) / 8;
			}

			gearBox.payload()->isSynchroRev = (rxBuf[0] & B01000000)
					== B01000000;
			break;
		}
	}
}
