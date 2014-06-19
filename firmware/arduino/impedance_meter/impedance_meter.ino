/*******************************************************************************
  Arduino firmware controller for the NeuroDot wireless EEG device
*******************************************************************************/
#include <SPI.h>
#include "SerialCommand.h"
#include "CircularBuffer.h"

#define SERIAL_BAUDRATE 115200
#define BUFFER_CAPACITY 256
//Warning: do not make the capacity too large or the controller may crash
//         and become unresponsive
#define MAX_SERIAL_COMMANDS 15
#define DAC_RESOLUTION 12
#define DAC_SINE_WAV_LENGTH 16
#define DAC_INTERVAL 20.0/DAC_SINE_WAV_LENGTH /*  microseconds*/

const unsigned short SineWave12bit[DAC_SINE_WAV_LENGTH] = {
        2048, 2831, 3496, 3940, 4096, 3940, 3496, 2831, 2048, 1264,  599,
        155,    0,  155,  599, 1264
/*       2048, 2098, 2148, 2198, 2248, 2298, 2348, 2398, 2447, 2496, 2545,*/
/*       2594, 2642, 2690, 2737, 2785, 2831, 2877, 2923, 2968, 3013, 3057,*/
/*       3100, 3143, 3185, 3227, 3267, 3307, 3347, 3385, 3423, 3460, 3496,*/
/*       3531, 3565, 3598, 3631, 3662, 3692, 3722, 3750, 3778, 3804, 3829,*/
/*       3854, 3877, 3899, 3920, 3940, 3958, 3976, 3992, 4007, 4021, 4034,*/
/*       4046, 4056, 4065, 4073, 4080, 4086, 4090, 4093, 4095, 4096, 4095,*/
/*       4093, 4090, 4086, 4080, 4073, 4065, 4056, 4046, 4034, 4021, 4007,*/
/*       3992, 3976, 3958, 3940, 3920, 3899, 3877, 3854, 3829, 3804, 3778,*/
/*       3750, 3722, 3692, 3662, 3631, 3598, 3565, 3531, 3496, 3460, 3423,*/
/*       3385, 3347, 3307, 3267, 3227, 3185, 3143, 3100, 3057, 3013, 2968,*/
/*       2923, 2877, 2831, 2785, 2737, 2690, 2642, 2594, 2545, 2496, 2447,*/
/*       2398, 2348, 2298, 2248, 2198, 2148, 2098, 2048, 1997, 1947, 1897,*/
/*       1847, 1797, 1747, 1697, 1648, 1599, 1550, 1501, 1453, 1405, 1358,*/
/*       1310, 1264, 1218, 1172, 1127, 1082, 1038,  995,  952,  910,  868,*/
/*        828,  788,  748,  710,  672,  635,  599,  564,  530,  497,  464,*/
/*        433,  403,  373,  345,  317,  291,  266,  241,  218,  196,  175,*/
/*        155,  137,  119,  103,   88,   74,   61,   49,   39,   30,   22,*/
/*         15,    9,    5,    2,    0,    0,    0,    2,    5,    9,   15,*/
/*         22,   30,   39,   49,   61,   74,   88,  103,  119,  137,  155,*/
/*        175,  196,  218,  241,  266,  291,  317,  345,  373,  403,  433,*/
/*        464,  497,  530,  564,  599,  635,  672,  710,  748,  788,  828,*/
/*        868,  910,  952,  995, 1038, 1082, 1127, 1172, 1218, 1264, 1310,*/
/*       1358, 1405, 1453, 1501, 1550, 1599, 1648, 1697, 1747, 1797, 1847,*/
/*       1897, 1947, 1997*/
};
       
volatile byte dacIndex=0;           // Index varies from 0 to 255

/******************************************************************************/
// Global Objects
SerialCommand sCmd_USB(Serial, MAX_SERIAL_COMMANDS);         // (Stream, int maxCommands)

                           
// Circular overwriting buffer for samples
CircularBuffer sampleBuffer(BUFFER_CAPACITY);

//byte rawDataRecord[RDATA_NUM_BYTES];

//misc pins

// Simple DAC sine wave test on Teensy 3.1


// Create an IntervalTimer object for scheduling DAC output changes
IntervalTimer dacTimer;

/******************************************************************************/
// Setup

void setup() {
  //----------------------------------------------------------------------------
  // Setup callbacks for SerialCommand commands
  // over USB interface
  sCmd_USB.addCommand("SER.DMODE", SER_DMODE_command); //set the data mode of the serial link
  sCmd_USB.setDefaultHandler(unrecognizedCommand);
  //----------------------------------------------------------------------------
  // intialize the serial devices
  Serial.begin(SERIAL_BAUDRATE);                  //USB serial on the Teensy 3.x
  //----------------------------------------------------------------------------
  //start up the SPI bus
/*  SPI.begin();*/
/*  SPI.setBitOrder(MSBFIRST);*/
/*  SPI.setDataMode(SPI_MODE1);*/
  //SPI.setClockDivider(21); //84MHz clock /21 = 4 MHz
  //----------------------------------------------------------------------------
  // initialize the ADS129x chip
  // first enable the analog power supply (5.0V) on controller board
  
  Serial.print(F("# Teensy3.1-IS v0.1\n"));
  Serial.print(F("# memory_free = "));
  Serial.print(memoryFree());
  Serial.print(F("\n"));
  Serial.print(F("# Initialized.\n"));
  
  // setup DAC
  analogWriteResolution(DAC_RESOLUTION);
  dacTimer.begin(dac_update_IRC, DAC_INTERVAL);  // blinkLED to run every 0.15 seconds
  dacIndex = 0;
}
/******************************************************************************/
// Loop

void loop() {
  
  
}

/******************************************************************************/
// Functions

// variables created by the build process when compiling the sketch
extern int __bss_end;
extern void *__brkval;

// function to return the amount of free RAM
int memoryFree()
{
  int freeValue;

  if ((int)__brkval == 0)
     freeValue = ((int)&freeValue) - ((int)&__bss_end);
  else
    freeValue = ((int)&freeValue) - ((int)__brkval);

  return freeValue;
}

/*void softwareReset() // Restarts program from beginning but does not reset the peripherals and registers*/
/*{*/
/*  asm volatile ("  jmp 0");*/
/*} */

void buffer_raw_data(byte *record, unsigned long *acq_time_micros){
  //store data in the buffer
  //sampleBuffer.write((byte *) acq_time_micros, sizeof(unsigned long));
  //sampleBuffer.write(record, RDATA_NUM_BYTES);
}

void send_raw_data(byte *record, unsigned long acq_time_micros, Print &output){
  //output.write(acq_time_micros);
  //for(int i = 0; i < RDATA_NUM_BYTES; i++){
  //   output.write(record[i]);  //output the data as binary
  //}
}

/******************************************************************************/
// Interrupt Service Routines
float phase = 0.0;
float twopi = 3.14159 * 2;
float amplitude = 2000.0;
elapsedMicros usec = 0;

void dac_update_IRC(){
  dacIndex = (dacIndex+1)&0x0F; 
  //process serial commands on both interfaces
  analogWrite(A14, SineWave12bit[dacIndex]);
}

/******************************************************************************/
// Serial Commands

void SER_DMODE_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  if (arg == NULL){
    this_scmd.println(F("### Error: SER.DMODE requires 1 argument (char *mode)"));
  }
  else{
    this_scmd.println(F("### Error: SER.DMODE argument '"));
    this_scmd.print(arg);
    this_scmd.print(F("' not recognized ###\n"));
  }
}



// Unrecognized command
void unrecognizedCommand(const char* command, SerialCommand this_scmd)
{
  this_scmd.print(F("### Error: command '"));
  this_scmd.print(command);
  this_scmd.print(F("' not recognized ###\n"));
}


/******************************************************************************/
