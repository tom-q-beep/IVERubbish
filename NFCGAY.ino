#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <string.h>
#include <ESP32Servo.h>

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID ""your WIFI here"
#define WIFI_PASSWORD "your WIFI Pass here"
#define API_KEY "AIzaSyA8YLTyR0jw45x5RIwhQak_W08tzc-6Bvw"
#define USER_EMAIL "admin1@foodwasteapp.com"
#define USER_PASSWORD "12345678"
#define DATABASE_URL "https://smart-foodwaste-system-default-rtdb.firebaseio.com"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

static const int servoPin = 27;
static const int trigPin = 32;
static const int echoPin = 34;
static const int LEDG = 25;
static const int LEDR = 33;

long duration;
float cm;
float recordx;
float diff;
bool fail;
int fbValue;
int pts;



String uid;
String databasePath;
String parentPath;
Servo sg90;

void setup() {
  Serial.begin(9600);
  initWiFi();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDR, OUTPUT);

  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDR, HIGH);
  nfc.begin();
  sg90.setPeriodHertz(50);
  sg90.attach(servoPin, 500, 2400);
  delay(2000);
  measure();

  digitalWrite(LEDG, LOW);
  digitalWrite(LEDR, LOW);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/RecyclerScore";
}

void loop() {
  digitalWrite(LEDR, LOW);
  digitalWrite(LEDR, LOW);
  Serial.println("\nNFC tag awaiting\n");
  sg90.write(75);
  if (nfc.tagPresent()) {
    NfcTag tag = nfc.read();
    if (tag.hasNdefMessage()) {

      NdefMessage message = tag.getNdefMessage();
      Serial.print("\nThis NFC Tag contains an NDEF Message with ");
      Serial.print(message.getRecordCount());
      Serial.print(" NDEF Record");
      if (message.getRecordCount() != 1) {
        Serial.print("s");
      }
      Serial.println(".");

      int recordCount = message.getRecordCount();
      for (int i = 0; i < recordCount; i++) {
        Serial.print("\nNDEF Record ");
        Serial.println(i + 1);
        NdefRecord record = message.getRecord(i);

        Serial.print("  TNF: ");
        Serial.println(record.getTnf());
        Serial.print("  Type: ");
        Serial.println(record.getType());

        int payloadLength = record.getPayloadLength();
        byte payload[payloadLength];
        record.getPayload(payload);
        String payloadAsString = "";
        String payloadAsString2 = "";
        for (int c = 3; c < payloadLength; c++) {
          payloadAsString += (char)payload[c];
        }
        for (int j = 3; j < 9; j++) {
          payloadAsString2 += (char)payload[j];
        }
        Serial.print("  Payload (as String): ");
        Serial.println(payloadAsString);
        Serial.println(payloadAsString2);
        if (payloadAsString.indexOf("SBINDEV") > 0) {
          pts = 0;
          digitalWrite(LEDG, HIGH);

          //obtain loyalty ID
          int CHID;
          CHID = payloadAsString2.toInt();
          Serial.print(CHID);
          Serial.println();
          sg90.write(0);
          delay(5000);
          sg90.write(75);  //close door
          digitalWrite(LEDR, HIGH);
          digitalWrite(LEDG, LOW);
          delay(5000);
          measure();
          parentPath = databasePath + "/" + CHID;
          // Point calculation
          if (Firebase.RTDB.getInt(&fbdo, databasePath + "/" + CHID + "/" + "SCORE")) {
            fbValue = fbdo.intData();
            Serial.println(fbValue);
          }
          if (diff > 0) {
            pts = fbValue + diff;
          } else if (diff < 0) {
            pts = fbValue + 1;
          } else {
            json.set("SCORE", 1);
          }
          Serial.println(pts);
          json.set("SCORE", pts);
          Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

          //end of calculation
          digitalWrite(LEDG, HIGH);
          digitalWrite(LEDG, LOW);
        } else {
          Serial.println("Not a valid SBIN tag");
          digitalWrite(LEDR, HIGH);
        }
        String uid = record.getId();
        if (uid != "") {
          Serial.print("  ID: ");
          Serial.println(uid);
        }
      }
    }
  }
}

void measure() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  cm = duration * 0.0343 / 2;
  Serial.print("CM=");
  Serial.print(cm);
  Serial.println();
  Serial.print("record");
  Serial.print(recordx);
  Serial.println();
  diff = recordx - cm;
  Serial.print("diff");
  Serial.print(diff);
  Serial.println();
  recordx = cm;
}

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}
