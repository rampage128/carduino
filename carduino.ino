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

bool shouldSleep = false;
bool isDriverDoorOpened = true;
bool isAccessoryOn = false;

void setup() {
    carduino.begin();

    powerManager.setup();
    carduino.addCan(&can);
    carduino.addPowerManager(&powerManager);
    can.setup(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
    carduino.triggerEvent(1);
}

void loop() {
    powerManager.update<2, RISING, ONE_SECOND / 2, ONE_MINUTE * 15>(onSleep,
            onWakeUp, onLoop);
}

void serialEvent() {
    carduino.readSerial();
}

void onCarduinoSerialEvent(uint8_t eventId, BinaryBuffer *payloadBuffer) {
    nissanClimateControl.push(eventId, payloadBuffer);
}

void onLoop() {
    if (carduino.isConnected()) {
        can.updateFromCan(onCan);

        nissanSteeringControl.check(&carduino);

        every(250) {
            nissanClimateControl.broadcast(can);
        }
    }
}

bool onSleep() {
    // Check sleep conditions
    if (isAccessoryOn) {
        // Reset timer when ACC is on to prevent premature sleep
        sleepTimer.reset();
    } else {
        // should go to sleep, when ACC is off and driver door is opened
        // Or allow operation without ACC in the closed vehicle for 30 minutes
        shouldSleep = shouldSleep || sleepTimer.check(ONE_MINUTE * 30);
    }

    if (shouldSleep) {
        carduino.triggerEvent(2);
        carduino.end();
    }

    return shouldSleep;
}

void onWakeUp() {
    // Reset driver door status, otherwise sleep is triggered after wake up
    shouldSleep = false;
    sleepTimer.reset();
    carduino.begin();
    can.setup(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
    carduino.triggerEvent(3);
}

void onCan(uint32_t canId, uint8_t data[], uint8_t len) {
    UNUSED(len);

    if (canId == 0x60D) {
        isAccessoryOn = Can::readFlag<1, B00000010>(data);
        bool isFrontLeftOpen = Can::readFlag<0, B00001000>(data);
        if (!isAccessoryOn && isFrontLeftOpen && !isDriverDoorOpened) {
            shouldSleep = true;
        }
        isDriverDoorOpened = isFrontLeftOpen;
    }
}
