/*
  DACSynth.cpp - Library for 12-Bit sine wave synthesis with the Teensy 3.1 
                    Digital-to-Analog Converter.
  Created by Craig Wm. Versek, 2014-06-24
 */

#include <Arduino.h>
#include <math.h>
#include "DACSynth.h"

#define PI 3.141592653589793
/******************************************************************************/
// Global Data

struct DACTableType{
  volatile byte index;
  byte  length;
  float interval_us;
  unsigned short data[DAC_TABLE_MAX_LENGTH];
} _dac_table;

/******************************************************************************/
//  DACSynthClass Public Methods


void DACSynthClass::begin() {
  // setup DAC
  _dac_table.index = 0;
  _dac_table.length = 16;//DAC_TABLE_MAX_LENGTH;
  analogWriteResolution(DAC_RESOLUTION);
}

void DACSynthClass::start() {
  _compute_dac_table();
  Serial.print(F("START interval = "));
  Serial.print(_dac_table.interval_us);
  Serial.println();
  _dac_table.index = 0;
  _running = true;
  dac_timer.begin(_dac_update_IRC, _dac_table.interval_us);  // intreval is in microseconds
}

void DACSynthClass::stop() {
  // shut off DAC
  dac_timer.end();
  analogWrite(DAC_OUTPUT_PIN, 0);
  _running = false;
}
/******************************************************************************/
//  DACSynthClass Private Methods

void DACSynthClass::_compute_dac_table(){
  int res = 12;
  float interval_us, y_max, amp_half, offset, x,y;
  Serial.println(F("compiling DAC table"));
  //compute the timer interval in microseconds
  interval_us = 1e6/_freq*1.0/_dac_table.length;
  _dac_table.interval_us = interval_us;
  //scale the amplitude and offset to midrange
  y_max = (1 << res) - 1;
  amp_half = y_max/2.0;
  offset   = y_max/2.0;
  for(int i = 0; i < _dac_table.length; i++){
    x = (float) i / _dac_table.length;
    y = amp_half*sin(2*PI*x) + offset;      //sample the function
    Serial.println((short) round(y));
    _dac_table.data[i] = (short) round(y);  //round to neareast short
  }
}


/******************************************************************************/
// IRCs
volatile unsigned long _dac_val;
void _dac_update_IRC() {
  _dac_table.index = (_dac_table.index+1) % _dac_table.length;
  analogWrite(DAC_OUTPUT_PIN, _dac_table.data[_dac_table.index]);
}
