/* 
connect: SDCARD <-> Pico <-> USB <-> PC
original: https://github.com/earlephilhower/arduino-pico/blob/master/libraries/FatFSUSB/examples/Listfiles-USB/Listfiles-USB.ino

Test results with H2testw v1.4 on Win11

Writing: 51.4 KByte/s
Reading: 62.3 KByte/s
*/


// FatFS + FatFSUSB listFiles example

#ifndef DISABLE_FS_H_WARNING
  #define DISABLE_FS_H_WARNING  // Disable warning for type File not defined. 
#endif
#ifndef ENABLE_DEDICATED_SPI
  #define ENABLE_DEDICATED_SPI 1
#endif

#include <SdFat.h>

//uncomment to use SPI
//#define USE_SPI


#ifndef USE_SPI
  #define pin_mmc_dat   36  // mmc (uses 4-bit sdio mode on data pins 36,37,38,39)
  #define pin_mmc_clk   34  // mmc
  #define pin_mmc_cmd   35  // mmc

  #define SD_CONFIG SdioConfig(pin_mmc_clk, pin_mmc_cmd, pin_mmc_dat) // Note: fourth paramter of SdioConfig is the PIO clkDiv with default 1.00.
#else
  #include <SPI.h>

  #define SD_SPI       SPI //SPI or SPI1
  #define SD_SPI0_SLCK 34
  #define SD_SPI0_MOSI 35
  #define SD_SPI0_MISO 36
  #define SD_SPI0_CS   39

  #define SD_CONFIG SdSpiConfig(SD_SPI0_CS, DEDICATED_SPI, SD_SCK_MHZ(16)) //TODO 4th argument is spi port, but SPI / SPI1 does not work
#endif


SdFs sd;

#include <FatFSUSB.h>

volatile bool updated = false;
volatile bool driveConnected = false;
volatile bool inPrinting = false;

// Called by FatFSUSB when the drive is released.  We note this, restart FatFS, and tell the main loop to rescan.
void unplug(uint32_t i) {
  (void) i;
  driveConnected = false;
  updated = true;
  //FatFS.begin();
}

// Called by FatFSUSB when the drive is mounted by the PC.  Have to stop FatFS, since the drive data can change, note it, and continue.
void plug(uint32_t i) {
  (void) i;
  driveConnected = true;
  //FatFS.end();
}

// Called by FatFSUSB to determine if it is safe to let the PC mount the USB drive.  If we're accessing the FS in any way, have any Files open, etc. then it's not safe to let the PC mount the drive.
bool mountable(uint32_t i) {
  (void) i;
  return !inPrinting;
}


void clearSerialInput() {
  uint32_t m = micros();
  do {
    if (Serial.read() >= 0) {
      m = micros();
    }
  } while (micros() - m < 10000);
}

void dmpVol() {
  if (sd.fatType() <= 32) {
    Serial.printf("Volume is FAT%d\n",(int)sd.fatType());
  } else {
    Serial.printf("Volume is exFAT\n");
  }
  Serial.printf("fatCount:          %d\n",(int)sd.fatCount());
  Serial.printf("bytesPerCluster:   %d\n",(int)sd.bytesPerCluster());
  Serial.printf("fatStartSector:    %d\n",(int)sd.fatStartSector());
  Serial.printf("dataStartSector:   %d\n",(int)sd.dataStartSector());
  Serial.printf("clusterCount:      %d\n",(int)sd.clusterCount());
  Serial.printf("freeClusterCount:  %d\n",(int)sd.freeClusterCount());
  Serial.printf("Size               %d MB\n",(int)((float)sd.clusterCount() * sd.bytesPerCluster() / 1e6));
  Serial.printf("Free               %d MB\n",(int)((float)sd.freeClusterCount() * sd.bytesPerCluster() / 1e6));

  if(sd.clusterCount()<=0) {
    Serial.printf("clusterCount failed\n");
    while(1);
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }

  Serial.println("\nSdFat version: " SD_FAT_VERSION_STR);

  #ifdef USE_SPI
    Serial.println("Using SPI interface");
    SPI.setMISO(SD_SPI0_MISO);
    SPI.setMOSI(SD_SPI0_MOSI);
    SPI.setSCK(SD_SPI0_SLCK);
    SPI.begin();
  #else
    Serial.println("Using SDIO interface");
  #endif

  if (!sd.cardBegin(SD_CONFIG)) {
    Serial.println("sd.cardBegin initialization failed!\n");
    while(1);
  }
  if (!sd.volumeBegin()) {
    Serial.println("sd.volumeBegin failed. Is the card formatted?\n");
    while(1);
  }

  dmpVol();

  // Set up callbacks
  FatFSUSB.onUnplug(unplug);
  FatFSUSB.onPlug(plug);
  FatFSUSB.driveReady(mountable);
  // Start FatFS USB drive mode
  FatFSUSB.begin();
  //Serial.println("FatFSUSB started."); //this doesn't print
  //Serial.println("Connect drive via USB to upload/erase files and re-display");
}

void loop() {
  if (updated && !driveConnected) {
    inPrinting = true;
    Serial.println("\n\nDisconnected");
    //FatFS.end();
    //FatFS.begin();
    updated = false;
    inPrinting = false;
  }
}
