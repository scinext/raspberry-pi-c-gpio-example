
#include <stdio.h>
#include <stdlib.h>

//getopt
#include <unistd.h>

#define MAIN_INIT
#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"
#include "../gpio/gpio-i2c.h"
#include "../gpio/gpio-arm-timer.h"
#include "main.h"
#include "adConvert.h"
#include "sensor.h"

#define MODE_TEMP		0x01
#define MODE_PRESS		0x02
#define MODE_HUMIDITY	0x03
#define MODE_LUX		0x04
#define MODE_LUX_OHM	0x05
#define MODE_ALL		0xFFFF


//湿度の温度補正用のtemp用
float	g_temp;

int		g_outLevel;

int main(int argc, char *argv[])
{
	int i, sleepTime;

	//コマンドラインを解析
	int opt;
	extern char *optarg;

	int ohm, mode;
	float ret;

	if(argc <= 1)
		return -1;

	/*
	 *  -D デバッグ出力
	 *  -t 温度	
	 *  -p 大気圧
	 *  -h 湿度
	 *  -l 照度
	 *  -L XX 照度(抵抗器使用) XXXXohm
	 */
	while( (opt = getopt(argc, argv, "DtphlL:")) != -1 )
	{
		switch( opt )
		{
			case 'D':
				SetSensorLogLevel(1, 1);
				break;
			case 't':
				mode = MODE_TEMP;
				break;
			case 'p':
				mode = MODE_PRESS;
				break;
			case 'h':
				mode = MODE_HUMIDITY;
				break;
			case 'l':
				mode = MODE_LUX;
				break;
			case 'L':
				mode = MODE_LUX_OHM;
				ohm = atoi(optarg);
				break;
			default:
				break;
		}
	}

	InitGpio();
	InitAD();
	InitLps331();
	//高分解能(0.1ms)
	InitArmTimer(1);
	
	//スレッドを高優先に
	SetPriority(HIGH_PRIO);
		
	ret = -1;
	switch( mode )
	{
		case MODE_TEMP:
			ret = GetTemp();
			g_temp = ret;
			break;
		case MODE_PRESS:
			ret = GetPress();
			break;
		case MODE_HUMIDITY:
			ret = GetHumidity();
			break;
		case MODE_LUX:
			ret = GetLux();
			break;
		case MODE_LUX_OHM:
			if( ohm >= 10 )
				ret = GetLuxOhm(ohm);
			else
				fprintf(stderr, "use %d ohm?\n", ohm);
			break;
		default:
			printf("mode select t-Temp, p-Press, h-Humidity, l-Lux(condenser) L-(ohm)\n");
			break;
	}

	SetPriority(NOMAL_PRIO);
	
	printf("ret = %f\n", ret);
	
	UnInitArmTimer();
	UnInitLps331();
	UnInitAD();


	return ret;
}
