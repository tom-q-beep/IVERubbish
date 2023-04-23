here you go, the IVE rubbish bin project
active until 1st may

*im not responsible for any data loss / library change 
+
i dont have 48 hours a day

NFCGAY => the rubbish bin
Tx => ESP32 transmitter node in the bin
Rx => ESP8266 LoRa gateway node

LoRa transmission settings are available as below:
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
