#ifndef CARSYSTEMS_H_
#define CARSYSTEMS_H_

#include "bitfield.h"

union ClimateControl {
	unsigned char data[3];
	BitFieldMember<0, 1> isAcOn;
	BitFieldMember<1, 1> isAuto;
	BitFieldMember<2, 1> isAirductWindshield;
	BitFieldMember<3, 1> isAirductFace;
	BitFieldMember<4, 1> isAirductFeet;
	BitFieldMember<5, 1> isWindshieldHeating;
	BitFieldMember<6, 1> isRearWindowHeating;
	BitFieldMember<7, 1> isRecirculation;
	BitFieldMember<8, 8> fanLevel;
	BitFieldMember<16, 8> desiredTemperature;
};

union GearBox {
	unsigned char data[2];
	int8_t gearNum;
	BitFieldMember<8, 1> isSynchroRev;
};

#endif /* CARSYSTEMS_H_ */
