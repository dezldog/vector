// dezldog cobbled and glued this together
// Playing with my gps and LCD
// some code mine, some code borrowed from Adafruit.
// buy your stuff from them - their tutorials in information are priceless

#include "Wire.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GPS.h"
#include "Adafruit_LiquidCrystal.h"
#include "DHT.h"
//#include "SPI.h"
//#include "Adafruit_GFX.h"
//#include "Adafruit_RA8875.h"
//#include "Fonts/FreeSans24pt7b.h"

//Stuff for the DHTs
float h0, t0, f0;
float h1, t1, f1;
float hif0, hic0;
float hif1, hic1;
#define DHTPIN0 22     // what pins DHTs are connected to
#define DHTPIN1 23
#define DHTTYPE DHT11   // We are using DHT11s
#define PROBE_0 "Outside"  // Let's name the DHT probes
#define PROBE_1 "Inside"
DHT dht0(DHTPIN0, DHTTYPE); // Initialize DHT sensors
DHT dht1(DHTPIN1, DHTTYPE);

// Needed for Adafruit RA8875 communication
// Library only supports hardware SPI at this time
// Connect SCLK to DUE SPI 3 (Hardware SPI clock)
// Connect MISO to DUE SPI 1 (Hardware SPI MISO)
// Connect MOSI to DUE SPI 4 (Hardware SPI MOSI)
//#define RA8875_INT 51
//#define RA8875_CS 52
//#define RA8875_RESET 53
//
//Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS, RA8875_RESET);
//uint16_t tx, ty;

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

//24 or 12hr time?
#define TIME_24_HOUR   false

// Format serial output for humans or computers
// Humans = 0 computers = 1
#define serialFormat  0

// Create LCD object
Adafruit_LiquidCrystal lcd0(0);
Adafruit_LiquidCrystal lcd1(1);

//Set up 7-seg displays
Adafruit_7segment matrix0 = Adafruit_7segment();
Adafruit_7segment matrix1 = Adafruit_7segment();

#define gpsSerial Serial1

Adafruit_GPS GPS(&gpsSerial);

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

void setup()
{
  // Start gps
  Serial.begin(115200);
  GPS.begin(GPSBaud);
  gpsSerial.begin (GPSBaud);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
  GPS.sendCommand(PGCMD_ANTENNA);  

  //Start 7 segment displays
  matrix0.begin(0x70);
  matrix1.begin(0x71);

/*
//  Initialise the sensor, make sure it is there
  if(!mag.begin())
    {
       Serial.println("LSM303 not detected");
       while(1);
    }
*/
/*  
  //Start Graphic LCD
  Serial.println("RA8875 start");

  if (!tft.begin(RA8875_800x480)) {
    Serial.println("RA8875 Not Found!");
    while (1);
  }

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);
  tft.fillScreen(0x024B);
  tft.textMode(); //Switch to text - keep it simple for now  
*/  
  //Start TXT LCDs
  lcd0.begin(16, 4);
  lcd1.begin(16, 4);

  //Print a fun message
  lcd0.print("Welcome to Dezldog");
  lcd0.setCursor(0, 1);
  lcd0.print("Start-Up Complete");
  lcd0.setCursor(0, 2);
  lcd0.print("Version 19FEB16.1");
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

  get_dhtData();

/*  
  //Get sensor event 
  sensors_event_t event; 
  mag.getEvent(&event);
  
  float Pi = 3.14159;
  
  // Calculate the angle of the vector y,x
  float heading = (atan2(event.magnetic.y,event.magnetic.x) * 180) / Pi;
  
  // Normalize to 0-360
  if (heading < 0)
    {
      heading = 360 + heading;
    }
*/
   //Write to Graphic LCD
   //displayGraphic();

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
          Serial.print(",,,,,,,,");
        }
      Serial.print(h0); Serial.print(","); 
      Serial.print(t0); Serial.print(","); 
      Serial.print(f0); Serial.print(",");
      Serial.print(hic0); Serial.print(","); 
      Serial.print(hif0); Serial.print(",");
      Serial.print(h1); Serial.print(","); 
      Serial.print(t1); Serial.print(","); 
      Serial.print(f1); Serial.print(",");
      Serial.print(hic1); Serial.print(",");
      Serial.print(hif1);
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
            Serial.print("Voltage: ");Serial.println(volts);
          }
    Serial.print(PROBE_0",Humidity,"); Serial.print(h0); Serial.println("%"); 
    Serial.print("Temp,"); Serial.print(t0); Serial.print("C, "); Serial.print(f0);Serial.println("F");
    Serial.print("Heat Index,"), Serial.print(hic0); Serial.print("C, "); Serial.print(hif0); Serial.println("F");
    Serial.println();
    Serial.print(PROBE_1",Humidity,"); Serial.print(h1); Serial.println("%");
    Serial.print("Temp,"); Serial.print(t1); Serial.print("C, "); Serial.print(f1);Serial.println("F");
    Serial.print("Heat Index,"), Serial.print(hic1); Serial.print("C, "); Serial.print(hif1); Serial.println("F");
    Serial.println();
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
  int displayValue = hours*100 + minutes;

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

void get_dhtData()
  {
    //Process DHT data
    h0 = dht0.readHumidity();
    t0 = dht0.readTemperature();
    f0 = dht0.readTemperature(true);

    h1 = dht1.readHumidity();
    t1 = dht1.readTemperature();
    f1 = dht1.readTemperature(true);


  // Check if any reads failed and exit early (to try again).
    if (isnan(h0) || isnan(t0) || isnan(f0)) {
      Serial.println("Failed to read from DHT0 sensor!");
  }
  if (isnan(h1) || isnan(t1) || isnan(f1)) {
    Serial.println("Failed to read from DHT1 sensor!");
  }

  hif0 = dht0.computeHeatIndex(f0, h0);    // Compute heat index in Fahrenheit (the default)
  hic0 = dht0.computeHeatIndex(t0, h0, false);  // Compute heat index in Celsius (isFahreheit = false)

  hif1 = dht1.computeHeatIndex(f1, h1);
  hic1 = dht1.computeHeatIndex(t1, h1, false);

  }
/*
void displayGraphic()
  {
    //The clock calculation bit
    int hours = GPS.hour + HOUR_OFFSET;
    int minutes = GPS.minute;
    int seconds = GPS.seconds;
    float Longitude = GPS.longitudeDegrees;
    float Latitude = GPS.latitudeDegrees;

    if (hours < 0) 
      {
        hours = 24+hours;
      }
    if (hours > 23) 
      {
        hours = 24-hours;
      }

    if (!TIME_24_HOUR) {
      if (hours > 12) {
        hours -= 12;
        }
      else if (hours == 0) {
        hours += 12;
        }
      }  

    //The display bit
    int months = GPS.month;
    int days = GPS.day;
    int years = GPS.year;
    int satellite = GPS.satellites;
    int fix = GPS.fix;
    char time_buff[32];
    char velo_buff[6];
    char elev_buff[6];
    char loc_buff[32];
    char date_buff[10];
    char sat_buff[3];
    char fix_buff[4];
    
    tft.fillScreen(0x024B); // darker cyan
    tft.setFont(&FreeSans24pt7b);
    tft.textEnlarge(1); 

    tft.textSetCursor(660, 10); 
    tft.textTransparent(0x373B); //funny green
    sprintf( time_buff, "%02d:%02d:%02d", hours, minutes, seconds);
    tft.textWrite( time_buff );

    tft.textSetCursor(10,10);
    tft.textTransparent(0x373B); //funny green
    sprintf( date_buff, "%02d/%02d/%02d", months, days, years);
    tft.textWrite( date_buff );

    tft.textSetCursor(400, 200);
    tft.textTransparent(0xF4E1); //orange
    tft.textEnlarge(6);
    sprintf(velo_buff, "%.1f Mph", velocity);
    tft.textWrite(velo_buff);

    tft.textSetCursor(10, 360);
    tft.textEnlarge(1);
    tft.textTransparent(0xF81F); //magenta
    sprintf(elev_buff, "Altitude %.1f ft", elevation);
    tft.textWrite(elev_buff);

    tft.textSetCursor(240, 420);
    tft.textTransparent(0xF81F); //magenta
    sprintf(sat_buff, "%02d satellites Available", satellite);
    tft.textWrite(sat_buff);

    tft.textSetCursor(10, 420);
    if (fix)  //Sets color for FIX as well...
      {
        tft.textTransparent(RA8875_GREEN);
        sprintf(fix_buff, "GPS FIX", fix);
      }
    else
      {
        tft.textTransparent(RA8875_RED);
        sprintf(fix_buff, "GPS NO FIX", fix);
      }
    tft.textWrite(fix_buff);

    //The Lat and Lon and FIX
    tft.textSetCursor(10, 390);
    tft.textEnlarge(1);
    sprintf( loc_buff,"Lat:%f      Lon:%f", Latitude,Longitude );
    tft.textWrite (loc_buff);
  }
*/

