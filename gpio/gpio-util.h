
#ifndef GPIO_UTIL_H
#define GPIO_UTIL_H


//clock_gettime gccで -lrtをつけないとコンパイルエラー
#include <time.h>

//#define UTIL_DEBUG
#ifdef UTIL_DEBUG
	#define UtilDprintf printf
#else
	#define UtilDprintf
#endif

#define LOG_FILE	"/var/log/gpio.log"

#define ARRAY_SIZE(a) ( sizeof(a) / sizeof((a)[0]) )

//32bitをしてされたbit数で区切ってfpへ表示
void PrintUintDelimiter(FILE *fp, unsigned int bin, int col);
//gpioレジスタの内容をすべて表示
void PrintGpioStatus(volatile unsigned int *gpio);
//モード部分のみ
void PrintGpioPinMode(volatile unsigned int *gpio);
//現在のHigh,Lowの値のみ
void PrintGpioLevStatus(volatile unsigned int *gpio);
////レジスタの内容の表示
//void PrintRegStatus(volatile unsigned int *reg, int addr, char *text, int dispDigit);
////上記の内容をLogに表示
//void PrintRegStatusLog(volatile unsigned int *reg, int addr, char *text, int dispDigit);
//fpがnullの場合はlogに出力それ以外は指定したfpに出力
void PrintRegStatus(FILE *fp, volatile unsigned int *reg, int addr, char *text, int dispDigit);

//fpがnullの場合はlogに出力それ以外は指定したfpに出力
void PrintLog(FILE *fp, const char *str, ...);


#define NOMAL_PRIO	0
#define HIGH_PRIO	1
int SetPriority(int mode);

void DelayNanoSecond(unsigned long delayNanoSecond);
void DelayNanoSecondLow(struct timespec *startTs, unsigned long delayNanoSecond);

unsigned int TimeDiff(struct timespec *startTs, struct timespec *endTs, long sleep);

static inline void TimeDiffSecond(struct timespec *startTs, struct timespec *endTs)
{
	unsigned int sec, nsec;
	if( endTs->tv_sec >= startTs->tv_sec )
		sec = endTs->tv_sec - startTs->tv_sec;
	else
		sec =  startTs->tv_sec - endTs->tv_sec;
	
	if( endTs->tv_nsec >= startTs->tv_nsec )
		nsec = endTs->tv_nsec - startTs->tv_nsec;
	else
		nsec = startTs->tv_nsec - endTs->tv_nsec;
		
	printf("s %u, us %u\n", sec, nsec/1000);
	return;
}

#define NANO	(nanoSecond)
#define MICRO	(nanoSecond * 1000)
#define MILLI	( MICRO(nanoSecond) * 1000 )

#endif