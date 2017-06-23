/**
 * This program logs data from the Arduino ADC to a binary file.
` *
 * Samples are logged at regular intervals. Each Sample consists of the ADC
 * values for the analog pins defined in the PIN_LIST array.  The pins numbers
 * may be in any order.
 *
 * Edit the configuration constants below to set the sample pins, sample rate,
 * and other configuration values.
 *
 * If your SD card has a long write latency, it may be necessary to use
 * slower sample rates.  Using a Mega Arduino helps overcome latency
 * problems since 13 512 byte buffers will be used.
 *
 * Each 512 byte data block in the file has a four byte header followed by up
 * to 508 bytes of data. (508 values in 8-bit mode or 254 values in 10-bit mode)
 * Each block contains an integral number of samples with unused space at the
 * end of the block.
 *
 * Data is written to the file using a SD multiple block write command.
 */
#ifdef __AVR__
#include <SPI.h>
#include <SdFat.h>
#include "SdFatUtil.h"
#include "AnalogBinLogger.h"
//------------------------------------------------------------------------------
// Analog pin number list for a sample.  Pins may be in any order and pin
// numbers may be repeated.
const uint8_t PIN_LIST[] = {0, 1, 2}; // Circuit du payload
const uint8_t TEC_PIN = 3;
const uint8_t PWM_PIN = 5;
//------------------------------------------------------------------------------
// Sample rate in samples per second.
const float SAMPLE_RATE = 8000;  // Must be 0.25 or greater. Maximum 40kHz 
// a 10 bits et 57kHz a 8 bits

// The interval between samples in seconds, SAMPLE_INTERVAL, may be set to a
// constant instead of being calculated from SAMPLE_RATE.  SAMPLE_RATE is not
// used in the code below.  For example, setting SAMPLE_INTERVAL = 2.0e-4
// will result in a 200 microsecond sample interval.
const float SAMPLE_INTERVAL = 1.0/SAMPLE_RATE;

// Setting ROUND_SAMPLE_INTERVAL non-zero will cause the sample interval to
// be rounded to a a multiple of the ADC clock period and will reduce sample
// time jitter.
#define ROUND_SAMPLE_INTERVAL 1
//------------------------------------------------------------------------------
// ADC clock rate.
// The ADC clock rate is normally calculated from the pin count and sample
// interval.  The calculation attempts to use the lowest possible ADC clock
// rate.
//
// You can select an ADC clock rate by defining the symbol ADC_PRESCALER to
// one of these values.  You must choose an appropriate ADC clock rate for
// your sample interval.
// #define ADC_PRESCALER 7 // F_CPU/128 125 kHz on an Uno
// #define ADC_PRESCALER 6 // F_CPU/64  250 kHz on an Uno
// #define ADC_PRESCALER 5 // F_CPU/32  500 kHz on an Uno
// #define ADC_PRESCALER 4 // F_CPU/16 1000 kHz on an Uno
// #define ADC_PRESCALER 3 // F_CPU/8  2000 kHz on an Uno (8-bit mode only)
//------------------------------------------------------------------------------
// Reference voltage.  See the processor data-sheet for reference details.
// uint8_t const ADC_REF = 0; // External Reference AREF pin.
uint8_t const ADC_REF = (1 << REFS0);  // Vcc Reference.
// uint8_t const ADC_REF = (1 << REFS1);  // Internal 1.1 (only 644 1284P Mega)
// uint8_t const ADC_REF = (1 << REFS1) | (1 << REFS0);  // Internal 1.1 or 2.56
//------------------------------------------------------------------------------
// File definitions.
//
// Maximum file size in blocks.
// The program creates a contiguous file with FILE_BLOCK_COUNT 512 byte blocks.
// This file is flash erased using special SD commands.  The file will be
// truncated if logging is stopped early.
const uint32_t FILE_BLOCK_COUNT = 64000;

// log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "analog"

// Set RECORD_EIGHT_BITS non-zero to record only the high 8-bits of the ADC.
#define RECORD_EIGHT_BITS 0
//------------------------------------------------------------------------------
// Pin definitions.
//
// Digital pin to indicate an error, set to -1 if not used.
// The led blinks for fatal errors. The led goes on solid for SD write
// overrun errors and logging continues.
const int8_t ERROR_LED_PIN = 8;
const int8_t DEBUG_LED_PIN = 9;

// SD chip select pin.
const uint8_t SD_CS_PIN = 10; //Pour fitter avec le circuit du payload
//------------------------------------------------------------------------------
// Buffer definitions.
//
// The logger will use SdFat's buffer plus BUFFER_BLOCK_COUNT additional
// buffers.  QUEUE_DIM must be a power of two larger than
//(BUFFER_BLOCK_COUNT + 1).
//
#if RAMEND < 0X8FF
#error Too little SRAM
//
#elif RAMEND < 0X10FF
// Use total of two 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 1;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 4;  // Must be a power of two!
//
#elif RAMEND < 0X20FF
// Use total of five 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 4;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 8;  // Must be a power of two!
//
#elif RAMEND < 0X40FF
// Use total of 13 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 12;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 16;  // Must be a power of two!
//
#else  // RAMEND
// Use total of 29 512 byte buffers.
const uint8_t BUFFER_BLOCK_COUNT = 28;
// Dimension for queues of 512 byte SD blocks.
const uint8_t QUEUE_DIM = 32;  // Must be a power of two!
#endif  // RAMEND


// constants won't change. They're used here to 
// set pin numbers:
//const int buttonPin = 9;    // the number of the pushbutton pin
//const int LEDlog = 7;

// Variables will change:
//int buttonState = LOW;             // the current reading from the input pin
//int lastButtonState = LOW;   // the previous reading from the input pin
//int reading = LOW;
unsigned long time = 0;
unsigned long timer = 0;
// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
//long lastDebounceTime = 0;  // the last time the output pin was toggled
//long debounceDelay = 50;    // the debounce time; increase if the output flickers
//==============================================================================
// End of configuration constants.
//==============================================================================
// Temporary log file.  Will be deleted if a reset or power failure occurs.
#define TMP_FILE_NAME "tmp_log.bin"

// Size of file base name.  Must not be larger than six.
const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;

// Number of analog pins to log.
const uint8_t PIN_COUNT = sizeof(PIN_LIST)/sizeof(PIN_LIST[0]);

// Minimum ADC clock cycles per sample interval
const uint16_t MIN_ADC_CYCLES = 15;

// Extra cpu cycles to setup ADC with more than one pin per sample.
const uint16_t ISR_SETUP_ADC = 100;

// Maximum cycles for timer0 system interrupt, millis, micros.
const uint16_t ISR_TIMER0 = 160;
//==============================================================================
SdFat sd;

SdBaseFile binFile;

char binName[13] = FILE_BASE_NAME "00.bin";

#if RECORD_EIGHT_BITS
const size_t SAMPLES_PER_BLOCK = DATA_DIM8/PIN_COUNT;
typedef block8_t block_t;
#else  // RECORD_EIGHT_BITS
const size_t SAMPLES_PER_BLOCK = DATA_DIM16/PIN_COUNT;
typedef block16_t block_t;
#endif // RECORD_EIGHT_BITS

block_t* emptyQueue[QUEUE_DIM];
uint8_t emptyHead;
uint8_t emptyTail;

block_t* fullQueue[QUEUE_DIM];
volatile uint8_t fullHead;  // volatile insures non-interrupt code sees changes.
uint8_t fullTail;

// queueNext assumes QUEUE_DIM is a power of two
inline uint8_t queueNext(uint8_t ht) {
  return (ht + 1) & (QUEUE_DIM -1);
}
//==============================================================================
// Interrupt Service Routines

// Pointer to current buffer.
block_t* isrBuf;

// Need new buffer if true.
bool isrBufNeeded = true;

// overrun count
uint16_t isrOver = 0;

// ADC configuration for each pin.
uint8_t adcmux[PIN_COUNT];
uint8_t adcsra[PIN_COUNT];
uint8_t adcsrb[PIN_COUNT];
uint8_t adcindex = 1;

// Insure no timer events are missed.
volatile bool timerError = false;
volatile bool timerFlag = false;
//------------------------------------------------------------------------------
// ADC done interrupt.
ISR(ADC_vect) {
  // Read ADC data.
#if RECORD_EIGHT_BITS
  uint8_t d = ADCH;
#else  // RECORD_EIGHT_BITS
  // This will access ADCL first.
  uint16_t d = ADC;
#endif  // RECORD_EIGHT_BITS

  if (isrBufNeeded && emptyHead == emptyTail) {
    // no buffers - count overrun
    if (isrOver < 0XFFFF) {
      isrOver++;
    }

    // Avoid missed timer error.
    timerFlag = false;
    return;
  }
  // Start ADC
  if (PIN_COUNT > 1) {
    ADMUX = adcmux[adcindex];
    ADCSRB = adcsrb[adcindex];
    ADCSRA = adcsra[adcindex];
    if (adcindex == 0) {
      timerFlag = false;
    }
    adcindex =  adcindex < (PIN_COUNT - 1) ? adcindex + 1 : 0;
  } else {
    timerFlag = false;
  }
  // Check for buffer needed.
  if (isrBufNeeded) {
    // Remove buffer from empty queue.
    isrBuf = emptyQueue[emptyTail];
    emptyTail = queueNext(emptyTail);
    isrBuf->count = 0;
    isrBuf->overrun = isrOver;
    isrBufNeeded = false;
  }
  // Store ADC data.
  isrBuf->data[isrBuf->count++] = d;

  // Check for buffer full.
  if (isrBuf->count >= PIN_COUNT*SAMPLES_PER_BLOCK) {
    // Put buffer isrIn full queue.
    uint8_t tmp = fullHead;  // Avoid extra fetch of volatile fullHead.
    fullQueue[tmp] = (block_t*)isrBuf;
    fullHead = queueNext(tmp);

    // Set buffer needed and clear overruns.
    isrBufNeeded = true;
    isrOver = 0;
  }
}
//------------------------------------------------------------------------------
// timer1 interrupt to clear OCF1B
ISR(TIMER1_COMPB_vect) {
  // Make sure ADC ISR responded to timer event.
  if (timerFlag) {
    timerError = true;
  }
  timerFlag = true;
}
//==============================================================================
// Error messages stored in flash.
#define error(msg) errorFlash(F(msg))
//------------------------------------------------------------------------------
void errorFlash(const __FlashStringHelper* msg) {
  //sd.errorPrint(msg);
  fatalBlink();
}
//------------------------------------------------------------------------------
//
void fatalBlink() {
  while (true) {
    if (ERROR_LED_PIN >= 0) {
      digitalWrite(ERROR_LED_PIN, HIGH);
      delay(200);
      digitalWrite(ERROR_LED_PIN, LOW);
      delay(200);
    }
  }
}
//==============================================================================
#if ADPS0 != 0 || ADPS1 != 1 || ADPS2 != 2
#error unexpected ADC prescaler bits
#endif
//------------------------------------------------------------------------------
// initialize ADC and timer1
void adcInit(metadata_t* meta) {
  uint8_t adps;  // prescaler bits for ADCSRA
  uint32_t ticks = F_CPU*SAMPLE_INTERVAL + 0.5;  // Sample interval cpu cycles.

  if (ADC_REF & ~((1 << REFS0) | (1 << REFS1))) {
    error("Invalid ADC reference");
  }
#ifdef ADC_PRESCALER
  if (ADC_PRESCALER > 7 || ADC_PRESCALER < 2) {
    error("Invalid ADC prescaler");
  }
  adps = ADC_PRESCALER;
#else  // ADC_PRESCALER
  // Allow extra cpu cycles to change ADC settings if more than one pin.
  int32_t adcCycles = (ticks - ISR_TIMER0)/PIN_COUNT;
  - (PIN_COUNT > 1 ? ISR_SETUP_ADC : 0);

  for (adps = 7; adps > 0; adps--) {
    if (adcCycles >= (MIN_ADC_CYCLES << adps)) {
      break;
    }
  }
#endif  // ADC_PRESCALER
  meta->adcFrequency = F_CPU >> adps;
  if (meta->adcFrequency > (RECORD_EIGHT_BITS ? 2000000 : 1000000)) {
    error("Sample Rate Too High");
  }
#if ROUND_SAMPLE_INTERVAL
  // Round so interval is multiple of ADC clock.
  ticks += 1 << (adps - 1);
  ticks >>= adps;
  ticks <<= adps;
#endif  // ROUND_SAMPLE_INTERVAL

  if (PIN_COUNT > sizeof(meta->pinNumber)/sizeof(meta->pinNumber[0])) {
    error("Too many pins");
  }
  meta->pinCount = PIN_COUNT;
  meta->recordEightBits = RECORD_EIGHT_BITS;

  for (int i = 0; i < PIN_COUNT; i++) {
    uint8_t pin = PIN_LIST[i];
    if (pin >= NUM_ANALOG_INPUTS) {
      error("Invalid Analog pin number");
    }
    meta->pinNumber[i] = pin;

    // Set ADC reference and low three bits of analog pin number.
    adcmux[i] = (pin & 7) | ADC_REF;
    if (RECORD_EIGHT_BITS) {
      adcmux[i] |= 1 << ADLAR;
    }

    // If this is the first pin, trigger on timer/counter 1 compare match B.
    adcsrb[i] = i == 0 ? (1 << ADTS2) | (1 << ADTS0) : 0;
#ifdef MUX5
    if (pin > 7) {
      adcsrb[i] |= (1 << MUX5);
    }
#endif  // MUX5
    adcsra[i] = (1 << ADEN) | (1 << ADIE) | adps;
    adcsra[i] |= i == 0 ? 1 << ADATE : 1 << ADSC;
  }

  // Setup timer1
  TCCR1A = 0;
  uint8_t tshift;
  if (ticks < 0X10000) {
    // no prescale, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10);
    tshift = 0;
  } else if (ticks < 0X10000*8) {
    // prescale 8, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    tshift = 3;
  } else if (ticks < 0X10000*64) {
    // prescale 64, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11) | (1 << CS10);
    tshift = 6;
  } else if (ticks < 0X10000*256) {
    // prescale 256, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12);
    tshift = 8;
  } else if (ticks < 0X10000*1024) {
    // prescale 1024, CTC mode
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12) | (1 << CS10);
    tshift = 10;
  } else {
    error("Sample Rate Too Slow");
  }
  // divide by prescaler
  ticks >>= tshift;
  // set TOP for timer reset
  ICR1 = ticks - 1;
  // compare for ADC start
  OCR1B = 0;

  // multiply by prescaler
  ticks <<= tshift;

  // Sample interval in CPU clock ticks.
  meta->sampleInterval = ticks;
  meta->cpuFrequency = F_CPU;
  float sampleRate = (float)meta->cpuFrequency/meta->sampleInterval;
}
//------------------------------------------------------------------------------
// enable ADC and timer1 interrupts
void adcStart() {
  // initialize ISR
  isrBufNeeded = true;
  isrOver = 0;
  adcindex = 1;

  // Clear any pending interrupt.
  ADCSRA |= 1 << ADIF;

  // Setup for first pin.
  ADMUX = adcmux[0];
  ADCSRB = adcsrb[0];
  ADCSRA = adcsra[0];

  // Enable timer1 interrupts.
  timerError = false;
  timerFlag = false;
  TCNT1 = 0;
  TIFR1 = 1 << OCF1B;
  TIMSK1 = 1 << OCIE1B;
}
//------------------------------------------------------------------------------
void adcStop() {
  TIMSK1 = 0;
  ADCSRA = 0;
}

//------------------------------------------------------------------------------
// log data
// max number of blocks to erase per erase call
uint32_t const ERASE_SIZE = 262144L;
void logData() {
  uint32_t bgnBlock, endBlock;

  // Allocate extra buffer space.
  block_t block[BUFFER_BLOCK_COUNT];

  // Initialize ADC and timer1.
  adcInit((metadata_t*) &block[0]);

  // Find unused file name.
  if (BASE_NAME_SIZE > 6) {
    error("FILE_BASE_NAME too long");
  }
  while (sd.exists(binName)) {
    if (binName[BASE_NAME_SIZE + 1] != '9') {
      binName[BASE_NAME_SIZE + 1]++;
    } else {
      binName[BASE_NAME_SIZE + 1] = '0';
      if (binName[BASE_NAME_SIZE] == '9') {
        error("Can't create file name");
      }
      binName[BASE_NAME_SIZE]++;
    }
  }
  // Delete old tmp file.
  //if (sd.exists(TMP_FILE_NAME)) {
    //Serial.println(F("Deleting tmp file"));
    //if (!sd.remove(TMP_FILE_NAME)) {
      //error("Can't remove tmp file");
    //}
  //}
  // Create new file.
  binFile.close();
  if (!binFile.createContiguous(sd.vwd(),
                                binName, 512 * FILE_BLOCK_COUNT)) {
    error("createContiguous failed");
  }
  // Get the address of the file on the SD.
  if (!binFile.contiguousRange(&bgnBlock, &endBlock)) {
    error("contiguousRange failed");
  }
  // Use SdFat's internal buffer.
  uint8_t* cache = (uint8_t*)sd.vol()->cacheClear();
  if (cache == 0) {
    error("cacheClear failed");
  }

  // Flash erase all data in the file.
  uint32_t bgnErase = bgnBlock;
  uint32_t endErase;
  while (bgnErase < endBlock) {
    endErase = bgnErase + ERASE_SIZE;
    if (endErase > endBlock) {
      endErase = endBlock;
    }
    if (!sd.card()->erase(bgnErase, endErase)) {
      error("erase failed");
    }
    bgnErase = endErase + 1;
  }
  // Start a multiple block write.
  if (!sd.card()->writeStart(bgnBlock, FILE_BLOCK_COUNT)) {
    error("writeBegin failed");
  }
  // Write metadata.
  if (!sd.card()->writeData((uint8_t*)&block[0])) {
    error("Write metadata failed");
  }
  // Initialize queues.
  emptyHead = emptyTail = 0;
  fullHead = fullTail = 0;

  // Use SdFat buffer for one block.
  emptyQueue[emptyHead] = (block_t*)cache;
  emptyHead = queueNext(emptyHead);

  // Put rest of buffers in the empty queue.
  for (uint8_t i = 0; i < BUFFER_BLOCK_COUNT; i++) {
    emptyQueue[emptyHead] = &block[i];
    emptyHead = queueNext(emptyHead);
  }
  // Give SD time to prepare for big write.
  delay(1000);
  // Wait for Serial Idle.
  delay(10);
  uint32_t bn = 1;
  uint32_t t0 = millis();
  uint32_t t1 = t0;
  uint32_t overruns = 0;
  uint32_t count = 0;
  uint32_t maxLatency = 0;

  // Start logging interrupts.
  adcStart();
  uint8_t toggle = 0;
  while (1) {
    if (fullHead != fullTail) {
      // Get address of block to write.
      block_t* pBlock = fullQueue[fullTail];

      // Write block to SD.
      uint32_t usec = micros();
      if (!sd.card()->writeData((uint8_t*)pBlock)) {
        error("write data failed");
      }
      usec = micros() - usec;
      t1 = millis();
      if (usec > maxLatency) {
        maxLatency = usec;
      }
      count += pBlock->count;

      // Add overruns and possibly light LED.
      if (pBlock->overrun) {
        overruns += pBlock->overrun;
        if (ERROR_LED_PIN >= 0) {
          digitalWrite(ERROR_LED_PIN, HIGH);
        }
      }
      // Move block to empty queue.
      emptyQueue[emptyHead] = pBlock;
      emptyHead = queueNext(emptyHead);
      fullTail = queueNext(fullTail);
      bn++;
      if (bn == FILE_BLOCK_COUNT) {
        // File full so stop ISR calls.
        adcStop();
        break;
      }
    }
    if (timerError) {
      error("Missed timer event - rate too high");
    }
    //digitalWrite(DEBUG_LED_PIN, HIGH);
    //uint16_t tec_value = analogRead(TEC_PIN);
    if (analogRead(TEC_PIN) > 550)
    {
      digitalWrite(DEBUG_LED_PIN, HIGH);
      analogWrite(PWM_PIN, 50);
    }
    else if (analogRead(TEC_PIN) < 450)
    {
      digitalWrite(DEBUG_LED_PIN, LOW);
      analogWrite(PWM_PIN, 255);
    }
    /*
    if(toggle == 100)
    {
      digitalWrite(DEBUG_LED_PIN, HIGH);
      toggle = 0;
    }
    else
    {
      digitalWrite(DEBUG_LED_PIN, LOW);
      toggle++;
    }
    */
  }
  
  if (!sd.card()->writeStop()) {
    error("writeStop failed");
  }
  // Truncate file if recording stopped early.
  if (bn != FILE_BLOCK_COUNT) {
    if (!binFile.truncate(512L * bn)) {
      error("Can't truncate file");
    }
  }
  if (!binFile.rename(sd.vwd(), binName)) {
    error("Can't rename file");
  }
}


//------------------------------------------------------------------------------
void setup(void) {
  if (ERROR_LED_PIN >= 0) {
    pinMode(ERROR_LED_PIN, OUTPUT);
    pinMode(DEBUG_LED_PIN, OUTPUT);
    pinMode(TEC_PIN, INPUT);
    pinMode(PWM_PIN, OUTPUT);
  }
  // Read the first sample pin to init the ADC.
  analogRead(PIN_LIST[0]);
  
  // initialize file system.
  if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) {
    sd.initErrorPrint();
    fatalBlink();
  }
  logData();
}
//------------------------------------------------------------------------------
void loop(void) {
  
}
#else  // __AVR__
#error This program is only for AVR.
#endif  // __AVR__


