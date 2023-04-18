#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include <string.h>
#include <Servo.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

static const int servoPin = 17;
Servo servo1;

void setup(void) {
  Serial.begin(9600);
  Serial.println("NDEF Reader");
  nfc.begin();
  servo1.attach(servoPin);
}

void loop(void) {
  Serial.println("\nScan a NFC tag\n");

  if (nfc.tagPresent())
  {
    NfcTag tag = nfc.read();
    if (tag.hasNdefMessage())
    {

      NdefMessage message = tag.getNdefMessage();
      Serial.print("\nThis NFC Tag contains an NDEF Message with ");
      Serial.print(message.getRecordCount());
      Serial.print(" NDEF Record");
      if (message.getRecordCount() != 1) {
        Serial.print("s");
      }
      Serial.println(".");

      int recordCount = message.getRecordCount();
      for (int i = 0; i < recordCount; i++)
      {
        Serial.print("\nNDEF Record "); Serial.println(i + 1);
        NdefRecord record = message.getRecord(i);

        Serial.print("  TNF: "); Serial.println(record.getTnf());
        Serial.print("  Type: "); Serial.println(record.getType());

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
          int CHID;
          CHID = payloadAsString2.toInt();
          Serial.print(CHID);
          Serial.println();
          delay(1000);
          servo1.write(0.0001f);
        } else {
          Serial.println("Not a valid SBIN tag");
        }
        String uid = record.getId();
        if (uid != "") {
          Serial.print("  ID: "); Serial.println(uid);
        }
      }
    }
  }
  delay(3000);
}
