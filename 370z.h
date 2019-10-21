#ifndef _370Z_H_
#define _370Z_H_

#include "carduino.h"
#include <AnalogMultiButton.h>

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
            carduino->triggerEvent(101);
        } else if (buttons1->onPressAfter(0, 500)) {
            carduino->triggerEvent(102);
        }
        // MENU UP
        if (this->buttons1->onPressAndAfter(1, 1000, 500)) {
            carduino->triggerEvent(103);
        }
        // MENU DOWN
        if (this->buttons1->onPressAndAfter(2, 1000, 500)) {
            carduino->triggerEvent(104);
        }
        // VOICE
        if (this->buttons1->onReleaseBefore(3, 500)) {
            carduino->triggerEvent(112);
        }
        // ENTER
        if (this->buttons1->onReleaseBefore(4, 500)) {
            carduino->triggerEvent(105);
        }

        this->buttons2->update();

        // VOL DOWN
        if (this->buttons2->onPressAndAfter(0, 500, 200)) {
            carduino->triggerEvent(110);
        }
        // VOL UP
        if (this->buttons2->onPressAndAfter(1, 500, 200)) {
            carduino->triggerEvent(111);
        }
        // PHONE
        if (this->buttons2->onReleaseBefore(2, 500)) {
            carduino->triggerEvent(113);
        }
        // BACK
        if (this->buttons2->onReleaseBefore(3, 500)) {
            carduino->triggerEvent(106);
        }
    }
private:
    AnalogMultiButton * buttons1;
    AnalogMultiButton * buttons2;
};

#endif /* _370Z_H_ */
