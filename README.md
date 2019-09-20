# Carduino

> Advanced car to USB-serial interface for Arduino

This is an [Arduino (Nano)](https://www.arduino.cc/en/Main/ArduinoBoardNano) 
library to allow a serial device to connect to a car. It provides scaffolding 
and helpers to easily create a customized firmware for your micro controller. 
The custom firmware can read data from the CAN-bus of the car. 

The data read from various sources (pins, CAN-bus, ...) is automatically 
translated into a standardized communication protocol for data exchange between 
the arduino and its serial host.

An example of a serial host is 
[CarDroid](https://github.com/rampage128/cardroid). It is an android app, that 
is able to exchange data with Carduino. It can display various metrics from 
your car on the android device and allows controlling some systems of the car.

__Please make sure to read the [requirements](#requirements)__!

## Background
Carduino was created as an interface to allow the app 
[CarDroid](https://github.com/rampage128/cardroid) to communicate with a nissan 
370z. Main goal was to replace the entertainment system with an Android tablet, 
while keeping the climate controls accessible within Android.

## Requirements
### Mandatory
- [Arduino Nano](https://www.arduino.cc/en/Main/ArduinoBoardNano) or 
  [other Arduino board](https://www.arduino.cc/en/Main/Products#entrylevel) 

### Optional 
- An [MCP2515 SPI board](https://www.amazon.de/dp/B01IV3ZSKO/) for can access.

## Installation
### Releases

Currently no releases, you have to build the project [From source](#from-source)

### From source

1. Get your favourite [Arduino IDE](https://www.arduino.cc/en/main/software)
2. Clone the repository into your library directory  
   ```
   git clone https://github.com/rampage128/carduino.git
   ```
3. Start a new sketch and add the library to it

## Usage

After importing `#import <carduino.h>`, you can use the following 
features:

### Serial Events

The serial device connected to your Arduino can send serial events. These can 
trigger actions on your Arduino. To do so, you have to add a callback to your 
sketch:
```
void onCarduinoSerialEvent(uint8_t eventId, BinaryBuffer * payloadBuffer) {
    [...]
}
```
This callback will be called whenever a user event is sent from the other 
serial device to your Arduino. The callback allows you to identify the type of 
event with the `eventId` parameter. And optional payload (data) for that 
event is provided as a `BinaryBuffer` in the `payloadBuffer` pointer.

`BinaryBuffer` is a data-structure that allows you to read data-types 
sequentially from it as if it was a stream.

Make sure to attach you callback to Carduino:
```
Carduino carduino(&Serial, onCarduinoSerialEvent);
```

Finally whenever you get serial data (for example in your `serialEvent()` 
function), you can call:
```
carduino.readSerial();
```
This let's Carduino know, that there is new serial data available.

### Car Systems

Car systems reflect different parts of your car. It can be the climate 
controls, engine, drive-train and many more. They provide a standardized 
communication-layer from your arduino to your Android device. Each car system 
contains a data structure, which describes the state of the system in question.

Car systems can directly be manipulated and serialized to the connected serial 
device. Carduino also provides scaffolding that allows you to easily manipulate 
these states. Changes are then automatically detected by the library and sent 
to the connected serial device.

As example see `ClimateControl`:

| Field Name          | Data type          | Description                                |
| ------------------- | ------------------ | ------------------------------------------ |
| isAcOn              | Bit                | Flags if air conditioning is active        |
| isAuto              | Bit                | Flags if automatic controls are active     |
| isAirductWindshield | Bit                | Flags if windshield airduct is active      |
| isAirductFace       | Bit                | Flags if direct airduct is active          |
| isAirductFeet       | Bit                | Flags if lower airduct is active           |
| isWindshieldHeating | Bit                | Flags if the windshield heating is active  |
| isRearWindowHeating | Bit                | Flags if the rear window heating is active |
| isRecirculation     | Bit                | Flags if air recirculation is active       |
| fanLevel            | 8-Bit unsigned int | Stores the level of the fans               |
| desiredTemperature  | 8-Bit unsigned int | Stores the user selected temperature       |

### CAN-Bus

The `Can` class allows easy access to the vehicles CAN-Bus. It provides 
helpers and scaffolding to easily read data and automatically sends changes in 
a car-system to the connected serial device. It uses SPI to communicate with a 
MCP2515 can module.

Create a `Can` object in your sketch:
```
Can can(&Serial, 2, 10);
```
(In this example 2 is the interrupt pin and 10 is the client select pin of the 
SPI setup).

In the `setup()` function you can initialize the can-connection and inform 
`Carduino` about your `Can` object.
```
can.setup(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
carduino.addCan(&can);
```
(The constants are taken from 
[MCP_CAN_lib](https://github.com/coryjfowler/MCP_CAN_lib))

After having initialized the CAN system, it can be used in the `loop()` 
function of your sketch in four steps:

1. Add a callback function to your sketch to read the CAN-data:
   ```
   void updateClimateControl(long unsigned int id, unsigned char len, 
           unsigned char data[8], ClimateControl * climateControl) {
       [...]
   }
   ```
   The callback function can have any name you like and provides the CAN-ID, 
   the length of the CAN-packet, the CAN-data in an array and a pointer to 
   the car system.
   Any changes applied to the car system will automatically be detected by 
   Carduino and sent to the connected serial device.
   (only if something actually changed!)
2. Begin a CAN-transaction by calling:
   ```
   can.beginTransaction();
   ```
3. Associate a car system to a CAN-ID and a callback function:
   ```
   can.updateFromCan<ClimateControl>(0x54A, climateControl, updateClimateControl);
   ```
   In this case the `ClimateControl` car system is associated to the 
   CAN-ID `0x54A`. Once the CAN-Module receives this CAN-ID, it calls the 
   callback function `updateClimateControl`.
4. End the transaction:
   ```
   can.endTransaction();
   ```

__Please note:__ You can add as many associations as you like within one 
transaction. Carduino is smart and only calls your callback functions if a 
matching CAN-ID is received.

## Serial Communication

The Arduino sends and receives serial packets to the USB-Serial interface.

Easiest way to make real use of the data is to connect an Android device to the Arduino's USB-Port
and get [CarDroid](https://github.com/rampage128/cardroid).

The hard way is to read the packets from the serial connection. The protocol is custom and subject
to change. Generally data is sent in frames. Each frame contains one single packet. 


Please see the table below for a general idea:

| Index  | Length      | Possible values | Description                                  |
| ------:| -----------:| --------------- | -------------------------------------------- |
| 0      | 1           | `0x7b`          | Start of a frame                             |
| 1      | 1           | `0x00` - `0xFF` | Packet type to indicate purpose              |
| 2      | 1           | `0x00` - `0xFF` | Packet id to indicate request                |
| 3      | 1           | `0x01` - `0x7c` | Payload length (L) (only if payload present) |
| 4      | (1 - 124) L | `0x01` - `0xFF` | Payload (only if present)                    |
| 3 + L  | 1           | `0x7d`          | End of a frame                               |

For more information, please refer to the [source](https://github.com/rampage128/carduino).

## Contribute

Feel free to [open an issue](https://github.com/rampage128/carduino/issues) or submit a PR

Also, if you like this or other of my projects, please feel free to support me using the Link below.

[![Buy me a beer](https://img.shields.io/badge/buy%20me%20a%20beer-PayPal-green.svg)](https://www.paypal.me/FrederikWolter/1)

## Dependencies

- [AnalogMultiButton](https://github.com/dxinteractive/AnalogMultiButton) to read the steering wheel remote
- [everytime](https://github.com/fesselk/everytime) for easy periodic code execution (send packets)
- [MCP_CAN_lib](https://github.com/coryjfowler/MCP_CAN_lib) to communicate with CAN-Bus on MCP2515
- [SPI library](https://www.arduino.cc/en/Reference/SPI) to utilize SPI interface
- [BitFieldMember](https://codereview.stackexchange.com/questions/54342/) to allow usage of ordered bitfields in unions
