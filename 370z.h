#ifndef _370Z_H_
#define _370Z_H_

#include "can.h"
#include "bitfield.h"
#include "binarydata.h"
#include "carsystems.h"
#include <AnalogMultiButton.h>

class NissanClimateControl {
  public:
	void read(SerialDataPacket<ClimateControl> &climateControl, long unsigned int id, unsigned char len, unsigned char rxBuf[8]) {
		switch (id) {
		// AC AUTO AMP 1
		case (0x54A):
			climateControl.payload()->desiredTemperature = rxBuf[4];
			break;
		// AC AUTO AMP 2
		case (0x54B):
			climateControl.payload()->fanLevel = (rxBuf[4] - 4) / 8;

			switch (rxBuf[2]) {
			case (0x80):
				climateControl.payload()->isAirductWindshield = 0;
				climateControl.payload()->isAirductFace = 0;
				climateControl.payload()->isAirductFeet = 0;
				break;
			case (0x98):
				climateControl.payload()->isAirductWindshield = 0;
				climateControl.payload()->isAirductFace = 0;
				climateControl.payload()->isAirductFeet = 1;
				break;
			case (0xA0):
				climateControl.payload()->isAirductWindshield = 1;
				climateControl.payload()->isAirductFace = 0;
				climateControl.payload()->isAirductFeet = 1;
				break;
			case (0x88):
				climateControl.payload()->isAirductWindshield = 0;
				climateControl.payload()->isAirductFace = 1;
				climateControl.payload()->isAirductFeet = 0;
				break;
			case (0x90):
				climateControl.payload()->isAirductWindshield = 0;
				climateControl.payload()->isAirductFace = 1;
				climateControl.payload()->isAirductFeet = 1;
				break;
			}

			climateControl.payload()->isWindshieldHeating = (rxBuf[3]
					& B10000000) == B10000000;
			climateControl.payload()->isRecirculation = (rxBuf[3]
					& B00000011) == B00000001;
			climateControl.payload()->isAuto = (rxBuf[1] & B00001110)
					== B00000110;
			break;
		// AC AUTO AMP 3
		case (0x54C):
			climateControl.payload()->isAcOn = (rxBuf[2] & B10000000)
					== B10000000;
			break;
		// AC AUTO AMP 4
		case (0x625):
			climateControl.payload()->isRearWindowHeating = (rxBuf[0]
					& B00000001) == B00000001;
			break;
		}
	}
	void push(uint8_t buttonId, BinaryBuffer *payloadBuffer) {
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
    	can.write(0x540, 0, sizeof(climateCanSignal1), (uint8_t*)climateCanSignal1.data);
    	can.write(0x541, 0, sizeof(climateCanSignal2), (uint8_t*)climateCanSignal2.data);
    	can.write(0x542, 0, sizeof(climateCanSignal3), (uint8_t*)climateCanSignal3.data);

    	// reset rear heating flag after sending ... this is a special case.
    	climateCanSignal2.rearWindowHeaterButton = 0;

    	if (climateCanSignal1.roll < 3) {
    		climateCanSignal1.roll += 1;
    		climateCanSignal2.roll += 1;
    		climateCanSignal3.roll += 1;
    	}
    	else {
    		climateCanSignal1.roll = 0;
    		climateCanSignal2.roll = 0;
    		climateCanSignal3.roll = 0;
    	}
    }
private:
	union {
		unsigned char data[8] = { 0x20, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		BitFieldMember<0, 8> identifier1;
		BitFieldMember<8, 8> identifier2;
		BitFieldMember<56, 8> roll;
	} climateCanSignal1;

	union {
		unsigned char data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
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
		unsigned char data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
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
		_buttons1 = new AnalogMultiButton(pin1, 5, buttonArray1);
		_buttons2 = new AnalogMultiButton(pin2, 4, buttonArray2);
		_serialPacket = new SerialDataPacket<uint8_t>(0x73, 0x72);
	}
	void check(Stream &serial) {
		_buttons1->update();

		uint8_t buttonId = 0;

		// SOURCE
		if (_buttons1->onReleaseBefore(0, 500)) {
			buttonId = 1;
		} else if (_buttons1->onPressAfter(0, 500)) {
			buttonId = 2;
		}
		// MENU UP
		if (_buttons1->onPressAndAfter(1, 1000, 500)) {
			buttonId = 10;
		}
		// MENU DOWN
		if (_buttons1->onPressAndAfter(2, 1000, 500)) {
			buttonId = 11;
		}
		// VOICE
		if (_buttons1->onReleaseBefore(3, 500)) {
			buttonId = 42;
		}
		// ENTER
		if (_buttons1->onReleaseBefore(4, 500)) {
			buttonId = 12;
		}

		_buttons2->update();

		// VOL DOWN
		if (_buttons2->onPressAndAfter(0, 500, 200)) {
			buttonId = 20;
		}
		// VOL UP
		if (_buttons2->onPressAndAfter(1, 500, 200)) {
			buttonId = 21;
		}
		// PHONE
		if (_buttons2->onReleaseBefore(2, 500)) {
			buttonId = 30;
		}
		// BACK
		if (_buttons2->onReleaseBefore(3, 500)) {
			buttonId = 13;
		}

		if (buttonId > 0) {
			_serialPacket->payload(buttonId);
			_serialPacket->serialize(serial);
		}
	}
  private:
    AnalogMultiButton *_buttons1;
    AnalogMultiButton *_buttons2;
    SerialDataPacket<uint8_t> *_serialPacket;
};

#endif /* _370Z_H_ */
