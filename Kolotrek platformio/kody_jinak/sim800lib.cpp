#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Battery18650Stats.h>
#include <Sim800L.h>

//constants
const int SIM_CHECK_INTERVAL = 60;
char text[] = "Hi there, I sent this message from an esp32 with an sim800L. Kolotrek is gonna be epic."; //buffer to store message
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
SoftwareSerial simSerial(25, 26);
Sim800L GSM(25, 26);
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
  // Start the Arduino hardware serial port at 9600 baud
  Serial.begin(9600);

  // Start the software serial port at the GPS's default baud
  gpsSerial.begin(GPSBaud);

  GSM.begin(4800);
  
  delay(5000);

  pinMode(p_BATTERY,INPUT);
}

void loop()
{
  //sim check
  if (millis() % (SIM_CHECK_INTERVAL*1000) == 0) {
    //Send the message and display the status
    bool error = GSM.sendSms(TARGET_NUMBER,text);
    if (error) Serial.println("Error Sending Message");
    else Serial.println("Message Sent Successfully!");
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