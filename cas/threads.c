
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//シグナル
#include <signal.h>

//va_...
#include <stdarg.h>

//thread
#include <pthread.h>
//errno
#include <errno.h>

//syslog
#include <syslog.h>

#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"

#include "../sensor/lps331.h"
#include "../sensor/sensor.h"

#include "main.h"
#include "mode.h"
#include "threads.h"

//1. STを下げて入力モードへ
//2. SHを下げてdata storeの入力待ち
//3. DS入力(0ならそのまま)
//4. SHをあげて入力の保存
//5. 2-4を繰り返す
//6. STを挙げて出力
const int ds = 18;	//data store
const int st = 15;	//st(strage register)
const int sh = 14;	//shift register

//スレッド
extern int		g_threadStatus;
extern int		g_dispData[SEG_COUNT];

//出力するモード
extern int		g_outputMode;

//ロガー用
extern float	g_press;
extern float	g_temp;
extern float	g_hum;
extern float	g_lux;
////ログ収集間隔(秒数)
//extern int		g_logInterval;
//データ収取間隔(秒数)
extern int		g_dataInterval;
//extern int		g_dataStatus;
//extern int		g_oldDataStatus;


void InitShiftRegister()
{
	//pinを出力モードへ //PIN_INIT
	InitPin(ds, PIN_OUT);
	InitPin(st, PIN_OUT);
	InitPin(sh, PIN_OUT);
}
void UninitShiftRegister()
{
	//消灯
	SendShiftRegister( 0x0000 );

	//pinを出力モードへ //PIN_INIT
	InitPin(ds, PIN_INIT);
	InitPin(st, PIN_INIT);
	InitPin(sh, PIN_INIT);
}
void SendShiftRegister(unsigned int dispNum)
{

	int i;
	GPIO_CLR(st);

	for(i=15; i>=0; i--)
	{
		GPIO_CLR(sh);
		if( (dispNum & (1<<i) ) != 0 )
		{
			//printf("1 ");
			GPIO_SET(ds);
		}
		else
		{
			//printf("0 ");
			GPIO_CLR(ds);
		}
		GPIO_SET(sh);
	}
	//printf("\n");

	GPIO_SET(st);
}

//7segの4文字を表示
void* DispDataThread(void* param)
{
	int i,j , over, digit, dispNum;

	////スレッドをjoin or detachしないとメモリリークが発生する
	////終了状態がわからない状態でよければdetach 必要なら呼び出しもとでjoin
	//if( pthread_detach( pthread_self() ) != 0)
	//	;//error
	InitShiftRegister();

	SetPriority(HIGH_PRIO);
	//シグナル処理にした同じ秒数なのにらちらつきがひどかったのでこちらはなし
	digit = 0;
	do{
		if( g_outputMode < MODE_ANI_0 )
		{
			//モードに応じたデータを表示用にデータに入れる
			DispModeData(g_outputMode);

			SendShiftRegister( g_dispData[digit] );

			digit = digit>=3 ? 0 : digit+1;

			usleep(DIGIT_SWITCH);
		}
		else
		{
			AnimationMode(g_outputMode);
		}

		//スレッドの状態を0に変更されたら終了
		if( g_threadStatus == 0 )
			break;
	}while( 1 );
	SetPriority(NOMAL_PRIO);

	UninitShiftRegister();

	g_threadStatus = 0;

	pthread_exit(NULL);

	return;
}


//シグナルの設定
int SetSignalBlock(sigset_t *ss)
{
	//使用するシグナルのマスク
	//マスクを一度空に
	sigemptyset( ss );
	//アプリケーションで使用が推奨されているSIGRTMINをセット
	sigaddset(ss, THREAD_MSG_EXIT);
	//シグナルをブロック
	if( pthread_sigmask( SIG_BLOCK, ss, 0 ) != 0 )
	{
		MySysLog(LOG_WARNING, "signal block error\n");
		return -1;
	}
	return 0;
}

//データ収集前にログ保存されないように簡易的にデータの収集はwaitの前に行いログはwait後に行う

//センサからデータを取得
void* SensorDataThread(void *param)
{
	sigset_t	ss;
	siginfo_t	sig;
	struct timespec timeOut;
	int sigNo;
	//int oldOutputMode = MODE_CLOCK;
	unsigned int i;
	const unsigned int dpSleepTime = 100000;

	//シグナルのタイムアウト
	timeOut.tv_sec = g_dataInterval;
	timeOut.tv_nsec = 0;
	if( SetSignalBlock(&ss) == -1 )
		pthread_exit(NULL);

	//スレッドを高優先に
	SetPriority(HIGH_PRIO);

	do{
		if( sigNo == THREAD_MSG_EXIT )
		{
			MySysLog(LOG_DEBUG, "Data Thread exit signal\n");
			break;
		}
		////センサー間隔が1分以上はセンサー実行時アニメーションモードにする
		//if( g_dataInterval >= 60 )
		//{
		//	oldOutputMode	= g_outputMode;
		//	g_outputMode	= MODE_ANI_0;
		//}
		
		//Lps331をone shotモードでたたき起こす
		WakeUpLps331();
		g_dispData[0] |= SEG_DP;
		usleep(dpSleepTime);
		
		
		g_temp	= GetTemp();
		g_dispData[1] |= SEG_DP;
		usleep(dpSleepTime);
		
		
		g_press	= GetPress();
		g_dispData[2] |= SEG_DP;
		usleep(dpSleepTime);
		
		g_lux	= GetLux();
		g_dispData[3] |= SEG_DP;
		usleep(dpSleepTime);
		
		//g_lux   = GetLuxOhm(100e+3); //100kohm
		g_hum	= GetHumidity();
		
		////モードを元に戻す
		//if( g_dataInterval >= 60 )
		//	g_outputMode = oldOutputMode;
		usleep(dpSleepTime);
		for( i=0; i<SEG_COUNT; i++)
			g_dispData[i] &= ~(SEG_DP);
			
		//正常にデータ取得後データの保存
		SensorLog();
		
		sigNo = sigtimedwait( &ss, &sig, &timeOut);
		//何らかの形でシグナルが来なかった時の保険
		if( g_threadStatus == 0 )
			break;
	}while( 1 );

	SetPriority(NOMAL_PRIO);

	g_threadStatus = 0;

	pthread_exit(NULL);
}
void SensorLog()
{
	time_t 		t;
	struct tm 	*ts;
	char		dateBuf[20];
	
	t  = time(NULL);
	ts = localtime(&t);
	//strftime(dateBuf, sizeof(dateBuf), "%F %T", ts);
	strftime(dateBuf, sizeof(dateBuf), "%T", ts);

	//加工しやすいようにTSV形式になるように
	SensorLogPrintf(SENSOR_LOG_LEVEL_0, "%s", dateBuf);
	SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.1f", 	g_temp);
	SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.1f",	g_press);
	if( g_lux < 1 )
		SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.4f", 	g_lux);
	else
		SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.1f", 	g_lux);
	SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.1f\n",	g_hum);
	
}

////センサからデータを取得
//void* SensorDataThread(void *param)
//{
//	sigset_t	ss;
//	siginfo_t	sig;
//	struct timespec timeOut;
//	int sigNo;
//	//int oldOutputMode = MODE_CLOCK;
//	unsigned int i;
//	const unsigned int dpSleepTime = 100000;
//
//	//シグナルのタイムアウト
//	timeOut.tv_sec = g_dataInterval;
//	timeOut.tv_nsec = 0;
//	if( SetSignalBlock(&ss) == -1 )
//		pthread_exit(NULL);
//
//	//スレッドを高優先に
//	SetPriority(HIGH_PRIO);
//
//	do{
//		if( sigNo == THREAD_MSG_EXIT )
//		{
//			MySysLog(LOG_DEBUG, "Data Thread exit signal\n");
//			break;
//		}
//		////センサー間隔が1分以上はセンサー実行時アニメーションモードにする
//		//if( g_dataInterval >= 60 )
//		//{
//		//	oldOutputMode	= g_outputMode;
//		//	g_outputMode	= MODE_ANI_0;
//		//}
//		
//		//データが正常に取れているか確認
//		g_dataStatus = 2;
//		//Lps331をone shotモードでたたき起こす
//		WakeUpLps331();
//		g_dispData[0] |= SEG_DP;
//		usleep(dpSleepTime);
//		
//		
//		g_dataStatus = 3;
//		g_temp	= GetTemp();
//		g_dispData[1] |= SEG_DP;
//		usleep(dpSleepTime);
//		
//		
//		g_dataStatus = 4;
//		g_press	= GetPress();
//		g_dispData[2] |= SEG_DP;
//		usleep(dpSleepTime);
//		
//		
//		g_dataStatus = 5;
//		g_lux	= GetLux();
//		g_dispData[3] |= SEG_DP;
//		usleep(dpSleepTime);
//		
//		
//		g_dataStatus = 6;
//		//g_lux   = GetLuxOhm(100e+3); //100kohm
//		g_hum	= GetHumidity();
//		
//		//正常にデータ取得
//		g_dataStatus = g_dataStatus==0 ? 1 : 0;
//
//		////モードを元に戻す
//		//if( g_dataInterval >= 60 )
//		//	g_outputMode = oldOutputMode;
//		usleep(dpSleepTime);
//		for( i=0; i<SEG_COUNT; i++)
//			g_dispData[i] &= ~(SEG_DP);
//		
//		sigNo = sigtimedwait( &ss, &sig, &timeOut);
//		//何らかの形でシグナルが来なかった時の保険
//		if( g_threadStatus == 0 )
//			break;
//	}while( 1 );
//
//	SetPriority(NOMAL_PRIO);
//
//	g_threadStatus = 0;
//
//	pthread_exit(NULL);
//}
//
//
////センサーのロギング
//void* SensorLoggerThread(void* param)
//{
//	//スレッドのシグナル用
//	sigset_t	ss;
//	siginfo_t	sig;
//	struct timespec timeOut;
//	int			sigNo;
//
//	time_t 		t;
//	struct tm 	*ts;
//	char		dateBuf[20];
//	
//	//シグナルのタイムアウト
//	timeOut.tv_sec = g_logInterval;
//	timeOut.tv_nsec = 0;
//	if( SetSignalBlock(&ss) == -1 )
//		pthread_exit(NULL);
//
//	do{
//		sigNo = sigtimedwait( &ss, &sig, &timeOut);
//		if( sigNo == THREAD_MSG_EXIT)
//		{
//			//MySysLog(LOG_DEBUG, "sig No 0x%X  0x%X  0x%X\n", sigNo , sig.si_signo, sig.si_errno, sig.si_code);
//			MySysLog(LOG_DEBUG, "Log Thread exit signal\n");
//			break;
//		}
//		//何らかの形でシグナルが来なかった時の保険
//		if( g_threadStatus == 0 )
//			break;
//
//		//データ自体は他のスレッドで取得してる
//		
//		//正常に取得てきていない場合はエラーを表示させる
//		if( g_dataStatus >= 2 || g_oldDataStatus == g_dataStatus)
//		{
//			g_outputMode = MODE_OUTPUT;
//			sprintf(dateBuf, "ERR%d", g_dataStatus);
//			ReverseInsert(dateBuf);
//		}
//		else
//		{
//			g_oldDataStatus = g_dataStatus;
//			
//			g_outputMode = MODE_CLOCK;
//			t  = time(NULL);
//			ts = localtime(&t);
//			//strftime(dateBuf, sizeof(dateBuf), "%F %T", ts);
//			strftime(dateBuf, sizeof(dateBuf), "%T", ts);
//	
//			//加工しやすいようにTSV形式になるように
//			SensorLogPrintf(SENSOR_LOG_LEVEL_0, "%s", dateBuf);
//			SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.1f", 	g_temp);
//			SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.1f",	g_press);
//			if( g_lux < 1 )
//				SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.4f", 	g_lux);
//			else
//				SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.1f", 	g_lux);
//			SensorLogPrintf(SENSOR_LOG_LEVEL_0, "\t%.1f\n",	g_hum);
//		}
//	}while( 1 );
//
//	g_threadStatus = 0;
//
//	pthread_exit(NULL);
//}
//
