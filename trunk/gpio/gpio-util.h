
#ifndef GPIO_UTIL_H
#define GPIO_UTIL_H


//clock_gettime gccで -lrtをつけないとコンパイルエラー
#include <time.h>

//#define UTIL_DEBUG
#if defined(UTIL_DEBUG) || defined(GPIO_DEBUG)
	#define UtilDprintf printf
#else
	#define UtilDprintf
#endif

#define LOG_FILE	"/var/log/gpio.log"

#define ARRAY_SIZE(a) ( sizeof(a) / sizeof((a)[0]) )

//32bitをしてされたbit数で区切ってfpへ表示
void PrintUintDelimiter(FILE *fp, unsigned int bin, int col);
void PrintGpioStatus();		//gpioレジスタの内容をすべて表示
void PrintGpioPinMode();	//モード部分のみ
void PrintGpioLevStatus();	//現在のHigh,Lowの値のみ

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

#define S_TO_NANOSECOND(x)	x*1e+9
static inline int TimespecDiff(struct timespec *startTs, struct timespec *endTs, struct timespec *diff)
{
	if( startTs == NULL || endTs == NULL || diff == NULL )
		return -1;

	diff->tv_sec  = endTs->tv_sec  - startTs->tv_sec;
	diff->tv_nsec = endTs->tv_nsec - startTs->tv_nsec;
	
	//nsecが0以下の場合はsecを桁借り(borrow)をする
	if( diff->tv_nsec < 0 )
	{
		--( diff->tv_sec );
		diff->tv_nsec += S_TO_NANOSECOND(1);
	}
	return 1;
}
static inline void DispTimespecDiff(struct timespec *diff)
{
	printf("\t  s: %10d  ns: %10d\n", diff->tv_sec, diff->tv_nsec );
}
static inline void DispTimespecDiffAll(struct timespec *startTs, struct timespec *endTs, struct timespec *diff)
{
	printf("\t  s: start %10d, end %10d, diff %10d\n", startTs->tv_sec, endTs->tv_sec, diff->tv_sec );
	printf("\t ns: start %10d, end %10d, diff %10d\n", startTs->tv_nsec, endTs->tv_nsec, diff->tv_nsec );
}


#endif