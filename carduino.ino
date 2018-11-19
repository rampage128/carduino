#include "Arduino.h"
#include "network.h"
#include "carsystems.h"
#include "can.h"
#include "serial.h"
#include "370z.h"
#include <everytime.h>

Can can(&Serial, 2, 10);
SerialReader serialReader(128, &can);

NissanClimateControl nissanClimateControl;
NissanSteeringControl nissanSteeringControl(A6, A7);

//The setup function is called once at startup of the sketch
void setup() {
	Serial.begin(115200);

	can.setup(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
}

// The loop function is called in an endless loop
void loop() {
	can.beginTransaction();
	can.updateFromCan<DriveTrain>(0x421, driveTrain, updateDriveTrain);
	can.updateFromCan<ClimateControl>(0x54A, climateControl,
			updateClimateControl);
	can.updateFromCan<ClimateControl>(0x54B, climateControl,
			updateClimateControl);
	can.updateFromCan<ClimateControl>(0x54C, climateControl,
			updateClimateControl);
	can.updateFromCan<ClimateControl>(0x625, climateControl,
			updateClimateControl);
	can.endTransaction();

	nissanSteeringControl.check(&Serial);

	every(250) {
		nissanClimateControl.broadcast(can);
	}

	every(1000)
	{
		climateControl->serialize(&Serial);
		driveTrain->serialize(&Serial);
	}
}

void serialEvent() {
	serialReader.read(Serial, onCarduinoSerialEvent);
}

void onCarduinoSerialEvent(uint8_t eventId, BinaryBuffer *payloadBuffer) {
	nissanClimateControl.push(eventId, payloadBuffer);
}

void updateClimateControl(long unsigned int id, unsigned char len,
		unsigned char data[8], ClimateControl * climateControl) {
	switch (id) {
	// AC AUTO AMP 1
	case (0x54A):
		climateControl->desiredTemperature = data[4];
		break;
		// AC AUTO AMP 2
	case (0x54B):
		climateControl->fanLevel = (data[4] - 4) / 8;

		switch (data[2]) {
		case (0x80):
			climateControl->isAirductWindshield = 0;
			climateControl->isAirductFace = 0;
			climateControl->isAirductFeet = 0;
			break;
		case (0x88):
			climateControl->isAirductWindshield = 0;
			climateControl->isAirductFace = 1;
			climateControl->isAirductFeet = 0;
			break;
		case (0x90):
			climateControl->isAirductWindshield = 0;
			climateControl->isAirductFace = 1;
			climateControl->isAirductFeet = 1;
			break;
		case (0x98):
			climateControl->isAirductWindshield = 0;
			climateControl->isAirductFace = 0;
			climateControl->isAirductFeet = 1;
			break;
		case (0xA0):
			climateControl->isAirductWindshield = 1;
			climateControl->isAirductFace = 0;
			climateControl->isAirductFeet = 1;
			break;
		case (0xA8):
			climateControl->isAirductWindshield = 1;
			climateControl->isAirductFace = 0;
			climateControl->isAirductFeet = 0;
			break;
		}

		climateControl->isWindshieldHeating =
				(data[3] & B10000000) == B10000000;
		climateControl->isRecirculation = (data[3] & B00000011) == B00000001;
		climateControl->isAuto = (data[1] & B00001110) == B00000110;
		break;
		// AC AUTO AMP 3
	case (0x54C):
		climateControl->isAcOn = (data[2] & B10000000) == B10000000;
		break;
		// AC AUTO AMP 4
	case (0x625):
		climateControl->isRearWindowHeating =
				(data[0] & B00000001) == B00000001;
		break;
	}
}

void updateDriveTrain(long unsigned int id, unsigned char len,
		unsigned char data[8], DriveTrain * driveTrain) {
	uint8_t gear = data[0];

	// gear is N (0) or R (-1)
	if (gear < 0x80) {
		driveTrain->gearNum = ((int8_t) gear / 8) - 3;
	}
	// gear is 1-6
	else {
		driveTrain->gearNum = ((int8_t) gear - 120) / 8;
	}

	driveTrain->isSynchroRev = (data[1] & B01000000) == B01000000;
}
