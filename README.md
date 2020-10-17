# WaterRower S4BL3 Bluetooth BLE for S4

WaterRower S4 BLE Arduino SAMD21 BLE

# ![BLE S4](https://raw.githubusercontent.com/vibr77/wr_s4bl3_samd/main/images/A2829-01.jpg)+![WR](https://raw.githubusercontent.com/vibr77/wr_s4bl3_samd/main/images/wrS4.jpeg)


## Introduction

The aim of this project is to interface your WaterRower with an App or with your Garmin Watch. the way to do it is to build a small & efficient BLE Module for the S4 WaterRower Monitor providing a Bluetooth Low Energy module.

IOS / Android Exemple app Water Rower Connect https://apps.apple.com/fr/app/waterrower-connect/id1463763094

The WaterRower is a really cool and beautifull Rower with a S4 Controller. This controller has a USB port at the back and we can use it to create a USB to BLE (Bluetooth Low Energy Bridge).

The WaterRower S4 Controller use USB CDC ACM protocol. This means that UART Over USB will not work, a device with USB Host and Virtual Serial is needed.

A device with BLE (Bluetooth Low Energy) and Native USB port capabilities is needed. **ESP32 will not** work as it has no Native USB Port.

SAMD MicroController can do USB CDC ACM, and the Adafruit Feather M0 Bluefruit LE is a One-In-All approach:
* ATSAMD21G18 ARM Cortex M0 processor
* nRF51822 chipset from Nordic as Bluetooth stack
* Built in battery charging, this will be very usefull to avoid heavy external power supply

https://www.adafruit.com/product/2995

It is also a quite competitive product $29.95

## BLUETOOTH BLE PART

As per the BLE GATT specification, The Fitness Machine Service implement the Rower Data Characteristics https://www.bluetooth.com/specifications/gatt/

Fitness Service UID is 0x1826
RowerData Characteristics UID is 0x2AD1

### Reading the Specification, each BLE message is made of 
- Bitfield data : define which field is present
- Datafield : datafield

### Bytefield construction :
```
// 0000000000001 - 1   - 0x001 - More Data 0
// 0000000000010 - 2   - 0x002 - Average Stroke present
// 0000000000100 - 4   - 0x004 - Total Distance Present
// 0000000001000 - 8   - 0x008 - Instantaneous Pace present
// 0000000010000 - 16  - 0x010 - Average Pace Present
// 0000000100000 - 32  - 0x020 - Instantaneous Power present
// 0000001000000 - 64  - 0x040 - Average Power present
// 0000010000000 - 128 - 0x080 - Resistance Level present
// 0000100000000 - 256 - 0x080 - Expended Energy present
// 0001000000000 - 512 - 0x080 - Heart Rate present
// 0010000000000 - 1024- 0x080 - Metabolic Equivalent present
// 0100000000000 - 2048- 0x080 - Elapsed Time present
// 1000000000000 - 4096- 0x080 - Remaining Time present
```

`

**Warning 1 <!> the first item is working the opposite way : 0 field are present, 1 field are not present**

### List of Field with size :
```
//  C1  Stroke Rate             uint8     Position    2 (After the ByteField 2 bytes)
//  C1  Stroke Count            uint16    Position    3 
//  C2  Average Stroke Rate     uint8     Position    5
//  C3  Total Distance          uint24    Position    6
//  C4  Instantaneous Pace      uint16    Position    9
//  C5  Average Pace            uint16    Position    11
//  C6  Instantaneous Power     sint16    Position    13
//  C7  Average Power           sint16    Position    15
//  C8  Resistance Level        sint16    Position    17
//  C9  Total Energy            uint16    Position    19
//  C9  Energy Per Hour         uint16    Position    21
//  C9  Energy Per Minute       uint8     Position    23
//  C10 Heart Rate              uint8     Position    24
//  C11 Metabolic Equivalent    uint8     Position    25
//  C12 Elapsed Time            uint16    Position    26
//  C13 Remaining Time          uint16    Position    28
```

**Warning 2 <!> the Nordic BLE module is able to manage 30 Bytes message including header meaning 20 Bytes.** 

If you need all the fields, then the BLE message will have to be splitted in 2 sub message.


## S4 PART

As mentionned above, the S4 Controller is able to manage only Native USB Host with CDC ACM Protocol. Thus, the Adafruit M0 needs to use and specific library for SAMD21/51 Microchip. Hopefully: GDsports has done a portage to SAMD from the library is based on the [USB Host Shield Library 2.0](https://github.com/felis/USB_Host_Shield_2.0)

https://github.com/gdsports/USB_Host_Library_SAMD

Looking also at the specification (Water Rower S4 S5 USB Protocol Iss 1 04.pdf), the S4 uses CDC ACM with :

- 115200 bps, way to fast for th SAMD21, max handled is 57600 bps
- Message has to be sent char y char every 25ms
- Make sure to clear the socket input buffer (only 64 bytes on CDC Host implementation), before sending command

Do not use delay() as it is a blocking function and it will flood the input USB Serial buffer. 

The USB Stack is ready after 32 loop iteration (see code).

A Reset is done at the beginning of the program. Normally the WaterRower should bip.

A conditon has also been set to request S4 Memory Data only when the BLE App is connected.

The current version starts to work, but there is a lot of work to be done. 

To debug this App while connected to the S4, I use a serial port on pin RX/TX +GND with an ESP32 receiving the Data and acting as a UART to USB Bridge.

Todo :

- Change BLE message format to only focus on : strokeCount, StrokeRate, totalDistance, InstaneousPower. Remove everything else
- Optimize startup time
- Do testing to finetune ACM port Management.
- Add a Poly Battery, Print a 3D case, 
- Create a Garmin Custom Field

I hope my code will help you, I was looking for such implementation with no luck and needless to say that It was a P..n in the A.s to make this stuf works.

Vincent