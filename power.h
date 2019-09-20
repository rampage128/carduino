#ifndef POWER_H_
#define POWER_H_

#include "Arduino.h"
#include <avr/sleep.h>
#include <SPI.h>

static volatile uint8_t interruptPinStateOnWake = 0;

template<uint8_t INTERRUPT_PIN>
static void wake() {
    sleep_disable();
    detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
    interruptPinStateOnWake = digitalRead(INTERRUPT_PIN);
}

class PinState {
private:
    uint8_t ddrb;
    uint8_t portb;
    uint8_t ddrc;
    uint8_t portc;
    uint8_t ddrd;
    uint8_t portd;
    uint8_t adcsra;
public:
    PinState() {
        this->ddrb = DDRB;
        this->portb = PORTB;
        this->ddrc = DDRC;
        this->portc = PORTC;
        this->ddrd = DDRD;
        this->portd = PORTD;
        this->adcsra = ADCSRA;
    }
    void turnOff() {
        DDRB = B00000000;
        PORTB = B00000000;
        DDRC = B00000000;
        PORTC = B00000000;
        DDRD = B00000000;
        PORTD = B00000000;
        ADCSRA = 0;
    }
    void restore() {
        DDRB = this->ddrb;
        PORTB = this->portb;
        DDRC = this->ddrc;
        PORTC = this->portc;
        DDRD = this->ddrd;
        PORTD = this->portd;
        ADCSRA = this->adcsra;
    }
};

static SerialPacket noSleepCallbackError(0x65, 0x40);

class PowerManager {
private:
    Stream * serial;
    uint8_t chargerPin;
    uint8_t peripheralPin;
    bool confirmInterrupt(uint8_t interruptPin, uint8_t desiredPinState,
            uint32_t duration) {
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
    template<uint8_t INTERRUPT_PIN>
    void sleep(void (*wakeCallback)(void), uint8_t interruptType,
            uint32_t interruptDuration, uint32_t chargeDuration) {
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
        if (chargeDuration > 0) {
            // Make sure charger is on!
            this->toggleCharger(true);

            uint8_t chargeInterrupt = digitalRead(INTERRUPT_PIN);
            uint32_t chargeTime = millis();
            while (chargeDuration >= millis() - chargeTime) {
                // wait for charge time to end or cancel sleep on interrupt
                if (this->confirmInterrupt(INTERRUPT_PIN, !chargeInterrupt,
                        interruptDuration)) {
                    this->setup();
                    if (wakeCallback) {
                        wakeCallback();
                    }
                    return;
                }
            }
        }

        // disable charger
        this->toggleCharger(false);

        // Store pin states and ADC and disable all pins (input, low) and ADC
        PinState powerState;
        powerState.turnOff();

        // Prepare sleep mode
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable()
        ;

        // prevent interrupts before sleeping to avoid accidentally
        // detaching the wake up interrupt.
        noInterrupts();
        attachInterrupt(interrupt, wake<INTERRUPT_PIN>, interruptType);

        // turn off brown-out enable in software
        // BODS must be set to one and BODSE must be set to zero within four clock cycles
        MCUCR = bit (BODS) | bit(BODSE);
        // The BODS bit is automatically cleared after three clock cycles
        MCUCR = bit(BODS);
        /*
         * DON NOT INSERT CODE HERE, BROWN-OUT RESETS AFTER 3 CLOCK CYCLES
         * Right after this we need to allow interrupts and sleep_cpu
         */
        interrupts();
        sleep_cpu();

        // Continue after waking up

        uint8_t desiredPinState = LOW;
        if (interruptType == RISING) {
            desiredPinState = HIGH;
        } else if (interruptType == CHANGE) {
            desiredPinState = interruptPinStateOnWake;
        }

        // Resume if interrupt can be confirmed (signal is longer than specified duration)
        if (this->confirmInterrupt(INTERRUPT_PIN, desiredPinState,
                interruptDuration)) {
            // Restore pin states and ADC before waking up
            powerState.restore();

            this->setup();

            // Sanity delay
            delay(500);

            if (wakeCallback) {
                wakeCallback();
            }
        } else {
            this->sleep<INTERRUPT_PIN>(wakeCallback, interruptType,
                    interruptDuration, 0);
        }
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
    template<uint8_t INTERRUPT_PIN, uint8_t INTERRUPT_TYPE,
            uint32_t INTERRUPT_DURATION, uint32_t CHARGE_DURATION>
    void update(bool (*sleepCallback)(void), void (*wakeCallback)(void),
            void (*loopCallback)(void)) {
        loopCallback();

        if (!sleepCallback) {
            noSleepCallbackError.serialize(&Serial, 1000);
            return;
        }

        if (sleepCallback()) {
            // Flush serial and terminate connection
            Serial.flush();
            Serial.end();

            // Turn off SPI
            SPI.end();

            this->sleep<INTERRUPT_PIN>(wakeCallback, INTERRUPT_TYPE,
                    INTERRUPT_DURATION, CHARGE_DURATION);
        }
    }
};

#endif /* POWER_H_ */
