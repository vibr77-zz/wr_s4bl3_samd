//  --------------------------------------------------------------------------
//  WaterRower S4 BLE Interface
//  Hardware: Using GATT Fitness Machine Service & Rower Data Characteristics 
//  Version: 0.15
//  Date: 2020/10/10
//  Author: Vincent Besson
//  Note: Testing on Adafruit Feather MO BLE Express
//  ---------------------------------------------------------------------------

#include <cdcacm.h>
#include <usbhub.h>

#include "pgmstrings.h"

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "Adafruit_BLEGatt.h"
#include "BluefruitConfig.h"

// Global Define 

#define BLE_SERVICE_NAME "WR S4BL3"
#define REFRESH_DATA_TIME 200
//#define USE_FAKE_DATA
//#define DEBUG
#define _BUFFSIZE 64
#define _S4_PORT_SPEED 57600

#define SerialDebug Serial1                // Need to fix this as it is not working
      
#define FitnessMachineService 0x1826

#define FitnessMachineControlPoint 0x2AD9 // Not implemented yet
#define FitnessMachineFeature      0x2ACC // Not implemented yet
#define FitnessMachineStatus       0x2ADA // Not implemented yet
#define FitnessMachineRowerData    0x2AD1

class ACMAsyncOper : public CDCAsyncOper{
  public:
    uint8_t OnInit(ACM *pacm);
};

uint8_t ACMAsyncOper::OnInit(ACM *pacm){
  uint8_t rcode;
  // Set DTR = 1 RTS=1
  rcode = pacm->SetControlLineState(3);

  if (rcode){
    ErrorMessage<uint8_t>(PSTR("SetControlLineState"), rcode);
    return rcode;  
  }

  LINE_CODING	lc;
  lc.dwDTERate	= _S4_PORT_SPEED;
  lc.bCharFormat	= 0;
  lc.bParityType	= 0;
  lc.bDataBits	= 8;

  rcode = pacm->SetLineCoding(&lc);

  if (rcode)
    ErrorMessage<uint8_t>(PSTR("SetLineCoding"), rcode);

  return rcode;
}

USBHost     UsbH;
ACMAsyncOper  AsyncOper;
ACM           AcmSerial(&UsbH, &AsyncOper);

// BLE PART 

// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


Adafruit_BLEGatt gatt(ble);

// A small helper
void error(const __FlashStringHelper*err) {
  Serial1.println(err);
  while (1);
}

// Service
int32_t fitnessMachineServiceId;

// Cx
int32_t fitnessMachineControlPointId;
int32_t fitnessMachineFeatureId;
int32_t fitnessMachineStatusId;
int32_t fitnessMachineRowerDataId;

// The BLE Stack can only send 20 Bytes at a time
// We need to split the BLE Message in 2 pieces
// The Bitfield has been manage accordingly
// Warining Reading the GATT BLE Specification the 0 at the end of Part 1 means includes the field <!>

uint16_t  rowerDataFlagsP1=0b0000011111110;
uint16_t  rowerDataFlagsP2=0b1111100000001;

  //P1
  // 0000000000001 - 1   - 0x001 - More Data 0 <!> Present !!! Read the F... Manual !!!
  // 0000000000010 - 2   - 0x002 - Average Stroke present
  // 0000000000100 - 4   - 0x004 - Total Distance Present
  // 0000000001000 - 8   - 0x008 - Instantaneous Pace present
  // 0000000010000 - 16  - 0x010 - Average Pace Present
  // 0000000100000 - 32  - 0x020 - Instantaneous Power present
  // 0000001000000 - 64  - 0x040 - Average Power present
  // 0000010000000 - 128 - 0x080 - Resistance Level present
  //P2
  // 0000100000000 - 256 - 0x080 - Expended Energy present
  // 0001000000000 - 512 - 0x080 - Heart Rate present
  // 0010000000000 - 1024- 0x080 - Metabolic Equivalent present
  // 0100000000000 - 2048- 0x080 - Elapsed Time present
  // 1000000000000 - 4096- 0x080 - Remaining Time present

  //  C1  Stroke Rate             uint8     Position    2 (After the Flag 2bytes)
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

struct rowerDataKpi{
  int bpm; // Start of Part 1
  int strokeCount;
  int strokeRate;
  int averageStokeRate;
  int totalDistance;
  int instantaneousPace;
  int averagePace;
  int instantaneousPower;
  int averagePower;
  int resistanceLevel;
  int totalEnergy; // Start of Part 2
  int energyPerHour;
  int energyPerMinute;
  int heartRate;
  int metabolicEquivalent;
  int elapsedTime;
  int elapsedTimeSec;
  int elapsedTimeMin;
  int elapsedTimeHour;
  int remainingTime;
};

struct rowerDataKpi rdKpi;

/*
 * InitBLE
 * Will init the BLE module of M0 Express
 * 
 */

void initBLE(){
  randomSeed(micros());

  /* Initialise the module */
  SerialDebug.print(F("Initialising the Bluefruit LE module:"));

  if ( !ble.begin(VERBOSE_MODE) ){
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  SerialDebug.println( F("OK!") );

  /* Perform a factory reset to make sure everything is in a known state */
  SerialDebug.println(F("Performing a factory reset: "));
  if (! ble.factoryReset() ){
       error(F("Couldn't factory reset"));
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  SerialDebug.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  /* Change the device name to make it easier to find */
  SerialDebug.print("Setting device name to:");
  SerialDebug.println(BLE_SERVICE_NAME);
  
  char atCmd[128];
  sprintf(atCmd,"AT+GAPDEVNAME=%s",BLE_SERVICE_NAME);
  if (! ble.sendCommandCheckOK(F(atCmd)) ) {
    error(F("Could not set device name?"));
  }

  /* Add the Fitness Machine Service definition */
  /* Service ID should be 1 */
  SerialDebug.println(F("Adding the Fitness Machine Service definition (UUID = 0x1826): "));
  fitnessMachineServiceId = gatt.addService(FitnessMachineService);
  if (fitnessMachineServiceId == 0) {
    error(F("Could not add Fitness Machine Service"));
  }
  
  /* Add the Fitness Machine Rower Data characteristic */
  /* Chars ID for Measurement should be 1 */
  fitnessMachineRowerDataId = gatt.addCharacteristic(FitnessMachineRowerData, GATT_CHARS_PROPERTIES_NOTIFY, 1, 30, BLE_DATATYPE_BYTEARRAY);
  if (fitnessMachineRowerDataId == 0) {
    error(F("Could not add Fitness Machine Rower Data characteristic"));
  }

  SerialDebug.println(F("Adding Fitness Machine Service UUID to the advertising payload "));
  uint8_t advdata[] { 0x02, 0x01, 0x06, 0x05, 0x02, 0x26, 0x18, 0x0a, 0x18 };
  ble.setAdvData( advdata, sizeof(advdata) );
  
  /* Reset the device for the new service setting changes to take effect */
  SerialDebug.println(F("Performing a SW reset (service changes require a reset)"));
  ble.reset();

  SerialDebug.println();
}

void initFakeBleData(){
  
  rdKpi.bpm=59;
  rdKpi.strokeCount=0;
  rdKpi.strokeRate=0;
  rdKpi.averageStokeRate=25;
  rdKpi.totalDistance=1000;
  rdKpi.instantaneousPace=90;
  rdKpi.averagePace=120;
  rdKpi.instantaneousPower=70;
  rdKpi.averagePower=100;
  rdKpi.resistanceLevel=17;
  rdKpi.totalEnergy=100;
  rdKpi.energyPerHour=210;
  rdKpi.energyPerMinute=300;
  rdKpi.heartRate=120;
  rdKpi.metabolicEquivalent=10;
  rdKpi.elapsedTime=100;
  rdKpi.elapsedTimeSec=4;
  rdKpi.elapsedTimeMin=5;
  rdKpi.elapsedTimeHour=1;
  rdKpi.remainingTime=600;

}

void sendFakeBleDataP1(){

  unsigned char cRower[30];
      
  cRower[0]=rowerDataFlagsP1 & 0x000000FF;
  cRower[1]=(rowerDataFlagsP1 & 0x0000FF00) >> 8;
         
  rdKpi.strokeRate=13;
  cRower[2] = rdKpi.strokeRate & 0x000000FF;
  
  rdKpi.strokeCount++;
  cRower[3] = rdKpi.strokeCount & 0x000000FF;
  cRower[4] = (rdKpi.strokeCount & 0x0000FF00) >> 8;
  
  rdKpi.averageStokeRate=(rdKpi.averageStokeRate+1);
  cRower[5] = rdKpi.averageStokeRate & 0x000000FF;
  
  rdKpi.totalDistance=rdKpi.totalDistance+10;
  cRower[6] = rdKpi.totalDistance &  0x000000FF;
  cRower[7] = (rdKpi.totalDistance & 0x0000FF00) >> 8;
  cRower[8] = (rdKpi.totalDistance & 0x00FF0000) >> 16;
  
  cRower[9] = rdKpi.instantaneousPace & 0x000000FF;
  cRower[10] = (rdKpi.instantaneousPace & 0x0000FF00) >> 8;
  
  cRower[11] = rdKpi.averagePace & 0x000000FF;
  cRower[12] = (rdKpi.averagePace & 0x0000FF00) >> 8;

  cRower[13] = rdKpi.instantaneousPower & 0x000000FF;
  cRower[14] = (rdKpi.instantaneousPower & 0x0000FF00) >> 8;

  cRower[15] = rdKpi.averagePower & 0x000000FF;
  cRower[16] = (rdKpi.averagePower & 0x0000FF00) >> 8;

  cRower[17] = rdKpi.resistanceLevel & 0x000000FF;
  cRower[18] = (rdKpi.resistanceLevel & 0x0000FF00) >> 8;

  gatt.setChar(fitnessMachineRowerDataId, cRower, 19);
  
}

void sendFakeBleDataP2(){
 
  unsigned char cRower[13];
      
  cRower[0]=rowerDataFlagsP2 & 0x000000FF;
  cRower[1]=(rowerDataFlagsP2 & 0x0000FF00) >> 8;
         
  rdKpi.totalEnergy++;
  cRower[2] = rdKpi.totalEnergy & 0x000000FF;
  cRower[3] = (rdKpi.totalEnergy & 0x0000FF00) >> 8;

  cRower[4] = rdKpi.energyPerHour & 0x000000FF;
  cRower[5] = (rdKpi.energyPerHour & 0x0000FF00) >> 8;

  cRower[6] = rdKpi.energyPerMinute & 0x000000FF;

  cRower[7] = rdKpi.bpm & 0x000000FF;

  cRower[8] = rdKpi.metabolicEquivalent& 0x000000FF;

  rdKpi.elapsedTime++;
  cRower[9] = rdKpi.elapsedTime & 0x000000FF;
  cRower[10] = (rdKpi.elapsedTime & 0x0000FF00) >> 8;

  rdKpi.remainingTime--;
  cRower[11] = rdKpi.remainingTime & 0x000000FF;
  cRower[12] = (rdKpi.remainingTime & 0x0000FF00) >> 8;
  
  gatt.setChar(fitnessMachineRowerDataId, cRower, 13);
  
}
void initBleData(){
  
  rdKpi.bpm=0;
  rdKpi.strokeCount=0;
  rdKpi.strokeRate=0;
  rdKpi.averageStokeRate=0;
  rdKpi.totalDistance=0;
  rdKpi.instantaneousPace=0;
  rdKpi.averagePace=0;
  rdKpi.instantaneousPower=0;
  rdKpi.averagePower=0;
  rdKpi.resistanceLevel=0;
  rdKpi.totalEnergy=0;
  rdKpi.energyPerHour=0;
  rdKpi.energyPerMinute=0;
  rdKpi.heartRate=0;
  rdKpi.metabolicEquivalent=0;
  rdKpi.elapsedTime=0;
  rdKpi.elapsedTimeSec=0;
  rdKpi.elapsedTimeMin=0;
  rdKpi.elapsedTimeHour=0;
  rdKpi.remainingTime=0;
}

void sendBleLightData(){
  // This function is a subset of field to be sent in one piece
  // An alternative to the sendBleData()
  uint16_t  rowerDataFlags=0b0000101111110;
  //P1
  // 0000000000001 - 1   - 0x001 - More Data 0 <!> Present !!! Read the F... Manual !!!
  // 0000000000010 - 2   - 0x002 - Average Stroke present
  // 0000000000100 - 4   - 0x004 - Total Distance Present
  // 0000000001000 - 8   - 0x008 - Instantaneous Pace present
  // 0000000010000 - 16  - 0x010 - Average Pace Present
  // 0000000100000 - 32  - 0x020 - Instantaneous Power present
  // 0000001000000 - 64  - 0x040 - Average Power present
  // 0000010000000 - 128 - 0x080 - Resistance Level present
  //P2
  // 0000100000000 - 256 - 0x080 - Expended Energy present
  // 0001000000000 - 512 - 0x080 - Heart Rate present
  // 0010000000000 - 1024- 0x080 - Metabolic Equivalent present
  // 0100000000000 - 2048- 0x080 - Elapsed Time present
  // 1000000000000 - 4096- 0x080 - Remaining Time present

  //  0000101111110  

  //  C1  Stroke Rate             uint8     Position    2  + (After the Flag 2bytes)
  //  C1  Stroke Count            uint16    Position    3  +
  //  C2  Average Stroke Rate     uint8     Position    5  +
  //  C3  Total Distance          uint24    Position    6  +
  //  C4  Instantaneous Pace      uint16    Position    9  +
  //  C5  Average Pace            uint16    Position    11 +
  //  C6  Instantaneous Power     sint16    Position    13 +
  //  C7  Average Power           sint16    Position    15 +
  //  C8  Resistance Level        sint16    Position    17 -
  //  C9  Total Energy            uint16    Position    19 +
  //  C9  Energy Per Hour         uint16    Position    21 +
  //  C9  Energy Per Minute       uint8     Position    23 +
  //  C10 Heart Rate              uint8     Position    24 -
  //  C11 Metabolic Equivalent    uint8     Position    25 -
  //  C12 Elapsed Time            uint16    Position    26 -
  //  C13 Remaining Time          uint16    Position    28 -

  unsigned char cRower[22];

  cRower[0]=rowerDataFlags & 0x000000FF;
  cRower[1]=(rowerDataFlags & 0x0000FF00) >> 8;
  rdKpi.strokeRate=rdKpi.strokeRate*2;
  cRower[2] = rdKpi.strokeRate & 0x000000FF;
  
  cRower[3] = rdKpi.strokeCount & 0x000000FF;
  cRower[4] = (rdKpi.strokeCount & 0x0000FF00) >> 8;
  
  cRower[5] = rdKpi.averageStokeRate & 0x000000FF;
  
  cRower[6] = rdKpi.totalDistance &  0x000000FF;
  cRower[7] = (rdKpi.totalDistance & 0x0000FF00) >> 8;
  cRower[8] = (rdKpi.totalDistance & 0x00FF0000) >> 16;
  
  cRower[9] = rdKpi.instantaneousPace & 0x000000FF;
  cRower[10] = (rdKpi.instantaneousPace & 0x0000FF00) >> 8;
  
  cRower[11] = rdKpi.averagePace & 0x000000FF;
  cRower[12] = (rdKpi.averagePace & 0x0000FF00) >> 8;

  cRower[13] = rdKpi.instantaneousPower & 0x000000FF;
  cRower[14] = (rdKpi.instantaneousPower & 0x0000FF00) >> 8;

  cRower[15] = rdKpi.averagePower & 0x000000FF;
  cRower[16] = (rdKpi.averagePower & 0x0000FF00) >> 8;

  cRower[17] = rdKpi.totalEnergy & 0x000000FF;
  cRower[18] = (rdKpi.totalEnergy & 0x0000FF00) >> 8;

  cRower[19] = rdKpi.energyPerHour & 0x000000FF;
  cRower[20] = (rdKpi.energyPerHour & 0x0000FF00) >> 8;

  cRower[21] = rdKpi.energyPerMinute & 0x000000FF;

  gatt.setChar(fitnessMachineRowerDataId, cRower, 22);

}


void sendBleData(){
  // Due the size limitation of the message in the BLE Stack of the NRF
  // the message will be split in 2 parts with the according Bitfield (read the spec :) )
  
  unsigned char cRower[19]; // P1 is the biggest part whereas P2 is 13
  
  // Send the P1 part of the Message
  cRower[0]=rowerDataFlagsP1 & 0x000000FF;
  cRower[1]=(rowerDataFlagsP1 & 0x0000FF00) >> 8;
  rdKpi.strokeRate=rdKpi.strokeRate*2;
  cRower[2] = rdKpi.strokeRate & 0x000000FF;
  
  cRower[3] = rdKpi.strokeCount & 0x000000FF;
  cRower[4] = (rdKpi.strokeCount & 0x0000FF00) >> 8;
  
  cRower[5] = rdKpi.averageStokeRate & 0x000000FF;
  
  cRower[6] = rdKpi.totalDistance &  0x000000FF;
  cRower[7] = (rdKpi.totalDistance & 0x0000FF00) >> 8;
  cRower[8] = (rdKpi.totalDistance & 0x00FF0000) >> 16;
  
  cRower[9] = rdKpi.instantaneousPace & 0x000000FF;
  cRower[10] = (rdKpi.instantaneousPace & 0x0000FF00) >> 8;
  
  cRower[11] = rdKpi.averagePace & 0x000000FF;
  cRower[12] = (rdKpi.averagePace & 0x0000FF00) >> 8;

  cRower[13] = rdKpi.instantaneousPower & 0x000000FF;
  cRower[14] = (rdKpi.instantaneousPower & 0x0000FF00) >> 8;

  cRower[15] = rdKpi.averagePower & 0x000000FF;
  cRower[16] = (rdKpi.averagePower & 0x0000FF00) >> 8;

  cRower[17] = rdKpi.resistanceLevel & 0x000000FF;
  cRower[18] = (rdKpi.resistanceLevel & 0x0000FF00) >> 8;

  gatt.setChar(fitnessMachineRowerDataId, cRower, 19);

  // Send the P2 part of the Message
  cRower[0]=rowerDataFlagsP2 & 0x000000FF;
  cRower[1]=(rowerDataFlagsP2 & 0x0000FF00) >> 8;
         
  cRower[2] = rdKpi.totalEnergy & 0x000000FF;
  cRower[3] = (rdKpi.totalEnergy & 0x0000FF00) >> 8;

  cRower[4] = rdKpi.energyPerHour & 0x000000FF;
  cRower[5] = (rdKpi.energyPerHour & 0x0000FF00) >> 8;

  cRower[6] = rdKpi.energyPerMinute & 0x000000FF;

  cRower[7] = rdKpi.bpm & 0x000000FF;

  cRower[8] = rdKpi.metabolicEquivalent& 0x000000FF;

  rdKpi.elapsedTime=rdKpi.elapsedTimeSec+rdKpi.elapsedTimeMin*60+rdKpi.elapsedTimeHour*3600;
  cRower[9] = rdKpi.elapsedTime & 0x000000FF;
  cRower[10] = (rdKpi.elapsedTime & 0x0000FF00) >> 8;

  cRower[11] = rdKpi.remainingTime & 0x000000FF;
  cRower[12] = (rdKpi.remainingTime & 0x0000FF00) >> 8;
  
  gatt.setChar(fitnessMachineRowerDataId, cRower, 13);

}

bool s4InitFlag=false;
bool s4SendUsb=false;
bool bleConnectionStatus=false;
int s4KpiTurn=0;
int usbCounterCycle=0;

struct s4MemoryMap{
  char  desc[32];
  char  addr[4]; // 3+1
  char  msize[2]; //1+1
  int * kpi; // void cause can be in or lon int
  int base; // 10 for Decimal, 16 for Hex used by strtol
};

#define S4SIZE 8
struct s4MemoryMap s4mmap[S4SIZE]; 

void setup(){
  
  SerialDebug.begin(19200);
  SerialDebug.println("*");
  SerialDebug.println("Starting");
  SerialDebug.println("WaterRower S4 BLE Module v0.12");
  SerialDebug.println("Vincent Besson");
  SerialDebug.println("CR 2020");

  

  sprintf(s4mmap[0].desc,"instantaneousPower");
  sprintf(s4mmap[0].addr,"088");
  sprintf(s4mmap[0].msize,"D");
  s4mmap[0].kpi=&rdKpi.instantaneousPower;
  s4mmap[0].base=16;

  sprintf(s4mmap[1].desc,"totalDistance");
  sprintf(s4mmap[1].addr,"057");
  sprintf(s4mmap[1].msize,"D");
  s4mmap[1].kpi=&rdKpi.totalDistance;
  s4mmap[1].base=16;

  sprintf(s4mmap[2].desc,"strokeCount");
  sprintf(s4mmap[2].addr,"140");
  sprintf(s4mmap[2].msize,"D");
  s4mmap[2].kpi=&rdKpi.strokeCount;
  s4mmap[2].base=16;

  sprintf(s4mmap[3].desc,"strokeRate");
  sprintf(s4mmap[3].addr,"1A9");
  sprintf(s4mmap[3].msize,"S");
  s4mmap[3].kpi=&rdKpi.strokeRate;
  s4mmap[3].base=16;

  sprintf(s4mmap[4].desc,"elapsedTime");
  sprintf(s4mmap[4].addr,"1E8");
  sprintf(s4mmap[4].msize,"D");
  s4mmap[4].kpi=&rdKpi.elapsedTime;
  s4mmap[4].base=16;

  sprintf(s4mmap[5].desc,"elapsedTimeSec");
  sprintf(s4mmap[5].addr,"1E1");
  sprintf(s4mmap[5].msize,"S");
  s4mmap[5].kpi=&rdKpi.elapsedTimeSec;
  s4mmap[5].base=10;

  sprintf(s4mmap[6].desc,"elapsedTimeMin");
  sprintf(s4mmap[6].addr,"1E2");
  sprintf(s4mmap[6].msize,"S");
  s4mmap[6].kpi=&rdKpi.elapsedTimeMin;
  s4mmap[6].base=10;

  sprintf(s4mmap[7].desc,"elapsedTimeHour");
  sprintf(s4mmap[7].addr,"1E3");
  sprintf(s4mmap[7].msize,"S");
  s4mmap[7].kpi=&rdKpi.elapsedTimeHour;
  s4mmap[7].base=10;
  
  if (UsbH.Init())
    SerialDebug.println("USB host failed to initialize");
  
  SerialDebug.println("USB Host init OK"); 
  initBLE();
  
#ifdef USE_FAKE_DATA
  initFakeBleData();
#else
  initBleData();
#endif
  
}

void writeCdcAcm(char str[]){
  
  // Important S4 can not handle more than 25 msec

  uint8_t rcode;  //return code of the USB port
  uint8_t c;
  int ll=strlen(str)+2;  
  char buf[ll+1]; // Buffer of Bytes to be sent
  sprintf(buf,"%s\r\n",str);

  if( AcmSerial.isReady()) {

    if (ll > 0) {
      /* sending to USB CDC ACM */
#ifdef DEBUG
      SerialDebug.print("<");
      SerialDebug.print(buf);
#endif

      for (int i=0;i<ll;i++){
        c=buf[i];
#ifdef DEBUG
        SerialDebug.write(c);
#endif
        delay(10);          // By the S4 Spec Wait 10/25msec between each char sent
        rcode = AcmSerial.SndData(1, &c);
        if (rcode)
          ErrorMessage<uint8_t>(PSTR("SndData"), rcode);
      } 
    }
  }else{
    SerialDebug.print("USB CDC ACM not ready for write\n");
  }
}

void readCdcAcm(){
  
  /* reading USB CDC ACM */
  /* buffer size must be greater or equal to max.packet size */
  /* it it set to 64 (largest possible max.packet size) here, can be tuned down for particular endpoint */
  //ASCSI TABLE https://fr.wikibooks.org/wiki/Les_ASCII_de_0_%C3%A0_127/La_table_ASCII
 
  if( AcmSerial.isReady()) {

    uint8_t rcode;
    char buf[64];    
    uint16_t rcvd = sizeof(buf);

    rcode = AcmSerial.RcvData(&rcvd, (uint8_t *)buf); 
    if(rcvd){ //more than zero bytes received
      buf[rcvd]='\0';

#ifdef DEBUG
      SerialDebug.print(">");
      SerialDebug.print(rcvd);
      SerialDebug.print(",");
      SerialDebug.print(buf);
#endif
      parseS4ReceivedData(buf,rcvd);
    }
    rcode = AcmSerial.RcvData(&rcvd, (uint8_t *)buf);
    if (rcode && rcode != USB_ERRORFLOW)
      ErrorMessage<uint8_t>(PSTR("Ret"), rcode);
     
  }else{
    SerialDebug.print("USB CDC ACM not ready for read\n");
  }
}

void parseS4ReceivedData(char data[],int len){

  SerialDebug.println(data);
  if (len>2){

    if (data[len-1]=='\n' && data[len-2]=='\r'){
      data[len-2]='\0';
    }
    else{
#ifdef DEBUG
      SerialDebug.println("");
#endif
    }
    decodeS4Message(data);
  }else{
      SerialDebug.println("Inv msg");
   }
}

int asciiHexToInt(char hex[],int base){
  //https://stackoverflow.com/questions/23576827/arduino-convert-a-string-hex-ffffff-into-3-int
  int number = (int) strtol( &hex[0], NULL, base);
  return number;
}

void decodeS4Message(char cmd[]){

#ifdef DEBUG
  SerialDebug.print("=");
  SerialDebug.print(cmd);
  SerialDebug.print("\n");
#endif  
  switch (cmd[0]){
    case '_':
    
      if (!strcmp(cmd,"_WR_")){
        writeCdcAcm((char*)"IV?");
        readCdcAcm();
      }
      break;
    case 'E':
    
      break;
    case 'I': // Information Handler

      if (cmd[1]=='V'){
        if (!strcmp(cmd,"IV40210")){ // 
          SerialDebug.println("S4 Good Firmware Version");
        }
        writeCdcAcm((char*)"RESET");         // You should here a Bip on the WaterRower
        readCdcAcm();
        s4InitFlag=true;
      }
      else if (cmd[1]=='D') { // Incomming data from S4
        if (strlen(cmd)>6){
          for (int i=0;i<S4SIZE;i++){
          
            if (!strncmp(cmd+3,s4mmap[i].addr,3)){
              *s4mmap[i].kpi=asciiHexToInt(cmd+6,s4mmap[i].base);
#ifdef DEBUG
              SerialDebug.print(s4mmap[i].desc);
              SerialDebug.print(",");
              SerialDebug.print(s4mmap[i].addr);
              SerialDebug.print("=");
              SerialDebug.println(*s4mmap[i].kpi);
#endif            
              break;  
            }
          }
        }
      }
      break;
     
    case 'O':
      break;
     
    case 'P':
      break;
     
    case 'S':
      break;
  }
}

unsigned long currentTime=0;
unsigned long previousTime=0;
unsigned long pCycleTime=0;

void loop(){
  // <!> Remember delay is Evil !!! 
  // readCdcAcm is here at the top for a reason 

  currentTime=millis();
  readCdcAcm(); 
  UsbH.Task();      // Todo: test if UsbH has to be at the very top
  
  if (s4InitFlag==false && AcmSerial.isReady() ){
    if (s4SendUsb==false){
      writeCdcAcm((char*)"USB");
      s4SendUsb=true;
    }
  }else{
    
    if ((currentTime-previousTime)>REFRESH_DATA_TIME){
      previousTime=currentTime;
      if ( s4InitFlag==true && bleConnectionStatus==true ){ // Get S4 Data Only if BLE is Connected

        char cmd[7];
        sprintf(cmd,"IR%s%s",s4mmap[s4KpiTurn].msize,s4mmap[s4KpiTurn].addr); // One KPI per cycle or SAMD21 will lost message
        writeCdcAcm(cmd);

        s4KpiTurn++;
        if (s4KpiTurn==4)
        s4KpiTurn=0; 
      }
      
      // Send BLE Data 
      if (ble.isConnected() && s4InitFlag==true ){ // Start Sending BLE Data only when BLE is connected and when S4 is fully initialized
        if (bleConnectionStatus==false)
          SerialDebug.println("BLE:Connected");   
        bleConnectionStatus=true;
        
#ifdef USE_FAKE_DATA
        sendFakeBleDataP1();
        sendFakeBleDataP2();
#else
        sendBleData();
#endif

      }else{
        if (bleConnectionStatus==true)
          SerialDebug.println("BLE:Disconnected");
        bleConnectionStatus=false;
      }
    }
  }
  
  if (!AcmSerial.isReady()){
    usbCounterCycle++;
    if (usbCounterCycle>10){         // Need 32 Cycle of USB.task to init the USB
      SerialDebug.println("USB Serial is not ready sleep for 1 sec");
      delay(1000);
      usbCounterCycle=0;
    }
  }
}
