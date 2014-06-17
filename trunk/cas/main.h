
#ifndef MAIN_H
#define MAIN_H

#define LOCK_FILE				"/tmp/cas.lock"
#define APPNAME					"clock&sensor"
#define MSG_QUEUE_NAME			"/mq-cas"
//本来はメッセージのバッファの長さはシステムが決めるが
//とりあえず使用する長さを仮決定する MODE_XX で10で充分
#define MSG_QUEUE_LEN			10
#define MSG_QUEUE_MODE_PREFIX	"MODE_"

#define LOG_DIR					"/var/log/cas/"


#define SERVER_PROCESS	0
#define CLIENT_PROCESS	1

typedef enum {
	MODE_CLOCK		= 0x0001,	//c 時計
	MODE_YEAR		,			//y 年
	MODE_DATE		,			//d 日付
	MODE_PRESS		,			//p 大気圧
	MODE_TEMP		,			//t 温度
	MODE_LUX		,			//l 照度
	MODE_HUMIDITY	,			//h 湿度
	MODE_CORE_TEMP	,			//T CPU温度
	MODE_WAIT_LOG	,			//w	次のログまでの待機時間
	
	MODE_OUTPUT		,			//o 入力された文字を出力 番兵としても使う
	MODE_ANI_0		= 0x0020,	//a 0 アニメーション1
	MODE_ANI_1		,			//a 1 アニメーション2
	MODE_ANI_2		,			//a 2 アニメーション3
	MODE_QUIT					//q 全体の番兵
}ModeSelect;

//データ取得間隔 初期値1分間隔
#define DATA_INTERVAL_DEF	60
#define DATA_INTERVAL_MIN	1
//データログ間隔 初期値2分間隔 0でログを保存しない
#define LOG_INTERVAL_DEF	120

//初期値 0だと気温が0度の時に誤作動するのでありえない-100にする
#define INIT_SENSOR			-100

#ifdef SYS_LOG
	#define MySysLog(LEVEL, str, ...)	syslog(LEVEL, str, ##__VA_ARGS__)
#else
	#define MySysLog(LEVEL, str, ...)	DebugOptPrintf(str, ##__VA_ARGS__)
#endif
#ifdef RELEASE
	#define DebugOptPrintf(str, ...)
#else
	#define DebugOptPrintf(str, ...)	Dprintf(str, ##__VA_ARGS__)
#endif
void Dprintf(const char *str, ...);

void PinInit();
void PinUnInit();

//interrupt
int GpioInterruptCallbackFunc(int pin, int value);


//メッセージループ内でのメッセージの処理
int MsgDisposition(char *msg);
//サーバ側 メッセージループ(各スレッドの起動)
void ReciveQueue();
//クライアント側 メッセージを送ったら終了
void SendQueue(char *sendData);
//サーバとクライアントのどちらになるかの判断
int InitServeRecive(int debug, int sensorOutLevel);
int UnInitServeRecive(int processStatus);

#endif
