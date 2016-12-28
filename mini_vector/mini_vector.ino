// dezldog cobbled and glued this together
// This eventually will be a dashboard for an electric car or off road vehicle.
// Some of the code is mine, some code borrowed from Adafruit, probably some borrowed from others, please share away!
// Buy your stuff from Adafruit - their tutorials and information are priceless

//This is written for Arduino Uno R3 and similar

#include "Wire.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GPS.h"
#include "Adafruit_LiquidCrystal.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_LSM303_U.h"
#include "Adafruit_MPL3115A2.h"

#define GPSECHO   false     //for debugging from GPS before codes
const int GPSBaud = 9600;

#define to smooth out velocity noise from gps
int speed = 0;

//define potentiometer input/variable
#define   potPin A0         // the potentiometer is connected to A0
float potReading;           // the analog reading from the potentiometer

//calibrate a/d for potentiometer
float yIntercept = 646.00;
float xIntercept = 3.3;
float slope = ((0 - xIntercept) / (yIntercept - 0));
float volts = 0;

//Should we display metric or imperial?
//metric = 0, imperial = 1
#define UNITS 1

//What timezone to display (in Standard time)?
//GMT = 0, PST = -8
#define HOUR_OFFSET -8

// Is it DST?
#define DST 0
const int dstPin = 13;
int dstON = 0;

int displayDSTValue = 0;
int displayValue;
  
//24 or 12hr time?
#define TIME_24_HOUR   false

// Format serial output for humans or computers
// Humans = 0 computers = 1
#define serialFormat  0

// Create LCD object
Adafruit_LiquidCrystal lcd0(0);

//Set up 7-seg displays
Adafruit_7segment matrix0 = Adafruit_7segment();
Adafruit_7segment matrix1 = Adafruit_7segment();

SoftwareSerial gpsSerial(8, 7);
//Set up GPS
Adafruit_GPS GPS(&gpsSerial);

int hours = 0;
int minutes = 0;
int seconds = 0;
int sats = 0;
float velocity = 0;
float elevation = 0;
float heading;
float accelX;
float accelY;
float accelZ;
float Pi = 3.14159;
float pascals;
float altB;
float tempB;
float pressure;
float presB;
uint32_t timer = millis();

//Stuff for Temperature probes
#define THERM0 A1               //Where is the first RTD connected?
#define THERM1 A2               //Where is the second RTD connected?
#define THERM_OHMS 10000        // RTD Type
#define THERM_TEMP 25           // RTD Ref Temp
#define THERM_SAMP 5            // Samples to average
#define BCOEFFICIENT 3950       // The beta coefficient of the thermistor
#define SERIESRESISTOR 10000    // the value of the voltage dividing resistor
float tempProbe0;               // The actual variables
float tempProbe1;               



////////////////////////////////////////////////////
void setup()
{
  // Start gps
  Serial.begin(115200);
  GPS.begin(GPSBaud);
  gpsSerial.begin (GPSBaud);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
  GPS.sendCommand(PGCMD_ANTENNA);
  enableGPSInterrupt();
  
  // for temperature measurement stability
  // likely not necessary for my circuit
  //analogReference(EXTERNAL);

    
  //DST button
  pinMode(dstPin, INPUT);

  //Start 7 segment displays
  matrix0.begin(0x70);
  matrix1.begin(0x71);
  
  //Start TXT LCDs
  lcd0.begin(20, 4);

  //Print a fun message
  lcd0.print("Welcome to Dezldog!");
  lcd0.setCursor(0, 1);
  lcd0.print("Starting Up, Yo!");
  lcd0.setCursor(0, 2);
  lcd0.print("Version -spectre-");
  lcd0.setCursor(0, 3);
  for (int x = 0; x < 20; x++)
  {
    lcd0.setCursor(x, 3);
    lcd0.print("->");
    delay(150);
    lcd0.setCursor(x - 2, 3);
    lcd0.print(" ");
  }
  lcd0.clear();
  
  // Setup labels so they don't have to written every update
  lcd0.setCursor(0, 1);
  lcd0.print("V=");
  lcd0.setCursor(9, 1);
  lcd0.print("Alt:");
  lcd0.setCursor(0, 2);
  lcd0.print("Lat:");
  lcd0.setCursor(14, 2);
  lcd0.print("Sat:");
  lcd0.setCursor(0, 3);
  lcd0.print("Lon:");
  
}


////////////////////////////////////////////////////
void loop()
{
 
  char c = GPS.read();
    
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) Serial.print(c);
 
  if (GPS.newNMEAreceived())
    {
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
    }

  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis())  timer = millis();
  
  // approximately every 1 seconds or so, print out the current stats
  if (millis() - timer > 1000)
    {
    timer = millis(); // reset the timer

  //Read and calculate Volts from potentiometer a/d value
  potReading = analogRead(potPin);
  volts = 3.3 + (potReading * slope);
 
  //duh
  getTemps();
  
  if (UNITS)
    {
      velocity = GPS.speed * 0.621371;
      elevation = GPS.altitude * 3.28084;  
    }
  else
    {
      velocity = GPS.speed;
      elevation = GPS.altitude;  
    }
  
  //Write to the TXT LCDs
  displayLcd0();
    
  //Send serial
  writeToSerial();

  //Send to 7 Segment displays
  displaySpeed();
  displayTime();
  }
}


////////////////////////////////////////////////////
void displayLcd0()
{
  //Where to we print the date?
  lcd0.setCursor(0, 0);
  lcd0.print(GPS.month);
  lcd0.print("/");
  lcd0.print(GPS.day);
  lcd0.print("/");
  lcd0.print(GPS.year);
    
  // Where are we ging to show the time?
  lcd0.setCursor(9, 0);
  hours = GPS.hour;
  if (hours < 10)
    {
    lcd0.print("0");
    lcd0.print(hours);
    }
  else
    {
    lcd0.print(hours);
    }
  lcd0.print(':');
  minutes = GPS.minute;
  if (minutes < 10)
    {
    lcd0.print("0");
    lcd0.print(minutes);
    }
  else
    {
    lcd0.print(GPS.minute);
    }
  lcd0.print(':');
  seconds = GPS.seconds;
  if (seconds < 10)
    {
    lcd0.print("0");
    lcd0.print(seconds);
    }
  else
    {
    lcd0.print(seconds);
    }
  lcd0.print("UTC");

  //Show the velocity
  lcd0.setCursor(2, 1);
  if (velocity < 10 )
    {
    lcd0.print("0");
    }
  lcd0.print(velocity, 1);

  //Show Altitude
  lcd0.setCursor(14, 1);
  lcd0.print(elevation);

  //Display the Latitude
  lcd0.setCursor(4, 2);
  lcd0.print(GPS.latitudeDegrees, 6);

  // How many satellites are we using?
  lcd0.setCursor(18, 2);
  sats = GPS.satellites;
  if (sats < 10)
    {
    lcd0.print("0");
    lcd0.print(sats);
    }
  else
    {
    lcd0.print(sats);
    }
  
  //Display Longitude
  lcd0.setCursor(4, 3);
  lcd0.print(GPS.longitudeDegrees, 6);

  //Do we have a fix?
  lcd0.setCursor(17, 3);
  if ((int)GPS.fix)
    {
    lcd0.print("Fix");
    }
  else
    {
    lcd0.print("NFX");
    }
}

void displaySpeed()
  {
    speed == velocity;
    if (speed <= 1) speed = 0;
    matrix0.print(speed);
    matrix0.writeDisplay();
  }

void displayTime()     //Display pretty-ified time to one of the 7 seg displays
  {
   int hours = GPS.hour + HOUR_OFFSET + isDST();

   if (hours < 0) 
    {
      hours = 24+hours;
    }
   if (hours > 23) 
    {
      hours = 24-hours;
    }
    
  int minutes = GPS.minute;
  int seconds = GPS.seconds;

  displayValue = hours*100 + minutes;
  
  if (!TIME_24_HOUR) {
    if (hours > 12) {
      displayDSTValue -= 1200;
    }
    // Handle hour 0 (midnight) being shown as 12.
    else if (hours == 0) {
      displayDSTValue += 1200;
    }
  }

  // Now print the time value to the display.
  matrix1.print(displayDSTValue, DEC);

  if (TIME_24_HOUR && hours == 0) {
    // Pad hour 0.
    matrix1.writeDigitNum(1, 0);
    // Also pad when the 10's minute is 0 and should be padded.
    if (minutes < 10) {
      matrix1.writeDigitNum(2, 0);
    }
  }

  // Blink the colon
  matrix1.drawColon(seconds % 2 == 0);

  // Now push out to the display the new values that were set above.
  matrix1.writeDisplay();
}

void writeToSerial()
  {
    if (serialFormat)
      {
      // The format is:
      // hr,min,sec,day,mon,yr,fix,fixqual,lat,long,speed,altitude,satellites,angle,geoidheight,voltage
      Serial.print(GPS.hour, DEC); Serial.print(',');
      Serial.print(GPS.minute, DEC); Serial.print(',');
      Serial.print(GPS.seconds, DEC); Serial.print(',');
      Serial.print(GPS.day, DEC); Serial.print(',');
      Serial.print(GPS.month, DEC); Serial.print(",20");
      Serial.print(GPS.year, DEC); Serial.print(',');
      Serial.print((int)GPS.fix); Serial.print(',');
      Serial.print((int)GPS.fixquality); Serial.print(',');
        if (GPS.fix)
          {
            Serial.print(GPS.latitudeDegrees, 6); Serial.print(",");
            Serial.print(GPS.longitudeDegrees, 6); Serial.print(",");
            Serial.print(GPS.speed); Serial.print(",");
            Serial.print(GPS.altitude); Serial.print(",");
            Serial.print((int)GPS.satellites); Serial.print(",");
          }  
        else
        {
          Serial.print(",,,,,,,,");
        }
      Serial.print(volts); Serial.print(",");
      Serial.print(tempProbe0); Serial.print(",");
      Serial.print(tempProbe1); Serial.print(",");
      Serial.print(heading); Serial.print(",");
      Serial.print(pressure); Serial.print(",");
      Serial.print(accelX); Serial.print(",");
      Serial.print(accelY); Serial.print(",");
      Serial.print(accelZ); //Serial.print(",");
      Serial.println();
 
      }
    else //print out human readable
      {
      Serial.print("\nTime: ");
      Serial.print(GPS.hour, DEC); Serial.print(':');
      Serial.print(GPS.minute, DEC); Serial.print(':');
      Serial.print(GPS.seconds, DEC); Serial.println();
      Serial.print("Date: ");
      Serial.print(GPS.day, DEC); Serial.print('/');
      Serial.print(GPS.month, DEC); Serial.print("/20");
      Serial.println(GPS.year, DEC);
      Serial.print("Fix: "); Serial.print((int)GPS.fix);
      Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
        if (GPS.fix)
          {
            Serial.print("Location: ");
            Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
            Serial.print(", ");
            Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
            Serial.print("Location (in degrees.decimals): ");
            Serial.print(GPS.latitudeDegrees, 6);
            Serial.print(", ");
            Serial.println(GPS.longitudeDegrees, 6);
            Serial.print("Speed (knots): "); Serial.println(GPS.speed);
            Serial.print("Altitude: "); Serial.println(GPS.altitude);
            Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
            Serial.print("Angle: ");Serial.println(GPS.angle);
            Serial.print("Geoid Height: ");Serial.println(GPS.geoidheight);
          }
      Serial.print("vPotentiometer: ");Serial.println(volts);Serial.print("Volts");
      Serial.print("Inlet Temp:  "); Serial.print(tempProbe0); Serial.println(" *C");
      Serial.print("Ambient Temp:  "); Serial.print(tempProbe1); Serial.println(" *C");
      Serial.print("Heading: ");Serial.print(heading); Serial.print("*");
      Serial.print("Pressure: ");Serial.print(pressure); Serial.print("inHg");
      Serial.print("Acceleration: ");Serial.print(accelX); Serial.print("g");
      Serial.print("Lateral Acceleration: ");Serial.print(accelY); Serial.print("g");
      Serial.print("Vertical Acceleration: ");Serial.print(accelZ); Serial.print("g");      
      Serial.println();
    }
  }

void getTemps()
  {
  uint8_t i;
  int samples0[THERM_SAMP];
  int samples1[THERM_SAMP];
  float thermAverage0 = 0;
  float thermAverage1 = 0;

  // take N samples in a row, with a slight delay
  for (i=0; i< THERM_SAMP; i++) {
    samples0[i] = analogRead(THERM0);
    samples1[i] = analogRead(THERM1);
    delay(20);
  }

  // average all the samples out
  for (i=0; i< THERM_SAMP; i++) {
     thermAverage0 += samples0[i];
     thermAverage1 += samples1[i];
  }
  thermAverage0 /= THERM_SAMP;
  thermAverage1 /= THERM_SAMP;


  // convert the value to resistance
  thermAverage0 = 1023 / thermAverage0 - 1;
  thermAverage1 = 1023 / thermAverage1 - 1;
  thermAverage0 = SERIESRESISTOR / thermAverage0;
  thermAverage1 = SERIESRESISTOR / thermAverage1;

  tempProbe0 = thermAverage0 / THERM_OHMS;
  tempProbe0 = log(tempProbe0);
  tempProbe0 /= BCOEFFICIENT;
  tempProbe0 += 1.0 / (THERM_TEMP + 273.15);
  tempProbe0 = 1.0 / tempProbe0;
  tempProbe0 -= 273.15;

  tempProbe1 = thermAverage1 / THERM_OHMS;
  tempProbe1 = log(tempProbe1);
  tempProbe1 /= BCOEFFICIENT;
  tempProbe1 += 1.0 / (THERM_TEMP + 273.15);
  tempProbe1 = 1.0 / tempProbe1;
  tempProbe1 -= 273.15;

}

SIGNAL(TIMER0_COMPA_vect) {
  GPS.read();
}

void enableGPSInterrupt() {
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
}


int isDST()
{
  dstON = digitalRead(dstPin);
  if (dstON == HIGH)
    {
      return 1;
    } 
  else
    {
      return 0;
    }
}
