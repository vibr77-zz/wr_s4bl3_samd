# wr_s4bl3_samd
WaterRower S4 BLE Arduino SAM21 BLE


##Introduction

Like many, I have a WaterRower. This is a really cool and beautifull Rower with a S4 Controller. This controller has a USB port at the back and we can use it to create a USB to Bluetooth Low Energy Bridge.

The WaterRower S4 Controller use USB CDC ACM protocol. This means that UART Over USB will not work, a device with USB Host and Virtual Serial is needed.

A device with BLE (Bluetooth Low Energy) and  Native USB port capabilities is needed. ESP32 will not work as it has no Native USB Port.

SAMD MicroController can do USB CDC ACM, and the Adafruit Feather M0 Bluefruit LE is a One-In-All approach:
- ATSAMD21G18 ARM Cortex M0 processor
- nRF51822 chipset from Nordic as Bluetooth stack
- Built in battery charging, this will be very usefull to avoid heavy external power supply

https://www.adafruit.com/product/2995

It is also a quite competitive product $29.95

to be continued


