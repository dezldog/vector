// dezldog cobbled and glued this together
// Playing with my gps and LCD
// some code mine, some code borrowed from Adafruit.
// buy your stuff from them - their tutorials in information are priceless

#include "Wire.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GPS.h"
#include "Adafruit_LiquidCrystal.h"


// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences.
#define GPSECHO  false

//define potentiometer input/variable
#define   potPin A0     // the potentiometer is connected to a1
float potReading;     // the analog reading from the potentiometer

//calibrate a/d for potentiometer
float yIntercept = 646.00;
float xIntercept = 3.3;
float slope = ((0 - xIntercept) / (yIntercept - 0));
float volts = 0;

//Should we display metric or imperial?
//metric = 0, imperial = 1
#define UNITS 1

//What timezone to display?
//GMT = 0, PST = -8, PDT = -7
#define HOUR_OFFSET -8

// Is it DST?
#define DST 0
const int dstPin = 9;
int dstON = 0;

int displayValue = 0;

//24 or 12hr time?
#define TIME_24_HOUR   false

// Format serial output for humans or computers
// Humans = 0 computers = 1
#define serialFormat  1

// Create LCD object
Adafruit_LiquidCrystal lcd0(0);
Adafruit_LiquidCrystal lcd1(1);

//Set up 7-seg displays
Adafruit_7segment matrix0 = Adafruit_7segment();
Adafruit_7segment matrix1 = Adafruit_7segment();

//#define SERIALPORT   (Serial)
Adafruit_GPS GPS(&Serial1);

// Variables for formatting
int hours = 0;
int minutes = 0;
int seconds = 0;
int sats = 0;
float velocity = 0;
float elevation = 0;
float heading;

//Other variables
uint32_t timer = millis();
int GPSBaud = 9600;

//Stuff for Temperature probes
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
// The actual variables
float tempProbe0;
float tempProbe1;

void setup()
{
  // Start gps
  Serial.begin(115200);
  GPS.begin(GPSBaud);
  Serial1.begin (GPSBaud);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
  GPS.sendCommand(PGCMD_ANTENNA);

  // for temperature measurement stability
  // likely not necessary for my circuit
  //analogReference(EXTERNAL);
  
  //DST button
  pinMode(dstPin, INPUT);

  //Start 7 segment displays
  matrix0.begin(0x70);
  matrix1.begin(0x71);
  
  //Start TXT LCDs
  lcd0.begin(16, 4);
  lcd1.begin(16, 4);

  //Print a fun message
  lcd0.print("Welcome to Dezldog");
  lcd0.setCursor(0, 1);
  lcd0.print("Start-Up Complete");
  lcd0.setCursor(0, 2);
  lcd0.print("Version -spectre-");
  lcd0.setCursor(0, 3);
  for (int x = 0; x < 20; x++)
  {
    lcd0.setCursor(x, 3);
    lcd0.print("0");
    delay(200);
    lcd0.setCursor(x - 1, 3);
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

  lcd1.setCursor(0, 0);
  lcd1.print("Voltage:");
  lcd1.setCursor(0, 1);
  lcd1.print("Angle:");
  lcd1.setCursor(0, 2);
  lcd1.print("Geoid:");
}

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
   displayLcd1();
    
   //Send serial
   writeToSerial();

   //Send to 7 Segment displays
   print7Seg();
   displayTime();
   }
}

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
  //velocity = GPS.speed; Set up earlier now
  lcd0.setCursor(2, 1);
  if (velocity < 10 )
    {
    lcd0.print("0");
    }
  lcd0.print(velocity);

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

void displayLcd1()
  {
    lcd1.setCursor(8, 0);
    lcd1.print(volts);
    lcd1.setCursor(8, 1);
    lcd1.print(GPS.angle);
    lcd1.setCursor(8, 2);
    lcd1.print(GPS.geoidheight);
  }

void print7Seg()
  {
    matrix0.print(velocity);
    matrix0.writeDisplay();
  }

void writeToSerial()
  {
    if (serialFormat)
      {
      // The format is
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
            Serial.print(GPS.angle); Serial.print(",");
            Serial.print(GPS.geoidheight); Serial.print(",");
            Serial.print(volts); Serial.print(",");
          }  
        else
        {
          Serial.print(",,,,,,,,,,");
        }
      Serial.print(tempProbe0); Serial.print(",");
      Serial.print(tempProbe1); //Serial.print(",");
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
      Serial.print("Voltage: ");Serial.println(volts);
      Serial.print("Temperature A:  "); Serial.print(tempProbe0); Serial.println(" *C");
      Serial.print("Temperature B:  "); Serial.print(tempProbe1); Serial.println(" *C");
      Serial.println();
    }
  }

//Display pretty-ified time to one of the 7 seg displays
void displayTime()
  {
   int hours = GPS.hour + HOUR_OFFSET;
 
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
  
  if (isDST())
    {
     displayValue = (hours + 1)*100 + minutes;
      
    }
  else
    {
      displayValue = hours*100 + minutes;
    }

  // Do 24 hour to 12 hour format conversion when required.
  if (!TIME_24_HOUR) {
    // Handle when hours are past 12 by subtracting 12 hours (1200 value).
    if (hours > 12) {
      displayValue -= 1200;
    }
    // Handle hour 0 (midnight) being shown as 12.
    else if (hours == 0) {
      displayValue += 1200;
    }
  }

  // Now print the time value to the display.
  matrix1.print(displayValue, DEC);

  // Add zero padding when in 24 hour mode and it's midnight.
  // In this case the print function above won't have leading 0's
  // which can look confusing.  Go in and explicitly add these zeros.
  if (TIME_24_HOUR && hours == 0) {
    // Pad hour 0.
    matrix1.writeDigitNum(1, 0);
    // Also pad when the 10's minute is 0 and should be padded.
    if (minutes < 10) {
      matrix1.writeDigitNum(2, 0);
    }
  }

  // Blink the colon by turning it on every even second and off
  // every odd second.  The modulus operator is very handy here to
  // check if a value is even (modulus 2 equals 0) or odd (modulus 2
  // equals 1).
  matrix1.drawColon(seconds % 2 == 0);

  // Now push out to the display the new values that were set above.
  matrix1.writeDisplay();
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
    delay(10);
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

bool isDST()
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
