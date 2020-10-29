# WaterRower S4BL3 Bluetooth BLE for S4

## Changelog
-18/10/2020 v0.30 First stable release, working (could have named it v1 ;)
  + Add BLE Battery Service to monitor Adafruit Feather Battery
  + Add BLE Factory Reset at the startup
  + Add Condition to start BLE only after USB S4 Reset confirmed 
-17/10/2020 v0.17 JTAG Debugging with JLINK EDU MINI
  + Seems that stability issue is now fixed (Added 5mc delay and the end)
  + More Reactivity with write / read collision avoid
  - Add some kpi that seems to stay at 0, m/s speed for exemple
- 12/10/2020 v0.15 BLE Finetunning
  + Avoid 2 parts BLE message to be sent
  + Fieldtest :)
  - Stability issue not fixed :( Waiting JTAG Debugger to identify Hex Core dump on SAMD21 Chipset
- 10/10/2020 v0.14 Serial flood Management
  + Core Serial fine tunning to avoid S4 flood with P&S command
  + Serial to UART Bridge
  + CDC ACM Driver modification
- 06/10/2020 v0.12 S4 Initial Work
  + USB CDC ACM initial test with USB & RESET Command
  + Serial port Management with external UART Management
- 02/10/2020 v0.1Initial Release
  + BLE data service test with fake data 
  + Initial BLE packet decoding and reverse engineering
  + GATT Fitness Service experimentation

