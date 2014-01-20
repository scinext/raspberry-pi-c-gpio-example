
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
	MODE_CLOCK		= 0x0001,		//c 時計
	MODE_YEAR		= 0x0002,		//y 年
	MODE_DATE		= 0x0003,		//d 日付
	MODE_PRESS		= 0x0004,		//p 大気圧
	MODE_TEMP		= 0x0005,		//t 温度
	MODE_LUX		= 0x0006,		//l 照度
	MODE_HUMIDITY	= 0x0007,		//h 湿度
	MODE_OUTPUT		= 0x0008,		//o 入力された文字を出力
	MODE_ANI_0		= 0x000A,		//a 0 アニメーション1
	MODE_ANI_1		= 0x000B,		//a 1 アニメーション2
	MODE_ANI_2		= 0x000C,		//a 2 アニメーション3
	MODE_QUIT						//q 番兵
}ModeSelect;

//データ取得間隔 初期値1分間隔
#define DATA_INTERVAL_DEF	60
#define DATA_INTERVAL_MIN	1
//データログ間隔 初期値2分間隔 0でログを保存しない
#define LOG_INTERVAL_DEF	120


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
