/*******************************************************************************
  Arduino firmware controller Teensy 3.1 Impedance Analyzer
*******************************************************************************/
#include <errno.h>
#include <SPI.h>
#include <SerialCommand.h>
#include <ADC.h>
#include <DACSynth.h>
#include "CircularBuffer.h"

#define SERIAL_BAUDRATE 115200
//Warning: do not make the capacity too large or the controller may crash
//         and become unresponsive
#define MAX_SERIAL_COMMANDS 15
//data consists of one short int for each of 2 ADC channels
#define RDATA_NUM_BYTES (2*sizeof(short)) 
#define BUFFER_CAPACITY 1024*RDATA_NUM_BYTES


/******************************************************************************/
// Global Objects
SerialCommand sCmd_USB(Serial, MAX_SERIAL_COMMANDS);         // (Stream, int maxCommands)
DACSynthClass synth;

ADC *adc = new ADC(); // adc object

IntervalTimer adcTimer; // timers

// it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED_16BITS, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
#define MY_ADC_SPEED ADC_VERY_HIGH_SPEED
const int ledPin = LED_BUILTIN;
const int adcPin0 = A9;
const int adcPin1 = A3;
float adc_sample_period_us = 10.0; // us

//these buffers implement DMA transfer from ADC
RingBufferDMA *dmaBuffer0 = new RingBufferDMA(2,ADC_0);  // use dma channel 2 and ADC0
RingBufferDMA *dmaBuffer1 = new RingBufferDMA(10,ADC_1); // use dma channel 10 and ADC1

// Circular overwriting buffer for samples
CircularBuffer sampleBuffer(BUFFER_CAPACITY);
byte rawDataRecord[RDATA_NUM_BYTES];
/******************************************************************************/
// Setup

void setup() {
  //----------------------------------------------------------------------------
  // Setup callbacks for SerialCommand commands
  // over USB interface
  sCmd_USB.addCommand("ADC.SAMPRATE",     ADC_SAMPRATE_command);       //get or set the frequency of synthesized wave
  sCmd_USB.addCommand("ADC.START",        ADC_START_command);      //starts the DAC wave synthesizer
  sCmd_USB.addCommand("ADC.STOP",         ADC_STOP_command);       //stops the DAC wave synthesizer
  sCmd_USB.addCommand("SYNTH.FREQ",       SYNTH_FREQ_command);       //get or set the frequency of synthesized wave
  sCmd_USB.addCommand("SYNTH.AMP",        SYNTH_AMP_command);        //get or set the amplitude of synthesized wave
  sCmd_USB.addCommand("SYNTH.SAMPNUM",    SYNTH_SAMPNUM_command);    //get or set the number of samples in the synthesized wave
  sCmd_USB.addCommand("SYNTH.INTERVAL",   SYNTH_INTERVAL_command);   //get the time interval between samples in the synthesized wave
  sCmd_USB.addCommand("SYNTH.IS_RUNNING", SYNTH_IS_RUNNING_command); //get state of the synthesizer, 0 = off, 1 = running
  sCmd_USB.addCommand("SYNTH.START",      SYNTH_START_command);      //starts the DAC wave synthesizer
  sCmd_USB.addCommand("SYNTH.STOP",       SYNTH_STOP_command);       //stops the DAC wave synthesizer
  sCmd_USB.addCommand("SER.DMODE", SER_DMODE_command); //set the data mode of the serial link
  sCmd_USB.addCommand("SER.BUFSZ", SER_BUFSZ_command); //get the number of records in the buffer
  sCmd_USB.addCommand("SER.RDBUF", SER_RDBUF_command); //dump the data buffer over the serial link
  sCmd_USB.addCommand("SER.FLBUF", SER_FLBUF_command); //flush contents of circular buffer
  sCmd_USB.setDefaultHandler(unrecognizedCommand);
  //----------------------------------------------------------------------------
  // initialize pins
  pinMode(ledPin, OUTPUT);   // led blinks every loop
  pinMode(ledPin+1, OUTPUT); // timer0 starts a measurement
  pinMode(ledPin+2, OUTPUT); // adc0_isr, measurement finished
  pinMode(ledPin+3, OUTPUT); // adc0_isr, measurement finished
  pinMode(adcPin0, INPUT);
  pinMode(adcPin1, INPUT);

  //----------------------------------------------------------------------------
  // intialize the serial devices
  Serial.begin(SERIAL_BAUDRATE);                  //USB serial on the Teensy 3.x
  //----------------------------------------------------------------------------
  // initialize the waveform synthesizer
  synth.begin();
  //----------------------------------------------------------------------------
  ///// ADC0 ////
  adc->setReference(ADC_REF_EXTERNAL, ADC_0);   //change all 3.3 to 1.2 if you change the reference
  adc->setAveraging(1, ADC_0);                  // set number of averages
  adc->setResolution(12, ADC_0);                // set bits of resolution
  adc->setConversionSpeed(MY_ADC_SPEED, ADC_0); // change the conversion speed
  adc->setSamplingSpeed(MY_ADC_SPEED, ADC_0);   // change the sampling speed
  // with 16 averages, 12 bits resolution and ADC_HIGH_SPEED conversion and sampling it takes about 32.5 us for a conversion
  ///// ADC1 ////
  adc->setReference(ADC_REF_EXTERNAL, ADC_1);   //change all 3.3 to 1.2 if you change the reference
  adc->setAveraging(1, ADC_1);                  // set number of averages
  adc->setResolution(12, ADC_1);                // set bits of resolution
  adc->setConversionSpeed(MY_ADC_SPEED, ADC_1); // change the conversion speed
  adc->setSamplingSpeed(MY_ADC_SPEED, ADC_1);   // change the sampling speed
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
unsigned int samp0, samp1;

void loop() {
  //----------------------------------------------------------------------------
  // manage buffers
  if(!dmaBuffer0->isEmpty()) {
    // read DMA buffer
    samp0 = dmaBuffer0->read();//*1.2/adc->getMaxValue();
    //delayMicroseconds(10);
    samp1 = dmaBuffer1->read();//*1.2/adc->getMaxValue();
    Serial.print(samp0);Serial.print(" ");Serial.print(samp1);Serial.print("\n");
    // pack the integers into the buffer as bytes
    //memcpy(rawDataRecord                , &samp0, sizeof(short));
    //memcpy(rawDataRecord + sizeof(short), &samp1, sizeof(short));
    //sampleBuffer.write(rawDataRecord, RDATA_NUM_BYTES);
  } else {delayMicroseconds(100);}
  //----------------------------------------------------------------------------
  // handle serial commands
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
  for(int i = 0; i < RDATA_NUM_BYTES; i++){
     output.write(record[i]);  //output the data as binary
  }
}

// This function will be called with the desired frequency
// start the measurement
// in my low-res oscilloscope this function seems to take 1.5-2 us.
void adcTimer_callback(void) {
  #ifdef DEBUG_SCOPE
  digitalWriteFast(ledPin+2, HIGH);
  #endif
  adc->startSynchronizedSingleRead(adcPin0, adcPin1);
  #ifdef DEBUG_SCOPE
  digitalWriteFast(ledPin+2, LOW);
  #endif
}

/******************************************************************************/
// Interrupt Service Routines
// this function is called everytime a new value is converted by the ADC and copied by DMA
void dma_ch2_isr(void) {
    #ifdef DEBUG_SCOPE
    digitalWriteFast(ledPin+3, HIGH);
    #endif
    //Serial.println("DMA_CH2_ISR"); //Serial.println(t);
    DMA_CINT = 2; // clear interrupt
    // call write to tell the buffer that we wrote a new value
    dmaBuffer0->write();
    #ifdef DEBUG_SCOPE
    digitalWriteFast(ledPin+3, LOW);
    #endif
}
void dma_ch10_isr(void) {
    #ifdef DEBUG_SCOPE
    digitalWriteFast(ledPin+3, HIGH);
    #endif
    DMA_CINT = 10; // clear interrupt
    // call write to tell the buffer that we wrote a new value
    dmaBuffer1->write();
    #ifdef DEBUG_SCOPE
    digitalWriteFast(ledPin+3, LOW);
    #endif
}

/******************************************************************************/
// Serial Commands

//ADC Module
void ADC_SAMPRATE_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  float rate;
  if (arg == NULL){ //get the value
    rate = 1e6/adc_sample_period_us;
    this_scmd.println(rate);
  }
  else{               //set the value
    errno = 0;
    rate = strtod(arg, NULL);
    if (errno != 0){  //parsing error
      this_scmd.print(F("### Error: ADC.SAMPRATE parsing argument 'rate': errno = "));
      this_scmd.println(errno);
    }
    else{             //value OK
        adc_sample_period_us = 1e6/rate;
    }
  }
}

void ADC_START_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  if (arg == NULL){
     // enable DMA 
     adc->enableDMA(ADC_0);
     adc->enableDMA(ADC_1);
     //adc->enableInterrupts(ADC_0);
     //adc->enableInterrupts(ADC_1);
     dmaBuffer0->start();
     dmaBuffer1->start();
     delay(500);
     adcTimer.begin(adcTimer_callback, adc_sample_period_us);
  }
  else{
    this_scmd.println(F("### Error: ADC.START requires no arguments"));
  }
}


void ADC_STOP_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  if (arg == NULL){
    adcTimer.end();
  }
  else{
    this_scmd.println(F("### Error: ADC.STOP requires no arguments"));
  }
}

// SYNTH Module

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

void SYNTH_AMP_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  float amp;
  if (arg == NULL){ //get the value
    this_scmd.println(synth.get_amp());
  }
  else{               //set the value
    errno = 0;
    amp = strtod(arg, NULL);
    if (errno != 0){  //parsing error
      this_scmd.print(F("### Error: SYNTH.AMP parsing argument 'amp': errno = "));
      this_scmd.println(errno);
    }
    else{             //value OK
      //this_scmd.print(F("amp = "));this_scmd.println(amp);
      if (synth.is_running()){
        synth.stop();
        synth.set_amp(amp);
        synth.start();
      }
      else{ synth.set_amp(amp); }
    }
  }
}

void SYNTH_SAMPNUM_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  unsigned long sampnum;
  if (arg == NULL){   //get the value
    this_scmd.println(synth.get_sampnum());
  }
  else{               //set the value
    errno = 0;
    sampnum = strtoul(arg, NULL, 10);
    if (errno != 0){  //parsing error
      this_scmd.print(F("### Error: SYNTH.SAMPNUM parsing argument 'sampnum': errno = "));
      this_scmd.println(errno);
    }
    else{             //value OK
      //this_scmd.print(F("sampnum = "));this_scmd.println(sampnum);
      if (synth.is_running()){
        synth.stop();
        synth.set_sampnum(sampnum);
        synth.start();
      }
      else{ synth.set_sampnum(sampnum); }
    }
  }
}

void SYNTH_INTERVAL_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  if (arg == NULL){ //get the value
    this_scmd.printf("%f\n",synth.get_interval());
  }
  else{               //set the value
    this_scmd.println(F("### Error: SYNTH.INTERVAL cannot be directly set"));
  }
}

void SYNTH_IS_RUNNING_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  if (arg == NULL){
    if (synth.is_running()){this_scmd.println(1);}
    else                   {this_scmd.println(0);}
  }
  else{
    this_scmd.println(F("### Error: SYNTH.IS_RUNNING requires no arguments"));
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

// SER Module

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

void SER_BUFSZ_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  size_t num;
  if (arg != NULL){
    this_scmd.println(F("### Error: SER.BUFSZ requires no arguments"));
  }
  else{
    num = sampleBuffer.size()/RDATA_NUM_BYTES;
    this_scmd.println(num);
  }
}

void SER_RDBUF_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  size_t num, max_num; //number of data records to read
  max_num = sampleBuffer.size()/RDATA_NUM_BYTES;
  if (arg != NULL){
    num = min(atoi(arg), max_num);
  }
  else{
    num = max_num;
  }
  unsigned long *acq_t;
  for (int i=0; i < num; i++){
    //grab record from buffer
    //sampleBuffer.read((byte *) acq_t, sizeof(unsigned long));
    sampleBuffer.read(rawDataRecord, RDATA_NUM_BYTES);
    //send it out to the interface that requested it
    send_raw_data(rawDataRecord, *acq_t, this_scmd);
  }
  
}

void SER_FLBUF_command(SerialCommand this_scmd) {
  char *arg = this_scmd.next();
  size_t records_flushed;
  if (arg != NULL){
    this_scmd.println(F("### Error: SER.FLBUF requires no arguments"));
  }
  else{
    records_flushed = sampleBuffer.flush()/RDATA_NUM_BYTES;
    this_scmd.println(records_flushed);
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
