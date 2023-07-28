#pragma once

#include "MQ135.h"

// MQ135 gas sensor
//
// Datasheet can be found here: https://www.olimex.com/Products/Components/Sensors/SNS-MQ135/resources/SNS-MQ135.pdf
//
// Application
// They are used in air quality control equipments for buildings/offices, are suitable for detecting of NH3, NOx, alcohol, Benzene, smoke, CO2, etc
//
// Original creator of MQ135.h library: https://github.com/GeorgK/MQ135

#define PIN_MQ135 A0

struct MQ135_Result
{
    float rzero;
    float correctedRZero;
    float resistance;
    float ppm;
    float correctedPPM;
};

MQ135_Result MQ135_loop(float temperature, float humidity);