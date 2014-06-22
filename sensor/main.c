
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
#include "lps331.h"
#include "touchSensor.h"

typedef enum {
	MODE_TEMP		= 0x01,
	MODE_PRESS		,
	MODE_HUMIDITY	,
	MODE_LUX		,
	MODE_LUX_OHM	,
	MODE_TEST		,
	MODE_ALL		= 0xFFFF
}Mode;

//基本的に個々のグローバル変数はダミー

//湿度の温度補正用のtemp用
float	g_temp;

int		g_outLevel;

#define MODE_ADC	0xFF
int main(int argc, char *argv[])
{
	int i, sleepTime;

	//コマンドラインを解析
	int opt;
	extern char *optarg;

	int ohm, mode, ch;
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
	 *	-i XX ADC XXch
	 *	-b テストプログラム
	 */
	while( (opt = getopt(argc, argv, "DtphlL:i:b")) != -1 )
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
			case 'i':
				mode = MODE_ADC;
				ch = atoi(optarg);
				break;
			case 'b':
				mode = MODE_TEST;
				break;
			default:
				break;
		}
	}

	InitGpio();
	InitAD();
	InitLps331();
	//高分解能(0.1ms)
	InitArmTimer(ARM_TIMER_HI_RES);
	
	//スレッドを高優先に
	SetPriority(HIGH_PRIO);
		
	ret = -1;
	switch( mode )
	{
		case MODE_TEMP:
			WakeUpLps331();
			ret = GetTemp();
			g_temp = ret;
			break;
		case MODE_PRESS:
			WakeUpLps331();
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
		case MODE_ADC:
			if( ch <= 1 )
				ret = GetAD(ch);
			else
				fprintf(stderr, "ADC %d ch?\n", ch);
			break;
		case MODE_TEST:
			TestProgram();
			break;
		default:
			printf("mode select t-Temp, p-Press, h-Humidity, l-Lux(condenser) L-(ohm) i-(ADC ch) DtphlL:i:\n");
			break;
	}

	SetPriority(NOMAL_PRIO);
	
	printf("ret = %f\n", ret);
	
	UnInitArmTimer();
	UnInitLps331();
	UnInitAD();


	return ret;
}

void TestProgram()
{
	//*touchsensor
		TouchSensorTest();
		//TouchSensorStart();
		//sleep(100);
		//TouchSensorEnd();
	//*/
}

