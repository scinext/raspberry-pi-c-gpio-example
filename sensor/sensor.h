
#ifndef SENSOR_H
#define SENSOR_H

//sensorのみの場合とcasで分けるためにわざとここに記述
#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"
#include "../gpio/gpio-i2c.h"
#include "../gpio/gpio-arm-timer.h"
#include "./adConvert.h"

//取得するi2cのアドレス 0x5d->LPS331
#define I2C_ADDR_LPS331 0x5d
//i2cのアドレスを7bitモードと10bitモードのどちらにするか 1->10bit 0->7bit
#define I2C_ADDR_BIT	0

//LPS331の分解能 データシートより
#define LPS331_PRESS_RES			4096	
#define LPS331_TEMP_RES				480
//LPS331の温度のオフセット
#define LPS331_TEMP_OFFSET			42.5

//i2cにデータを送るときのread write指定
#define I2C_READ					1
#define I2C_WRITE					0
#define LPS331_MULTI_DATA(addr)		(addr | 0x80)
#define LPS331_SINGLE_DATA(addr)	(addr)

#define LPS331_WHO_AM_I				0x0F
#define LPS331_PRESS				0x28
#define LPS331_TEMP					0x2B
#define LPS331_ACTIVE				0x20
	#define LPS331_ACTIVE_ON			0x90
	#define LPS331_ACTIVE_OFF			0x00

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
void Drain(int pin);
void GetLuxTest(int loop, unsigned int sleepTime, unsigned int lsb, int type);
float GetLux();
float GetLuxOhm(int ohm);

float GetHumidity();

int InitLps331();
int UnInitLps331();
float GetPress();
float GetTemp();

#endif
