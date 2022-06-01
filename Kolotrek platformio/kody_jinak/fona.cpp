#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Battery18650Stats.h>
#include "Adafruit_FONA.h"

//constants
#define FONA_RX 26
#define FONA_TX 25
#define FONA_RST 27
const int SIM_CHECK_INTERVAL = 60;
char message_sms[] = "Hi there, 1."; //buffer to store message
char TARGET_NUMBER[] = "+420724946949"; //phone number to send message


const int GPS_CHECK_INTERVAL = 5; //in seconds

const int BATTERY_CHECK_INTERVAL = 5;
const int p_BATTERY = 13;

//sim800L
const char simPIN[] = "";

//gps NEO-6M
int GPSBaud = 9600;

//Objects
TinyGPSPlus gps;
SoftwareSerial gpsSerial(33, 32);
SoftwareSerial simSerial(FONA_TX, FONA_RX);
Adafruit_FONA gsm = Adafruit_FONA(FONA_RST);
Battery18650Stats battery(p_BATTERY,1.29);

void displayInfo()
{
  if (gps.location.isValid())
  {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());
  }
  else
  {
    Serial.println("Location: Not Available");
  }
  
  Serial.print("Date: ");
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.println(gps.date.year());
  }
  else
  {
    Serial.println("Not Available");
  }

  Serial.print("Time: ");
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(".");
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.println(gps.time.centisecond());
  }
  else
  {
    Serial.println("Not Available");
  }

  Serial.println();
  Serial.println();
}

void setup()
{
  pinMode(p_BATTERY,INPUT);

  // Start the Arduino hardware serial port at 9600 baud
  Serial.begin(9600);

  // Start the software serial port at the GPS's default baud
  gpsSerial.begin(GPSBaud);

  simSerial.begin(9600); 
  if (!gsm.begin(simSerial)) {            
    Serial.println(F("Couldn't find SIM800L!"));
    while (1);
  }
  
  delay(1000);
}

void loop()
{
  //sim check
  if (millis() % (SIM_CHECK_INTERVAL*1000) == 0) {
    //Send the message and display the status
    gsm.sendSMS(TARGET_NUMBER, message_sms);
  }

  //get battery percent
  /*if (millis() % (BATTERY_CHECK_INTERVAL*1000) == 0) {
    Serial.print("Volts: ");
    Serial.println(battery.getBatteryVolts());

    Serial.print("Charge level: ");
    Serial.println(battery.getBatteryChargeLevel());

    Serial.print("Charge level (using the reference table): ");
    Serial.println(battery.getBatteryChargeLevel(true));
  }*/

  //get gps
  /*if (millis() % (GPS_CHECK_INTERVAL*1000) == 0 && gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      displayInfo();
    }
    delay(1);
  }*/

  /*if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected"));
    while(true);
  }*/
}