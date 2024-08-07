// RAWX_Logger_F9P_I2C

// Change Log:
// ===========
//
// July 15th 2024:
// ===============
// Updated for:
// Adafruit SAMD Boards (1.7.16) :   http://boardsmanager/All#Adafruit_SAMD_Boards
// SparkFun u-blox GNSS v3 (3.1.5) : http://librarymanager/All#SparkFun_u-blox_GNSS_v3
// Bill Greiman's SdFat (2.2.3) :    http://librarymanager/All#SdFat
// RTCZero (1.6.0) :                 http://librarymanager/All#RTCZero
// Adafruit Neopixel (1.12.3) :      http://librarymanager/All#Adafruit_NeoPixel
//
// Changes the RTC alarm code to match on HHMMSS - allowing longer log INTERVAL (up to 24 hours)
// Updates the UBX configuration functions to use addCfgValset and the values from u-blox_config_keys.h
// Corrects sd.begin
// Default to logging at 1Hz, not 4Hz
// Works with exFAT microSD

// Logs RXM-RAWX, RXM-SFRBX and TIM-TM2 data from u-blox ZED_F9P GNSS to SD card
// Also logs NAV_PVT messages (which provide the carrSoln status) and NAV-STATUS messages (which indicate a time fix for Survey_In mode)
// Also logs high precision NMEA GNGGA position solution messages which can be extracted by RTKLIB

// This code is written for the Adalogger M0 Feather
// ** Select "Adafruit Feather M0" as the board type **
// ** Set Optimize to "Fast" **
// https://www.adafruit.com/products/2796
// https://learn.adafruit.com/adafruit-feather-m0-adalogger
// Adafruit invests time and resources providing this open source design, please support Adafruit and open-source hardware by purchasing products from Adafruit!

// GNSS data is provided by the ZED-F9P FeatherWing
// ** For best results, update the ZED-F9P firmware to v1.32 **
// https://github.com/PaulZC/ZED-F9P_FeatherWing_USB

// This version uses **version 3.1.5** the SparkFun u-blox library to configure the RAWX messages via I2C,
// leaving the UART dedicated for the messages to be logged to SD card
// Feel like supporting open source hardware? Buy a board from SparkFun!
// ZED-F9P RTK2: https://www.sparkfun.com/products/15136

// Choose a good quality SD card. Some cheap cards can't handle the write rate.

// Open a new log file every INTERVAL minutes

// Define how long we should log in minutes before changing to a new file
// Sensible values are: 5, 10, 15, 20, 30, 60, etc.
// Max is 1440 (24 Hours)
const int INTERVAL = 60;

// Define how long we should wait in msec (approx.) for residual RAWX data before closing the last log file
// For a measurement rate of 1Hz (1000msec), 1100msec is a sensible value. i.e. slightly more than one measurement interval
const int dwell = 1100;

// Send serial debug messages
//#define DEBUG // Comment this line out to disable debug messages
//#define DEBUGi2c // Comment this line out to disable I2C debug messages

// Debug SerialBuffer
// Displays a "Max bufAvail:" message each time SerialBuffer.available reaches a new maximum
//#define DEBUGserialBuffer // Comment this to disable serial buffer maximum available debugging

// Connect modePin to GND to select base mode. Leave open for rover mode.
#define modePin 14 // A0 / Digital Pin 14

// Connect a normally-open push-to-close switch between swPin and GND.
// Press it to stop logging and close the log file.
#define swPin 15 // A1 / Digital Pin 15 (0.2" away from the GND pin on the Adalogger)

// Pin A2 (Digital Pin 16) is reserved for the ZED-F9P EXTINT signal
// The code uses this an an interrupt to set the NeoPixel to white
#define ExtIntPin 16 // A2 / Digital Pin 16
#define white_flash 1000 // Flash the NeoPxel white for this many milliseconds on every ExtInt

// Connect A3 (Digital Pin 17) to GND to select SURVEY_IN mode when in BASE mode
#define SurveyInPin 17 // A3 / Digital Pin 17

// Include the SparkFun u-blox Library
#include <Wire.h> //Needed for I2C to GPS
#include <SparkFun_u-blox_GNSS_v3.h> //http://librarymanager/All#SparkFun_u-blox_GNSS_v3
SFE_UBLOX_GNSS i2cGPS;

// LEDs

//#define NoLED // Uncomment this line to completely disable the LEDs
//#define NoLogLED // Uncomment this line to disable the LEDs during logging only

// NeoPixel Settings
#define NeoPixel // Uncomment this line to enable a NeoPixel on the same pin as RedLED

// The red LED flashes during SD card writes
#define RedLED 13 // The red LED on the Adalogger is connected to Digital Pin 13
// The green LED indicates that the GNSS has established a fix 
#define GreenLED 8 // The green LED on the Adalogger is connected to Digital Pin 8

// Include the Adafruit NeoPixel Library
#ifdef NeoPixel
#include <Adafruit_NeoPixel.h> // Support for the WB2812B
#define swap_red_green // Uncomment this line if your WB2812B has red and green reversed
#ifdef swap_red_green
  Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, RedLED, NEO_GRB + NEO_KHZ800); // GRB WB2812B
#else
  Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, RedLED, NEO_RGB + NEO_KHZ800); // RGB WB2812B
#endif
#define LED_Brightness 32 // 0 - 255 for WB2812B
#endif

// Fast SD card logging using Bill Greiman's SdFat
// https://github.com/greiman/SdFat
// From FatFile.h:
//   * \note Data is moved to the cache but may not be written to the
//   * storage device until sync() is called.

#include <SPI.h>
#include <SdFat.h>
const uint8_t cardSelect = 4; // Adalogger uses D4 as the SD SPI select
SdFat sd;
SdFile rawx_dataFile;
// The log filename starts with "r_" for the rover and "b_" for the static base
bool base_mode = true; // Flag to indicate if the code is in base or rover mode
char rawx_filename[] = "20000000/b_000000.ubx"; // the b will be replaced by an r if required
char dirname[] = "20000000";
long bytes_written = 0;

bool survey_in_mode = false; // Flag to indicate if the code is in survey_in mode

// Timer to indicate if an ExtInt has been received
volatile unsigned long ExtIntTimer; // Load this with millis plus white_flash to show when the ExtInt LED should be switched off

// Define packet size, buffer and buffer pointer for SD card writes
const size_t SDpacket = 512;
uint8_t serBuffer[SDpacket];
size_t bufferPointer = 0;
int numBytes;

// Battery voltage
float vbat;
#define LOWBAT 3.55 // Low battery voltage

// Include Real Time Clock support for the M0
// https://github.com/arduino-libraries/RTCZero
#include <RTCZero.h> // M0 Real Time Clock
RTCZero rtc; // Create an rtc object
volatile bool alarmFlag = false; // RTC alarm (interrupt) flag

// Count number of valid fixes before starting to log
#define maxvalfix 10 // Collect at least this many valid fixes before logging starts
int valfix = 0;

bool stop_pressed = false; // Flag to indicate if stop switch was pressed to stop logging

// Define SerialBuffer as a large RingBuffer which we will use to store the Serial1 receive data
// Actual Serial1 receive data will be copied into SerialBuffer by a timer interrupt
// https://gist.github.com/jdneo/43be30d85080b175cb5aed3500d3f989
// That way, we do not need to increase the size of the Serial1 receive buffer (by editing RingBuffer.h)
// You can use DEBUGserialBuffer to determine how big the buffer should be. Increase it if you see bufAvail get close to or reach the buffer size.
RingBufferN<16384> SerialBuffer; // Define SerialBuffer as a RingBuffer of size 16k bytes

// Loop Steps
#define init          0
#define start_rawx    1
#define open_file     2
#define write_file    3
#define new_file      4
#define close_file    5
#define restart_file  6
int loop_step = init;

// UBX and NMEA Parse State
#define looking_for_B5_dollar   0
#define looking_for_62          1
#define looking_for_class       2
#define looking_for_ID          3
#define looking_for_length_LSB  4
#define looking_for_length_MSB  5
#define processing_payload      6
#define looking_for_checksum_A  7
#define looking_for_checksum_B  8
#define sync_lost               9
#define looking_for_asterix     10
#define looking_for_csum1       11
#define looking_for_csum2       12
#define looking_for_term1       13
#define looking_for_term2       14
int ubx_nmea_state = looking_for_B5_dollar;
int ubx_length = 0;
int ubx_class = 0;
int ubx_ID = 0;
int ubx_checksum_A = 0;
int ubx_checksum_B = 0;
int ubx_expected_checksum_A = 0;
int ubx_expected_checksum_B = 0;
int nmea_char_1 = '0'; // e.g. G
int nmea_char_2 = '0'; // e.g. P
int nmea_char_3 = '0'; // e.g. G
int nmea_char_4 = '0'; // e.g. G
int nmea_char_5 = '0'; // e.g. A
int nmea_csum = 0;
int nmea_csum1 = '0';
int nmea_csum2 = '0';
int nmea_expected_csum1 = '0';
int nmea_expected_csum2 = '0';
#define max_nmea_len 100 // Maximum length for an NMEA message: use this to detect if we have lost sync while receiving an NMEA message

// Definitions for u-blox F9P UBX-format (binary) messages

// Disable NMEA output on the I2C port
bool disableI2cNMEA() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_I2COUTPROT_NMEA, 0);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Set UART1 to 230400 Baud
bool setUART1BAUD() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_UART1_BAUDRATE, 230400);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// setRAWXoff: this is the message which disables all of the messages being logged to SD card
// It also clears the NMEA high precision mode for the GPGGA message
// It also sets the main talker ID to 'GP'
bool setRAWXoff() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_TIM_TM2_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_NMEA_MAINTALKERID, UBX_NMEA_MAINTALKERID_GP);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_NMEA_HIGHPREC, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1, 0);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// setRAWXon: this is the message which enables all of the messages to be logged to SD card in one go
// It also sets the NMEA high precision mode for the GNGGA message
// It also sets the main talker ID to 'GN'
bool setRAWXon() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_UART1, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_UART1, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_TIM_TM2_UART1, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART1, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_UART1, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_NMEA_MAINTALKERID, UBX_NMEA_MAINTALKERID_GN);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_NMEA_HIGHPREC, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1, 1);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Enable the NMEA GGA and RMC messages on UART1
bool setNMEAon() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_INFMSG_NMEA_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART1, 1);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Disable the NMEA messages
bool setNMEAoff() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_INFMSG_NMEA_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART1, 0);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Set the Main NMEA Talker ID to "GP"
bool setTALKERID() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_NMEA_MAINTALKERID, UBX_NMEA_MAINTALKERID_GP);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Set the measurement rate (actually interval in ms)
bool setRATE(uint16_t interval) {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_RATE_MEAS, interval);
  result &= i2cGPS.sendCfgValset();
  return result;
}
bool setRATE_20Hz() { return setRATE(50); }
bool setRATE_10Hz() { return setRATE(100); }
bool setRATE_5Hz() { return setRATE(200); }
bool setRATE_4Hz() { return setRATE(250); }
bool setRATE_2Hz() { return setRATE(500); }
bool setRATE_1Hz() { return setRATE(1000); }

// Set the navigation dynamic model
bool setNAV(uint8_t model) {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_NAVSPG_DYNMODEL, model);
  result &= i2cGPS.sendCfgValset();
  return result;
}
bool setNAVportable() { return setNAV(DYN_MODEL_PORTABLE); }
bool setNAVstationary() { return setNAV(DYN_MODEL_STATIONARY); }
bool setNAVpedestrian() { return setNAV(DYN_MODEL_PEDESTRIAN); }
bool setNAVautomotive() { return setNAV(DYN_MODEL_AUTOMOTIVE); }
bool setNAVsea() { return setNAV(DYN_MODEL_SEA); }
bool setNAVair1g() { return setNAV(DYN_MODEL_AIRBORNE1g); }
bool setNAVair2g() { return setNAV(DYN_MODEL_AIRBORNE2g); }
bool setNAVair4g() { return setNAV(DYN_MODEL_AIRBORNE4g); }
bool setNAVwrist() { return setNAV(DYN_MODEL_WRIST); }

// Set UART2 to 230400 Baud
bool setUART2BAUD_230400() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_UART2_BAUDRATE, 230400);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Set UART2 to 115200 Baud
bool setUART2BAUD_115200() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_UART2_BAUDRATE, 115200);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Set Survey_In mode
bool setSurveyIn() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_TMODE_MODE, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_TMODE_SVIN_ACC_LIMIT, 50000); // 50000 * 0.1mm = 5m
  result &= i2cGPS.addCfgValset(UBLOX_CFG_TMODE_SVIN_MIN_DUR, 60); // 60s
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Disable Survey_In mode
bool disableSurveyIn() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_TMODE_MODE, 0);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Enable RTCM message output on UART2
// Set the value byte to 1 to send an RTCM message at RATE_MEAS; set the value to 4 to send an RTCM message at 1/4 RATE_MEAS etc.
bool setRTCMon() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_UART2, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_UART2, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_UART2, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_UART2, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_UART2, 1);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART2, 10); // Output 1230 every 10s
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Disable RTCM message output on UART2
bool setRTCMoff() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_UART2, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_UART2, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_UART2, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_UART2, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_UART2, 0);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART2, 0);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Set TimeGrid for TP1 to GPS (instead of UTC) so TIM_TM2 messages are aligned with GPS time
bool setTimeGrid() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_TP_TIMEGRID_TP1, 1);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Enable NMEA messages on UART2 for test purposes
bool setUART2nmea() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_UART2OUTPROT_NMEA, 1);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// Disable timepulse TP1
bool disableTP1() {
  bool result = i2cGPS.newCfgValset(VAL_LAYER_RAM);
  result &= i2cGPS.addCfgValset(UBLOX_CFG_TP_TP1_ENA, 0);
  result &= i2cGPS.sendCfgValset();
  return result;
}

// ExtInt interrupt service routine
void ExtInt() {
  ExtIntTimer = millis() + white_flash; // Set the timer value to white_flash milliseconds from now
}

// RTC alarm interrupt
// Must be kept as short as possible. Update the alarm time in the main loop, not here.
void alarmMatch()
{
  alarmFlag = true; // Set alarm flag
}

// TimerCounter3 functions to copy Serial1 receive data into SerialBuffer
// https://gist.github.com/jdneo/43be30d85080b175cb5aed3500d3f989
#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 16

// Set TC3 Interval (sec)
void setTimerInterval(float intervalS) {
  int compareValue = intervalS * CPU_HZ / TIMER_PRESCALER_DIV;
  if (compareValue > 65535) compareValue = 65535;
  TcCount16* TC = (TcCount16*) TC3;
  // Make sure the count is in a proportional position to where it was
  // to prevent any jitter or disconnect when changing the compare value.
  TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
  TC->CC[0].reg = compareValue;
  while (TC->STATUS.bit.SYNCBUSY == 1);
}

// Start TC3 with a specified interval
void startTimerInterval(float intervalS) {
  REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3) ;
  while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync

  TcCount16* TC = (TcCount16*) TC3;

  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use the 16-bit timer
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use match mode so that the timer counter resets when the count matches the compare register
  TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Set prescaler to 16
  TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV16;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  setTimerInterval(intervalS);

  // Enable the compare interrupt
  TC->INTENSET.reg = 0;
  TC->INTENSET.bit.MC0 = 1;

  NVIC_SetPriority(TC3_IRQn, 3); // Set the TC3 interrupt priority to 3 (lowest)
  NVIC_EnableIRQ(TC3_IRQn);

  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
}

// TC3 Interrupt Handler
void TC3_Handler() {
  TcCount16* TC = (TcCount16*) TC3;
  // If this interrupt is due to the compare register matching the timer count
  // copy any available Serial1 data into SerialBuffer
  if (TC->INTFLAG.bit.MC0 == 1) {
    TC->INTFLAG.bit.MC0 = 1;
    int available1 = Serial1.available(); // Check if there is any data waiting in the Serial1 RX buffer
    while (available1 > 0) { 
        SerialBuffer.store_char(Serial1.read()); // If there is, copy it into our RingBuffer
        available1--;
    }
  }
}

// NeoPixel Functions
// WB2812B blue LED has the highest forward voltage and is slightly dim at 3.3V. The red and green values are adapted accordingly (222 instead of 255).

#ifdef NeoPixel

// Define the NeoPixel colors
#define black 0
#define blue 1
#define cyan 2
#define green 3
#define yellow 4
#define red 5
#define magenta 6
#define white 7
#define dim_blue 8
#define dim_cyan 9
#define dim_green 10
#define dim_yellow 11
#define dim_red 12
#define dim_magenta 13
#define dim_white 14

#define to_dim 7 // Offset from bright to dim colors

int write_color = green; // Flash the NeoPixel this color during SD writes (can be set to magenta or yellow too)

volatile int last_color = black;
volatile int this_color = black;

void setLED(int color) // Set NeoPixel color
{
  if (color >= dim_blue)
  {
    pixels.setBrightness(LED_Brightness / 2); // Dim the LED brightness
  }
  if (color == black)
  {
    pixels.setPixelColor(0,0,0,0);
  }
  else if ((color == dim_blue) || (color == blue))
  {
    pixels.setPixelColor(0, pixels.Color(0,0,255)); // Set color
  }
  else if ((color == dim_cyan) || (color == cyan))
  {
    pixels.setPixelColor(0, pixels.Color(0,222,255)); // Set color
  }
  else if ((color == dim_green) || (color == green))
  {
    pixels.setPixelColor(0, pixels.Color(0,222,0)); // Set color
  }
  else if ((color == dim_yellow) || (color == yellow))
  {
    pixels.setPixelColor(0, pixels.Color(222,222,0)); // Set color
  }
  else if ((color == dim_red) || (color == red))
  {
    pixels.setPixelColor(0, pixels.Color(222,0,0)); // Set color
  }
  else if ((color == dim_magenta) || (color == magenta))
  {
    pixels.setPixelColor(0, pixels.Color(222,0,255)); // Set color
  }
  else // must be dim_white or white
  {
    pixels.setPixelColor(0, pixels.Color(222,222,255)); // Set color
  }
  pixels.show();
  if (color >= dim_blue)
  {
    pixels.setBrightness(LED_Brightness); // Reset the LED brightness
  }
  last_color = this_color;
  this_color = color;
}

#endif

// SerialBuffer DEBUG
#ifdef DEBUGserialBuffer
int maxSerialBufferAvailable = 0;
#endif

void setup()
{
#ifdef NeoPixel
  // Initialise the NeoPixel
  pixels.begin(); // This initializes the NeoPixel library.
  delay(100); // Seems necessary to make the NeoPixel start reliably 
  pixels.setBrightness(LED_Brightness); // Initialize the LED brightness
  setLED(black); // Set NeoPixel off
#ifndef NoLED
  setLED(dim_blue); // Set NeoPixel to dim blue
#endif
#else
  // initialize digital pins RedLED and GreenLED as outputs.
  pinMode(RedLED, OUTPUT); // Red LED
  pinMode(GreenLED, OUTPUT); // Green LED
  digitalWrite(RedLED, LOW); // Turn Red LED off
  digitalWrite(GreenLED, LOW); // Turn Green LED off
#ifndef NoLED
  // flash red and green LEDs on reset
  for (int i=0; i <= 4; i++) {
    digitalWrite(RedLED, HIGH);
    delay(200);
    digitalWrite(RedLED, LOW);
    digitalWrite(GreenLED, HIGH);
    delay(200);
    digitalWrite(GreenLED, LOW);
  }
#endif
#endif

  // initialize modePin (A0) as an input for the Base/Rover mode select switch
  pinMode(modePin, INPUT_PULLUP);

  // initialize swPin (A1) as an input for the stop switch
  pinMode(swPin, INPUT_PULLUP);

  // initialise ExtIntPin (A2) as an input for the EVENT switch
  pinMode(ExtIntPin, INPUT_PULLUP);
  // Attach the interrupt service routine
  // Interrupt on falling edge of the ExtInt signal
  attachInterrupt(ExtIntPin, ExtInt, FALLING);
  ExtIntTimer = millis(); // Initialise the ExtInt LED timer

  // initialise SurveyInPin (A3) as an input for the SURVEY_IN switch
  pinMode(SurveyInPin, INPUT_PULLUP);

  delay(10000); // Allow 10 sec for user to open serial monitor (Comment this line if required)
  //while (!Serial); // OR Wait for user to open the serial monitor (Comment this line as required)

  Serial.begin(115200);

  Serial.println("RAWX Logger F9P");
  Serial.println("Log GNSS RAWX data to SD card");
#ifndef NeoPixel
  Serial.println("Green LED = Initial GNSS Fix");
  Serial.println("Red LED Flash = SD Write");
#else
  Serial.println("Blue = Init");
  Serial.println("Dim Cyan = Waiting for GNSS Fix");
  Serial.println("Cyan = Checking GNSS Fix");
  Serial.println("Green flash = SD Write");
  Serial.println("Magenta flash = TIME fix in Survey_In mode");
  Serial.println("Yellow flash = fixed carrier solution");
  Serial.println("White = EVENT (ExtInt) detected");
#endif
  Serial.println("Continuous Red indicates a problem or that logging has been stopped");
  Serial.println("Initializing GNSS...");

#ifndef NoLED
#ifdef NeoPixel
  setLED(blue); // Set NeoPixel to blue
#endif
#endif

  // Initialise UBX communication over I2C
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz

#ifdef DEBUG
#ifdef DEBUGi2c
  i2cGPS.enableDebugging(); //Enable debug messages over Serial (default)
#endif
#endif

  if (i2cGPS.begin(Wire,0x42) == false) //Connect to the u-blox module using Wire port
  {
    Serial.println(F("Panic!! u-blox GNSS not detected at default I2C address. Please check wiring. Freezing!"));
#ifndef NoLED
#ifdef NeoPixel
    setLED(red); // Set NeoPixel to red
#else
    digitalWrite(RedLED, HIGH); // Turn red LED on
#endif
#endif    
    while (1);
  }
  Serial.println(F("u-blox GNSS found!"));

  // These sendCommands will timeout as the commandAck checking in processUBXpacket expects the packet to be in packetCfg, not our custom packet!
  // Turn on DEBUG to see if the commands are acknowledged (Received: CLS:5 ID:1 Payload: 6 8A) or not acknowledged (CLS:5 ID:0)
  bool response = true;
  response &= disableI2cNMEA(); //Disable NMEA messages on the I2C port leaving it clear for UBX messages
  response &= setUART1BAUD(); // Change the UART1 baud rate to 230400
  response &= setRAWXoff(); // Disable RAWX messages on UART1. Also disables the NMEA high precision mode
  response &= setNMEAoff(); // Disable NMEA messages on UART1
  response &= setTALKERID(); // Set NMEA TALKERID to GP
  response &= setRATE_1Hz(); // Set Navigation/Measurement Rate to 1Hz
  response &= setUART2BAUD_115200(); // Set UART2 Baud rate
  response &= disableSurveyIn(); // Disable Survey_In mode
  response &= setRTCMoff(); // Disable RTCM output on UART2
  response &= setTimeGrid(); // Set the TP1 TimeGrid to GPS so TIM_TM2 messages are aligned with GPS time

  if (response == false) {
    Serial.println("Panic!! Unable to initialize GNSS!");
    Serial.println("Waiting for reset...");
#ifndef NoLED
#ifdef NeoPixel
    setLED(red); // Set NeoPixel to red
#else
    digitalWrite(RedLED, HIGH); // Turn red LED on
#endif
#endif
    // don't do anything more:
    while(1);
  }

  // Check the modePin and set the navigation dynamic model
  if (digitalRead(modePin) == LOW) {
    Serial.println("BASE mode selected");
    setNAVstationary(); // Set Static Navigation Mode (use this for the Base Logger)    
  }
  else {
    base_mode = false; // Clear base_mode flag
    Serial.println("ROVER mode selected");
    // Select one mode for the mobile Rover Logger
    //setNAVportable(); // Set Portable Navigation Mode
    //setNAVpedestrian(); // Set Pedestrian Navigation Mode
    //setNAVautomotive(); // Set Automotive Navigation Mode
    //setNAVsea(); // Set Sea Navigation Mode
    setNAVair1g(); // Set Airborne <1G Navigation Mode
    //setNAVair2g(); // Set Airborne <2G Navigation Mode
    //setNAVair4g(); // Set Airborne <4G Navigation Mode
    //setNAVwrist(); // Set Wrist Navigation Mode
  }

#if defined(NoLED) || defined(NoLogLED)
  disableTP1(); // Disable the timepulse to stop the LED from flashing
#endif

  Serial1.begin(230400); // Start Serial1 at 230400 baud

  Serial.println("GNSS initialized!");

#ifndef NoLED
#ifndef NeoPixel
  // flash the red LED during SD initialisation
  digitalWrite(RedLED, HIGH);
#endif
#endif

  // Initialise SD card
  Serial.println("Initializing SD card...");
  // See if the SD card is present and can be initialized
  // Tell SdFat it has exclusive use of SPI (DEDICATED_SPI). (Change to SHARED_SPI if needed.)
  // Run the SPI bus at 8MHz.
  if (!sd.begin(SdSpiConfig(cardSelect, DEDICATED_SPI, SD_SCK_MHZ(8)))) {
    Serial.println("Panic!! SD Card Init failed, or not present!");
    Serial.println("Waiting for reset...");
#ifndef NoLED
#ifdef NeoPixel
    setLED(red); // Set NeoPixel to red
#endif
#endif
    // don't do anything more:
    while(1);
  }
  Serial.println("SD Card initialized!");

#ifndef NoLED
#ifdef NeoPixel
  setLED(dim_cyan); // Set NeoPixel to dim cyan now that the SD card is initialised
#else
  // turn red LED off
  digitalWrite(RedLED, LOW);
#endif
#endif

#ifdef NeoPixel
      write_color = green; // Reset the write color to green
#endif
          
  Serial.println("Waiting for GNSS fix...");
}

void loop() // run over and over again
{
  switch(loop_step) {
    case init: {

      static uint8_t hour = 0;
      static uint8_t min = 0;
      static uint8_t sec = 0;
      static uint16_t msec = 0;
      static uint8_t day = 0;
      static uint8_t month = 0;
      static uint16_t year = 0;
      static uint8_t fix = 0;
      static int32_t lat = 0;
      static int32_t lon = 0;
      static int32_t alt = 0;
      static int32_t speed = 0;
      static int32_t heading = 0;
      static uint16_t pdop = 0;
      static uint8_t siv = 0;

      if (i2cGPS.getPVT()) { // Poll PVT. Returns true when new data is received
        hour = i2cGPS.getHour();
        min = i2cGPS.getMinute();
        sec = i2cGPS.getSecond();
        msec = i2cGPS.getMillisecond();
        day = i2cGPS.getDay();
        month = i2cGPS.getMonth();
        year = i2cGPS.getYear();
        fix = i2cGPS.getFixType();
        lat = i2cGPS.getLatitude();
        lon = i2cGPS.getLongitude();
        alt = i2cGPS.getAltitude();
        speed = i2cGPS.getGroundSpeed();
        heading = i2cGPS.getHeading();
        pdop = i2cGPS.getPDOP();
        siv = i2cGPS.getSIV();
      }

#ifdef DEBUG
      Serial.print("\nTime: ");
      Serial.print(hour, DEC); Serial.print(':');
      Serial.print(min, DEC); Serial.print(':');
      Serial.print(sec, DEC); Serial.print('.');
      Serial.println(msec);
      Serial.print("Date: ");
      Serial.print(day, DEC); Serial.print('/');
      Serial.print(month, DEC); Serial.print("/");
      Serial.println(year, DEC);
      Serial.print("Fix: "); Serial.println(fix);
      if (fix > 0) {
        Serial.print("Location: ");
        float latitude = ((float)lat) / 10000000.0;
        Serial.print(F("Lat: "));
        Serial.print(latitude, 6);
        float longitude = ((float)lon) / 10000000.0;
        Serial.print(F(" Lon: "));
        Serial.print(longitude, 6);
        Serial.print(F(" (degrees)"));
        float altitude = ((float)alt) / 1000.0;
        Serial.print(F(" Alt: "));
        Serial.print(altitude, 2);
        Serial.println(F(" (m)"));

        float speed_f = ((float)speed) / 1000.0;
        Serial.print("Ground Speed (m/s): "); Serial.println(speed_f, 3);
        float heading_f = ((float)heading) / 10000000.0;
        Serial.print("Heading: "); Serial.println(heading_f, 1);
        Serial.print("Satellites: "); Serial.println(siv);
        float PDOP = ((float)pdop) / 100.0;
        Serial.print("PDOP: "); Serial.println(PDOP, 2);
      }
#endif
  
      // read battery voltage
      vbat = analogRead(A7) * (2.0 * 3.3 / 1023.0);
#ifdef DEBUG
      Serial.print("Battery(V): ");
      Serial.println(vbat, 2);
#endif
    
      // turn green LED on to indicate GNSS fix
      // or set NeoPixel to cyan
      if (fix >= 3) {
#ifndef NoLED
#ifdef NeoPixel
        setLED(cyan); // Set NeoPixel to cyan
#else
        digitalWrite(GreenLED, HIGH);
#endif
#endif
        // increment valfix and cap at maxvalfix
        // don't do anything fancy in terms of decrementing valfix as we want to keep logging even if the fix is lost
        valfix += 1;
        if (valfix > maxvalfix) valfix = maxvalfix;
      }
      else {
#ifndef NoLED
#ifdef NeoPixel
        setLED(dim_cyan); // Set NeoPixel to dim cyan
#else
        digitalWrite(GreenLED, LOW); // Turn green LED off
#endif
#endif
      }

      if (valfix == maxvalfix) { // wait until we have enough valid fixes

        // Set and start the RTC
        alarmFlag = false; // Make sure alarm flag is clear
        rtc.begin(); // Start the RTC
        rtc.setTime(hour, min, sec); // Set the time
        rtc.setDate(day, month, (uint8_t)(year - 2000)); // Set the date
        delay(1100); // Wait for 1s in case interval is 1440
        uint8_t nextAlarmHour = hour;
        uint16_t interval = INTERVAL; // Get the INTERVAL
        if (interval >= 1440) // Limit the interval to 24 hours
          interval = 0; // An interval of zero produces an alarm every 24 hours
        uint16_t nextAlarmMin = ((uint16_t)min) + interval; // Add on the INTERVAL
        while (nextAlarmMin >= 60) { // Increment the hour
          nextAlarmMin -= 60;
          nextAlarmHour++;
        }
        nextAlarmHour %= 24; // Day rollover
        rtc.setAlarmTime(nextAlarmHour, nextAlarmMin, sec); // Set the alarm time
        rtc.enableAlarm(rtc.MATCH_HHMMSS); // Alarm Match on hours, minutes and seconds
        rtc.attachInterrupt(alarmMatch); // Attach alarm interrupt

        // check if voltage is > LOWBAT(V), if not then don't try to log any data
        if (vbat < LOWBAT) {
          Serial.println("Low Battery!");
          break;
        }

        // Set the RAWX measurement rate
        //setRATE_20Hz(); // Set Navigation/Measurement Rate to 20 Hz
        //setRATE_10Hz(); // Set Navigation/Measurement Rate to 10 Hz
        //setRATE_5Hz(); // Set Navigation/Measurement Rate to 5 Hz
        //setRATE_4Hz(); // Set Navigation/Measurement Rate to 4 Hz
        //setRATE_2Hz(); // Set Navigation/Measurement Rate to 2 Hz
        setRATE_1Hz(); // Set Navigation/Measurement Rate to 1 Hz
        
        // If we are in BASE mode, check the SURVEY_IN pin
        if (base_mode == true) {
          if (digitalRead(SurveyInPin) == LOW) {
            // We are in BASE mode and the SURVEY_IN pin is low so send the extra UBX messages:
            Serial.println("SURVEY_IN mode selected");
            survey_in_mode = true; // Set the survey_in_mode flag true
            setRTCMon(); // Enable the RTCM messages on UART2
            delay(1100);
            setSurveyIn(); // Enable SURVEY_IN mode
            delay(1100);
          }
        }
        
        while(Serial1.available()){Serial1.read();} // Flush RX buffer to clear any old data

        // Now that Serial1 should be idle and the buffer empty, start TC3 interrupts to copy all new data into SerialBuffer
        // Set the timer interval to 10 * 10 / 230400 = 0.000434 secs (10 bytes * 10 bits (1 start, 8 data, 1 stop) at 230400 baud)
        startTimerInterval(0.000434); 
        
        loop_step = start_rawx; // start rawx messages
      }
    }
    break;

    // (Re)Start RAWX messages
    case start_rawx: {
      setRAWXon(); // (Re)Start the UBX and NMEA messages

      bufferPointer = 0; // (Re)initialise bufferPointer

      loop_step = open_file; // start logging rawx data
    }
    break;

    // Open the log file
    case open_file: {
      
      // Get the RTC time and date
      uint8_t RTCseconds = rtc.getSeconds();
      uint8_t RTCminutes = rtc.getMinutes();
      uint8_t RTChours = rtc.getHours();
      uint8_t RTCday = rtc.getDay();
      uint8_t RTCmonth = rtc.getMonth();
      uint8_t RTCyear = rtc.getYear();
      
      // Do the divides to convert date and time to char
      char secT = RTCseconds/10 + '0';
      char secU = RTCseconds%10 + '0';
      char minT = RTCminutes/10 + '0';
      char minU = RTCminutes%10 + '0';
      char hourT = RTChours/10 + '0';
      char hourU = RTChours%10 + '0';
      char dayT = RTCday/10 +'0';
      char dayU = RTCday%10 +'0';
      char monT = RTCmonth/10 +'0';
      char monU = RTCmonth%10 +'0';
      char yearT = RTCyear/10 +'0';
      char yearU = RTCyear%10 +'0';
  
      // filename is limited to 8.3 characters so use format: YYYYMMDD/b_HHMMSS.ubx or YYYYMMDD/r_HHMMSS.ubx
      rawx_filename[2] = yearT;
      rawx_filename[3] = yearU;
      rawx_filename[4] = monT;
      rawx_filename[5] = monU;
      rawx_filename[6] = dayT;
      rawx_filename[7] = dayU;
      if (base_mode == false) rawx_filename[9] = 'r';
      rawx_filename[11] = hourT;
      rawx_filename[12] = hourU;
      rawx_filename[13] = minT;
      rawx_filename[14] = minU;
      rawx_filename[15] = secT;
      rawx_filename[16] = secU;
      
      dirname[2] = yearT;
      dirname[3] = yearU;
      dirname[4] = monT;
      dirname[5] = monU;
      dirname[6] = dayT;
      dirname[7] = dayU;


      // flash red LED to indicate SD write (leave on if an error occurs)
      // or flash NeoPixel green
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(green); // Set the NeoPixel to green
#else
      setLED(black); // Turn NeoPixel off if NoLogLED
#endif
#else
#ifndef NoLogLED
      digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#else
      digitalWrite(RedLED, LOW); // Turn the red LED off for NoLogLED
      digitalWrite(GreenLED, LOW); // Turn the green LED off for NoLogLED
#endif
#endif
#endif

      // try to create subdirectory (even if it exists already)
      sd.mkdir(dirname);
      
      // Open the rawx file for fast writing
      if (rawx_dataFile.open(rawx_filename, O_CREAT | O_WRITE | O_EXCL)) {
        Serial.print("Logging to ");
        Serial.println(rawx_filename);
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.println("Panic!! Error opening RAWX file!");
        Serial.println("Waiting for reset...");
#ifndef NoLED
#ifdef NeoPixel
      setLED(red); // Set the NeoPixel to red to indicate a problem
#else
      digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate a problem
#endif
#endif
        // don't do anything more:
        while(1);
      }

#ifdef DEBUG
      // Set the log file creation time
      if (!rawx_dataFile.timestamp(T_CREATE, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds)) {
        Serial.println("Warning! Could not set file create timestamp!");
      }
#else
      rawx_dataFile.timestamp(T_CREATE, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds);
#endif

      // Now that SD write is complete
      // Turn the Red LED off or set NeoPixel to dim green
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(dim_green);
#endif
#else
      digitalWrite(RedLED, LOW); // turn red LED off
#endif
#endif

      bytes_written = 0; // Clear bytes_written

      ubx_nmea_state = looking_for_B5_dollar; // set ubx_nmea_state to expect B5 or $
      ubx_length = 0; // set ubx_length to zero

      loop_step = write_file; // start logging rawx data
    }
    break;

    // Stuff bytes into serBuffer and write when we have reached SDpacket
    case write_file: {
      
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      // Check if NeoPixel should be flashed white
      if (this_color != white) { // Skip if the NeoPixel is already white
        // Check if the NeoPixel should be flashed white
        if (millis() < ExtIntTimer) {
          setLED(white); // Set the NeoPixel to white to indicate an ExtInt
        }
      }
      else { // NeoPixel must already be white so check if it should be turned off
        // Check if the timer has expired
        if (millis() > ExtIntTimer) {
          setLED(last_color); // Set the NeoPixel to the previous color
        }
      }
#endif
#endif
#endif

      int bufAvail = SerialBuffer.available();
      if (bufAvail > 0) {
#ifdef DEBUGserialBuffer
        if (bufAvail > maxSerialBufferAvailable) {
          maxSerialBufferAvailable = bufAvail;
          Serial.print("Max bufAvail: ");
          Serial.println(maxSerialBufferAvailable);
        }
#endif  
        uint8_t c = SerialBuffer.read_char();
        serBuffer[bufferPointer] = c;
        bufferPointer++;
        if (bufferPointer == SDpacket) {
          bufferPointer = 0;
          // Flash the red LED to indicate an SD write
          // or flash the NeoPixel green
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
          if (this_color != white) { // If the NeoPixel is not currently white
            setLED(write_color); // Set the NeoPixel
          }
          else { // If the NeoPixel is white, set last_color to write_color so it will revert to that when the white flash is complete
            last_color = write_color;
          }
#endif
#else
#ifndef NoLogLED
          digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#endif
#endif
#endif
          numBytes = rawx_dataFile.write(&serBuffer, SDpacket);
          //rawx_dataFile.sync(); // Sync the file system
          bytes_written += SDpacket;
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
          if (this_color != white) { // If the NeoPixel is not currently white
            setLED(write_color + to_dim); // Set the NeoPixel
          }
          else { // If the NeoPixel is white, set last_color to dim_write_color so it will revert to that when the white flash is complete
            last_color = write_color + to_dim;
          }
#endif
#else
#ifndef NoLogLED
          digitalWrite(RedLED, LOW); // Turn the red LED off to indicate SD card write is complete
#endif
#endif
#endif
#ifdef DEBUG
          if (numBytes != SDpacket) {
            Serial.print("SD write error! Write size was ");
            Serial.print(SDpacket);
            Serial.print(". ");
            Serial.print(numBytes);
            Serial.println(" were written.");
          }
#endif
        }
        // Process data bytes according to ubx_nmea_state
        // For UBX messages:
        // Sync Char 1: 0xB5
        // Sync Char 2: 0x62
        // Class byte
        // ID byte
        // Length: two bytes, little endian
        // Payload: length bytes
        // Checksum: two bytes
        // For NMEA messages:
        // Starts with a '$'
        // The next five characters indicate the message type (stored in nmea_char_1 to nmea_char_5)
        // Message fields are comma-separated
        // Followed by an '*'
        // Then a two character checksum (the logical exclusive-OR of all characters between the $ and the * as ASCII hex)
        // Ends with CR LF
        // Only allow a new file to be opened when a complete packet has been processed and ubx_nmea_state has returned to "looking_for_B5_dollar"
        // Or when a data error is detected (sync_lost)
        switch (ubx_nmea_state) {
          case (looking_for_B5_dollar): {
            if (c == 0xB5) { // Have we found Sync Char 1 (0xB5) if we were expecting one?
              ubx_nmea_state = looking_for_62; // Now look for Sync Char 2 (0x62)
            }
            else if (c == '$') { // Have we found an NMEA '$' if we were expecting one?
              ubx_nmea_state = looking_for_asterix; // Now keep going until we receive an asterix
              ubx_length = 0; // Reset ubx_length then use it to track which character has arrived
              nmea_csum = 0; // Reset the nmea_csum. Update it as each character arrives
              nmea_char_1 = '0'; // Reset the first five NMEA chars to something invalid
              nmea_char_2 = '0';
              nmea_char_3 = '0';
              nmea_char_4 = '0';
              nmea_char_5 = '0';
            }
            else {
              Serial.println("Panic!! Was expecting Sync Char 0xB5 or an NMEA $ but did not receive one!");
              ubx_nmea_state = sync_lost;
            }
          }
          break;
          case (looking_for_62): {
            if (c == 0x62) { // Have we found Sync Char 2 (0x62) when we were expecting one?
              ubx_expected_checksum_A = 0; // Reset the expected checksum
              ubx_expected_checksum_B = 0;
              ubx_nmea_state = looking_for_class; // Now look for Class byte
            }
            else {
              Serial.println("Panic!! Was expecting Sync Char 0x62 but did not receive one!");
              ubx_nmea_state = sync_lost;
            }
          }
          break;
          // RXM_RAWX is class 0x02 ID 0x15
          // RXM_SFRBF is class 0x02 ID 0x13
          // TIM_TM2 is class 0x0d ID 0x03
          // NAV_POSLLH is class 0x01 ID 0x02
          // NAV_PVT is class 0x01 ID 0x07
          // NAV-STATUS is class 0x01 ID 0x03
          case (looking_for_class): {
            ubx_class = c;
            ubx_expected_checksum_A = ubx_expected_checksum_A + c; // Update the expected checksum
            ubx_expected_checksum_B = ubx_expected_checksum_B + ubx_expected_checksum_A;
            ubx_nmea_state = looking_for_ID; // Now look for ID byte
#ifdef DEBUG
            // Class syntax checking
            if ((ubx_class != 0x02) and (ubx_class != 0x0d) and (ubx_class != 0x01)) {
              Serial.println("Panic!! Was expecting Class of 0x02 or 0x0d or 0x01 but did not receive one!");
              ubx_nmea_state = sync_lost;
            }
#endif
          }
          break;
          case (looking_for_ID): {
            ubx_ID = c;
            ubx_expected_checksum_A = ubx_expected_checksum_A + c; // Update the expected checksum
            ubx_expected_checksum_B = ubx_expected_checksum_B + ubx_expected_checksum_A;
            ubx_nmea_state = looking_for_length_LSB; // Now look for length LSB
#ifdef DEBUG
            // ID syntax checking
            if ((ubx_class == 0x02) and ((ubx_ID != 0x15) and (ubx_ID != 0x13))) {
              Serial.println("Panic!! Was expecting ID of 0x15 or 0x13 but did not receive one!");
              ubx_nmea_state = sync_lost;
            }
            else if ((ubx_class == 0x0d) and (ubx_ID != 0x03)) {
              Serial.println("Panic!! Was expecting ID of 0x03 but did not receive one!");
              ubx_nmea_state = sync_lost;
            }
            else if ((ubx_class == 0x01) and ((ubx_ID != 0x02) and (ubx_ID != 0x07) and (ubx_ID != 0x03))) {
              Serial.println("Panic!! Was expecting ID of 0x02 or 0x07 or 0x03 but did not receive one!");
              ubx_nmea_state = sync_lost;
            }
#endif
          }
          break;
          case (looking_for_length_LSB): {
            ubx_length = c; // Store the length LSB
            ubx_expected_checksum_A = ubx_expected_checksum_A + c; // Update the expected checksum
            ubx_expected_checksum_B = ubx_expected_checksum_B + ubx_expected_checksum_A;
            ubx_nmea_state = looking_for_length_MSB; // Now look for length MSB
          }
          break;
          case (looking_for_length_MSB): {
            ubx_length = ubx_length + (c * 256); // Add the length MSB
            ubx_expected_checksum_A = ubx_expected_checksum_A + c; // Update the expected checksum
            ubx_expected_checksum_B = ubx_expected_checksum_B + ubx_expected_checksum_A;
            ubx_nmea_state = processing_payload; // Now look for payload bytes (length: ubx_length)
          }
          break;
          case (processing_payload): {
            // If this is a NAV_PVT message, check the flags byte (byte offset 21) and report the carrSoln
            if ((ubx_class == 0x01) and (ubx_ID == 0x07)) { // Is this a NAV_PVT message (class 0x01 ID 0x07)?
              if (ubx_length == 71) { // Is this byte offset 21? (ubx_length will be 92 for byte offset 0, so will be 71 for byte offset 21)
#ifdef DEBUG
                Serial.print("NAV_PVT carrSoln: ");
                if ((c & 0xc0) == 0x00) {
                  Serial.println("none");
                }
                else if ((c & 0xc0) == 0x40) {
                  Serial.println("floating");
                }
                else if ((c & 0xc0) == 0x80) {
                  Serial.println("fixed");
                }
#endif
                if ((c & 0xc0) == 0x80) { // Have we got a fixed carrier solution?
#ifndef NoLED
#ifdef NeoPixel
                  if (write_color == green) { // Check that write_color is green before changing it to yellow, to give magenta priority
                    write_color = yellow; // Change the SD write color to yellow to indicate fixed carrSoln
                  }
#else
#ifndef NoLogLED
                  digitalWrite(GreenLED, !digitalRead(GreenLED)); // Toggle the green LED
#endif
#endif
#endif         
                }
                else { // carrSoln is not fixed
#ifndef NoLED
#ifdef NeoPixel
                  if (write_color == yellow) {
                    write_color = green; // Reset the SD write color to green only if it was yellow previously
                  }
#else
#ifndef NoLogLED
                  digitalWrite(GreenLED, HIGH); // If the fix is not TIME, leave the green LED on
#endif
#endif
#endif
                }
              }
            }
            // If this is a NAV_STATUS message, check the gpsFix byte (byte offset 4) and flash the green LED (or make the NeoPixel magenta) if the fix is TIME
            if ((ubx_class == 0x01) and (ubx_ID == 0x03)) { // Is this a NAV_STATUS message (class 0x01 ID 0x03)?
              if (ubx_length == 12) { // Is this byte offset 4? (ubx_length will be 16 for byte offset 0, so will be 12 for byte offset 4)
#ifdef DEBUG
                Serial.print("NAV_STATUS gpsFix: ");
                if (c == 0x00) {
                  Serial.println("no fix");
                }
                else if (c == 0x01) {
                  Serial.println("dead reckoning");
                }
                else if (c == 0x02) {
                  Serial.println("2D-fix");
                }
                else if (c == 0x03) {
                  Serial.println("3D-fix");
                }
                else if (c == 0x04) {
                  Serial.println("GPS + dead reckoning");
                }
                else if (c == 0x05) {
                  Serial.println("time");
                }
                else {
                  Serial.println("reserved");
                }
#endif
                if (c == 0x05) { // Have we got a TIME fix?
#ifndef NoLED
#ifdef NeoPixel
                  write_color = magenta; // Change the SD write color to magenta to indicate time fix (trumps yellow!)
#else
#ifndef NoLogLED
                  digitalWrite(GreenLED, !digitalRead(GreenLED)); // Toggle the green LED
#endif
#endif
#endif            
                }
                else {
#ifndef NoLED
#ifdef NeoPixel
                  if (write_color == magenta) {
                    write_color = green; // Reset the SD write color to green only if it was magenta previously (not yellow)
                  }
#else
#ifndef NoLogLED
                  digitalWrite(GreenLED, HIGH); // If the fix is not TIME, leave the green LED on
#endif
#endif
#endif
                }
              }
            }
            ubx_length = ubx_length - 1; // Decrement length by one
            ubx_expected_checksum_A = ubx_expected_checksum_A + c; // Update the expected checksum
            ubx_expected_checksum_B = ubx_expected_checksum_B + ubx_expected_checksum_A;
            if (ubx_length == 0) {
              ubx_expected_checksum_A = ubx_expected_checksum_A & 0xff; // Limit checksums to 8-bits
              ubx_expected_checksum_B = ubx_expected_checksum_B & 0xff;
              ubx_nmea_state = looking_for_checksum_A; // If we have received length payload bytes, look for checksum bytes
            }
          }
          break;
          case (looking_for_checksum_A): {
            ubx_checksum_A = c;
            ubx_nmea_state = looking_for_checksum_B;
          }
          break;
          case (looking_for_checksum_B): {
            ubx_checksum_B = c;
            ubx_nmea_state = looking_for_B5_dollar; // All bytes received so go back to looking for a new Sync Char 1 unless there is a checksum error
            if ((ubx_expected_checksum_A != ubx_checksum_A) or (ubx_expected_checksum_B != ubx_checksum_B)) {
              Serial.println("Panic!! UBX checksum error!");
              ubx_nmea_state = sync_lost;
            }
          }
          break;
          // NMEA messages
          case (looking_for_asterix): {
            ubx_length++; // Increase the message length count
            if (ubx_length > max_nmea_len) { // If the length is greater than max_nmea_len, something bad must have happened (sync_lost)
              Serial.println("Panic!! Excessive NMEA message length!");
              ubx_nmea_state = sync_lost;
              break;
            }
            // If this is one of the first five characters, store it
            // May be useful for on-the-fly message parsing or DEBUG
            if (ubx_length <= 5) {
              if (ubx_length == 1) {
                nmea_char_1 = c;
              }
              else if (ubx_length == 2) {
                nmea_char_2 = c;
              }
              else if (ubx_length == 3) {
                nmea_char_3 = c;
              }
              else if (ubx_length == 4) {
                nmea_char_4 = c;
              }
              else { // ubx_length == 5
                nmea_char_5 = c;
#ifdef DEBUG
                Serial.print("NMEA message type is: ");
                Serial.print(char(nmea_char_1));
                Serial.print(char(nmea_char_2));
                Serial.print(char(nmea_char_3));
                Serial.print(char(nmea_char_4));
                Serial.println(char(nmea_char_5));
#endif              
              }
            }
            // Now check if this is an '*'
            if (c == '*') {
              // Asterix received
              // Don't exOR it into the checksum
              // Instead calculate what the expected checksum should be (nmea_csum in ASCII hex)
              nmea_expected_csum1 = ((nmea_csum & 0xf0) >> 4) + '0'; // Convert MS nibble to ASCII hex
              if (nmea_expected_csum1 >= ':') { nmea_expected_csum1 += 7; } // : follows 9 so add 7 to convert to A-F
              nmea_expected_csum2 = (nmea_csum & 0x0f) + '0'; // Convert LS nibble to ASCII hex
              if (nmea_expected_csum2 >= ':') { nmea_expected_csum2 += 7; } // : follows 9 so add 7 to convert to A-F
              // Next, look for the first csum character
              ubx_nmea_state = looking_for_csum1;
              break; // Don't include the * in the checksum
            }
            // Now update the checksum
            // The checksum is the exclusive-OR of all characters between the $ and the *
            nmea_csum = nmea_csum ^ c;
          }
          break;
          case (looking_for_csum1): {
            // Store the first NMEA checksum character
            nmea_csum1 = c;
            ubx_nmea_state = looking_for_csum2;
          }
          break;
          case (looking_for_csum2): {
            // Store the second NMEA checksum character
            nmea_csum2 = c;
            // Now check if the checksum is correct
            if ((nmea_csum1 != nmea_expected_csum1) or (nmea_csum2 != nmea_expected_csum2)) {
              // The checksum does not match so sync_lost
              Serial.println("Panic!! NMEA checksum error!");
              ubx_nmea_state = sync_lost;
            }
            else {
              // Checksum was valid so wait for the terminators
              ubx_nmea_state = looking_for_term1;
            }
          }
          break;
          case (looking_for_term1): {
            // Check if this is CR
            if (c != '\r') {
              Serial.println("Panic!! NMEA CR not found!");
              ubx_nmea_state = sync_lost;
            }
            else {
              ubx_nmea_state = looking_for_term2;
            }
          }
          break;
          case (looking_for_term2): {
            // Check if this is LF
            if (c != '\n') {
              Serial.println("Panic!! NMEA LF not found!");
              ubx_nmea_state = sync_lost;
            }
            else {
              // LF was received so go back to looking for B5 or a $
              ubx_nmea_state = looking_for_B5_dollar;
            }
          }
          break;
        }
      }
      else {
        // read battery voltage
        vbat = analogRead(A7) * (2.0 * 3.3 / 1023.0);
      }
      // Check if the stop button has been pressed or battery is low
      // or if there has been an RTC alarm and it is time to open a new file
      if (digitalRead(swPin) == LOW) stop_pressed = true;
      if ((stop_pressed == true) or (vbat < LOWBAT)) {
        loop_step = close_file; // now close the file
        break;
      }
      else if ((alarmFlag == true) and (ubx_nmea_state == looking_for_B5_dollar)) {
        loop_step = new_file; // now close the file and open a new one
        break;
      }
      else if (ubx_nmea_state == sync_lost) {
        loop_step = restart_file; // Sync has been lost so stop RAWX messages and open a new file before restarting RAWX
      }
    }
    break;

    // Close the current log file and open a new one without stopping RAWX messages
    case new_file: {
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(green); // Set the NeoPixel to green
#endif
#else
#ifndef NoLogLED
      digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#endif
#endif
#endif
      // If there is any data left in serBuffer, write it to file
      if (bufferPointer > 0) {
        numBytes = rawx_dataFile.write(&serBuffer, bufferPointer); // Write remaining data
        rawx_dataFile.sync(); // Sync the file system
        bytes_written += bufferPointer;
#ifdef DEBUG
        if (numBytes != bufferPointer) {
          Serial.print("SD write error! Write size was ");
          Serial.print(bufferPointer);
          Serial.print(". ");
          Serial.print(numBytes);
          Serial.println(" were written.");
        }
#endif
        bufferPointer = 0; // reset bufferPointer
      }
      // Get the RTC time and date
      uint8_t RTCseconds = rtc.getSeconds();
      uint8_t RTCminutes = rtc.getMinutes();
      uint8_t RTChours = rtc.getHours();
      uint8_t RTCday = rtc.getDay();
      uint8_t RTCmonth = rtc.getMonth();
      uint8_t RTCyear = rtc.getYear();
#ifdef DEBUG
      // Set log file write time
      if (!rawx_dataFile.timestamp(T_WRITE, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds)) {
        Serial.println("Warning! Could not set file write timestamp!");
      }
#else
      rawx_dataFile.timestamp(T_WRITE, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds);
#endif
#ifdef DEBUG
      // Set log file access time
      if (!rawx_dataFile.timestamp(T_ACCESS, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds)) {
        Serial.println("Warning! Could not set file access timestamp!");
      }
#else
      rawx_dataFile.timestamp(T_ACCESS, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds);
#endif      
      rawx_dataFile.close(); // close the file
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(dim_green); // Set the NeoPixel to dim green
#endif
#else
      digitalWrite(RedLED, LOW); // Turn the red LED off to indicate SD card write is complete
#endif
#endif
#ifdef DEBUG
      uint32_t filesize = rawx_dataFile.fileSize(); // Get the file size
      Serial.print("File size is ");
      Serial.println(filesize);
      Serial.print("File size should be ");
      Serial.println(bytes_written);
#endif
      Serial.println("File closed!");
      // An RTC alarm was detected, so add INTERVAL to the RTC alarm time and loop back to open_file.
      alarmFlag = false; // Clear the RTC alarm flag
      delay(1100); // Wait for 1s in case interval is 1440
      uint8_t alarmH = rtc.getAlarmHours();
      uint16_t alarmM = rtc.getAlarmMinutes();
      uint8_t alarmS = rtc.getAlarmSeconds();
      uint16_t interval = INTERVAL; // Get the INTERVAL
      if (interval >= 1440) // Limit the interval to 24 hours
        interval = 0; // An interval of zero produces an alarm every 24 hours
      alarmM += interval; // Add on the INTERVAL
      while (alarmM >= 60) { // Increment the hour
        alarmM -= 60;
        alarmH++;
      }
      alarmH %= 24; // Day rollover
      rtc.setAlarmTime(alarmH, alarmM, alarmS); // Set the alarm time
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(cyan); // Set the NeoPixel to cyan
#else
      setLED(black); // Turn NeoPixel off if NoLogLED
#endif
#endif
#endif
      loop_step = open_file; // loop round again and open a new file
      bytes_written = 0; // Clear bytes_written
    }
    break;

    // Disable RAWX messages, save any residual data and close the file, possibly for the last time
    case close_file: {
      setRAWXoff(); // Disable RAWX messages
      int waitcount = 0;
      while (waitcount < dwell) { // Wait for residual data
        while (SerialBuffer.available()) {
          serBuffer[bufferPointer] = SerialBuffer.read_char(); // Put extra bytes into serBuffer
          bufferPointer++;
          if (bufferPointer == SDpacket) { // Write a full packet
            bufferPointer = 0;
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
            setLED(green); // Set the NeoPixel to green
#endif
#else
#ifndef NoLogLED
            digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#endif
#endif
#endif
            numBytes = rawx_dataFile.write(&serBuffer, SDpacket);
            //rawx_dataFile.sync(); // Sync the file system
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
            setLED(dim_green); // Set the NeoPixel to dim green
#endif
#else
            digitalWrite(RedLED, LOW); // Turn the red LED off to indicate SD card write is complete
#endif
#endif
            bytes_written += SDpacket;
#ifdef DEBUG
            if (numBytes != SDpacket) {
              Serial.print("SD write error! Write size was ");
              Serial.print(SDpacket);
              Serial.print(". ");
              Serial.print(numBytes);
              Serial.println(" were written.");
            }
#endif
          }
        }
        waitcount++;
        delay(1);
      }
      // If there is any data left in serBuffer, write it to file
      if (bufferPointer > 0) {
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
        setLED(green); // Set the NeoPixel to green
#endif
#else
#ifndef NoLogLED
        digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#endif
#endif
#endif
        numBytes = rawx_dataFile.write(&serBuffer, bufferPointer); // Write remaining data
        rawx_dataFile.sync(); // Sync the file system
        bytes_written += bufferPointer;
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
        setLED(dim_green); // Set the NeoPixel to dim green
#endif
#else
        digitalWrite(RedLED, LOW); // Turn the red LED off to indicate SD card write is complete
#endif
#endif
#ifdef DEBUG
        if (numBytes != bufferPointer) {
          Serial.print("SD write error! Write size was ");
          Serial.print(bufferPointer);
          Serial.print(". ");
          Serial.print(numBytes);
          Serial.println(" were written.");
        }
        Serial.print("Final SD Write: ");
        Serial.print(bufferPointer);
        Serial.println(" Bytes");
        Serial.print(bytes_written);
        Serial.println(" Bytes written");
#endif
        bufferPointer = 0; // reset bufferPointer
      }
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(green); // Set the NeoPixel to green
#endif
#else
#ifndef NoLogLED
      digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#endif
#endif
#endif
      // Get the RTC time and date
      uint8_t RTCseconds = rtc.getSeconds();
      uint8_t RTCminutes = rtc.getMinutes();
      uint8_t RTChours = rtc.getHours();
      uint8_t RTCday = rtc.getDay();
      uint8_t RTCmonth = rtc.getMonth();
      uint8_t RTCyear = rtc.getYear();
#ifdef DEBUG
      // Set log file write time
      if (!rawx_dataFile.timestamp(T_WRITE, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds)) {
        Serial.println("Warning! Could not set file write timestamp!");
      }
#else
      rawx_dataFile.timestamp(T_WRITE, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds);
#endif
#ifdef DEBUG
      // Set log file access time
      if (!rawx_dataFile.timestamp(T_ACCESS, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds)) {
        Serial.println("Warning! Could not set file access timestamp!");
      }
#else
      rawx_dataFile.timestamp(T_ACCESS, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds);
#endif      
      rawx_dataFile.close(); // close the file
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(dim_green); // Set the NeoPixel to dim green
#endif
#else
      digitalWrite(RedLED, LOW); // Turn the red LED off to indicate SD card write is complete
#endif
#endif
#ifdef DEBUG
      uint32_t filesize = rawx_dataFile.fileSize(); // Get the file size
      Serial.print("File size is ");
      Serial.println(filesize);
      Serial.print("File size should be ");
      Serial.println(bytes_written);
#endif
      Serial.println("File closed!");
      // Either the battery is low or the user pressed the stop button:
      if (stop_pressed == true) {
        // Stop switch was pressed so just wait for a reset
#ifndef NoLED
#ifdef NeoPixel
        setLED(red); // Set the NeoPixel to red
#else
        digitalWrite(RedLED, HIGH); // Turn the red LED on
#endif
#endif
        Serial.println("Waiting for reset...");
        while(1); // Wait for reset
      }
      else {
        // Low battery was detected so wait for the battery to recover
        Serial.println("Battery must be low - waiting for it to recover...");
#ifndef NoLED
#ifdef NeoPixel
        setLED(red); // Set the NeoPixel to red
#else
        digitalWrite(RedLED, HIGH); // Turn the red LED on
#endif
#endif
        // Check the battery voltage. Make sure it has been OK for at least 5 seconds before continuing
        int high_for = 0;
        while (high_for < 500) {
          // read battery voltage
          vbat = analogRead(A7) * (2.0 * 3.3 / 1023.0);
          if (vbat < LOWBAT) {
            high_for = 0; // If battery voltage is low, reset the count
          }
          else {
            high_for++; // Increase the count
          }
          delay(10); // Wait 10msec
        }
        // Now loop round again and restart rawx messages before opening a new file
#ifndef NoLED
#ifdef NeoPixel
        setLED(cyan); // Set the NeoPixel to cyan
#else
        digitalWrite(RedLED, LOW); // Turn the red LED off
#endif
#endif
        loop_step = start_rawx;
      }
    }
    break;

    // RAWX data lost sync so disable RAWX messages, save any residual data, close the file, open another and restart RAWX messages
    // Don't update the next RTC alarm - leave it as it is
    case restart_file: {
      setRAWXoff(); // Disable RAWX messages
      int waitcount = 0;
      while (waitcount < dwell) { // Wait for residual data
        while (SerialBuffer.available()) {
          serBuffer[bufferPointer] = SerialBuffer.read_char(); // Put extra bytes into serBuffer
          bufferPointer++;
          if (bufferPointer == SDpacket) { // Write a full packet
            bufferPointer = 0;
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
            setLED(green); // Set the NeoPixel to green
#endif
#else
#ifndef NoLogLED
            digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#endif
#endif
#endif
            numBytes = rawx_dataFile.write(&serBuffer, SDpacket);
            //rawx_dataFile.sync(); // Sync the file system
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
            setLED(dim_green); // Set the NeoPixel to dim green
#endif
#else
            digitalWrite(RedLED, LOW); // Turn the red LED off to indicate SD card write is complete
#endif
#endif
            bytes_written += SDpacket;
#ifdef DEBUG
            if (numBytes != SDpacket) {
              Serial.print("SD write error! Write size was ");
              Serial.print(SDpacket);
              Serial.print(". ");
              Serial.print(numBytes);
              Serial.println(" were written.");
            }
#endif
          }
        }
        waitcount++;
        delay(1);
      }
      // If there is any data left in serBuffer, write it to file
      if (bufferPointer > 0) {
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
        setLED(green); // Set the NeoPixel to green
#endif
#else
#ifndef NoLogLED
        digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#endif
#endif
#endif
        numBytes = rawx_dataFile.write(&serBuffer, bufferPointer); // Write remaining data
        rawx_dataFile.sync(); // Sync the file system
        bytes_written += bufferPointer;
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
        setLED(dim_green); // Set the NeoPixel to dim green
#endif
#else
        digitalWrite(RedLED, LOW); // Turn the red LED off to indicate SD card write is complete
#endif
#endif
#ifdef DEBUG
        if (numBytes != bufferPointer) {
          Serial.print("SD write error! Write size was ");
          Serial.print(bufferPointer);
          Serial.print(". ");
          Serial.print(numBytes);
          Serial.println(" were written.");
        }
        Serial.print("Final SD Write: ");
        Serial.print(bufferPointer);
        Serial.println(" Bytes");
        Serial.print(bytes_written);
        Serial.println(" Bytes written");
#endif
        bufferPointer = 0; // reset bufferPointer
      }
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(green); // Set the NeoPixel to green
#endif
#else
#ifndef NoLogLED
      digitalWrite(RedLED, HIGH); // Turn the red LED on to indicate SD card write
#endif
#endif
#endif
      // Get the RTC time and date
      uint8_t RTCseconds = rtc.getSeconds();
      uint8_t RTCminutes = rtc.getMinutes();
      uint8_t RTChours = rtc.getHours();
      uint8_t RTCday = rtc.getDay();
      uint8_t RTCmonth = rtc.getMonth();
      uint8_t RTCyear = rtc.getYear();
#ifdef DEBUG
      // Set log file write time
      if (!rawx_dataFile.timestamp(T_WRITE, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds)) {
        Serial.println("Warning! Could not set file write timestamp!");
      }
#else
      rawx_dataFile.timestamp(T_WRITE, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds);
#endif
#ifdef DEBUG
      // Set log file access time
      if (!rawx_dataFile.timestamp(T_ACCESS, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds)) {
        Serial.println("Warning! Could not set file access timestamp!");
      }
#else
      rawx_dataFile.timestamp(T_ACCESS, (RTCyear+2000), RTCmonth, RTCday, RTChours, RTCminutes, RTCseconds);
#endif      
      rawx_dataFile.close(); // close the file
#ifndef NoLED
#ifdef NeoPixel
#ifndef NoLogLED
      setLED(dim_green); // Set the NeoPixel to dim green
#endif
#else
      digitalWrite(RedLED, LOW); // Turn the red LED off to indicate SD card write is complete
#endif
#endif
#ifdef DEBUG
      uint32_t filesize = rawx_dataFile.fileSize(); // Get the file size
      Serial.print("File size is ");
      Serial.println(filesize);
      Serial.print("File size should be ");
      Serial.println(bytes_written);
#endif
      Serial.println("File closed!");
      loop_step = start_rawx; // loop round again and restart rawx messages before opening a new file
    }
    break;  
  }
}
