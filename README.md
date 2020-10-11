# WaterRower S4BL3 Bluetooth BLE for S4

WaterRower S4 BLE Arduino SAMD21 BLE


## Introduction

The aim of this project is to build a small & efficient BLE Module for the S4 WaterRower Monitor to be used by a Smartphone App or a Sport Watch (Garmin)

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
`
>  // 0000000000001 - 1   - 0x001 - More Data 0
>  // 0000000000010 - 2   - 0x002 - Average Stroke present
>  // 0000000000100 - 4   - 0x004 - Total Distance Present
>  // 0000000001000 - 8   - 0x008 - Instantaneous Pace present
>  // 0000000010000 - 16  - 0x010 - Average Pace Present
>  // 0000000100000 - 32  - 0x020 - Instantaneous Power present
>  // 0000001000000 - 64  - 0x040 - Average Power present
>  // 0000010000000 - 128 - 0x080 - Resistance Level present
>  // 0000100000000 - 256 - 0x080 - Expended Energy present
>  // 0001000000000 - 512 - 0x080 - Heart Rate present
>  // 0010000000000 - 1024- 0x080 - Metabolic Equivalent present
>  // 0100000000000 - 2048- 0x080 - Elapsed Time present
>  // 1000000000000 - 4096- 0x080 - Remaining Time present
`

**Warning 1 <!> the first item is working the opposite way : 0 field are present, 1 field are not present**

### List of Field with size :
`
>  //  C1  Stroke Rate             uint8     Position    2 (After the ByteField 2 bytes)
>  //  C1  Stroke Count            uint16    Position    3 
>  //  C2  Average Stroke Rate     uint8     Position    5
>  //  C3  Total Distance          uint24    Position    6
>  //  C4  Instantaneous Pace      uint16    Position    9
>  //  C5  Average Pace            uint16    Position    11
>  //  C6  Instantaneous Power     sint16    Position    13
>  //  C7  Average Power           sint16    Position    15
>  //  C8  Resistance Level        sint16    Position    17
>  //  C9  Total Energy            uint16    Position    19
>  //  C9  Energy Per Hour         uint16    Position    21
>  //  C9  Energy Per Minute       uint8     Position    23
>  //  C10 Heart Rate              uint8     Position    24
>  //  C11 Metabolic Equivalent    uint8     Position    25
>  //  C12 Elapsed Time            uint16    Position    26
>  //  C13 Remaining Time          uint16    Position    28
`
** Warning 2 <!> the Nordic BLE module is able to manage 30 Bytes message including header meaning 20 Bytes. ** 

If you need all the fields, then the BLE message will have to be splitted in 2 sub message.

## S4 PART


