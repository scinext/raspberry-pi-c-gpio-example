﻿
#ifndef SENSOR_H
#define SENSOR_H

typedef struct{
	int	sleepTime;
	int diff;
	int lsb;
}LuxRangeData;


#define SENSOR_LOG_LEVEL_0	0
#define SENSOR_LOG_LEVEL_1	1
#define LOG_DIR					"/var/log/cas/"
int SetSensorLogLevel(int level, int console);
void SensorLogPrintf(int level, const char *str, ...);
void Drain(int pin, int ch);
void GetLuxTest(int loop, unsigned int sleepTime, unsigned int lsb, int type);
float GetLux();
float GetLuxOhm(int ohm);

float GetHumidity();

float GetPress();
float GetTemp();

#endif