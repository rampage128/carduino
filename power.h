#ifndef POWER_H_
#define POWER_H_

#include "Arduino.h"
#include <avr/sleep.h>
#include <SPI.h>

static volatile uint8_t interruptPinStateOnWake = 0;

template<uint8_t INTERRUPT_PIN>
static void wake(void) {
	sleep_disable();
	detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
	interruptPinStateOnWake = digitalRead(INTERRUPT_PIN);
}

static void noop() {}

static bool forceSleep() {
	return true;
}

static SerialPacket noSleepCallbackError(0x65, 0x40);

class PowerManager {
private:
	Stream * serial;
	uint8_t chargerPin;
	uint8_t peripheralPin;
	bool confirmInterrupt(uint8_t interruptPin, uint8_t desiredPinState, uint32_t duration) {
		uint32_t sampleDelay = duration / 10;

		// Count samples with matching pin state
		uint8_t counter = 0;
		for (int i = 0; i < 10; i++) {
			delay(sampleDelay);
			if (digitalRead(interruptPin) == desiredPinState) {
				counter++;
			}
		}

		// Confirm interrupt if at least 3 samples match the desired pin state
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
	template<uint8_t INTERRUPT_PIN, uint8_t INTERRUPT_TYPE, uint32_t INTERRUPT_DURATION, uint32_t CHARGE_DURATION>
	void update(bool (*sleepCallback)(void), void (*wakeCallback)(void), void (*loopCallback)(void)) {
		loopCallback();

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
				delay(50);
			}

			// get interrupt handle
			uint8_t interrupt = digitalPinToInterrupt(INTERRUPT_PIN);

			// disable built in power modules
			this->togglePeripherals(false);

			// If specified by user charge tablet for a while
			if (CHARGE_DURATION > 0) {
				// Make sure charger is on!
				this->toggleCharger(true);

				uint8_t chargeInterrupt = digitalRead(INTERRUPT_PIN);
				uint32_t chargeTime = millis();
				while (chargeTime >= millis() - CHARGE_DURATION
						&& !this->confirmInterrupt(INTERRUPT_PIN,
								!chargeInterrupt, INTERRUPT_DURATION)) {
					// wait for charge time to end or interrupt pin to change
				}
			}

			// disable charger
			this->toggleCharger(false);

			// Store pins state and disable all pins (input, low)
			byte ddrbState 	= DDRB;
			byte portbState = PORTB;
			byte ddrcState 	= DDRC;
			byte portcState = PORTC;
			byte ddrdState 	= DDRD;
			byte portdState = PORTD;

			DDRB 	= B00000000;
			PORTB 	= B00000000;
			DDRC 	= B00000000;
			PORTC 	= B00000000;
			DDRD 	= B00000000;
			PORTD 	= B00000000;

			// Remember ADC state and turn it off
			byte adcsraState = ADCSRA;
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

			uint8_t desiredPinState = LOW;
			if (INTERRUPT_TYPE == RISING) {
				desiredPinState = HIGH;
			}
			else if (INTERRUPT_TYPE == CHANGE) {
				desiredPinState = interruptPinStateOnWake;
			}

			// Resume if interrupt can be confirmed (signal is longer than specified duration)
			if (this->confirmInterrupt(INTERRUPT_PIN, desiredPinState, INTERRUPT_DURATION)) {
				// Restore pin states before sleep
				DDRB 	= ddrbState;
				PORTB 	= portbState;
				DDRC 	= ddrcState;
				PORTC 	= portcState;
				DDRD 	= ddrdState;
				PORTD 	= portdState;

				// Restore ADC state
				ADCSRA = adcsraState;

				this->setup();

				if (wakeCallback) {
					wakeCallback();
				}
			}
			else {
				this->update<INTERRUPT_PIN, INTERRUPT_TYPE, INTERRUPT_DURATION, CHARGE_DURATION>(forceSleep, wakeCallback, noop);
			}
		}
	}
};

#endif /* POWER_H_ */
