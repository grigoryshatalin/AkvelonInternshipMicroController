#include <Arduino.h>
#include "mq.h"
#include "dht22.h"
#include "azure.h"

void setup()
{
    Serial.begin(9600);
    azure_setup();
    DHT_setup();
}

void loop()
{
    DHT_Result dhtResult = DHT_loop();
    MQ135_Result mqResult = MQ135_loop(dhtResult.temperature, dhtResult.humidity);
    String dataString = "\"temperature\": " + String(dhtResult.temperature) + ", \"humidity\": " + String(dhtResult.humidity) + ", \"rzero\": " + String(mqResult.rzero) + ", \"correctedRZero\": " + String(mqResult.correctedRZero) + ", \"resistance\": " + String(mqResult.resistance) + ", \"ppm\": " + String(mqResult.ppm) + ", \"correctedPPM\": " + String(mqResult.correctedPPM);
    char* char_array = new char[dataString.length() + 1]; // +1 for null terminator
    strcpy(char_array, dataString.c_str());
    azure_loop(char_array);
    delay(500);
}