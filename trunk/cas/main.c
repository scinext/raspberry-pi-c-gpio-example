
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//POSIXメッセージキュー
//open O_CREATE などに必要
#include <fcntl.h>
//読み書きのモード 0666とかの定義 #include <sys/stat.h>
#include <mqueue.h>
//シグナル
#include <signal.h>
//va_...
#include <stdarg.h>
//getopt
#include <unistd.h>
//thread
#include <pthread.h>
//errno
#include <errno.h>
//syslog
#include <syslog.h>

#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"

#define MAIN_INIT
#include "main.h"
#include "mode.h"
#include "threads.h"

//モード変更のタッチセンサ
#include "../sensor/touchSensor.h"

//スレッド
int		g_threadStatus;
int		g_dispData[SEG_COUNT];

//メッセージキュー
mqd_t	g_mq;

//デバッグ状態のフラグ
int		g_debugOpt;

//出力するモード
int		g_outputMode;

//スクロール表示のバッファとりあえず256文字s
char	g_scrollBuf[SCROLL_BUF];

//ロガー用
float	g_press		= INIT_SENSOR;
float	g_temp		= INIT_SENSOR;
float	g_hum		= INIT_SENSOR;
float	g_lux		= INIT_SENSOR;
float	g_coreTemp	= INIT_SENSOR;
////ログ収集間隔(秒数)
//int		g_logInterval = LOG_INTERVAL_DEF;
//データ収取間隔(秒数)
int		g_dataInterval = LOG_INTERVAL_DEF;//DATA_INTERVAL_DEF;
//int		g_dataStatus;
//int		g_oldDataStatus;
//次のログまでの時間(s)
struct timespec	g_waitLog;

//次のモードにするための割り込みボタン
const int nextModePin = 22;

void Dprintf(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	//デバッグ出力
	if( g_debugOpt == DEBUG_OUTPUT )
		vprintf(str, args);

	va_end(args);
}

void PinInit()
{
	InitGpio();
	InitAD();
	InitLps331();
	//高分解能(0.1ms)
	InitArmTimer(1);

}
void PinUnInit()
{
	UnInitArmTimer();
	UnInitLps331();
	UnInitAD();
}

void SignalHandler(int signum)
{
	switch( signum )
	{
		case SIGTERM:
			{
				//scroll[ good by ]
				ScrollOutputInit( SCROLL_EXIT );
				g_outputMode = MODE_OUTPUT;
				LogBukup();
			}
			break;
	}
}
void TouchSensorInterruptCallback(unsigned int cap)
{	
	SendShiftRegister( 0x0000 );
	if( cap < 20000 )
	{
		//モードを切り替える
		
		if( g_outputMode+1 < MODE_OUTPUT )
			++g_outputMode;
		else
			g_outputMode = MODE_CLOCK;
	}
	else if( cap < 60000 )
	{
		//logをpiのホームディレクトリへ保存
		
		
		g_outputMode = MODE_ANI_0;
		LogBukup();
		sleep(2);
		
		//scroll[ log save ]
		ScrollOutputInit( SCROLL_LOG_SAVE );
		g_outputMode = MODE_OUTPUT;
		sleep(5);
		
		g_outputMode = MODE_CLOCK;
	}
	else
	{
		//終了させる
		
		
		//アニメーション
		g_outputMode = MODE_ANI_1;
		LogBukup();
		sleep(2);
		
		//scroll[ good by ]
		ScrollOutputInit( SCROLL_EXIT );
		g_outputMode = MODE_OUTPUT;
		sleep(5);
		
		//MySysLog(LOG_DEBUG, "  send message touch sensor : %d  %s\n", g_mq, EXIT_MSG);
		MySysLog(LOG_DEBUG, "  cap %u\n", cap);
		//g_outputMode = MODE_CLOCK;
		mq_send(g_mq, EXIT_MSG, strlen(EXIT_MSG), 0);
	}
	MySysLog(LOG_DEBUG, "Interrupt touch sensor mode to %d\n", g_outputMode);
}

int MsgDisposition(char *msg)
{
	char *msgQueuePrefix;

	if( strcmp(msg, EXIT_MSG) == 0 )
	{
		MySysLog(LOG_DEBUG, "  exit message\n");
		return -1;
	}

	SendShiftRegister( 0x0000 );
	//MODE_XXかどうかの判定
	msgQueuePrefix = strstr(msg, MSG_QUEUE_MODE_PREFIX);

	//消灯
	SendShiftRegister( 0x0000 );
	if( msgQueuePrefix != NULL )
	{
		int mode;
		//見つかったのでprefix分進める
		msgQueuePrefix += strlen(MSG_QUEUE_MODE_PREFIX);
		MySysLog(LOG_DEBUG, "  mode %s\n", msgQueuePrefix);
		mode = atoi(msgQueuePrefix);
		g_outputMode = DispModeName( mode )!=-1 ? mode : MODE_CLOCK;
	}
	else
	{
		//違うならとりあえず送付
		MySysLog(LOG_DEBUG, "  output mode %s\n", msg);
		ScrollOutputInit( msg );
		g_outputMode = MODE_OUTPUT;
	}
	return 1;
}
void ReciveQueue()
{
	pthread_t dispThreadId, logThreadId, dataThread;
	struct mq_attr mqAttr;
	char *msg;
	int msgSize;

	//gpioの初期化
	PinInit();
	
	//ログの処理
	LogOpen();
	
	//シグナルの設定
	if( SIG_ERR == signal(SIGTERM, SignalHandler) )
		MySysLog(LOG_WARNING, "not recive SIGTERM \n");
	
	//スレッドの初期化
	g_threadStatus = 1;
	//データ取得用
	MySysLog(LOG_DEBUG, "data interval %d\n", g_dataInterval);
	pthread_create(&dataThread, NULL, SensorDataThread, (void *)NULL);
	//7segのスレッド作成
	pthread_create(&dispThreadId, NULL, DispDataThread, (void *)NULL);
	
	////ロガー用 ログのインターバルが1秒以上なら行う
	//if( g_logInterval >= 1 )
	//{
	//	//データ収集間隔のほうが大きい場合はそちらになる
	//	g_logInterval = g_logInterval > g_dataInterval ? g_logInterval : g_dataInterval;
	//	pthread_create(&logThreadId, NULL, SensorLoggerThread, (void *)NULL);
	//	MySysLog(LOG_DEBUG, "log interval %d\n", g_logInterval);
	//}
	//else
	//{
	//	MySysLog(LOG_DEBUG, "no log\n");
	//}
	
	//touch sensor interrupt
	TouchSensorStart( TouchSensorInterruptCallback );

	//メッセージキューでやり取りするメッセージのサイズの取得(設定はできない)
	mq_getattr(g_mq, &mqAttr);
	//メッセージのやり取りをするバッファの取得
	msg = (char*)malloc(mqAttr.mq_msgsize);
	MySysLog(LOG_DEBUG, "msg loop\n");
	while(1)
	{
		msgSize = mq_receive(g_mq, msg, mqAttr.mq_msgsize, NULL);
		MySysLog(LOG_DEBUG, "  receive message %s\n", msg);

		if( MsgDisposition(msg) == -1 )
			break;
		memset(msg, 0, strlen(msg) );
	}
	
	//interruptを終了させる
	//GpioInterruptEnd();
	
	//7segのスレッドを終了させる
	g_threadStatus = 0;

	////ログスレッドの終了
	//if( g_logInterval >= 1 )
	//{
	//	pthread_sigqueue(logThreadId, THREAD_MSG_EXIT, 0);
	//	pthread_join(logThreadId, NULL);
	//	MySysLog(LOG_DEBUG, "Log Thread end\n");
	//}
	//else
	//{
	//	MySysLog(LOG_DEBUG, "No Log\n");
	//}

	//表示スレッドの終了
	pthread_join(dispThreadId, NULL);
	MySysLog(LOG_DEBUG, "Disp Thread end\n");

	//センサのデータ収集スレッドの終了
	pthread_sigqueue(dataThread, THREAD_MSG_EXIT, 0);
	pthread_join(dataThread, NULL);
	MySysLog(LOG_DEBUG, "Data Thread end\n");

	//メッセージのやり取りをするバッファの解放
	free(msg);

	PinUnInit();


	return;
}
void SendQueue(char *sendData)
{
	//最後は優先度
	mq_send(g_mq, sendData, strlen(sendData), 0);

	//すでにあったらコマンドラインを解析してそれに対応してメッセージを送信
	MySysLog(LOG_DEBUG, "send message queue : %d  %s\n", g_mq, sendData);

	return;
}

int InitServeRecive(int debug, int sensorOutLevel)
{
	//メッセージキューでの二重起動防止は失敗したかもしれないので疑似ロックファイルでの確認へ
	//statでファイルの存在確認をしてたがopen関数のo_exclとo_createを同時に指定すると
	//既にファイルがある場合エラーが返るのでそれで存在確認&作成を同時に行うように
	int fd;
	fd = open(LOCK_FILE, O_RDWR | O_CREAT | O_EXCL, 0666);
	close(fd);
	MySysLog(LOG_DEBUG, LOCK_FILE " exist =%d\n", fd);
	//-1のときはファイルがないので失敗 == まだ他のプロセスが起動していない
	if( fd != -1 )
	{
		MySysLog(LOG_DEBUG, "server process\n");
		MySysLog(LOG_DEBUG, "-----server init\n");
		MySysLog(LOG_DEBUG, "    create lock file " LOCK_FILE " \n");

		//デバッグ起動でなければデーモン化させる
		if( debug == 0 )
		{
			MySysLog(LOG_DEBUG, "    become deamon\n");
			daemon(0, 0);
		}

		//ユーザ、グループ、他人に読み書きの許可をする(0666)
		g_mq = mq_open(MSG_QUEUE_NAME, O_RDWR | O_CREAT | O_EXCL, 0666, NULL);
		if( g_mq == -1 )
		{
			MySysLog(LOG_WARNING, "    message queue create failed\n");
			perror(NULL);
			return -1;
		}
		MySysLog(LOG_DEBUG, "    create recive message queue : %d\n", g_mq);

		MySysLog(LOG_DEBUG, "    sensor log setting level: %d, debugMode: %d\n", sensorOutLevel, debug);
		//センサーの出力情報の設定
		SetSensorLogLevel(sensorOutLevel, debug);

		MySysLog(LOG_DEBUG, "-----server init end\n");
		return SERVER_PROCESS;
	}
	else
	{
		MySysLog(LOG_DEBUG, "client process\n");
		MySysLog(LOG_DEBUG, "-----client init\n");

		//メッセージキューを一度送信専用で開く
		g_mq = mq_open(MSG_QUEUE_NAME, O_WRONLY);
		MySysLog(LOG_DEBUG, "    open sednd message queue : %d\n", g_mq);

		MySysLog(LOG_DEBUG, "-----client init end\n");
		return CLIENT_PROCESS;
	}
}
int UnInitServeRecive(int processStatus)
{
	int ret, result;
	ret = 0;

	MySysLog(LOG_DEBUG, processStatus==SERVER_PROCESS ? "-----server Uninit\n": "-----client Uninit\n");

	//メッセージキューのクローズと削除
	ret = mq_close(g_mq);
	MySysLog(LOG_DEBUG, "    msg queue close %s\n", ret==0 ? "OK" : "NG");

	if( processStatus == SERVER_PROCESS )
	{
		//statでのファイル確認からopenでのファイル確認へ
		int fd;
		ret = mq_unlink(MSG_QUEUE_NAME);
		MySysLog(LOG_DEBUG, "    msg queue unlink %s\n", ret==0 ? "OK" : "NG");

		fd = open(LOCK_FILE, O_RDWR | O_CREAT | O_EXCL, 0666);
		MySysLog(LOG_DEBUG, "    " LOCK_FILE " %s\n", fd!=-1 ? "exist" : "no");
		if( fd == -1 )
		{
			//ロックファイルの削除
			ret = remove(LOCK_FILE);
			MySysLog(LOG_DEBUG, "        " LOCK_FILE " remove %s\n", ret==0 ? "OK" : "NG");
		}
		MySysLog(LOG_DEBUG, "-----server Uninit end\n");
	}
	else
	{
		MySysLog(LOG_DEBUG, "-----client Uninit end \n");
	}
	return ret;
}
int main(int argc, char *argv[])
{
	//SetPriority(NOMAL_PRIO);

	//コマンドラインを解析
	int opt;
	extern char *optarg;
	//何もない場合は時計になる
	char buf[MSG_QUEUE_LEN] = {"MODE_01"};
	//サーバークライアントどちらになるか
	int processStatus;

	int reset		= 0; //リセットフラグ
	int quit		= 0; //終了フラグ
	int	outLevel	= 0; 
	int tmp;


	//システムログのオープン
	openlog(APPNAME, LOG_PID | LOG_CONS, LOG_DAEMON);

	//Dqrcydptlha:m:o:i:f:
	if( argc <= 1 )
	{
	}
	while( (opt = getopt(argc, argv, "HDqrcydptlhTwa:m:o:i:f:I:")) != -1 )
	{
		switch( opt )
		{
			case 'H':
				printf("-c 時計                    -y 年(+和暦)           -d 日付\n");
				printf("-p 大気圧                  -t 温度                -l 照度                -h 湿度\n");
				printf("-T CPU温度                 -w ログ取得までの時間\n");
				printf("\n");
				printf("-a XX アニメーションXX     -m XX XXモードの表示   -o XXXX XXXXを7segに表示(できれば)\n");
				printf("-i XX データ収集の間隔(s)\n");
				//printf("-I XX データ収集の間隔(s)  -i XX ログをとる間隔(s)\n");
				printf("\n");
				printf("-H これ                    -q 終了                -r プロセスをリセット\n");
				printf("\n");
				printf("-D デバッグ                -f XX ログやデバッグ時の情報の細かさ 0 or それ以外1\n");
				return 0;
			case 'D':
				g_debugOpt = DEBUG_OUTPUT;
				#ifdef SYS_LOG
				setlogmask( LOG_UPTO(LOG_DEBUG) );
				#endif
				MySysLog(LOG_DEBUG, "debug on\n");
				break;
			case 'f':
				outLevel =  atoi(optarg);
				MySysLog(LOG_DEBUG, "output level %d\n", outLevel);
				break;
			case 'c':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_CLOCK);
				break;
			case 'y':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_YEAR);
				break;
			case 'd':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_DATE);
				break;
			case 'p':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_PRESS);
				break;
			case 't':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_TEMP);
				break;
			case 'h':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_HUMIDITY);
				break;
			case 'l':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_LUX);
				break;
			case 'T':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_CORE_TEMP);
				break;
			case 'w':
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", MODE_WAIT_LOG);
				break;
			case 'a':
				tmp = atoi(optarg);
				//-m XXで来るので値が正常かどうかのチェック
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", tmp > MODE_QUIT ? MODE_ANI_0 : MODE_ANI_0+tmp);
				break;
			case 'm':
				tmp = atoi(optarg);
				//-m XXで来るので値が正常かどうかのチェック
				sprintf(buf, MSG_QUEUE_MODE_PREFIX"%02d", tmp > MODE_QUIT ? MODE_CLOCK : tmp);
				break;
			case 'o':
				strncpy(buf, optarg, sizeof(buf));
				break;
			//case 'i':
			//	g_logInterval = atoi(optarg);
			//	break;
			case 'i':
				g_dataInterval = atoi(optarg);
				//g_dataInterval = g_dataInterval > DATA_INTERVAL_MIN ? g_dataInterval : DATA_INTERVAL_MIN;
				break;
			case 'q':
				quit = 1;
				sprintf(buf, EXIT_MSG);
				break;
			case 'r':
				reset = 1;
				MySysLog(LOG_DEBUG, "cas Reset\n");
				break;
		}
	}

	if( reset != 1 )
	{
		processStatus = InitServeRecive(g_debugOpt, outLevel);

		//quitが0以外ならサーバにはなれない
		if( processStatus == SERVER_PROCESS && quit == 0)
			ReciveQueue();
		else
			SendQueue(buf);

		UnInitServeRecive(processStatus);
	}
	else
	{
		//終了時の消灯を利用する
		InitGpio();
		InitShiftRegister();
		UninitShiftRegister();

		//強制的にロックファイルとメッセージキューを削除してすぐに終わる
		UnInitServeRecive(SERVER_PROCESS);

		MySysLog(LOG_DEBUG, "my pid %d  | pkill cas\n", getpid() );
		system("pkill cas");
	}

	//システムログのクローズ
	closelog();

	return 0;
}
