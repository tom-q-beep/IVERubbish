//Final version, with battery status. 30/4/2023
#include <RadioLib.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

//wifi cred.
#define WIFI_SSID "Ko's Home"
#define WIFI_PASSWORD "soviett90blyat"

//database cred.
#define API_KEY "AIzaSyA8YLTyR0jw45x5RIwhQak_W08tzc-6Bvw"
#define USER_EMAIL "admin1@foodwasteapp.com"
#define USER_PASSWORD "12345678"
#define DATABASE_URL "https://smart-foodwaste-system-default-rtdb.firebaseio.com"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

FirebaseJson json;

String uid;
String databasePath;
String parentPath;
String cipid;
String BATTERYSTAT;
const long utcOffsetInSeconds = 28800;

int ChipID;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// NSS pin:   15
// DIO0 pin:  5
// RESET pin: 4
// DIO1 pin:  16
SX1278 radio = new Module(15, 5, 4, 16);

void setup() {
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
    Serial.println("Performing Configuration");
    radio.setFrequency(433.5);
    radio.setBandwidth(500.0);
    radio.setSpreadingFactor(12);
    radio.setCodingRate(6);
    radio.setOutputPower(17);
    radio.setCurrentLimit(0);
    radio.setGain(1);
    radio.setPreambleLength(8);
    radio.setSyncWord(0xF8);
    radio.setCRC(true);
    pinMode(2, OUTPUT);
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
  initWiFi();
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  databasePath = "/UsersData/" + uid + "/Reading";
  parentPath = databasePath + "/" + ChipID;
  timeClient.begin();
}


void loop() {
  Serial.print(F("[SX1278] Waiting for incoming transmission ... "));
  String str;
  int state = radio.receive(str);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print(F("[SX1278] Data:\t\t\t"));
    Serial.println(str);

    // print the RSSI (Received Signal Strength Indicator)
    // of the last received packet
    Serial.print(F("[SX1278] RSSI:\t\t\t"));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    // print the SNR (Signal-to-Noise Ratio)
    // of the last received packet
    Serial.print(F("[SX1278] SNR:\t\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));

    // print frequency error
    // of the last received packet
    Serial.print(F("[SX1278] Frequency error:\t"));
    Serial.print(radio.getFrequencyError());
    Serial.println(F(" Hz"));

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.println(F("timeout!"));

  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println(F("CRC error!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
  DynamicJsonDocument doc(512);
  deserializeJson(doc, str);
  JsonObject obj = doc.as<JsonObject>();
  ChipID = obj[String("GBINID")];
  cipid = String(ChipID);
  float BATTERY = obj[String("BAT")];
  float DISTANCE = obj[String("DISTANCE1")];
  float PERCENTAGE = obj[String("FULLPERC")];
  bool LIGHT = obj[String("LIGHT")];
  Serial.println(ChipID);
  Serial.println(BATTERY);
  if (BATTERY >= 3.8) {
    BATTERYSTAT = "HIGH";
  } else if (BATTERY < 3.8 && BATTERY >= 3.61) {
    BATTERYSTAT = "MEDIUM";
  } else if (BATTERY <= 3.6) {
    BATTERYSTAT = "LOW";
  }
  Serial.println(DISTANCE);
  Serial.println(PERCENTAGE);
  if (ChipID > 0) {
    timeClient.update();
    parentPath = databasePath + "/" + ChipID;
    json.set("DEVICEID", String(ChipID));
    json.set("BATTERYVOLT", String(BATTERY));
    json.set("BATTERYSTAT", String(BATTERYSTAT));
    json.set("DISTANCE", String(DISTANCE));
    json.set("PERCENTAGE", String(PERCENTAGE));
    json.set("LIGHT", String(LIGHT));
    json.set("RSSI", String(radio.getRSSI()));
    json.set("TIMESTAMP", String(timeClient.getFormattedTime()));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    digitalWrite(2, HIGH);
    delay(1000);
    digitalWrite(2, LOW);
  }
  WiFi.setSleep(false);
  Serial.println(WiFi.localIP());
  Serial.println();
}