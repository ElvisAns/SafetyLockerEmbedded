/**
  @ Bootstrapper for the IoT linking to the app!
  @ By Elvis
  @ You have to install the ArduinoJson library in order to compile this code!
  @ https://arduinojson.org/v6/doc/installation/
*/

#include <Arduino.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#include <WiFiClientSecureBearSSL.h>
// Fingerprint for demo URL, expires on June 2, 2021, needs to be updated well before this date
const uint8_t fingerprint[20] = { 0x40, 0xaf, 0x00, 0x6b, 0xec, 0x90, 0x22, 0x41, 0x8e, 0xa3, 0xad, 0xfa, 0x1a, 0xe8, 0x25, 0x41, 0x1d, 0x1a, 0x54, 0xb3 };


ESP8266WiFiMulti WiFiMulti;

const char* ssid = "perfectoGroup";     //change this with the name of your wifi
const char* password = "perfecto2022";  //change this with the wifi password

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  pinMode(D2, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D0, OUTPUT);

  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D1, OUTPUT);

  digitalWrite(D2, LOW);
  digitalWrite(D4, LOW);
  digitalWrite(D0, HIGH); //power led

  digitalWrite(D5, HIGH);  //relays have logic inverted
  digitalWrite(D6, HIGH);
  digitalWrite(D7, HIGH);
  digitalWrite(D1, HIGH);


  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
}
int current_status_running = -1; // -1 for unkown , 0 running and 1 in maintance
void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;

    Serial.print("[HTTP] begin...\n");
    if (https.begin(client, "https://safetylocker.fly.dev/api/iot/sync/")) {  // HTTP

      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {
          String input = https.getString();
          size_t inputLength;
          //Serial.println(input);
          StaticJsonDocument<400> doc;
          DeserializationError error = deserializeJson(doc, input);

          if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
          }

          for (JsonObject item : doc.as<JsonArray>()) {
            int state = item["state"].as<int>();                        // 1, 1, 1, 1
            String connected_pin = item["connected_pin"].as<String>();  // "D5", "D6", "D7", "D1",Gl

            if (connected_pin == "D5") digitalWrite(D5, !state);
            if (connected_pin == "D6") digitalWrite(D6, !state);
            if (connected_pin == "D7") digitalWrite(D7, !state);
            if (connected_pin == "D1") digitalWrite(D1, !state);
            if (connected_pin == "Gl") current_status_running = state;

            Serial.print(String(!state) + " state written to the line ");
            Serial.println("connected on " + connected_pin);
          }
          if(current_status_running){
            digitalWrite(D2, LOW);  //update the LED state
            digitalWrite(D4, HIGH);
          }
          else{
            digitalWrite(D2, HIGH);  //update the LED state
            digitalWrite(D4, LOW);
          }
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
  if(current_status_running==0){ //in maintance
    for(int i=0;i<4;i++){
      digitalWrite(D2, LOW);  //update the LED state
      digitalWrite(D4, LOW);

      delay(1000);

      digitalWrite(D2, HIGH);  //update the LED state
      digitalWrite(D4, LOW);

      delay(1000);
    }
  }

  if(current_status_running==1){
    digitalWrite(D2, LOW);  //update the LED state
    digitalWrite(D4, HIGH);
    delay(8000); //we will request to update every 8seconds
  }
}
