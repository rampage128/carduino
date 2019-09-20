#ifndef CARSYSTEMS_H_
#define CARSYSTEMS_H_

#include "bitfield.h"
#include "serialpacket.h"

union ClimateControl {
    unsigned char data[3] = { 0x00, 0x00, 0x00 };
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
SerialDataPacket<ClimateControl>* climateControl = new SerialDataPacket<
        ClimateControl>(0x73, 0x63);

union DriveTrain {
    unsigned char data[2] = { 0x00, 0x00 };
    BitFieldMember<0, 8> gearNum;
    BitFieldMember<8, 1> isSynchroRev;
};
SerialDataPacket<DriveTrain>* driveTrain = new SerialDataPacket<DriveTrain>(
        0x73, 0x67);

union PowerState {
    unsigned char data[1] = { 0x00 };
    BitFieldMember<0, 1> isAccessoryOn;
    BitFieldMember<1, 1> isIgnitionOn;
};
SerialDataPacket<PowerState>* powerState = new SerialDataPacket<PowerState>(
        0x73, 0x70);

union Doors {
    unsigned char data[1] = { 0x00 };
    BitFieldMember<0, 1> isFrontLeftOpen;
    BitFieldMember<1, 1> isFrontRightOpen;
    BitFieldMember<2, 1> isRearLeftOpen;
    BitFieldMember<3, 1> isRearRightOpen;
    BitFieldMember<4, 1> isTrunkOpen;
    BitFieldMember<5, 1> isHoodOpen;
};
SerialDataPacket<Doors>* doors = new SerialDataPacket<Doors>(0x73, 0x64);

#endif /* CARSYSTEMS_H_ */
