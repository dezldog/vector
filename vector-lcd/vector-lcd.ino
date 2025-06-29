// dezldog cobbled and glued this together
// Complete dashboard for an electric car or off road vehicle
// Some of the code is mine, some code borrowed from Adafruit, probably some borrowed from others, please share away!
// Buy your stuff from Adafruit - their tutorials and information are priceless

// This is written for Arduino Zero and similar

#include "Wire.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GPS.h"
#include "Adafruit_LiquidCrystal.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_LSM303_U.h"
#include "Adafruit_MPL3115A2.h"

#define GPSECHO false     // for debugging from GPS before codes
const int GPSBaud = 9600;

// Define potentiometer input/variable
#define potPin A0         // the potentiometer is connected to A0
float potReading;         // the analog reading from the potentiometer

// Calibrate a/d for potentiometer
float yIntercept = 646.00;
float xIntercept = 3.3;
float slope = ((0 - xIntercept) / (yIntercept - 0));
float volts = 0;

// Should we display metric or imperial?
// metric = 0, imperial = 1
#define UNITS 1

// What timezone to display (in Standard time)?
// GMT = 0, PST = -8
#define HOUR_OFFSET -8

// Is it DST?
#define DST 0
const int dstPin = 13;
int dstON = 1;

int displayDSTValue = 0;
int displayValue;

// 24 or 12hr time?
#define TIME_24_HOUR false

// Format serial output for humans or computers
// Humans = 0 computers = 1
#define serialFormat 1

// Create LCD objects
Adafruit_LiquidCrystal lcd0(0);
Adafruit_LiquidCrystal lcd1(1);

// Set up 7-seg displays
Adafruit_7segment matrix0 = Adafruit_7segment();
Adafruit_7segment matrix1 = Adafruit_7segment();

// Set up GPS
Adafruit_GPS GPS(&Serial1);

// GPS and sensor variables
int hours = 0;
int minutes = 0;
int seconds = 0;
int sats = 0;
float velocity = 0;
float elevation = 0;
float heading = 0;
float accelX = 0;
float accelY = 0;
float accelZ = 0;
float Pi = 3.14159;
float pascals = 0;
float altB = 0;
float tempB = 0;
float pressure = 0;
float presB = 0;
uint32_t timer = millis();

// Temperature probes configuration
#define THERM0 A1               // Where is the first RTD connected?
#define THERM1 A2               // Where is the second RTD connected?
#define THERM_OHMS 10000        // RTD Type
#define THERM_TEMP 25           // RTD Ref Temp
#define THERM_SAMP 5            // Samples to average
#define BCOEFFICIENT 3950       // The beta coefficient of the thermistor
#define SERIESRESISTOR 10000    // the value of the voltage dividing resistor
float tempProbe0 = 0;           // The actual variables
float tempProbe1 = 0;

// Set up accelerometer/magnetic sensor/barometer sensors
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(12345);
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();

////////////////////////////////////////////////////
void setup()
{
  // Start serial communication
  Serial.begin(115200);
  
  // Start GPS
  GPS.begin(GPSBaud);
  Serial1.begin(GPSBaud);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
  GPS.sendCommand(PGCMD_ANTENNA);

  // For temperature measurement stability
  // analogReference(EXTERNAL); // Uncomment if using external reference

  // Start accelerometer/magnetometer
  if (!accel.begin()) {
    Serial.println("Ooops, no LSM303 accelerometer detected ... Check your wiring!");
  }
  if (!mag.begin()) {
    Serial.println("Ooops, no LSM303 magnetometer detected ... Check your wiring!");
  }
  mag.enableAutoRange(true); // Enable AGC

  // Start Barometer
  if (!baro.begin()) {
    Serial.println("Couldnt find MPL3115A2 barometer sensor");
  }
  
  // DST button
  pinMode(dstPin, INPUT);

  // Start 7 segment displays
  matrix0.begin(0x70);
  matrix1.begin(0x71);
  
  // Start TXT LCDs
  lcd0.begin(20, 4);
  lcd1.begin(20, 4);

  // Print startup message on LCD0
  lcd0.print("Welcome to Dezldog");
  lcd0.setCursor(0, 1);
  lcd0.print("Starting Up, Yo!");
  lcd0.setCursor(0, 2);
  lcd0.print("Version -spectre-");
  lcd0.setCursor(0, 3);
  for (int x = 0; x < 20; x++) {
    lcd0.setCursor(x, 3);
    lcd0.print("->");
    delay(150);
    lcd0.setCursor(x - 2, 3);
    lcd0.print(" ");
  }
  lcd0.clear();
  
  // Setup labels on LCD0 so they don't have to be written every update
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

  // Setup labels on LCD1
  lcd1.setCursor(0, 0);
  lcd1.print("Hdng:");
  lcd1.setCursor(8, 0);
  lcd1.print((char)223); // Degree symbol
  lcd1.setCursor(10, 0);
  lcd1.print("Vpot:");
  lcd1.setCursor(0, 1);
  lcd1.print("Baro:");
  lcd1.setCursor(10, 1);
  lcd1.print("Accl:");
  lcd1.setCursor(10, 2);
  lcd1.print("Side:");
  lcd1.setCursor(0, 2);
  lcd1.print("Inlt:");
  lcd1.setCursor(0, 3);
  lcd1.print("Ambt:");
  lcd1.setCursor(10, 3);
  lcd1.print("UpDn:");
}

////////////////////////////////////////////////////
void loop()
{
  char c = GPS.read();
    
  // If you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) Serial.print(c);
 
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  // If millis() or timer wraps around, we'll just reset it
  if (timer > millis()) timer = millis();

  // Approximately every 1 second or so, print out the current stats
  if (millis() - timer > 1000) {
    timer = millis(); // reset the timer

    // Read and calculate Volts from potentiometer a/d value
    potReading = analogRead(potPin);
    volts = 3.3 + (potReading * slope);

    // Get Accel/Mag sensor data
    getAccelMag();
    
    // Get Barometer Data
    getBarometer();
    
    // Get temperature readings
    getTemps();
    
    // Convert units if needed
    if (UNITS) {
      velocity = GPS.speed * 0.621371;  // Convert knots to MPH
      elevation = GPS.altitude * 3.28084; // Convert meters to feet
    } else {
      velocity = GPS.speed;
      elevation = GPS.altitude;  
    }
    
    // Write to the TXT LCDs
    displayLcd0();
    displayLcd1();
      
    // Send serial data
    writeToSerial();

    // Send to 7 Segment displays
    displaySpeed();
    displayTime();
  }
}

////////////////////////////////////////////////////
void displayLcd0()
{
  // Display date
  lcd0.setCursor(0, 0);
  lcd0.print(GPS.month);
  lcd0.print("/");
  lcd0.print(GPS.day);
  lcd0.print("/");
  lcd0.print(GPS.year);
    
  // Display time
  lcd0.setCursor(9, 0);
  hours = GPS.hour;
  if (hours < 10) {
    lcd0.print("0");
    lcd0.print(hours);
  } else {
    lcd0.print(hours);
  }
  lcd0.print(':');
  minutes = GPS.minute;
  if (minutes < 10) {
    lcd0.print("0");
    lcd0.print(minutes);
  } else {
    lcd0.print(GPS.minute);
  }
  lcd0.print(':');
  seconds = GPS.seconds;
  if (seconds < 10) {
    lcd0.print("0");
    lcd0.print(seconds);
  } else {
    lcd0.print(seconds);
  }
  lcd0.print("UTC");

  // Show the velocity
  lcd0.setCursor(2, 1);
  if (velocity < 10) {
    lcd0.print("0");
  }
  lcd0.print(velocity, 1);

  // Show Altitude
  lcd0.setCursor(14, 1);
  lcd0.print(elevation);

  // Display the Latitude
  lcd0.setCursor(4, 2);
  lcd0.print(GPS.latitudeDegrees, 6);

  // How many satellites are we using?
  lcd0.setCursor(18, 2);
  sats = GPS.satellites;
  if (sats < 10) {
    lcd0.print("0");
    lcd0.print(sats);
  } else {
    lcd0.print(sats);
  }
  
  // Display Longitude
  lcd0.setCursor(4, 3);
  lcd0.print(GPS.longitudeDegrees, 6);

  // Do we have a fix?
  lcd0.setCursor(17, 3);
  if ((int)GPS.fix) {
    lcd0.print("Fix");
  } else {
    lcd0.print("NFX");
  }
}

void displayLcd1()
{
  // Display heading
  lcd1.setCursor(5, 0);
  lcd1.print(heading, 0);
  
  // Display potentiometer voltage
  lcd1.setCursor(15, 0);
  lcd1.print(volts, 2);
  
  // Display barometric pressure
  lcd1.setCursor(5, 1);
  lcd1.print(presB, 1);
  
  // Display forward/back acceleration
  lcd1.setCursor(15, 1);
  lcd1.print(accelX, 1);
  
  // Display inlet temperature
  lcd1.setCursor(5, 2);
  lcd1.print(tempProbe0, 1);
  
  // Display side acceleration
  lcd1.setCursor(15, 2);
  lcd1.print(accelY, 1);
  
  // Display ambient temperature
  lcd1.setCursor(5, 3);
  lcd1.print(tempProbe1, 1);
  
  // Display vertical acceleration
  lcd1.setCursor(15, 3);
  lcd1.print(accelZ, 1);
}

void displaySpeed()
{
  if (velocity < 10) {
    matrix0.print(0);
  }
  matrix0.print(velocity, 1);
  matrix0.writeDisplay();
}

void displayTime() // Display pretty-ified time to one of the 7 seg displays
{
  int hours = GPS.hour + HOUR_OFFSET + isDST();

  if (hours < 0) {
    hours = 24 + hours;
  }
  if (hours > 23) {
    hours = 24 - hours;
  }
    
  int minutes = GPS.minute;
  int seconds = GPS.seconds;
 
  displayValue = hours * 100 + minutes;
  displayDSTValue = displayValue;
  
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
  if (serialFormat) {
    // The format is:
    // hr,min,sec,day,mon,yr,fix,fixqual,lat,long,speed,altitude,satellites,angle,geoidheight,voltage,temp0,temp1,heading,pressure,accelX,accelY,accelZ
    Serial.print(GPS.hour, DEC); Serial.print(',');
    Serial.print(GPS.minute, DEC); Serial.print(',');
    Serial.print(GPS.seconds, DEC); Serial.print(',');
    Serial.print(GPS.day, DEC); Serial.print(',');
    Serial.print(GPS.month, DEC); Serial.print(",20");
    Serial.print(GPS.year, DEC); Serial.print(',');
    Serial.print((int)GPS.fix); Serial.print(',');
    Serial.print((int)GPS.fixquality); Serial.print(',');
    
    if (GPS.fix) {
      Serial.print(GPS.latitudeDegrees, 6); Serial.print(",");
      Serial.print(GPS.longitudeDegrees, 6); Serial.print(",");
      Serial.print(GPS.speed); Serial.print(",");
      Serial.print(GPS.altitude); Serial.print(",");
      Serial.print((int)GPS.satellites); Serial.print(",");
      Serial.print(GPS.angle); Serial.print(",");
      Serial.print(GPS.geoidheight); Serial.print(",");
    } else {
      Serial.print(",,,,,,");
    }
    
    Serial.print(volts); Serial.print(",");
    Serial.print(tempProbe0); Serial.print(",");
    Serial.print(tempProbe1); Serial.print(",");
    Serial.print(heading); Serial.print(",");
    Serial.print(presB); Serial.print(",");
    Serial.print(accelX); Serial.print(",");
    Serial.print(accelY); Serial.print(",");
    Serial.print(accelZ);
    Serial.println();
  } else { // Print out human readable
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
    
    if (GPS.fix) {
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
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Geoid Height: "); Serial.println(GPS.geoidheight);
    }
    
    Serial.print("Potentiometer: "); Serial.print(volts); Serial.println(" Volts");
    Serial.print("Inlet Temp: "); Serial.print(tempProbe0); Serial.println(" *C");
    Serial.print("Ambient Temp: "); Serial.print(tempProbe1); Serial.println(" *C");
    Serial.print("Heading: "); Serial.print(heading); Serial.println("*");
    Serial.print("Pressure: "); Serial.print(presB); Serial.println(" inHg");
    Serial.print("Forward Acceleration: "); Serial.print(accelX); Serial.println(" g");
    Serial.print("Lateral Acceleration: "); Serial.print(accelY); Serial.println(" g");
    Serial.print("Vertical Acceleration: "); Serial.print(accelZ); Serial.println(" g");      
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

  // Take N samples in a row, with a slight delay
  for (i = 0; i < THERM_SAMP; i++) {
    samples0[i] = analogRead(THERM0);
    samples1[i] = analogRead(THERM1);
    delay(20);
  }

  // Average all the samples out
  for (i = 0; i < THERM_SAMP; i++) {
     thermAverage0 += samples0[i];
     thermAverage1 += samples1[i];
  }
  thermAverage0 /= THERM_SAMP;
  thermAverage1 /= THERM_SAMP;

  // Convert the value to resistance
  thermAverage0 = 1023 / thermAverage0 - 1;
  thermAverage1 = 1023 / thermAverage1 - 1;
  thermAverage0 = SERIESRESISTOR / thermAverage0;
  thermAverage1 = SERIESRESISTOR / thermAverage1;

  // Calculate temperature from thermistor resistance using Steinhart-Hart equation
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

int isDST()
{
  dstON = digitalRead(dstPin);
  if (dstON == HIGH) {
    return 1;
  } else {
    return 0;
  }
}

void getBarometer()
{
  pascals = baro.getPressure();
  presB = (pascals / 3377); // Convert to inches Hg
  // altB = baro.getAltitude(); // Uncomment if altitude from barometer is needed
  // tempB = baro.getTemperature(); // Uncomment if temperature from barometer is needed
}

void getAccelMag()
{
  sensors_event_t accelEvent; 
  sensors_event_t magEvent;
  
  accel.getEvent(&accelEvent);
  mag.getEvent(&magEvent);
  
  accelX = accelEvent.acceleration.x;
  accelY = accelEvent.acceleration.y;
  accelZ = accelEvent.acceleration.z;
  
  // Calculate heading from magnetometer
  heading = (atan2(magEvent.magnetic.y, magEvent.magnetic.x) * 180) / Pi;
  if (heading < 0) {
    heading = 360 + heading;
  }
}