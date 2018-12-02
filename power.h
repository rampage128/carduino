#ifndef POWER_H_
#define POWER_H_

#include "Arduino.h"
#include <avr/sleep.h>

static void nothing(void) {
}

static SerialPacket noSleepCallbackError(0x65, 0x40);

class PowerManager {
private:
	Stream * serial;
public:
	PowerManager(Stream * serial) {
		this->serial = serial;
	}
	void setup() {
		pinMode(4, OUTPUT);
		pinMode(5, OUTPUT);

		digitalWrite(4, HIGH);
		digitalWrite(5, HIGH);
	}
	template<uint8_t INTERRUPT_PIN, uint8_t INTERRUPT_TYPE>
	void sleep(bool (*sleepCallback)(void), void (*wakeCallback)(void)) {
		// Disable external peripherals
		if (!sleepCallback) {
			noSleepCallbackError.serialize(&Serial);
			return;
		}
		if (sleepCallback()) {
			uint8_t interrupt = digitalPinToInterrupt(INTERRUPT_PIN);

			digitalWrite(4, LOW);
			digitalWrite(5, LOW);

			// Prepare sleep mode
			sleep_enable()
			;
			noInterrupts();
			attachInterrupt(interrupt, nothing, INTERRUPT_TYPE);
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			interrupts();

			// Send to sleep
			sleep_cpu()
			;

			// Continue after waking up
			sleep_disable()
			;
			detachInterrupt(interrupt);
			this->setup();
			if (wakeCallback) {
				wakeCallback();
			}
		}
	}
};

#endif /* POWER_H_ */
