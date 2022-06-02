#include <Arduino.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Battery18650Stats.h>
#include "Adafruit_FONA.h"

//constants
const bool DEBUG = false;

#define FONA_RX 26
#define FONA_TX 25
#define FONA_RST 23
const int SIM_CHECK_INTERVAL = 60;
char message_sms[] = "Hi there, 1."; //buffer to store message
char TARGET_NUMBER[] = "+420724946949"; //phone number to send message
char fonaNotificationBuffer[64];          //for notifications from the FONA
char smsBuffer[250];
bool new_message = false;


const int GPS_CHECK_INTERVAL = 10; //in seconds

const int BATTERY_CHECK_INTERVAL = 10;
const int p_BATTERY = 13;

//battery
bool byl_pod_10 = false;
bool byl_pod_5 = false;

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

//globals
bool parking_mode = false;
bool location_available = false;
bool location_loaded_once = false;
double location[3] = {0.0,0.0,0.0}; //latitude,longitude,altitude
String location_time = "";

void sim_sleep(bool sleep) {
  /*if (sleep) {
    Serial.println("sim sleep");
    simSerial.println("AT+CSCLK=2");
  } else {
    Serial.println("sim not sleep");
    simSerial.println("AT");
    simSerial.println("AT+CSCLK=0");
  }
  delay(10000);*/
}

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
  sim_sleep(false);

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
  sim_sleep(true);
}

void read_sms() {
  char* bufPtr = fonaNotificationBuffer;    //handy buffer pointer
  if (gsm.available())      //any data available from the FONA?
  {
    int slot = 0;            //this will be the slot number of the SMS
    int charCount = 0;
    //Read the notification into fonaInBuffer
    do  {
      *bufPtr = gsm.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (gsm.available()) && (++charCount < (sizeof(fonaNotificationBuffer)-1)));
    
    //Add a terminal NULL to the notification string
    *bufPtr = 0;

    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)) {
      Serial.print("slot: "); Serial.println(slot);
      
      char TARGET_NUMBER[32];  //we'll store the SMS sender number in here
      
      // Retrieve SMS sender address/phone number.
      if (! gsm.getSMSSender(slot, TARGET_NUMBER, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(TARGET_NUMBER);

        // Retrieve SMS value.
        uint16_t smslen;
        if (gsm.readSMS(slot, smsBuffer, 250, &smslen)) { // pass in buffer and max len!
          Serial.println(smsBuffer);
        }

      //Send back an automatic response
      /*Serial.println("Sending reponse...");
      if (!gsm.sendSMS(TARGET_NUMBER, "Hey, I got your text!")) {
        Serial.println(F("Failed"));
      } else {
        Serial.println(F("Sent!"));
      }*/
      
      // delete the original msg after it is processed
      //   otherwise, we will fill up all the slots
      //   and then we won't be able to receive SMS anymore
      if (gsm.deleteSMS(slot)) {
        Serial.println(F("OK!"));
      } else {
        Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
        gsm.print(F("AT+CMGD=?\r\n"));
      }

      new_message = true;
    }
  }
}

void setup()
{
  pinMode(p_BATTERY,INPUT);

  // Start the Arduino hardware serial port at 9600 baud
  Serial.begin(9600);

  delay(5000);

  // Start the software serial port at the GPS's default baud
  gpsSerial.begin(GPSBaud);

  simSerial.begin(9600); 
  if (!gsm.begin(simSerial)) {            
    Serial.println(F("Couldn't find SIM800L!"));
    while (1);
  }

  simSerial.print("AT+CNMI=2,1\r\n");
  
  //sleep sim
  sim_sleep(true);

  //remove all messages
  for (int i = 0; i < 20; i++) {
    if (gsm.deleteSMS(i)) {
        Serial.println(F("OK!"));
      } else {
        Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(i);
        gsm.print(F("AT+CMGD=?\r\n"));
      }
  }

  Serial.println("done setup");
}

void loop()
{
  unsigned int milis_this_loop = millis();

  //debugh
  if (DEBUG) {
    updateSerial();
  }

  //receive sms
  read_sms();

  //response
  if (new_message) {
    Serial.println("got new message");
    String sms_buffer = smsBuffer;
    if (sms_buffer.equals("status")) {
      Serial.println("sending status");
      send_status();
    } else if (sms_buffer.equals("help")) {
      Serial.println("sending help");
      bool error = gsm.sendSMS(TARGET_NUMBER, "status\nparking mode on/off\nhelp");
      if (error) Serial.println("error sending");
      else Serial.println("sent");
    } else {
      Serial.println("sending help");
      bool error = gsm.sendSMS(TARGET_NUMBER, "didnt get that\n\nuse:\nstatus\nparking mode on/off\nhelp");
      if (error) Serial.println("error sending");
      else Serial.println("sent");
    }
    new_message = false;
  }


  //status
  /*if (milis_this_loop % (SIM_CHECK_INTERVAL*1000) == 0) {
    //Send the message and display the status
    Serial.println("sending status");
    send_status();
    delay(1);
  }*/

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