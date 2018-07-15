#ifndef CARSYSTEMS_H_
#define CARSYSTEMS_H_

#include "bitfield.h"
#include "serialpacket.h"

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

union DriveTrain {
	unsigned char data[2];
	int8_t gearNum;
	BitFieldMember<8, 1> isSynchroRev;
};

class CarSystemManager {
public:
	enum Systems { ClimateControlSystem, DriveTrainSystem };

	CarSystemManager(Stream &serial) {
		this->serial = serial;
	}

	template<typename T>
	T getCarSystemData(Systems key) {
		SerialDataPacket<T> * packet = this->template getCarSystem<T>(key);

		return *(packet->payload());
	}

	template<typename T>
	void updateCarSystem(Systems key, T carsystem, boolean send) {
		SerialDataPacket<T> * packet = this->template getCarSystem<T>(key);

		if (packet != NULL && memcmp(carsystem, packet->payload(), sizeof(carsystem)) == 0) {
			packet->payload.data = carsystem.data;
			if (send) {
				packet->serialize(this->serial);
			}
		}
	}

	template<typename T>
	SerialDataPacket<T> * getCarSystem(Systems key) {
		switch (key) {
		case ClimateControlSystem:
			return this->climateControl;
		case DriveTrainSystem:
			return this->driveTrain;
		}

		return NULL;
	}
private:
	SerialDataPacket<ClimateControl>* climateControl 	= new climateControl(0x73, 0x63);
	SerialDataPacket<DriveTrain>* driveTrain 			= new driveTrain(0x73, 0x67);
	Stream * serial;
};

#endif /* CARSYSTEMS_H_ */
