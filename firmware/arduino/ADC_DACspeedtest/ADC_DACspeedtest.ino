/*
    This example shows how to use the IntervalTimer library and the ADC library in the tensy 3.0/3.1

    The three important objects are: the ADC, the (one or more) IntervalTimer and the same number of RingBuffer.

    The ADC sets up the internal ADC, you can change the settings as usual. Enable interrupts.
    The IntervalTimer (timerX) executes a function every 'period' microseconds.
    This function, timerX_callback, starts the desired ADC measurement, for example:
        startSingleRead, startSingleDifferential, startSynchronizedSingleRead, startSynchronizedSingleDifferential
    you can use the ADC0 or ADC1 (only for Teensy 3.1).

    When the measurement is done, the adc_isr is executed:
        - If you have more than one timer per ADC module you need to know which pin was measured.
        - Then you store/process the data
        - Finally, if you have more than one timer you can check whether the last measurement interruped a
          a previous one (using adc->adcX->adcWasInUse), and restart it if so.
          The settings of the interrupted measurement are stored in the adc->adcX->adc_config struct.


*/


#include <DACSynth.h>
#include <ADC.h>
// and IntervalTimer
#include <IntervalTimer.h>

//#define DEBUG_SCOPE
#define MY_ADC_SPEED ADC_VERY_HIGH_SPEED
const int ledPin = LED_BUILTIN;

const int readPin0 = A9;
const int readPin1 = A3;
const int sample_period = 10; // us



const int readPeriod = 100000; // us

DACSynthClass *dacSynth = new DACSynthClass();
ADC *adc = new ADC(); // adc object

IntervalTimer adcTimer, dacTimer; // timers

RingBufferDMA *dmaBuffer0 = new RingBufferDMA(2,ADC_0); // use dma channel 2 and ADC0
RingBufferDMA *dmaBuffer1 = new RingBufferDMA(10,ADC_1); // use dma channel 10 and ADC1

int startTimerValue0 = 0;

void setup() {
    pinMode(ledPin, OUTPUT);   // led blinks every loop
    pinMode(ledPin+1, OUTPUT); // timer0 starts a measurement
    pinMode(ledPin+2, OUTPUT); // adc0_isr, measurement finished
    pinMode(ledPin+3, OUTPUT); // adc0_isr, measurement finished
    pinMode(readPin0, INPUT);
    pinMode(readPin1, INPUT);

    Serial.begin(9600);

    delay(1000);

    /// DACSynth ///
    dacSynth->begin();
    dacSynth->set_sampnum(16);
    dacSynth->set_freq(1e4);
    dacSynth->set_amp(1.0);
    dacSynth->start();
    //dacSynth->start_continous();

    ///// ADC0 ////
    //adc->setReference(ADC_REF_INTERNAL, ADC_0); change all 3.3 to 1.2 if you change the reference

    adc->setAveraging(1, ADC_0); // set number of averages
    adc->setResolution(12, ADC_0); // set bits of resolution
    // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED_16BITS, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
    // see the documentation for more information
    adc->setConversionSpeed(MY_ADC_SPEED, ADC_0); // change the conversion speed
    // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
    adc->setSamplingSpeed(MY_ADC_SPEED, ADC_0); // change the sampling speed
    // with 16 averages, 12 bits resolution and ADC_HIGH_SPEED conversion and sampling it takes about 32.5 us for a conversion

    ///// ADC1 ////
    //adc->setReference(ADC_REF_INTERNAL, ADC_1); change all 3.3 to 1.2 if you change the reference

    adc->setAveraging(1, ADC_1); // set number of averages
    adc->setResolution(12, ADC_1); // set bits of resolution
    // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED_16BITS, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
    // see the documentation for more information
    adc->setConversionSpeed(MY_ADC_SPEED, ADC_1); // change the conversion speed
    // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
    adc->setSamplingSpeed(MY_ADC_SPEED, ADC_1); // change the sampling speed
    // with 16 averages, 12 bits resolution and ADC_HIGH_SPEED conversion and sampling it takes about 32.5 us for a conversion

    Serial.println("Starting Timer");

    

    // initiated by timer0. The adc_isr will restart the timer0's measurement.
    // You can check with an oscilloscope:
    // Pin 14 corresponds to the timer0 initiating a measurement
    // Pin 16 is the adc_isr.
    // enable DMA and interrupts
    adc->enableDMA(ADC_0);
    adc->enableDMA(ADC_1);
    //adc->enableInterrupts(ADC_0);
    //adc->enableInterrupts(ADC_1);
    dmaBuffer0->start();
    dmaBuffer1->start();
    
    //Serial.println("Timer started");
    delay(500);

    // start the timers, if it's not possible, startTimerValuex will be false
    dacTimer.begin(dacTimer_callback, sample_period);
    //delayMicroseconds(sample_period/2.0);
    //adcTimer.begin(adcTimer_callback, sample_period);
}

int value = 0;
char c=0;

void loop() {

/*  if(startTimerValue0==false) {*/
/*    Serial.println("Timer0 setup failed");*/
/*  }*/
  if(!dmaBuffer0->isEmpty()) { // read the values in the buffer
    //Serial.print("Read pin 0: ");
    Serial.print(dmaBuffer0->read()*3.3/adc->getMaxValue());
    Serial.print(" ");
    Serial.println(dmaBuffer1->read()*3.3/adc->getMaxValue());
    //Serial.println("New value!");
  }
  //adc->analogRead(readPin0, ADC_0);
  //digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN) );
  delayMicroseconds(readPeriod);
}


// This function will be called with the desired frequency
// start the measurement
// in my low-res oscilloscope this function seems to take 1.5-2 us.
void dacTimer_callback(void) {
  #ifdef DEBUG_SCOPE
  digitalWriteFast(ledPin+1, HIGH);
  #endif
  //adc->startSingleRead(readPin0, ADC_0); // also: startSingleDifferential, analogSynchronizedRead, analogSynchronizedReadDifferential
  //adc->startSingleRead(readPin1, ADC_1); 
  dacSynth->update();
  adc->startSynchronizedSingleRead(readPin0, readPin1);
  //delayMicroseconds(25);
  #ifdef DEBUG_SCOPE
  digitalWriteFast(ledPin+1, LOW);
  #endif
}


// This function will be called with the desired frequency
// start the measurement
// in my low-res oscilloscope this function seems to take 1.5-2 us.
void adcTimer_callback(void) {
  #ifdef DEBUG_SCOPE
  digitalWriteFast(ledPin+2, HIGH);
  #endif
  adc->startSynchronizedSingleRead(readPin0, readPin1);
  #ifdef DEBUG_SCOPE
  digitalWriteFast(ledPin+2, LOW);
  #endif
}


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


/*// when the measurement finishes, this will be called*/
/*// first: see which pin finished and then save the measurement into the correct buffer*/
/*void adc0_isr() {*/
/*    ADC::Sync_result result;*/
/*    digitalWriteFast(ledPin+3, HIGH);*/
/*    delayMicroseconds(2);*/
/*    //result = adc->readSynchronizedSingle();*/
/*    //dmaBuffer0->write();*/
/*    //dmaBuffer1->write();*/
/*    ADC0_RA; // clear interrupt*/
/*    digitalWriteFast(ledPin+3, LOW);*/
/*}*/
