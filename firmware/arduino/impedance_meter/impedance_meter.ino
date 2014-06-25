/*******************************************************************************
  Arduino firmware controller Teensy 3.1 Impedance Analyzer
*******************************************************************************/
#include <errno.h>
#include <SPI.h>
#include "SerialCommand.h"
#include "CircularBuffer.h"
#include "DACSynth.h"

#define SERIAL_BAUDRATE 115200
#define BUFFER_CAPACITY 256
//Warning: do not make the capacity too large or the controller may crash
//         and become unresponsive
#define MAX_SERIAL_COMMANDS 15


/******************************************************************************/
// Global Objects
SerialCommand sCmd_USB(Serial, MAX_SERIAL_COMMANDS);         // (Stream, int maxCommands)
// Circular overwriting buffer for samples
CircularBuffer sampleBuffer(BUFFER_CAPACITY);
// DAC Synthesizer
DACSynthClass synth;

/******************************************************************************/
// Setup

void setup() {
  //----------------------------------------------------------------------------
  // Setup callbacks for SerialCommand commands
  // over USB interface
  sCmd_USB.addCommand("SYNTH.FREQ",   SYNTH_FREQ_command);  //get or set the frequency of synthesized wave
  sCmd_USB.addCommand("SYNTH.START", SYNTH_START_command);  //starts the DAC wave synthesizer
  sCmd_USB.addCommand("SYNTH.STOP",  SYNTH_STOP_command);   //stops the DAC wave synthesizer
  sCmd_USB.addCommand("SER.DMODE", SER_DMODE_command); //set the data mode of the serial link
  sCmd_USB.setDefaultHandler(unrecognizedCommand);
  //----------------------------------------------------------------------------
  // intialize the serial devices
  Serial.begin(SERIAL_BAUDRATE);                  //USB serial on the Teensy 3.x
  //----------------------------------------------------------------------------
  // initialize the waveform synthesizer
  synth.begin();
  //----------------------------------------------------------------------------
  delay(5000);
  Serial.print(F("# Teensy3.1-IS v0.1\n"));
  Serial.print(F("# memory_free = "));
  Serial.print(memoryFree());
  Serial.print(F("\n"));
  Serial.print(F("# Initialized.\n"));

}
/******************************************************************************/
// Loop

void loop() {
  sCmd_USB.readSerial();
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


/******************************************************************************/
// Serial Commands

void SYNTH_FREQ_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  float freq;
  if (arg == NULL){ //get the value
    this_scmd.println(synth.get_freq());
  }
  else{               //set the value
    errno = 0;
    freq = strtod(arg, NULL);
    if (errno != 0){  //parsing error
      this_scmd.print(F("### Error: SYNTH.FREQ parsing argument 'freq': errno = "));
      this_scmd.println(errno);
    }
    else{             //value OK
      if (synth.is_running()){
        synth.stop();
        synth.set_freq(freq);
        synth.start();
      }
      else{ synth.set_freq(freq); }
    }
  }
}

void SYNTH_START_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  if (arg == NULL){
    synth.start();
  }
  else{
    this_scmd.println(F("### Error: SYNTH.START requires no arguments"));
  }
}


void SYNTH_STOP_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  if (arg == NULL){
    synth.stop();
  }
  else{
    this_scmd.println(F("### Error: SYNTH.STOP requires no arguments"));
  }
}

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
