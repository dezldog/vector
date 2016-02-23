// which pins are we using?
#define THERM0 A1
#define THERM1 A2
// RTD Type and Ref Temp
#define THERM_OHMS 10000
#define THERM_TEMP 25
// Sample for noise reduction
#define THERM_SAMP 5
// The beta coefficient of the thermistor
#define BCOEFFICIENT 3950
// the value of the voltage dividing resistor
#define SERIESRESISTOR 10000

float tempProbe0;
float tempProbe1;

void setup(void) {
  Serial.begin(9600);
  analogReference(EXTERNAL);
}

void loop(void)
  {
  uint8_t i;
  int samples0[THERM_SAMP];
  int samples1[THERM_SAMP];
  float thermAverage0;
  float thermAverage1;

  // take N samples in a row, with a slight delay
  for (i=0; i< THERM_SAMP; i++) {
    samples0[i] = analogRead(THERM0);
    samples1[i] = analogRead(THERM1);
    delay(10);
  }

  // average all the samples out
  thermAverage0 = 0;
  thermAverage1 = 0;
  for (i=0; i< THERM_SAMP; i++) {
     thermAverage0 += samples0[i];
     thermAverage1 += samples1[i];
  }
  thermAverage0 /= THERM_SAMP;
  thermAverage1 /= THERM_SAMP;


  // convert the value to resistance
  thermAverage0 = 1023 / thermAverage0 - 1;
  thermAverage0 = SERIESRESISTOR / thermAverage0;
  thermAverage1 = SERIESRESISTOR / thermAverage1;

  float tempProbe0;
  tempProbe0 = thermAverage0 / THERM_OHMS;
  tempProbe0 = log(tempProbe0);
  tempProbe0 /= BCOEFFICIENT;
  tempProbe0 += 1.0 / (THERM_TEMP + 273.15);
  tempProbe0 = 1.0 / tempProbe0;
  tempProbe0 -= 273.15;


  float tempProbe1;
  tempProbe1 = thermAverage1 / THERM_OHMS;
  tempProbe1 = log(tempProbe1);
  tempProbe1 /= BCOEFFICIENT;
  tempProbe1 += 1.0 / (THERM_TEMP + 273.15);
  tempProbe1 = 1.0 / tempProbe1;
  tempProbe1 -= 273.15;

  Serial.print("Temperature A:  "); Serial.print(tempProbe0); Serial.println(" *C");
  Serial.print("Temperature B:  "); Serial.print(tempProbe1); Serial.println(" *C");

}
