
#include <stdio.h>
//clock_gettime gccで -lrtをつけないとコンパイルエラー
#include <time.h>
//uint8_t,16_t
#include <stdint.h>
//va_...
#include <stdarg.h>
//抵抗値を求めるためにlog(Lnが必要)試算用にexpも使う
#include <math.h>
//syslog
#include <syslog.h>
//stat chmod
#include <sys/stat.h>
//passwd
#include <pwd.h>


//sensorのみの場合とcasで分けるためにわざとここに記述
#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"
#include "../gpio/gpio-i2c.h"
#include "../gpio/gpio-arm-timer.h"
#include "adConvert.h"
#include "lps331.h"
#include "sensor.h"


//湿度の温度補正用のtemp用
extern float	g_temp;

int g_consoleOutput;
int	g_sensorLogLevel;
//ロギング
int SetSensorLogLevel(int level, int console)
{
	int old;
	old	= g_sensorLogLevel;

	g_sensorLogLevel	= level;
	g_consoleOutput		= console;
	return old;
}
void SensorLogPrintf(int level, const char *str, ...)
{
	if( g_sensorLogLevel >= level )
	{
		FILE *fp;
		va_list args;

		va_start(args, str);

		if( g_consoleOutput == 1 )
		{
			vfprintf(stdout, str, args);
		}
		else
		{
			time_t t;
			struct tm *ts;
			char file[100];
			struct stat status;
			int exist;
			mode_t proccessMask;

			t	= time(NULL);
			ts	= localtime(&t);
			strftime(file, sizeof(file), LOG_DIR "%F.log", ts);

			//logディレクトリの確認 後でここにグラフを作成するためマスクを外してall-OKに
			exist = stat(LOG_DIR, &status);
			if( exist == -1 )
			{
				proccessMask = umask(0);
				mkdir(LOG_DIR, 0777);
				umask(proccessMask);
			}

			//ファイルの存在確認ない場合はデータの見出しを挿入するのでフラグを立てる
			exist = stat(file, &status);
			fp = fopen(file, "a");
			if( fp != NULL )
			{
				if( exist == -1 )
				{
					char header[256];
					char date[20]; //YYYY-MM-DD

					//const char userName[] = "pi"; //ファイルの所有者をpiにする(ハードコード)
					//struct passwd *pw = NULL;
					//作成時のみ実行
					proccessMask = umask(0);
					chmod(file, 0766);
					umask(proccessMask);
					//pw = getpwnam(userName);
					//if( pw != NULL )
					//	chown(file, pw->pw_uid, pw->pw_gid);

					strftime(date, sizeof(date), "%F", ts);
					snprintf(header, sizeof(header),
						"%s\n"
						"time\t"
						"Temp\t"
						"Press\t"
						"Lux\t"
						"Humidity\t"
						"CoreTemp",
						date
					);
					fprintf(fp, "%s\n", header);
					//fprintf(fp, "%s\n", date);
					//fprintf(fp, "time\tTemp\tPress\tLux\tHumidity\n");
				}
				vfprintf(fp, str, args);
				fclose(fp);
			}
			else
			{
				syslog(LOG_WARNING, "log file create failed\n");
			}
		}
		va_end(args);
	}
}

void Drain(int pin, int ch)
{
	////ドレインピンを使用して放電
	int i, ad1;

	//PIN_OUTのCLRにしてコンデンサの電荷をGNDに流す
	//必ず電流制限抵抗をつけること!!
	InitPin(pin, PIN_OUT);
	GPIO_CLR(pin);

	//たまにうまく動かないことがあるので中止
	//GPIO_CLR(pin);
	//InitPin(pin, PIN_IN);
	//PullUpDown(pin, PULL_DOWN);

	//PrintGpioPinMode();
	//PrintGpioLevStatus();

	//ドレインピンを使用して放電 最大20ms*50 = 1s
	for(i=0; i<50; i++)
	{
		ad1 = GetAD(ch);
		//printf("\td i-%02d %04d\n", i, ad1);
		//if( (i+1) % 7 == 0 )
		//	printf("\n");

		//20ms
		usleep(20000);

		if( ad1 == 0 )
			break;
	}
	//printf("\n");

	//ハイインピーダンスへ
	//ハイインピーダンスでもリーク電流が発生するので注意
	// (0.000947-0.000030)=0.000917[uA]=9.17e-4[uA]ぐらい
	GPIO_CLR(pin);
	InitPin(pin, PIN_IN);
	PullUpDown(pin, PULL_NONE);

	//PrintGpioPinMode();
	//PrintGpioLevStatus();
	return;
}
void GetLuxTest(int loop, unsigned int sleepTime, unsigned int lsb, int type)
{
	const unsigned int pin = 25;
	const unsigned int drainPin = 23;
	const unsigned int ch  = 1;

	float mV;
	unsigned int ad, incSleep, useTime;
	int i;


	InitPin(pin, PIN_OUT);
	InitPin(drainPin, PIN_IN);

	// Vc(V)=(I/C)*t
	// t(s)=(Vc*C)/I
	// I(A)=(Vc*C)/t	I(A)=(Vc(mV) * C(uF)) / t(us)
	//真っ暗(豆なし)
	//Lux use ohm 1000000
	//    ohm     adc            9        ad      7.250977
	//    I(A)    0.007251        I(uA)   0.000000
	//    Lux     0.015735
	//ret = 0.015735

	//蛍光灯下
	//pi@raspberrypi ~/tmp/cas/sensor $ sudo ./sensor -DL 100000
	//  Lux use ohm 100000
	//                ohm     adc          240        ad      193.359375
	//                I(A)    1.933594        I(uA)   0.000002
	//                Lux     4.195899
	//ret = 4.195899
	//pi@raspberrypi ~/tmp/cas/sensor $ sudo ./sensor -DL 1000000
	//  Lux use ohm 1000000
	//                ohm     adc         3664        ad      2951.953125
	//                I(A)    2.951953        I(uA)   0.000003
	//                Lux     6.405738
	//ret = 6.405738


	incSleep = sleepTime;
	Drain(drainPin, ch);
	if( type )
	{
		for(i=0; i<loop; i++)
		{
			Drain(drainPin, ch);
			ad = GetADNoPadPin(pin, ch, incSleep);
			mV = AtoDmV(ad, lsb);
			SensorLogPrintf(SENSOR_LOG_LEVEL_1, "%d\t%d\t%f\n", incSleep, ad>>lsb, mV);
			incSleep += sleepTime;
			if( mV > 3200.0 )
				break;
		}
	}
	else
	{
		float a, refa, inpropC;
		const float con = 0.47e-6;
		inpropC = 3.3 * con;
		//毎回on/offをやるとある一定の電圧から上がらなくなるので
		//一回のスリープがでかいときは切らないでそのまま計測する
		GPIO_SET(pin);
		for(i=0; i<loop; i++)
		{
			DelayArmTimerCounter( sleepTime*10-AD_NO_PADDING_GAP );

			ad = GetADNoPad(ch);
			mV = AtoDmV(ad, lsb);
			SensorLogPrintf(SENSOR_LOG_LEVEL_1, "%d\t%d\t%f\n", incSleep, ad>>lsb, mV);
			incSleep += sleepTime;

			//比例グラフのため計算上ずっと電圧が上がり続けるが
			//RPIのgpioの電圧以上はあり得ないため、
			//現在の時間で満電圧になる電流値を求めそれ以上の数値は出ないことから充電を終了させる
			//実際は同じ数字になることはなく、最後のほうは充電しにくくなって
			//ある一定の電圧以上にはならないため、実測で95%まで近づくのを確認したので
			//コンデンサの電圧から求めた電流値と計算値が90%以上なら終了
			refa	= inpropC / (incSleep*1e-6) * 1e+6;
			a		= (mV*1e-3*con) / (incSleep*1e-6) * 1e+6;
			if( a/refa > 0.93 )
				break;
		}
		GPIO_CLR(pin);
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "ref a %f\t a %f\n", refa, a);
	}
	return;
}

unsigned int GetAllDrainVoltage(int pin, int ch, int drainPin, LuxRangeData *range, int quantity)
{
	unsigned int oldAd, ad, sumDiffAd;
	int j, diff;
	float mv;

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\tall drain(s,ad,mv,diff,sum diff)\n");
	sumDiffAd	= 0;
	oldAd		= 0;

	//一回目は差分を取らないので外で計測 外で放電を行ってるのでここは行わない
	ad		= GetADNoPadPin(pin, ch, range->sleepTime );
	oldAd	= ad;
	mv = AtoDmV(ad, range->lsb );
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\t%8d\t%d\t%f\n", range->sleepTime, ad, mv);
	for(j=2; j<=quantity; j++)
	{
		Drain(drainPin, ch);
		ad			= GetADNoPadPin(pin, ch, range->sleepTime*j );

		diff		= ad - oldAd;
		sumDiffAd	+= diff;
		oldAd		= ad;
		mv = AtoDmV(ad, range->lsb );
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\t%8d\t%d\t%f\t%d\t%d\n",
			range->sleepTime*j, ad, mv, diff, sumDiffAd);
		if( diff < range->diff )
			return 0;
	}
	return sumDiffAd;
}
unsigned int GetNoDrainVoltage(int pin, int ch, LuxRangeData *range, int quantity)
{
	unsigned int oldAd, ad, sumDiffAd;
	unsigned int eachSleep;
	int j, diff;
	float mv;

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\tno drain(s,ad,mv,diff,sum diff)\n");

	eachSleep	= range->sleepTime*10 - AD_NO_PADDING_GAP;
	sumDiffAd	= 0;

	GPIO_SET(pin);

	//一回目は差分を取らないので外で計測
	DelayArmTimerCounter( eachSleep );
	ad		= GetADNoPad(ch);
	oldAd	= ad;
	mv = AtoDmV(ad, range->lsb );
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\t%8d\t%d\t%f\n", range->sleepTime, ad, mv);
	for(j=2; j<=quantity; j++)
	{
		DelayArmTimerCounter( eachSleep );
		ad		= GetADNoPad(ch);

		diff		= ad - oldAd;
		sumDiffAd	+= diff;
		oldAd		= ad;
		mv = AtoDmV(ad, range->lsb );
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t\t%8d\t%d\t%f\t%d\t%d\n",
			range->sleepTime*j, ad, mv, diff, sumDiffAd);
		if( diff < range->diff )
			return 0;
	}
	GPIO_CLR(pin);

	return sumDiffAd;
}
//10ms以上ならADコンバートでかかる時間
//約0.027ms(27us)を足しても誤差としてで収まる(予想)
#define NO_DRAIN	10000
float GetLux()
{
	const unsigned int pin = 25;
	const unsigned int drainPin = 23;
	const unsigned int ch  = 1;

	unsigned int diffAd, quantity;
	float mv, a, lux, avgAd;
	int i;

	//12bitを2bit削ってノイズ除去したとき
	//25 		(25us)		-> 10,000uA
	//100		(100us)		->  1,000uA
	//500		(500us)		->    100uA
	//1000		(1ms)		->	   10uA
	//3000		(3ms)		->		1uA
	//15000		(15ms)		->		0.1uA
	//30000		(30ms)		->		0.05uA
	//160000 	(160ms)		->		0.01uA	本来は150msだがちょっと余裕を見て160msへ
	//300000	(300ms)		->		0.005uA
	//800000	(800ms)		->		0.002uA 100ms余裕を入れてる
	//1600000	(1.6s)		->		0.001uA 0.1s余裕を入れてある 長いのでなし
	//豆 1Mohm		9.667969mV		0.009668uA	Lux	0.020979
	//コンデンサ    0.000030mV/us	0.014200uA	Lux	0.030814

	//0.1uA以降は時間がかかるのでノイズ上等で12bitを11bitにする
	//8000		(8ms)		->		0.1uA(7500		+ 500us)
	//16000		(16ms)		->		0.05uA(15000	+ 1000us)
	//80000		(80ms)		->		0.01uA(75000	+ 5000us)
	//160000	(160ms)		->		0.005uA(150000	+ 10000us)
	//800000	(800ms)		->		0.001uA(750000	+ 50000us) 長いのでなし
	LuxRangeData range[] = {
		{        25,	20,	2},	//us
		{       100,	20,	2},
		{       500,	20,	2},
		{      1000,	 8,	2},	//ms
		{	   3000,	 8,	2},
		//{     10000,	 4,	2},
		{	  16000,	 4,	2},
		{	  30000,	 4,	2},
		//{    100000,	 2,	1},
		{	 160000,	 2,	1},	//4,	2},
		{	 300000,	 2,	1},	//4,	2},
		{	 800000,	 2,	1},	//4,	2},
		{   1000000,	 2,	1},	//4,	2},		//s
		{  10000000,	 2,	1},	//4,	2},
		{ 100000000,	 2,	1},	//4,	2},
		{1000000000,	 2,	1} 	//4,	2}
	};

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "  Lux use condensare\n");

	//ピンの初期設定
	//ドレイン用のピンはハイインピーダンスでもリーク電流があるので
	//( (0.000947-0.000030)=0.000917[uA]=9.17e-4[uA]=91.7[nA]ぐらい )初期値としては出力用にしておく
	InitPin(pin, PIN_OUT);
	GPIO_CLR(pin);
	
	//一度Pull up/downを動かさないとnoneがうまく動かないっポイ
	InitPin(drainPin, PIN_IN);
	PullUpDown(drainPin, PULL_UP);
	PullUpDown(drainPin, PULL_DOWN);
	//PrintGpioStatus();
	//PrintGpioPinMode();
	//PrintGpioLevStatus();

	for(i=0; i<ARRAY_SIZE(range); i++)
	{
		//この時点でコンデンサの充電電圧から電流を計測すると
		//突入電流の影響で計算上かなり上の数値が出るため
		//倍の時間計測して1us当たりの充電電圧を計算しその上昇値を利用する
		
		//GPIOの準備で最初の一回だけ時間が長めにかかるのかも

		//放電
		Drain(drainPin, ch);
		quantity = 5;
		if( range[i].sleepTime < NO_DRAIN )
			diffAd = GetAllDrainVoltage(pin, ch, drainPin, &range[i], quantity);
		else
			diffAd = GetNoDrainVoltage(pin, ch, &range[i], quantity);

		if( diffAd == 0 )
			continue;

		break;
	}

	//ピンの初期化
	InitPin(pin, PIN_OUT);
	GPIO_CLR(pin);
	//drainPinを出力用にしてLにするので放電も兼ねる
	//ただし絶対に16mAを超えないように抵抗を入れる V/R=I
	InitPin(drainPin, PIN_OUT);
	GPIO_CLR(drainPin);

	//電圧の差分の平均値を求める
	avgAd = (float)diffAd/(quantity-1);
	mv = AtoDmV(avgAd, range[i].lsb);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "\t%uus\t%f\t%f\n", range[i].sleepTime, avgAd, mv);

	// Vc(V)=(I/C)*t
	// t(s)=(Vc*C)/I
	// I(A)=(Vc*C)/t	I(A)=(Vc(mV) * C(uF)) / t(us)

	//a = ((mv*1e-3)*0.47e-6)/(range[i].sleepTime*1e-6) * 1e+6;

	//a = (mv*0.47*1e-9) / (range[i].sleepTime*1e-6) * 1e+6;
	//a = (mv*0.47*1e-3) / (range[i].sleepTime) * 1e+6;
	//a = (mv*0.47*1e+3) / range[i].sleepTime;

	//a = ((mv*1e-3)*(0.47*1e-6))*1e+6/(range[i].sleepTime*1e-6);
	//a = (mv*1e-3*0.47)/(range[i].sleepTime*1e-6);
	//a = (mv*0.47)/(range[i].sleepTime*1e-3);
	a = (mv*0.47)/(range[i].sleepTime*1e-3);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "I(uA)\t%f\n", a);


	//温度補正グラフ[Relative Photocurrent(相対光電流) : Ambient Temperature(周囲温度)]
	float temp, correction;
	//温度によって流れる光電流が変化する? Light source A(以下A) White LED(以下L)
	//基準	5v 25度 -> 100%
	//A 10度 -> 90%
	//L 80度 -> 130%
	//傾き
	//A	| (100-90)/(25-10)	-> 10/15 -> 25度を基準に1度ごと0.66..%変わる
	//L | (130-100)/(80-25) -> 30/55 -> 25度を基準に1度ごと0.54(54)...%変わる
	//切片
	//A | 100-(25*0.67)	-> 83.25%
	//L | 100-(25*0.55)	-> 86.25%
	//式
	//A | y=0.67*x + 83.25
	//L | y=0.55*x + 86.25
	//温度補正用
	if( g_temp == 0 )
		temp = GetTemp();
	else
		temp = g_temp;
	correction = 0.67*temp + 83.25;
	a /= (correction/100);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "temp correction\ttemp\t%f\tcorrection\t%.2f%%\n", temp, correction);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "correction I(uA)\t%f\n", a);

	//A	| 5v 100Lux I(uA)      t 46       | 1uA  2.17Lux
	//L | 5v 100Lux I(uA) m 15 t 33 M 73  | 1uA  3.03Lux
	lux = a * 2.17;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "Lux\t%f\n", lux);

	return lux;
}
float GetLuxOhm(int ohm)
{
	const unsigned int pin = 25;
	const unsigned int ch  = 1;
	float ad, a, lux;
	int adc, sleepTime;
	int lsb;

	//100ms
	//時間を短くするとなぜかADコンバータがうまく動かず
	//短くすればするほど高い電圧値が測定される
	//100ms以上は値が一定になる
	sleepTime = 200000;
	//lsb
	lsb = 0;//AD_LSB;

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "Lux use ohm %d\n", ohm);
	//時間経過
	GPIO_SET(pin);

	DelayMicroSecond(sleepTime);
	adc = GetAD(ch);
	ad = AtoDmV(adc, lsb);

	GPIO_CLR(pin);
	//100ms
	DelayMicroSecond(sleepTime/100);


	//
	//E=IR	I=E/R	R=E/I
	a = (ad*1e-3)/ohm;

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "adc\tad\tI(A)\tI(uA)\n");
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "%d\t%f\t%f\t%f\n", adc, ad, a, a*1e+6);
	a *= 1e+6;

	//温度補正
	float temp, correction;
	if( g_temp == 0 )
		temp = GetTemp();
	else
		temp = g_temp;
	correction = 0.67*temp + 83.25;
	a /= (correction/100);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "temp correction\ttemp\t%f\tcorrection\t%.2f%%\n", temp, correction);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "correction I(uA)\t%f\n", a);

	//100Luxで46uA(5v), 1uAで2.17Lux
	lux = a * 2.17;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "Lux\t%f\n", lux);

	return lux;
}
float GetHumidity()
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
		adc = GetAD(ch);
		ad = AtoDmV(adc, AD_LSB);
		if( adc != 0 )
		{
			//放電しきってなかったらもう一度
			DelayMicroSecond(100000);
		}

		sleepTime *= 10;

		//adc = GetADpin(pin, ch, sleepTime);
		//adc = GetADmcp3204(pin, ch, sleepTime);
		adc = GetADNoPadPin(pin, ch, sleepTime);
		ad = AtoDmV(adc, 2);
		adc >>= 2;
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    ch sleepTime %d    %2d    adc %8d    volt %fmV\n",
			sleepTime, ch, adc, ad);

		if( adc > 195 )	//0.63v
			break;
	}


	//取得した値から抵抗値を試算 テスト時は150オーム
	//R=T/(-C*ln(-(Et/Vcc)+1))
	//Et=Vcc(1-e^(-T/CR))
	//T=R*(-C*LN(-(Et/Vcc)+1))

	//計算に必要な数字の表示
	const int 	ohm		= 1e+3;		//150(ohm) //1e+6
	//pinの出力電圧が低い可能性がある
	//3.2～3.1あたりにすると高抵抗時の充電の値が合う
	//とりあえず3160なら1K～1Mまでほぼ理論値になるのでそれで
	//1K	   10us	1192.382812		refV 3.262	220.766312	理論値 220
	//10K	  100us	1163.378906		refV 3.186	180.100143	理論値 180
	//100K	 1000us	1160.156250		refV 3.176	140.100143	理論値 140
	//1M	10000us	1153.710815		refV 3.158	99.916054	理論値 100
	const float	refV	= 3160;
	const float con		= 0.022;	//0.022(uF)

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    Vcc(V) %.1f,  C(uF) %.3f,  T(us)  %d,  Et(mV)  %f \n",
		refV, con, sleepTime, ad);

	//adをミリボルトからボルトへ
	//r = (sleepTime*1e-6)/( (-1*(con*1e-6)) * log( (-1 * (ad/refV)) +1 ) );
	//e = refV * ( 1 - exp( -1 * ( (sleepTime*1e-6)/((con*1e-6)*r) ) ) );
	//t = ohm*( (-1*(con*1e-6)) * log( (-1*(ad/refV)) + 1)) * 1e+6;
	r = sleepTime/( (-1*con) * log( (-1 * (ad/refV)) +1 ) );
	e = refV * ( 1 - exp( -1 * ( sleepTime/(con*r) ) ) );
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    R =   %f  Et(mV) =    %f\n", r, e);

	t = ohm*( (-1*con) * log( (-1*(ad/refV)) + 1));
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    R==%d -> T =    %f\n", ohm, t);


	//抵抗を対数値へ
	//1K 219, 10K 180, 100K 140, 1M 99
	//1K 219.808823, 10K  180.180405, 100K 138.982422, 1M 98.982414
	Rs = 240-40*log10(r/316);
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    Rs = %f\n", Rs);

	//元データより
	//Rs += (Rs*35-13056) /64 *Tmp /128;
	//Rh=(5*Rs /16 *Rs + 44* Rs )/256 +20

	//温度補正用 //元データが0.5℃で1のため
	if( g_temp == 0 )
	{
		temp = GetTemp() * 2;
		SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    GetTemp*2 %f\n", temp);
	}
	else
	{
		temp = g_temp * 2;
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
	return Rh;
}
float GetPress()
{
	int i, adPress;
	float press;
	uint8_t send;
	uint8_t recive[LPS331_PRESS_REG_COUNT]	= {0,};

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "  press\n");

	//気圧 0x28 0x29 0x2A
	send = LPS331_MULTI_DATA( LPS331_PRESS );
	I2cWrite(&send, 1);
	I2cRead(recive, LPS331_PRESS_REG_COUNT);

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

	return press;
}
float GetTemp()
{
	int i, adTemp;
	float temp;
	uint8_t send;
	uint8_t recive[LPS331_TEMP_REG_COUNT]	= {0,};

	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "  temp\n");

	//気温 0x2B 0x2C
	send = LPS331_MULTI_DATA( LPS331_TEMP );
	I2cWrite(&send, 1);
	I2cRead(recive, LPS331_TEMP_REG_COUNT);

	//データの変換
	adTemp = 0;
	for(i=0; i<ARRAY_SIZE(recive); i++)
	{
		//printf("recive[%d]  =0x%x\n", i, recive[i]);
		adTemp |= recive[i] << (i*8);
	}
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    adTemp 0x%X\n", adTemp);
	adTemp = (int16_t)adTemp;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    adTemp 0x%X\n", adTemp);

	//マイナス値を含む場合(最上位ビットが1のとき)
	//取得したデータの2の歩数を取得してその絶対値をマイナスにする
	//if( (adTemp&0x8000) != 0 )
	//{
	//	//adTemp = 0xE07C;
	//	//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    ~adTemp+1 0x%X\n", adTemp);
	//	//SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    -adTemp 0x%X\n", adTemp);
	//	//adTemp = ((uint16_t)(~adTemp) + 1) * -1;
	//	adTemp = (int16_t)adTemp;
	//	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    (-1 * (~adTemp)+1)) 0x%X\n", adTemp);
	//}
	temp = ((float)adTemp/LPS331_TEMP_RES) + LPS331_TEMP_OFFSET;
	SensorLogPrintf(SENSOR_LOG_LEVEL_1, "    temp     :  %.1f\n", temp);

	return temp;
}

#define CORE_TEMP_BUF	10
float GetCoreTemp()
{
	// /opt/vc/bin/vcgencmd measure_temp
	// /sys/class/thermal/thermal_zone0/temp
	FILE *coreTempFile;
	char buf[CORE_TEMP_BUF];
	//float coreTemp;


	coreTempFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	if( coreTempFile == NULL )
		return -1;

	fread(buf, sizeof(buf)*sizeof(char), sizeof(buf), coreTempFile);
	
	fclose(coreTempFile);

	//coreTemp = (float)( atoi(buf)/1000.0f );
	//printf("%f\n", coreTemp);

	//return atoi(buf);
	return (float)( atoi(buf)/1000.0f );
}


