
#include <stdio.h>
#include <stdlib.h>

//getopt
#include <unistd.h>

#define MAIN_INIT
#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"
#include "../gpio/gpio-i2c.h"
#include "../gpio/gpio-arm-timer.h"
#include "sensor.h"
#include "touchSensor.h"

#define TOUCH_JUDGE 10000	//armtimerが高分解能のため*10されてる, 2本指で反応させる
#define OLD_TOUCH	2*1e+6	//2*1000[us]*100[ms]*10[高分解]
#define ON_TOUCH	1
#define OFF_TOUCH	0

const int touchOutPin = 27;
const int touchInPin  = 22;
unsigned int startCounter, endCounter, diffCounter;
int oldValue;
OnTouchCallback onTouchCallback;


void onTouchCallbackTest(unsigned int cap)
{
	printf("\t\t onTouchCallback Test cap %u\n", cap);
}

void TouchSensorTest()
{
	long i;
	int value, mem;
		
	
	if( !1 ){
		TouchSensorInit();
		
		//レジスタでの計測
		printf("direct ");
		GPIO_GET(touchInPin) == 0 ? printf("Low\n") : printf("High\n");
		
		startCounter = GetArmTimer();
		GPIO_SET(touchOutPin);
		while( GPIO_GET(touchInPin) == 0 )
			GPIO_GET(touchInPin) == 0 ? printf("Low\n") : printf("High\n");
		endCounter = GetArmTimer();
		diffCounter = endCounter - startCounter;
		printf("\t counter diff: %d\n\n", diffCounter);
		
		GPIO_GET(touchInPin) == 0 ? printf("Low\n") : printf("High\n");
		
		//PrintGpioLevStatus();
		//タッチでの計測
		oldValue = GPIO_GET(touchInPin);
		printf("touch ");
		GPIO_GET(touchInPin) == 0 ? printf("Low\n") : printf("High\n");
		for(i=0; i<2e+8; i++)
		{
			value = GPIO_GET(touchInPin);
			if( oldValue != value )
			{
				if( value == 0)
				{
					startCounter = GetArmTimer();
					oldValue = value;
					//printf("\t Low start mesure capacitance to high\n");
				}
				else
				{
					endCounter = GetArmTimer();
					diffCounter = endCounter - startCounter;
					oldValue = value;
					printf("\t High end mesure capacitance   counter diff: %d\n\n", diffCounter);
				}
			}
			
		}
		//出力の停止
		GPIO_CLR(touchOutPin);
		
	}else{
		TouchSensorStart(onTouchCallbackTest);
		
		printf("touch\n");
		GPIO_SET(touchOutPin);
		sleep(50);
		
		//出力の停止
		GPIO_CLR(touchOutPin);
		
		//interruptを終了させる
		GpioInterruptEnd();
	}
	
	//PrintGpioLevStatus();
}


void TouchSensorInit()
{
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "out %d, in %d\n", touchOutPin, touchInPin);
	
	InitPin(touchOutPin, PIN_OUT);
	GPIO_CLR(touchOutPin);
	
	//PULL_NONEじゃないと1Mohm以上は検知できない pull up/down 50～60Kohmらしい
	InitPin(touchInPin, PIN_IN);
	PullUpDown(touchInPin, PULL_DOWN);
	PullUpDown(touchInPin, PULL_NONE);
	//PrintGpioStatus();
	//PrintGpioLevStatus();
}
void TouchSensorStart(OnTouchCallback callback)
{
	TouchSensorInit();
	
	onTouchCallback = callback;
	
	//割り込みでの計測開始
	RegisterInterruptPin(touchInPin, EDGE_TYPE_BOTH);
	RegisterInterruptCallback(TouchSensorInterrupt);
	GpioInterruptStart();
	
	////1500us辺りがデバイスドライバで反応できる限界っぽい 念の為2000us辺りにしとく
	//int sleepDirect = 2000;
	//printf("direct\n");
	//sleep(1);
	//GPIO_CLR(touchOutPin);	DelayMicroSecond(sleepDirect);
	//GPIO_SET(touchOutPin);	DelayMicroSecond(sleepDirect);
	//GPIO_CLR(touchOutPin);	DelayMicroSecond(sleepDirect);
	//GPIO_SET(touchOutPin);	DelayMicroSecond(sleepDirect);
	//printf("second direct\n");
	//sleep(1);
	//GPIO_CLR(touchOutPin);	DelayMicroSecond(sleepDirect);
	//GPIO_SET(touchOutPin);	DelayMicroSecond(sleepDirect);
	//GPIO_CLR(touchOutPin);	DelayMicroSecond(sleepDirect);
	//GPIO_SET(touchOutPin);	DelayMicroSecond(sleepDirect);
	
	//電源ON
	GPIO_SET(touchOutPin);
	//DelayMicroSecond(sleepDirect);
	//GPIO_CLR(touchOutPin);
	//DelayMicroSecond(sleepDirect);
	//GPIO_SET(touchOutPin);
}
void TouchSensorEnd()
{
	//出力の停止
	GPIO_CLR(touchOutPin);
	
	//interruptを終了させる
	GpioInterruptEnd();
}

int TouchSensorInterrupt(int pin, int value)
{
	static unsigned int oldCounter, onTouchDiffCounter;
	static int oldTouch, onTouch;
	if( pin == touchInPin )
	{	
		//value = GPIO_GET(touchInPin);
		if( oldValue == value || onTouchCallback == NULL )
			return INTERRUPT_CONTINUE;
		
		oldValue = value;
		
		if( value == LOW )
		{
			startCounter = GetArmTimer();
			//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "t Low start mesure capacitance to high\n");
		}
		else if( startCounter != 0 )
		{
			//電源オン後一回目はstartCounterが0のため数字がおかしくなるのでskip
			
			endCounter = GetArmTimer();
			
			diffCounter = endCounter - startCounter;
			//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t High end mesure capacitance   counter diff: %d\n", diffCounter);
			
			//人体の静電容量600pF～100pFのあるなしの充電時間の違いから計算する
			//接触中は充電(Low)時間が長いので規定以上で接触と判断
			onTouch = diffCounter>TOUCH_JUDGE ? ON_TOUCH : OFF_TOUCH;
						
			//以前と違う接触状態なら続行
			if( onTouch != oldTouch )
			{
				//接触状態の更新
				oldTouch = onTouch;
				onTouch == ON_TOUCH ? 
					SensorLogPrintf(SENSOR_LOG_LEVEL_1, "switch on [touch]  %d \n", diffCounter) :
					SensorLogPrintf(SENSOR_LOG_LEVEL_1, "switch off [not touch]  %d \n", diffCounter);
				
				//離れた場合にモードを変える
				if( onTouch == OFF_TOUCH )
				{
					//ごく短い間に静電容量の変化でon/offを繰り返すことがあるので極短時間のスイッチ動作は弾く
					SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t old diff %u\n", endCounter - oldCounter);
					if( oldCounter + OLD_TOUCH < endCounter )
					{
						//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\t Mode Change\n");
						onTouchCallback(onTouchDiffCounter);
					}
					oldCounter = endCounter;
				}
				else
				{
					//タッチ時のcap時間の初期化
					onTouchDiffCounter = 0;
				}
			}
			//タッチの場合 最大cap時間の更新
			//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t counter diff: %u, old diff: %u\n", diffCounter, onTouchDiffCounter);
			if( onTouch == ON_TOUCH && onTouchDiffCounter < diffCounter )
				onTouchDiffCounter = diffCounter;
		}
	}
	return INTERRUPT_CONTINUE;
}
