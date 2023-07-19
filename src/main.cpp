#include <Arduino.h>
#include "mq.h"
#include "dht22.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

void setup() {
    Serial.begin(9600);
    WiFi.begin(SSID, PWD);
    Serial.println("");
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}


void loop() {
    if (WiFi.status() != WL_CONNECTED) return;
    WiFiClientSecure client;
    client.setInsecure(); // TODO: use certificate instead

    HTTPClient https;
    String url = "https://google.com";
    Serial.println("Requesting " + url);
    if (https.begin(client, url)) {
        int httpCode = https.GET();
        Serial.println("============== Response code: " + String(httpCode));
        if (httpCode > 0) {
            Serial.println(https.getString());
        }
        https.end();
    } else {
        Serial.printf("[HTTPS] Unable to connect\n");
    }
    delay(5000);
}

// void setup()
// {
//     Serial.begin(9600);
//     DHT_setup();
// }

// void loop()
// {
//     DHT_Result result = DHT_loop();
//     MQ135_loop(result.temperature, result.humidity);
// }