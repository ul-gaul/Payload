#include <math.h>

// ____________ TEMPERATURE CONVERTER SETTINGS ______________
#define PHOTOPIN A1
// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3900
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    
// Operating Temperature (C)


// ___________ TEC CONTROL SETTINGS ______________

#define TREF 30
// Fine control Temperature range (C)
#define DELTA 3
// Max drive current (0 to 1000 mA)
#define Imax 200
// which analog pin to send Current
#define DRIVERPIN A2 
// Set pins
// Current out pin: Currently A2 with 0 to 1000 mA with 0 to 1000 bits (CLEARLY BAD, just coding purpose)


uint16_t samples[NUMSAMPLES];
 
void setup(void) {
  Serial.begin(9600);
  analogReference(EXTERNAL);
}
 
void loop(void) {
  uint8_t i;
  int sendBits;

  // _________________ TEMPERATURE CONVERTER _________________
  float average;

  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   delay(10);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
  
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
 
  float steinhart;
  float Tmeas;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  Tmeas = 1.0 / steinhart;                 // Invert
  Tmeas -= 273.15;                         // convert to C

  // _____________________ TEC CONTROLLER ________________________
   float sendCurrent;
   float Err = Tmeas - float(TREF);
   
   if( Err > DELTA)    // cooling at max  - positive voltage
       {
          sendCurrent = Imax; 
       }
        
   if(Err < DELTA)   // cooling at fine tuning
       {
          // PID or other control
          sendCurrent =  fscale( 0, DELTA, 0, Imax, Err, 0);        // Map Error from (0:DELTA) to (0:Imax) scale. Last argument is the map Curve (0 = linear).    
          constrain(sendCurrent, 0, Imax);
       }

   // _____________ Send Current ________________
   sendBits = int(sendCurrent * 1023/1000);                               // CONVERT Current to Bits (sendCurrent in mA) [Ex: B = I * 4096/1000] for 1000mA = 4096 bits
   // analogWrite(DRIVERPIN, sendBits);

   Serial.print("Error ");
   Serial.print(Err);
   Serial.print("  PD : ");
   Serial.print(analogRead(PHOTOPIN));
   Serial.print("   T : ");
   Serial.println(Tmeas);
   
   delay(50); 
}

// __________ END __________



// -------------------------
// ____ fscale FUNCTION ____

float fscale( float originalMin, float originalMax, float newBegin, float
newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  /*
   Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution  
   Serial.println(); 
   */

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  /*
  Serial.print(OriginalRange, DEC);  
   Serial.print("   ");  
   Serial.print(NewRange, DEC);  
   Serial.print("   ");  
   Serial.println(zeroRefCurVal, DEC);  
   Serial.println();  
   */

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}

