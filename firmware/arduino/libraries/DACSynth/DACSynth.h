/*
  DACSynth.h - Library for 12-Bit sine wave synthesis with the Teensy 3.1 
                  Digital-to-Analog Converter.
  Created by Craig Wm. Versek, 2014-06-24
 */
#ifndef _DACSynth_H_INCLUDED
#define _DACSynth_H_INCLUDED

#include <Arduino.h>

#define DAC_OUTPUT_PIN A14
#define DAC_RESOLUTION 12
#define DAC_TABLE_MAX_LENGTH 4096
#define DAC_TABLE_DEFAULT_LENGTH 64
#define DAC_VOLTAGE_REF 3.3

/******************************************************************************/
// Global Types

struct DACTableType{
  volatile unsigned long index;
  unsigned long length;
  float interval_us;
  unsigned short data[DAC_TABLE_MAX_LENGTH];
};

// Function Prototypes
void _dac_update();
/*******************************************************************************
  DACSynthClass
  
*******************************************************************************/
class DACSynthClass {
public:
  DACSynthClass(){};
  //Configuration methods
  void begin();
  // Mutators and Accessors
  float get_freq()           {return _freq;}
  void  set_freq(float freq) {_freq = freq;}
  float get_amp()            {return _amp;}
  void  set_amp(float amp)   {_amp = amp;}
  unsigned long get_sampnum()  {return _sampnum;}
  void  set_sampnum(unsigned long sampnum){_sampnum = sampnum;}
  float get_interval() {return _interval;}
  bool  is_running() {return _running;}
  //Functionality methods
  void start();
  void start_continous();
  void update();
  void stop();
private:
  //Helper Functions
  void _compute_dac_table();
  //Attributes
  IntervalTimer dac_timer;      // Create an IntervalTimer object for scheduling DAC output changes
  bool  _running = false;
  float _freq = 0.0;
  float _amp  = 0.0;
  unsigned long _sampnum = DAC_TABLE_DEFAULT_LENGTH;
  float _interval = 0.0;
};



#endif /* _DACSynth_H_INCLUDED */
