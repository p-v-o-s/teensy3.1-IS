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
DACTableType _dac_table;
/******************************************************************************/
//  DACSynthClass Public Methods


void DACSynthClass::begin() {
  // setup DAC
  _dac_table.index = 0;
  _dac_table.length = DAC_TABLE_DEFAULT_LENGTH;
  analogWriteResolution(DAC_RESOLUTION);
}

void DACSynthClass::start() {
  _compute_dac_table();
  //Serial.print(F("START interval = "));
  //Serial.print(_dac_table.interval_us);
  //Serial.println();
  _dac_table.index = 0;
  _running = true;
}

void DACSynthClass::start_continous() {
  start();
  dac_timer.begin(_dac_update, _dac_table.interval_us);  // intreval is in microseconds
}

void DACSynthClass::update() {
  _dac_update();  // intreval is in microseconds
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
  float interval_us, y_max, y_a, y_o, x,y;
  //Serial.println(F("compiling DAC table"));
  _dac_table.length = _sampnum;
  //compute the timer interval in seconds
  _interval = 1.0/(_freq*_sampnum);
  _dac_table.interval_us = 1e6*_interval;
  //scale the amplitude and offset to midrange
  y_max = (1 << res) - 1;
  y_a = _amp*y_max/DAC_VOLTAGE_REF;
  y_o = 0.5*y_max;
  for(int i = 0; i < _dac_table.length; i++){
    x = (float) i / _dac_table.length;
    y = 0.5*y_a*sin(2*PI*x) + y_o;      //sample the function
    //Serial.println((short) round(y));
    _dac_table.data[i] = (short) round(y);  //round to neareast short
  }
}


/******************************************************************************/
// ISRs
void _dac_update() {
  _dac_table.index = (_dac_table.index+1) % _dac_table.length;
  analogWrite(DAC_OUTPUT_PIN, _dac_table.data[_dac_table.index]);
}
