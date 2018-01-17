#ifndef CARSYSTEMS_H_
#define CARSYSTEMS_H_

#include "bitfield.h"

struct ClimateControl {
	union {
		unsigned char data[1];
		BitFieldMember<0, 1> isAcOn;
		BitFieldMember<1, 1> isAuto;
		BitFieldMember<2, 1> isAirductWindshield;
		BitFieldMember<3, 1> isAirductFace;
		BitFieldMember<4, 1> isAirductFeet;
		BitFieldMember<5, 1> isWindshieldHeating;
		BitFieldMember<6, 1> isRearWindowHeating;
		BitFieldMember<7, 1> isRecirculation;
	} flags;
	uint8_t fanLevel;
	uint8_t desiredTemperature;
};

struct GearBox {
	int8_t gearNum;
	union {
		unsigned char data[1];
		BitFieldMember<0, 1> isSynchroRev;
	} flags;
};

#endif /* CARSYSTEMS_H_ */
