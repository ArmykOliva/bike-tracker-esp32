#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Battery18650Stats.h>
#include "Adafruit_FONA.h"
#include <BMI160Gen.h>

//constants
const bool DEBUG = false;

//bmi160


//sim
#define FONA_RX 26
#define FONA_TX 25
#define FONA_RST 23
const int SMS_CHECK_INTERVAL = 5;
char TARGET_NUMBER[25] = "+420724946949"; //phone number to send message
bool new_message = false;
char smsBuffer[250];

//gps
const int GPS_CHECK_INTERVAL = 10; //in seconds
int GPSBaud = 9600;

//battery
bool byl_pod_10 = false;
bool byl_pod_5 = false;
const int BATTERY_CHECK_INTERVAL = 10;
const int p_BATTERY = 13;

//Objects
TinyGPSPlus gps;
SoftwareSerial gpsSerial(33, 32);
SoftwareSerial simSerial(FONA_TX, FONA_RX);
Adafruit_FONA gsm = Adafruit_FONA(FONA_RST);
Battery18650Stats battery(p_BATTERY,1.29);

//globals
bool parking_mode = false;
bool location_available = false;
bool location_loaded_once = false;
double location[3] = {0.0,0.0,0.0}; //latitude,longitude,altitude
String location_time = "";

void updateSerial()
{
  delay(500);
  while (Serial.available()) 
  {
    simSerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(simSerial.available()) 
  {
    Serial.write(simSerial.read());//Forward what Software Serial received to Serial Port
  }
}

void gps_info()
{
  if (gps.location.isValid())
  {
    location_available = true;
    location_loaded_once = true;

    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    location[0] = gps.location.lat();

    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    location[1] = gps.location.lng();

    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());
    location[2] = gps.altitude.meters();
  } else {
    location_available = false;
    Serial.println("Location: Not Available");
  }


  Serial.print("Time: ");
  if (gps.time.isValid()) {
    location_time = String(gps.time.hour() + 2) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
    Serial.println(location_time);
  }

  Serial.println();
}

void send_status() {

  String message = "Status of Kolotrek:\n\n";

  //battery
  message += "Battery: ";
  message += String(battery.getBatteryChargeLevel());
  message += "% \n";

  //gps
  if (location_loaded_once) {
    if (!location_available) message += "\nLocation unavailable, showing older location:\n\n";
    else message += "\nLocation available:\n\n";
    message += "Latitude: " + String(location[0],6);
    message += "\nLongitude: " + String(location[1],6);
    message += "\nTime: " + location_time;
  } else {
    message += "Location not yet available. Gps didn't get a clear view of sky.";
  }
  message += "\n";

  //parking
  message += "Parking mode: ";
  message += parking_mode ? "On" : "Off";
  message += "\n";

  //send
  Serial.println(message);
  char message_char[160];
  message.toCharArray(message_char, 160);
  bool error = gsm.sendSMS(TARGET_NUMBER, message_char);
  if (error) Serial.println("error sending");
  else Serial.println("sent");

}

void read_latest_sms(){
  uint16_t smslen;
  unsigned int message_index = 1;

  //get latest index
  while (gsm.getSMSSender(message_index, smsBuffer, 250)) {
    message_index ++;
  }
  message_index --;

  //get sender
  if (! gsm.getSMSSender(message_index, smsBuffer, 250)) {
    Serial.println("Failed getting sms sender!");
    return;
  }
  Serial.print(F("FROM: ")); 
  Serial.println(smsBuffer);
  strcpy(smsBuffer,TARGET_NUMBER);
  
  //read sms
  Serial.print(F("\n\rReading SMS #")); Serial.println(message_index);
  if (!gsm.readSMS(message_index, smsBuffer, 250, &smslen)) {  // pass in buffer and max len!
    Serial.println(F("Failed reading sms!"));
    return;
  }
  Serial.println(smsBuffer);
  new_message = true;

  //delete all sms
  for (int i = 0; i < 5; i++) {
    gsm.deleteSMS(i);
  }
}

void send_sms(char *message_char) {
  Serial.println(message_char);
  bool error = gsm.sendSMS(TARGET_NUMBER, message_char);
  if (error) Serial.println("error sending");
  else Serial.println("sent");
}

void callNumber(char * number){
   if (!gsm.callPhone(number)) {
       Serial.println(F("Failed calling"));
   } else {
       Serial.println(F("Calling number!"));
   }
}

void setup()
{
  //setup
  pinMode(p_BATTERY,INPUT);
  Serial.begin(9600);
  //BMI160.begin(BMI160GenClass::I2C_MODE, bmi160_i2c_addr);
  delay(5000);

  // Gps
  gpsSerial.begin(GPSBaud);

  //sim
  simSerial.begin(9600); 
  if (!gsm.begin(simSerial)) {            
    Serial.println(F("Couldn't find SIM800L!"));
    while (1);
  }
  
  //end
  Serial.println("done setup");
}

void loop()
{
  unsigned int milis_this_loop = millis();
  //receive sms
  if (milis_this_loop % (SMS_CHECK_INTERVAL*1000) == 0) {
    read_latest_sms();
  }

  //response
  if (new_message) {
    Serial.println("got new message");
    String sms_buffer = smsBuffer;
    sms_buffer.toLowerCase();
    if (sms_buffer.equals("status")) {
      Serial.println("sending status");
      send_status();
    } else if (sms_buffer.equals("parking on")) {
      parking_mode = true;
      send_sms("Parking on");
    } else if (sms_buffer.equals("parking off")) {
      parking_mode = false;
      send_sms("Parking off");
    } else if (sms_buffer.equals("help")) {
      Serial.println("sending help");
      send_sms("status - displays stats of Kolotrek (Gps, battery, etc..)\nparking on/off - turns on/off parking mode\nhelp - displays all commands");
    } else {
      Serial.println("sending help");
      send_sms("This command does not exist.\n\nuse:\nhelp\nstatus\nparking on/off");
    }
    new_message = false;
  }

  //get battery percent
  if (milis_this_loop % (BATTERY_CHECK_INTERVAL*1000) == 0) {
    Serial.println("Battery status check");
    if (battery.getBatteryChargeLevel() >= 20) {
      byl_pod_10 = false;
      byl_pod_5 = false;
    } else if (battery.getBatteryChargeLevel() <= 10 && !byl_pod_10) {
      send_status();
      byl_pod_10 = true;
    } else if (battery.getBatteryChargeLevel() <= 5 && !byl_pod_5) {
      send_status();
      byl_pod_5 = true;
    }
    delay(1);
  }

  //get gps
  if (milis_this_loop % (GPS_CHECK_INTERVAL*1000) == 0) {
    Serial.println("Gps check");
    while (gpsSerial.available()) {
      if (gps.encode(gpsSerial.read())) { 
        gps_info();
      }
    }
    delay(1);
  }
}