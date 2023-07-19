#include <Arduino.h>
#include "mq.h"
#include "dht22.h"

void setup()
{
    Serial.begin(9600);
    DHT_setup();
}

void loop()
{
    DHT_Result result = DHT_loop();
    MQ135_loop(result.temperature, result.humidity);
}