
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

//stat chmod
#include <sys/stat.h>

#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"

#include "../sensor/lps331.h"
#include "../sensor/sensor.h"

#include "main.h"
#include "mode.h"
#include "threads.h"
#include "shiftRegister.h"

//スレッド
extern int		g_threadStatus;
extern int		g_dispData[SEG_COUNT];

extern int		g_debugOpt;

//出力するモード
extern int		g_outputMode;

//ロガー用
extern float	g_press;
extern float	g_temp;
extern float	g_hum;
extern float	g_lux;
extern float	g_coreTemp;
////ログ収集間隔(秒数)
//extern int		g_logInterval;
//データ収取間隔(秒数)
extern int		g_dataInterval;
//extern int		g_dataStatus;
//extern int		g_oldDataStatus;
//次のログまでの時間(s)
extern struct timespec	g_waitLog;



//7segの4文字を表示
void* DispDataThread(void* param)
{
	////スレッドをjoin or detachしないとメモリリークが発生する
	////終了状態がわからない状態でよければdetach 必要なら呼び出しもとでjoin
	//if( pthread_detach( pthread_self() ) != 0)
	//	;//error
	InitShiftRegister();

	SetPriority(HIGH_PRIO);
	//シグナル処理にした同じ秒数なのにらちらつきがひどかったのでこちらはなし
	do{
		if( g_outputMode < MODE_ANI_0 )
		{
			//モードに応じたデータを表示用にデータに入れる
			DispModeData(g_outputMode);

			//Send7Seg(g_dispData[digit++], DIGIT_SWITCH);
			Dips7segData();
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
		
		//DPの桁
		
		i = 0;
		//Lps331をone shotモードでたたき起こす
			SetDP(i++, DP_ON);
			usleep(dpSleepTime);
			WakeUpLps331();

		//外気温
			SetDP(i++, DP_ON);
			usleep(dpSleepTime);
			g_temp	= GetTemp();

		//気圧
			SetDP(i++, DP_ON);
			usleep(dpSleepTime);
			g_press	= GetPress();

		//照度
			SetDP(i++, DP_ON);
			usleep(dpSleepTime);
			//g_lux   = GetLuxOhm(100e+3); //100kohm
			g_lux	= GetLux();
			Set7segLightControl(g_lux);

		//湿度
			g_hum	= GetHumidity();

		//CPU温度
			g_coreTemp = GetCoreTemp();

		////モードを元に戻す
		//if( g_dataInterval >= 60 )
		//	g_outputMode = oldOutputMode;
		usleep(dpSleepTime);
		for( i=0; i<SEG_COUNT; i++)
			SetDP(i, DP_OFF);
			//g_dispData[i] &= ~(SEG_DP);

		//正常にデータ取得後データの保存
		SensorLog();

		//残り時間用に現在の時間の取得
		clock_gettime(CLOCK_MONOTONIC, &g_waitLog);

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

	//SensorLogPrintf(SENSOR_LOG_LEVEL_0, 	"%s\t",		dateBuf);		//時刻
	//SensorLogPrintf(SENSOR_LOG_LEVEL_0, 	"%.1f\t", 	g_temp);		//外気温
	//SensorLogPrintf(SENSOR_LOG_LEVEL_0, 	"%.1f\t",	g_press);		//大気圧
	//if( g_lux < 1 )
	//	SensorLogPrintf(SENSOR_LOG_LEVEL_0, "%.4f\t", 	g_lux);			//照度
	//else
	//	SensorLogPrintf(SENSOR_LOG_LEVEL_0, "%.1f\t", 	g_lux);			//照度
	//SensorLogPrintf(SENSOR_LOG_LEVEL_0, 	"%.1f\t",	g_hum);			//湿度
	//SensorLogPrintf(SENSOR_LOG_LEVEL_0, 	"%.1f\n",	g_coreTemp);	//CPU温度
	if( g_lux < 1 )
	{
		SensorLogPrintf(SENSOR_LOG_LEVEL_0,
			"%s\t"		//時刻
			"%.1f\t"	//外気温
			"%.1f\t"	//大気圧
			"%.4f\t"	//照度
			"%.1f\t"	//湿度
			"%.1f\n"	//CPU温度
				, dateBuf		//時刻
				, g_temp        //外気温
				, g_press       //大気圧
				, g_lux         //照度
				, g_hum         //湿度
				, g_coreTemp    //CPU温度
		);
	}
	else
	{
		SensorLogPrintf(SENSOR_LOG_LEVEL_0,
			"%s\t"		//時刻
			"%.1f\t"	//外気温
			"%.1f\t"	//大気圧
			"%.1f\t"	//照度
			"%.1f\t"	//湿度
			"%.1f\n"	//CPU温度
				, dateBuf		//時刻
				, g_temp        //外気温
				, g_press       //大気圧
				, g_lux         //照度
				, g_hum         //湿度
				, g_coreTemp    //CPU温度
		);
	}

}


//logのバックアップ
void LogBukup()
{
	//コマンドの実行
	if( g_debugOpt == DEBUG_OUTPUT )
	{
		//MySysLog(LOG_DEBUG, "%s\n", LOG_SAVE_ZIP_D );
		//system( LOG_SAVE_ZIP_D );
		MySysLog(LOG_DEBUG, "%s\n", LOG_SAVE_ZIP );
	}
	else
	{
		system( LOG_SAVE_ZIP );
	}

}
//logの復元
void LogOpen()
{
	struct stat status;
	mode_t proccessMask;
	int exist;

	if( g_debugOpt == DEBUG_OUTPUT )
	{
		//MySysLog(LOG_DEBUG, "%s\n", LOG_OPEN_ZIP_D);
		//system( LOG_OPEN_ZIP_D );
		MySysLog(LOG_DEBUG, "%s\n", LOG_OPEN_ZIP);
		return;
	}
	
	//logのzipファイルの存在確認
	exist = stat(LOG_ZIP, &status);
	if( exist == -1 )
	{
		MySysLog(LOG_DEBUG, "no log zip\n");
		return;
	}

	//logディレクトリの確認 後でここにグラフを作成するためマスクを外してall-OKに
	exist = stat(LOG_DIR, &status);
	if( exist == -1 )
	{
		proccessMask = umask(0);
		mkdir(LOG_DIR, 0777);
		umask(proccessMask);

		MySysLog(LOG_DEBUG, "no log directory\n");
	}

	//コマンドの実行
	system( LOG_OPEN_ZIP );
}

