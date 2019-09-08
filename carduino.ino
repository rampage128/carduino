#include "Arduino.h"
#include "timer.h"
#include "network.h"
#include "carsystems.h"
#include "power.h"
#include "carduino.h"
#include "370z.h"
#include <everytime.h>

#define UNUSED(x) (void)(x)
#define ONE_SECOND 1000UL
#define ONE_MINUTE ONE_SECOND * 60

Can can(&Serial, 5, 6);
PowerManager powerManager(&Serial, 3, 4);
Carduino carduino(&Serial, onCarduinoSerialEvent);

NissanClimateControl nissanClimateControl;
NissanSteeringControl nissanSteeringControl(A0, A1);

Timer sleepTimer;

bool wasDriverDoorOpened = false;

void setup() {
	carduino.begin();

	powerManager.setup();
	carduino.addCan(&can);
	carduino.addPowerManager(&powerManager);
	can.setup(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
	carduino.triggerEvent(1);
}

void loop() {
	powerManager.update<2, RISING, ONE_SECOND / 2, ONE_MINUTE * 15>(onSleep, onWakeUp, onLoop);
}

void serialEvent() {
	carduino.readSerial();
}

void onCarduinoSerialEvent(uint8_t eventId, BinaryBuffer *payloadBuffer) {
	nissanClimateControl.push(eventId, payloadBuffer);
}

void onLoop() {
	if (carduino.isConnected()) {
		can.beginTransaction();
		can.updateFromCan<PowerState>(0x60D, powerState, updatePowerState);
		can.updateFromCan<Doors>(0x60D, doors, updateDoors);
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

		nissanSteeringControl.check(&carduino);

		every(250) {
			nissanClimateControl.broadcast(can);
		}

		every(1000)
		{
			climateControl->serialize(&Serial);
			driveTrain->serialize(&Serial);
		}
	}
}

bool onSleep() {
	bool shouldSleep = false;

	// Check sleep conditions
	if (powerState->payload()->isAccessoryOn) {
		// Reset timer when ACC is on to prevent premature sleep
		sleepTimer.reset();
	} else {
		// should go to sleep, when ACC is off and driver door is opened
		// Or allow operation without ACC in the closed vehicle for 30 minutes
		shouldSleep = wasDriverDoorOpened || sleepTimer.check(ONE_MINUTE * 30);
	}

	if (shouldSleep) {
		// Reset driver door status, otherwise sleep is triggered after wake up
		wasDriverDoorOpened = false;
		carduino.triggerEvent(2);
		carduino.end();
	}

	return shouldSleep;
}

void onWakeUp() {
	sleepTimer.reset();
	carduino.begin();
	can.setup(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
	carduino.triggerEvent(3);
}

void updatePowerState(long unsigned int id, unsigned char len,
		unsigned char data[8], PowerState * powerState) {
	UNUSED(id);
	UNUSED(len);

	powerState->isAccessoryOn = Can::readFlag<1, B00000010>(data);
	powerState->isIgnitionOn = Can::readFlag<1, B00000100>(data);
}

void updateDoors(long unsigned int id, unsigned char len, unsigned char data[8],
		Doors * doors) {
	UNUSED(id);
	UNUSED(len);

	// Detect if driver door was physically opened (used as sleep condition)
	bool isFrontLeftOpen = Can::readFlag<0, B00001000>(data);
	if (isFrontLeftOpen && !doors->isFrontLeftOpen) {
		wasDriverDoorOpened = true;
	} else if (!isFrontLeftOpen) {
		wasDriverDoorOpened = false;
	}

	doors->isFrontLeftOpen = isFrontLeftOpen;
	doors->isFrontRightOpen = Can::readFlag<0, B00010000>(data);
	doors->isTrunkOpen = Can::readFlag<0, B10000000>(data);
}

void updateClimateControl(long unsigned int id, unsigned char len,
		unsigned char data[8], ClimateControl * climateControl) {
	UNUSED(len);

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

		climateControl->isWindshieldHeating = Can::readFlag<3, B10000000>(data);
		climateControl->isRecirculation =
				Can::readFlag<3, B00000011, B00000001>(data);
		climateControl->isAuto = Can::readFlag<1, B00001110, B00000110>(data);
		break;
		// AC AUTO AMP 3
	case (0x54C):
		climateControl->isAcOn = Can::readFlag<2, B10000000>(data);
		break;
		// AC AUTO AMP 4
	case (0x625):
		climateControl->isRearWindowHeating = Can::readFlag<0, B00000001>(data);
		break;
	}
}

void updateDriveTrain(long unsigned int id, unsigned char len,
		unsigned char data[8], DriveTrain * driveTrain) {
	UNUSED(id);
	UNUSED(len);

	uint8_t gear = data[0];

	// gear is N (0) or R (-1)
	if (gear < 0x80) {
		driveTrain->gearNum = (int8_t)((gear / 8) - 3);
	}
	// gear is 1-6
	else {
		driveTrain->gearNum = (int8_t)((gear - 120) / 8);
	}

	driveTrain->isSynchroRev = Can::readFlag<1, B01000000>(data);
}
