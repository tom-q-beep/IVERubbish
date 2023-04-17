//last modified 00:17 17/4/2023

#include <RadioLib.h>
#include <ArduinoJson.h>

StaticJsonBuffer<255> Transdata;
JsonObject& root  = Transdata.createObject();

#define TRIG_PIN 21
#define ECHO_PIN 22
#define PWRPIN 13

char jsonChar[255];
int resvalue;
int batadc;
uint32_t chipId = 0;
bool txflag;
long duration;
float distance;
float voltage;
float percent;

int binheight = 100;

// NSS pin:   5
// DIO0 pin:  2
// RESET pin: 12
// DIO1 pin:  4
SX1278 radio = new Module(5, 2, 12, 4);

void setup() {
  Serial.begin(9600);
  pinMode(PWRPIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  ChipID();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 1);
  Serial.print("Device ID: ");
  Serial.print(chipId);
  Serial.println();
  Serial.print(F("SX1278 Initializing......"));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
    Serial.println("Self test OK! Configurating...");
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
    Serial.println("Ready");
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    ESP.restart();
  }
}

void loop() {
  bool lid;
  lid = digitalRead(35);
  if (lid == 0) {
    Serial.println("Lid closed");
    measure();
    battcal();
    root["GBINID"] = chipId;
    root["BAT"] = voltage;
    root["DISTANCE1"] = distance;
    root["FULLPERC"] = percent;
    root.printTo((char*)jsonChar, root.measureLength() + 1);

    delay(1200); // do not remove to ensure transmission integrity

    if (txflag == false) {
      Serial.print(F("Transmitting ... "));
      transmission();
      Serial.println("I sleep");
      delay(500);
      esp_deep_sleep_start();
    }
  }
}

void ChipID() {
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
}

void transmission() {
  int state = radio.transmit(jsonChar);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F(" success!"));

    // print measured data rate
    Serial.print("At datarate:");
    Serial.print(radio.getDataRate());
    Serial.print(" bps");
    Serial.println();
  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));
    txflag = false;
  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occurred while transmitting packet
    Serial.println(F("timeout!"));
    txflag = false;
  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
    txflag = false;
  }
  txflag = true;
}

void measure() {
  digitalWrite(PWRPIN, HIGH);
  delay(2500);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.0343 / 2;
  percent = (distance / binheight)*100;
  delay(500);
  digitalWrite(PWRPIN, LOW);
}

void battcal() {
  batadc = analogRead(32);
  voltage = batadc * 0.00123; //calibrated ADC value, i guess. ESP32 adc is a joke.
}
