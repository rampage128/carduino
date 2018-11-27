#ifndef _370Z_H_
#define _370Z_H_

#include "can.h"
#include "carduino.h"
#include "bitfield.h"
#include "binarydata.h"
#include "carsystems.h"
#include <AnalogMultiButton.h>

class NissanClimateControl {
public:
	void push(uint8_t buttonId, BinaryBuffer * payloadBuffer) {
		switch (buttonId) {
		case 0x01: // OFF BUTTON
			climateCanSignal2.offButton ^= 1;
			break;
		case 0x02: // ac button
			climateCanSignal2.acButton ^= 1;
			break;
		case 0x03: // auto button
			climateCanSignal2.autoButton ^= 1;
			break;
		case 0x04: // recirculation button
			climateCanSignal2.recirculationButton ^= 1;
			break;
		case 0x05: // windshield heating button
			climateCanSignal2.windshieldButton ^= 1;
			break;
		case 0x06: // rear window heating button
			climateCanSignal2.rearWindowHeaterButton = 1;
			break;
		case 0x07: // mode button
			climateCanSignal2.modeButton ^= 1;
			break;
		case 0x08: // temperature knob
			if (payloadBuffer->available() > 0) {
				climateCanSignal2.temperatureKnob ^= 1;
				climateCanSignal3.desiredTemperature =
						payloadBuffer->readByte().data;
			}
			break;
		case 0x09: // fan level
			if (payloadBuffer->available() > 0) {
				climateCanSignal2.fanKnob ^= 1;
				climateCanSignal3.fanSpeed =
						(uint8_t) (payloadBuffer->readByte().data * 8);
			}
			break;
		}
	}
	void broadcast(Can &can) {
		can.write(0x540, 0, sizeof(climateCanSignal1),
				(uint8_t*) climateCanSignal1.data);
		can.write(0x541, 0, sizeof(climateCanSignal2),
				(uint8_t*) climateCanSignal2.data);
		can.write(0x542, 0, sizeof(climateCanSignal3),
				(uint8_t*) climateCanSignal3.data);

		// reset rear heating flag after sending ... this is a special case.
		climateCanSignal2.rearWindowHeaterButton = 0;

		if (climateCanSignal1.roll < 3) {
			climateCanSignal1.roll += 1;
			climateCanSignal2.roll += 1;
			climateCanSignal3.roll += 1;
		} else {
			climateCanSignal1.roll = 0;
			climateCanSignal2.roll = 0;
			climateCanSignal3.roll = 0;
		}
	}
private:
	union {
		unsigned char data[8] =
				{ 0x20, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		BitFieldMember<0, 8> identifier1;
		BitFieldMember<8, 8> identifier2;
		BitFieldMember<56, 8> roll;
	} climateCanSignal1;

	union {
		unsigned char data[8] =
				{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		BitFieldMember<3, 1> modeButton;
		BitFieldMember<7, 1> windshieldButton;
		BitFieldMember<8, 1> rearWindowHeaterButton;
		BitFieldMember<14, 1> recirculationButton;
		BitFieldMember<16, 1> temperatureKnob;
		BitFieldMember<21, 1> acButton;
		BitFieldMember<23, 1> offButton;
		BitFieldMember<24, 1> autoButton;
		BitFieldMember<27, 1> fanKnob;
		BitFieldMember<56, 8> roll;
	} climateCanSignal2;

	union {
		unsigned char data[8] =
				{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		BitFieldMember<0, 8> fanSpeed;
		BitFieldMember<8, 8> desiredTemperature;
		BitFieldMember<56, 8> roll;
	} climateCanSignal3;
};

class NissanSteeringControl {
public:
	NissanSteeringControl(uint8_t pin1, uint8_t pin2) {
		int buttonArray1[] = { 0, 210, 416, 620, 830 };
		int buttonArray2[] = { 0, 210, 416, 620 };
		this->buttons1 = new AnalogMultiButton(pin1, 5, buttonArray1);
		this->buttons2 = new AnalogMultiButton(pin2, 4, buttonArray2);
	}
	void check(Carduino * carduino) {
		this->buttons1->update();

		// SOURCE
		if (this->buttons1->onReleaseBefore(0, 500)) {
			carduino->triggerEvent(1);
		} else if (buttons1->onPressAfter(0, 500)) {
			carduino->triggerEvent(2);
		}
		// MENU UP
		if (this->buttons1->onPressAndAfter(1, 1000, 500)) {
			carduino->triggerEvent(10);
		}
		// MENU DOWN
		if (this->buttons1->onPressAndAfter(2, 1000, 500)) {
			carduino->triggerEvent(11);
		}
		// VOICE
		if (this->buttons1->onReleaseBefore(3, 500)) {
			carduino->triggerEvent(42);
		}
		// ENTER
		if (this->buttons1->onReleaseBefore(4, 500)) {
			carduino->triggerEvent(12);
		}

		this->buttons2->update();

		// VOL DOWN
		if (this->buttons2->onPressAndAfter(0, 500, 200)) {
			carduino->triggerEvent(20);
		}
		// VOL UP
		if (this->buttons2->onPressAndAfter(1, 500, 200)) {
			carduino->triggerEvent(21);
		}
		// PHONE
		if (this->buttons2->onReleaseBefore(2, 500)) {
			carduino->triggerEvent(30);
		}
		// BACK
		if (this->buttons2->onReleaseBefore(3, 500)) {
			carduino->triggerEvent(13);
		}
	}
private:
	AnalogMultiButton * buttons1;
	AnalogMultiButton * buttons2;
};

#endif /* _370Z_H_ */
