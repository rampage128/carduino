#ifndef POWER_H_
#define POWER_H_

#include "Arduino.h"
#include <avr/sleep.h>
#include <SPI.h>

template<uint8_t INTERRUPT_PIN>
static void wake(void) {
	sleep_disable();
	detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
}

static bool forceSleep() {
	return true;
}

static SerialPacket noSleepCallbackError(0x65, 0x40);

class PowerManager {
private:
	Stream * serial;
	uint8_t chargerPin;
	uint8_t peripheralPin;
	bool debounceInterrupt(uint8_t interruptPin, uint8_t signalType) {
		delay(100);
		uint8_t counter = 0;
		for (int i = 0; i < 10; i++) {
			if (digitalRead(interruptPin) == signalType) {
				counter++;
			}
			delay(100);
		}

		return counter >= 3;
	}
public:
	PowerManager(Stream * serial, uint8_t chargerPin, uint8_t peripheralPin) {
		this->serial = serial;
		this->chargerPin = chargerPin;
		this->peripheralPin = peripheralPin;
	}
	void setup() {
		pinMode(this->chargerPin, OUTPUT);
		pinMode(this->peripheralPin, OUTPUT);

		this->toggleCharger(true);
		this->togglePeripherals(true);
	}
	void toggleCharger(bool state) {
		digitalWrite(this->chargerPin, state);
	}
	void togglePeripherals(bool state) {
		digitalWrite(this->peripheralPin, state);
	}
	template<uint8_t INTERRUPT_PIN, uint8_t INTERRUPT_TYPE, uint8_t SIGNAL_TYPE>
	void sleep(bool (*sleepCallback)(void), void (*wakeCallback)(void)) {
		// Disable external peripherals
		if (!sleepCallback) {
			noSleepCallbackError.serialize(&Serial);
			return;
		}
		if (sleepCallback()) {
			// Flush serial and terminate connection
			Serial.flush();
			Serial.end();

			// Turn off SPI
			SPI.end();

			// flash LED
			pinMode(13, OUTPUT);
			for (int i = 0; i < 3; i++) {
				digitalWrite(13, LOW);
				delay(100);
				digitalWrite(13, HIGH);
				delay(100);
			}

			// get interrupt handle
			uint8_t interrupt = digitalPinToInterrupt(INTERRUPT_PIN);

			// disable built in power modules
			this->toggleCharger(false);
			this->togglePeripherals(false);

			// Disable all digital pins
			for (int i = 0; i <= 13; i++) {
				digitalWrite(i, LOW);
				pinMode(i, INPUT);
			}

			// disable ADC
			ADCSRA = 0;

			// Prepare sleep mode
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_enable();

			// prevent interrupts before sleeping to avoid accidentally
			// detaching the wake up interrupt.
			noInterrupts();
			attachInterrupt(interrupt, wake<INTERRUPT_PIN>, INTERRUPT_TYPE);

			// turn off brown-out enable in software
			// BODS must be set to one and BODSE must be set to zero within four clock cycles
			MCUCR = bit (BODS) | bit (BODSE);
			// The BODS bit is automatically cleared after three clock cycles
			MCUCR = bit (BODS);
			/*
			 * DON NOT INSERT CODE HERE, BROWN-OUT RESETS AFTER 3 CLOCK CYCLES
			 * Right after this we need to allow interrupts and sleep_cpu
			 */
			interrupts();
			sleep_cpu();

			// Continue after waking up

			// Crude debouncing of sleep mode
			if (this->debounceInterrupt(INTERRUPT_PIN, SIGNAL_TYPE)) {
				this->setup();
				if (wakeCallback) {
					wakeCallback();
				}
			}
			else {
				this->sleep<INTERRUPT_PIN, INTERRUPT_TYPE, SIGNAL_TYPE>(forceSleep, wakeCallback);
			}
		}
	}
};

#endif /* POWER_H_ */
