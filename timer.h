#ifndef TIMER_H_
#define TIMER_H_

#include "Arduino.h"

class Timer {
private:
    unsigned long startTime = 0;
public:
    bool check(unsigned long duration) {
        if (this->startTime == 0) {
            this->startTime = millis();
        }

        return (millis() - this->startTime) >= duration;
    }
    void reset() {
        this->startTime = 0;
    }
};

#endif /* TIMER_H_ */
