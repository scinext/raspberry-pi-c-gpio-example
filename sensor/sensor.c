
#include <stdio.h>
//clock_gettime gccで -lrtをつけないとコンパイルエラー
#include <time.h>

#include <stdint.h>

//抵抗値を求めるためにlog(Lnが必要)試算用にexpも使う
#include <math.h>


#include "main.h"
#include "sensor.h"


extern int		g_outLevel;

//湿度の温度補正用のtemp用
extern int		g_temp;

void Drain(int pin)
{
	//ドレインピンを使用して放電
	InitPin(pin, PIN_OUT);
	GPIO_CLR(pin);
	//200ms
	DelayMicroSecond(200000);
	InitPin(pin, PIN_IN);
	
	//20ms
	//DelayMicroSecond(20000);
}
int GetLux()
{
	const unsigned int pin = 25;
	const unsigned int drainPin = 23;
	const unsigned int ch  = 1;
	float firstAD, secondAD, avgAD, diffAD, lux, a;
	int firstADC, secondADC, avgADC;
	int i, j;
	int quantity = 5;
	
	int range[] = {
		20, 50, 100, 500,				//us
		1000, 10000, 100000, 250000,	//ms
		1000000							//s
	};

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "  Lux use condensare\n");

	InitPin(pin, PIN_OUT);
	//放電
	Drain(drainPin);
	
	diffAD	= 10.0;
	for(i=0; i<ARRAY_SIZE(range); i++)
	{
		avgAD	= 0;
		avgADC	= 0;
		
		//高分解能してるのでsleepを10倍
		firstADC = PinGetADCH(pin, ch, &firstAD, range[i] * 10);
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t%d\t%f\t%u\n", firstADC, firstAD, range[i]);
		Drain(drainPin);

		//この時点でコンデンサの充電電圧から電流を計測すると
		//突入電流の影響で計算上かなり上の数値が出るため
		//倍の時間計測して1us当たりの充電電圧を計算しその上昇値を利用する
		
		//差分でrangeが10ms以上の場合は1,以下の場合は10以上で充電上昇値を取得し始める
		diffAD = range[i] > 10000 ? 1.0 : 10.0;
		for(j=2; j<=quantity; j++)
		{
			secondADC = PinGetADCH(pin, ch, &secondAD, (range[i]*j) * 10);
			SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\t%d\t%f\t%u\n", secondADC, secondAD, range[i]*j );
			Drain(drainPin);
			
			if( (secondAD - firstAD) < diffAD )
			{
				break;
			}
			else
			{
				//差分の平均値を求める
				avgAD  += secondAD - firstAD;
				avgADC += secondADC - firstADC;
				
				firstADC = secondADC;
				firstAD  = secondAD;
			}
		}
		if( j > quantity )
		{
			SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\tsum\tadc\t%u\tad\t%f\n", avgADC, avgAD);
			//平均値を求めるため
			//差分のため全体の個数-1で平均値に
			--quantity;
			avgAD /= quantity;
			avgADC /= quantity;
			SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\tavg\tadc\t%u\tad\t%f\n", avgADC, avgAD);
			
			break;
		}
	}
	//測定できなかった
	if( avgADC == 0 )
		return 0;
	
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t%uus\t%f\n", range[i], avgAD);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t1us\t%f\n", avgAD/range[i]);
	// Vc(V)=(I/C)*t
	// t(s)=(Vc*C)/I
	// I(A)=(Vc*C)/t	I(A)=(Vc(mV) * C(uF)) / t(us)
	//a = (((avgAD/range[i])*1e-3)*0.47e-6)/(1e-6);
	//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "I(A) = %f,  I(uA) = %f\n", a, a*1e+6);

	a = (((avgAD/range[i])*1e-3)*0.47e-6)/(1e-6) * 1e+6;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\tI(uA)\t%f\n", a);

	//100Luxで46uA(5v), 1uAで2.17Lux
	lux = a * 2.17;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\tLux\t%f\n", lux);

	lux += 0.05;
	return (int)(lux*FLOAT_SCALE);
}
int GetLuxOhm(int ohm)
{
	const unsigned int pin = 25;
	const unsigned int ch  = 1;
	float ad, a, lux;
	int adc, sleepTime;

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "  Lux use ohm %d\n", ohm);
	//時間経過
	GPIO_SET(pin);
	//100ms
	sleepTime = 100000;

	DelayMicroSecond(sleepTime);
	adc = GetADCH(ch, &ad);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\tohm\tadc\t%8d\tad\t%f\n", adc, ad);

	GPIO_CLR(pin);

	//
	//E=IR	I=E/R	R=E/I
	a = (ad*1e-3)/ohm;

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\tI(A)\t%f\tI(uA)\t%f\n", a, a*1e+6);
	a *= 1e+6;

	//100Luxで46uA(5v), 1uAで2.17Lux
	lux = a * 2.17;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\tLux\t%f\n", lux);

	lux += 0.05;
	return (int)(lux*FLOAT_SCALE);
}
int GetHumidity()
{
	const unsigned int pin = 24;
	long sleepTime;
	int chageTime;
	int ch, adc;
	float ad;

	float hum, r, e, t, Rs, Rs2, Rh ,temp;

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "  Humidity\n");

	sleepTime = 1;
	ch = 0;

	InitPin(pin, PIN_OUT);

	for(chageTime=1; chageTime<5; chageTime++)
	{

		//放電用 0.1s
		DelayMicroSecond(100000);
		adc = GetADCH(ch, &ad);
		if( adc != 0 )
		{
			//放電しきってなかったらもう一度
			DelayMicroSecond(100000);
		}

		sleepTime *= 10;

		//高分解能してるのでsleepを10倍
		adc = PinGetADCH(pin, ch, &ad, sleepTime*10);
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    ch sleepTime %d    %2d    adc %8d    volt %fmV\n", sleepTime, ch, adc, ad);

		if( adc > 10 )
			break;
	}
	
	//adをミリボルトからボルトへ
	ad /= 1000;
	//取得した値から抵抗値を試算 テスト時は150オーム
	//R=T/(-C*ln(-(Et/Vcc)+1))
	//Et=Vcc(1-e^(-T/CR))
	//T=R*(-C*LN(-(Et/Vcc)+1))

	//計算に必要な数字の表示
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    Vcc(V) 3.3,  C(uF) 0.022,  T(us)  %d,  Et(mV)  %f \n", sleepTime, ad);
	r = sleepTime/(-0.022 * log(-1*(ad/3.3)+1));
	e = 3.3*(1-exp(-1*((sleepTime)/(0.022*r))));
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    R =   %f  Et =    %f\n", r, e);
	t = 150*(-0.022 * log(-1*(ad/3.3)+1));
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    R==150 -> T =    %f\n", t);


	//抵抗を対数値へ
	Rs = 240-40*log10(r/316);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    Rs = %f\n", Rs);

	//元データより
	//Rs += (Rs*35-13056) /64 *Tmp /128;
	//Rh=(5*Rs /16 *Rs + 44* Rs )/256 +20

	//温度補正用
	if( g_temp == 0 )
	{
		temp = GetTemp() / 5.0;//(/10*2)
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    GetTemp*2 %f\n", temp);
	}
	else
	{
		temp = g_temp / 5.0; 
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    g_temp*2 %f\n", temp);
	}
	//温度補正値
	Rs2 = (Rs*35-13056) /64 *temp /128;
	//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    Temp to Rs  %f\n", Rs2);

	Rs += Rs2;
	Rh = (5*Rs /16 *Rs + 44* Rs )/256 +20;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    Rs %f, Rh %f\n", Rs, Rh);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    Humidity :  %.1f%\n", Rh);

	//InitPin(pin, PIN_INIT);

	Rh = Rh + 0.05;
	return (int)(Rh * FLOAT_SCALE);
}
int InitLps331()
{
	uint8_t send[3]   = {0,};
	InitI2c(REV_2);

	I2cSetSlaveAddr(I2C_ADDR_LPS331);

	send[0] = LPS331_SINGLE_DATA( LPS331_ACTIVE );
	send[1] = LPS331_ACTIVE_ON;
	I2cWrite(send, 2);
}
int UnInitLps331()
{
	//UnInitI2c();
}
int GetPress()
{
	int i, adPress;
	float press;
	uint8_t send;
	uint8_t recive[3]	= {0,};

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "  press\n");

	//気圧 0x28 0x29 0x2A
	send = LPS331_MULTI_DATA( LPS331_PRESS );
	I2cWrite(&send, 1);
	I2cRead(recive, 3);

	//データの変換
	adPress = 0;
	for(i=0; i<ARRAY_SIZE(recive); i++)
	{
		//printf("recive[%d]  =0x%x\n", i, recive[i]);
		adPress |= recive[i] << (i*8);
	}
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    adPress 0x%X\n", adPress);

	press = ((float)adPress / LPS331_PRESS_RES); //1mbar = 1hPa(1000Pa)
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    press    :  %.1f\n", press);

	press += 0.05;
	return (int)(press * FLOAT_SCALE);
}
int GetTemp()
{
	int i, adTemp;
	float temp;
	uint8_t send;
	uint8_t recive[2]	= {0,};

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "  temp\n");

	//気温 0x2B 0x2C
	send = LPS331_MULTI_DATA( LPS331_TEMP );
	I2cWrite(&send, 1);
	I2cRead(recive, 2);

	//データの変換
	adTemp = 0;
	for(i=0; i<ARRAY_SIZE(recive); i++)
	{
		//printf("recive[%d]  =0x%x\n", i, recive[i]);
		adTemp |= recive[i] << (i*8);
	}
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    adTemp 0x%X\n", adTemp);

	//マイナス値を含む場合(最上位ビットが1のとき)
	//取得したデータの2の歩数を取得してその絶対値をマイナスにする
	if( (adTemp&0x8000) != 0 )
	{
		//adTemp = 0xE07C;
		//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    ~adTemp+1 0x%X\n", adTemp);
		//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    -adTemp 0x%X\n", adTemp);
		adTemp = ((uint16_t)(~adTemp) + 1) * -1;
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    (-1 * (~adTemp)+1)) 0x%X\n", adTemp);
	}

	temp = ((float)adTemp/LPS331_TEMP_RES) + LPS331_TEMP_OFFSET;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    temp     :  %.1f\n", temp);

	//小数点以下2位を四捨五入してintにするために10倍
	temp += 0.05;
	return (int)(temp * FLOAT_SCALE);
}
