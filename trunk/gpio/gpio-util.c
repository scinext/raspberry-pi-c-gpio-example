
#include <stdio.h>
#include <stdlib.h>

//errno
#include <errno.h>

//sched_get_priority_min sched_get_priority_max sched_setscheduler
#include <sched.h>
//getpriority PRIO_PROCESS
#include <sys/resource.h>

//va_...
#include <stdarg.h>

#include "gpio.h"
#include "gpio-util.h"


#define INTSIZE 32
void PrintUintDelimiter(FILE *fp, unsigned int bin, int col)
{
	int i;
	int delimiter = 0;
	int temp;
	const int intSize = INTSIZE;
	char buf[50] = {'\0'};
	
	if( fp == NULL)
		fp = stdout;
		
	fprintf(fp, "%10d = ", bin);
	for(i=intSize; i>0; --i)
	{
		//区切る
		if( i % col == 0)
		{
			buf[intSize - i + delimiter] = ' ';
			++delimiter;
		}

		//最下位ビットの取得
		temp = (bin >> i-1) & 1 ;

		//1なら'0'に+1されて'1'になる(文字コードが繋がっている前提)
		buf[intSize - i + delimiter] = '0' + temp;
	}

	fprintf(fp, "%s\n", buf);
	return;
}

void PrintGpioStatus()
{
	int i;
	int initGpfsel, initEtc;
	initGpfsel = initEtc = 0;
	for( i=0; i<40; ++i)
	{
		if( i <= 5 && initGpfsel == 0 )
		{
			printf("                                          R   9   8   7   6   5   4   3   2   1   0\n");
			initGpfsel = 1;
		}
		else if( i >=6 && initEtc == 0 )
		{
			initEtc = 1;
			printf("                                             28   24   20   16   12    8    4    0\n");
		}
		switch(i)
		{
			case 0 : 
			case 1 : 
			case 2 : 
			case 3 : 
			case 4 : 
			case 5 : 
						printf("GPFSEL%d        ", i); break;	//GPIO Function Select 5
			case 7 :	printf("GPSET0          "); break;		//GPIO Pin Output Set 0
			case 8 :	printf("GPSET1          "); break;		//GPIO Pin Output Set 1
			case 10:	printf("GPCLR0          "); break;		//GPIO Pin Output Clear 0
			case 11:	printf("GPCLR1          "); break;		//GPIO Pin Output Clear 1
			case 13:	printf("GPLEV0          "); break;		//GPIO Pin Level 0
			case 14:	printf("GPLEV1          "); break;		//GPIO Pin Level 1
			case 16:	printf("GPEDS0          "); break;		//GPIO Pin Event Detect Status 0
			case 17:	printf("GPEDS1          "); break;		//GPIO Pin Event Detect Status 1
			case 19:	printf("GPREN0          "); break;		//GPIO Pin Rising Edge Detect Enable 0
			case 20:	printf("GPREN1          "); break;		//GPIO Pin Rising Edge Detect Enable 1
			case 22:	printf("GPFEN0          "); break;		//GPIO Pin Falling Edge Detect Enable 0
			case 23:	printf("GPFEN1          "); break;		//GPIO Pin Falling Edge Detect Enable 1
			case 25:	printf("GPHEN0          "); break;		//GPIO Pin High Detect Enable 0
			case 26:	printf("GPHEN1          "); break;		//GPIO Pin High Detect Enable 1
			case 28:	printf("GPLEN0          "); break;		//GPIO Pin Low Detect Enable 0
			case 29:	printf("GPLEN1          "); break;		//GPIO Pin Low Detect Enable 1
			case 31:	printf("GPAREN0         "); break;		//GPIO Pin Async. Rising Edge Detect 0
			case 32:	printf("GPAREN1         "); break;		//GPIO Pin Async. Rising Edge Detect 1
			case 34:	printf("GPAFEN0         "); break;		//GPIO Pin Async. Falling Edge Detect 0
			case 35:	printf("GPAFEN1         "); break;		//GPIO Pin Async. Falling Edge Detect 1
			case 37:	printf("GPPUD           "); break;			//GPIO Pin Pull-up/down Enable
			case 38:	printf("GPPUDCLK0       "); break;		//GPIO Pin Pull-up/down Enable Clock 0
			case 39:	printf("GPPUDCLK1       "); break;		//GPIO Pin Pull-up/down Enable Clock 1
			default:	printf("Reserved        "); break;
		}
		printf("0x%X = ", (gpio+i));
		i <= 5 ? PrintUintDelimiter(stdout, *(gpio + i), 3 ) : PrintUintDelimiter(stdout, *(gpio + i), 4 );
	}
	printf("\n");
}

void PrintGpioPinMode()
{
	int i;
	printf("                                          R   9   8   7   6   5   4   3   2   1   0\n");
	for( i=0; i<=5; ++i)
	{
		//GPIO Function Select 5
		printf("GPFSEL%d        ", i);
		printf("0x%X = ", (gpio+i));
		PrintUintDelimiter(stdout, *(gpio + i), 3 );
	}
	printf("\n");
	//PrintRegStatus(stdout, gpio, 0, "GPFSEL1         ", 1);
	//PrintRegStatus(stdout, gpio, 1, "GPFSEL2         ", 0);
	//PrintRegStatus(stdout, gpio, 2, "GPFSEL3         ", 0);
	//PrintRegStatus(stdout, gpio, 3, "GPFSEL4         ", 0);
	//PrintRegStatus(stdout, gpio, 5, "GPFSEL5         ", 0);
}
void PrintGpioLevStatus()
{
	PrintRegStatus(stdout, gpio, GPIO_LEV_0, "GPLEV0          ", 1);
	PrintRegStatus(stdout, gpio, GPIO_LEV_1, "GPLEV1          ", 0);
}

void PrintRegStatus(FILE *fp, volatile unsigned int *reg, int addr, char *text, int dispDigit)
{
	int log = 0;
	if( fp == NULL )
	{
		fp = fopen(LOG_FILE, "a");
		log = 1;
	}
	
	if( fp != NULL )
	{
		if( dispDigit == 1 )
			fprintf(fp, "                                             28   24   20   16   12    8    4    0\n");
	
		if( text != NULL )
			fprintf(fp, "%s", text);
		else
			fprintf(fp, "                ", text);
		
		fprintf(fp, "0x%X = ", (reg + addr) );
		PrintUintDelimiter(fp, *(reg + addr), 4 );
		
		//log出力だったら
		if( log == 1 )
			fclose(fp);
	}
}
void PrintLog(FILE *fp, const char *str, ...)
{
	va_list args;
	
	va_start(args, str);
	
	if( fp != NULL )
	{
		vfprintf(fp, str, args);
	}
	else
	{
		fp = fopen(LOG_FILE, "a");
		if( fp != NULL )
		{
			vfprintf(fp, str, args);
			fclose(fp);
		}
		else
		{
			printf(LOG_FILE "%s open error\n");
			vprintf(str, args);
		}
	}
	va_end(args);
}


#define TIME_DETAILE
//#define TIME_DETAILE_MORE
unsigned int TimeDiff(struct timespec *startTs, struct timespec *endTs, long sleep)
{
	long diffNanoSecond, diffSecond;
	long diffSleep;
	struct timespec tmpTs1, tmpTs2, useTs;

	TimespecDiff(startTs, endTs, &useTs);
	
	diffSecond		= useTs.tv_sec;
	diffNanoSecond	= useTs.tv_nsec;
	diffSleep		= diffNanoSecond - sleep;
	
	#if defined(TIME_DETAILE) || defined(TIME_DETAILE_MORE)
		printf("     time diff:%ld  sleep:%ld   d-s= %ld\n", diffNanoSecond, sleep, diffSleep);
	#endif

	#ifdef TIME_DETAILE_MORE
		printf("     start  : %10ld %10ld\n", startTs->tv_sec, startTs->tv_nsec);
		printf("     end    : %10ld %10ld\n", endTs->tv_sec, endTs->tv_nsec);
		printf("         second diff %ld %ld\n", diffSecond, diffNanoSecond);

		clock_gettime(CLOCK_MONOTONIC, &tmpTs1);
		clock_gettime(CLOCK_MONOTONIC, &tmpTs2);
		TimespecDiff(tmpTs1, tmpTs2, &useTs);
		diffSecond		= endTs->tv_sec - startTs->tv_sec - useTs.tv_sec;
		diffNanoSecond	= endTs->tv_nsec - startTs->tv_nsec - useTs.tv_nsec;
		printf("         clock_gettime use time %ld %ld\n", useTs.tv_sec, useTs.tv_nsec);
		printf("         second clock_gettime use time diff %ld %ld\n", diffSecond, diffNanoSecond);
	#endif

	//スリープとの差をナノ秒の絶対値として返す
	if( diffSleep < 0 )
		diffSleep *= -1;
	return diffSleep;
}

//ある程度大きい数ならclock_nanosleepにする
#define SMALL_SLEEP	1000000
//5us(5000ns)の遅延が発生する
#define DELAY_HIGH	0
void DelayNanoSecond(unsigned long delayNanoSecond)
{
	struct timespec startTs, endTs;
	unsigned long delay;
	
	clock_gettime(CLOCK_MONOTONIC, &startTs);
	if( delayNanoSecond < SMALL_SLEEP )
	{
		delay = delayNanoSecond - DELAY_HIGH;
		do{
			clock_gettime(CLOCK_MONOTONIC, &endTs);
		}while( endTs.tv_nsec < startTs.tv_nsec + delay );
		return;
	}
	else
	{
		struct timespec  sleepTs;
	    sleepTs.tv_sec = startTs.tv_sec;
	    sleepTs.tv_nsec = startTs.tv_nsec + delayNanoSecond;
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepTs, NULL);
		return;
	}
}

//1-2us(1-2000ns)の遅延
#define DELAY_LOW	1500
void DelayNanoSecondLow(struct timespec *startTs, unsigned long delayNanoSecond)
{	
	struct timespec endTs;
	unsigned long delay;
	if( delayNanoSecond < SMALL_SLEEP )
	{
		delay = delayNanoSecond - DELAY_LOW;
		do{
			clock_gettime(CLOCK_MONOTONIC, &endTs);
		}while( endTs.tv_nsec < startTs->tv_nsec + delay );
		return;
	}
	else
	{
		struct timespec  sleepTs;
	    sleepTs.tv_sec = startTs->tv_sec;
	    sleepTs.tv_nsec = startTs->tv_nsec + delayNanoSecond;
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepTs, NULL);
		return;
	}
}


int SetPriority(int mode)
{
	static int normalPrio	= -1;
	static int normalPolicy	= -1;
	struct sched_param schedParam;

	/* //分解能の確認 CLOCK_MONOTONIC 1ns CLOCK_MONOTONIC_COARSE 10000000ns
	struct timespec ts;
	clock_getres(CLOCK_MONOTONIC, &ts);
	printf("CLOCK_MONOTONIC: %ldns\n", ts.tv_nsec);
	clock_getres(CLOCK_MONOTONIC_COARSE, &ts);
	printf("CLOCK_MONOTONIC_COARSE: %ldns\n", ts.tv_nsec);
	*/
	if( mode == HIGH_PRIO )
	{
		int minPrio, maxPrio;

		UtilDprintf("set high priority\n");
		//現在のプログラム実行の優先度と種類の取得 pid=0で自プロセス
		normalPolicy = sched_getscheduler(0);
		if( normalPolicy == -1 )
		{
			perror("get priority");
			return -1;
		}
		normalPrio = getpriority(PRIO_PROCESS, 0);
		UtilDprintf("now policy %d, priority %d\n", normalPolicy, normalPrio);

		//システムの優先度の最大/最少を取得
		minPrio = sched_get_priority_min(SCHED_FIFO);
		maxPrio = sched_get_priority_max(SCHED_FIFO);
		UtilDprintf("priority min %d, max %d\n", minPrio, maxPrio);
		schedParam.sched_priority = maxPrio;
		if( sched_setscheduler(0, SCHED_FIFO, &schedParam) != 0 )
		{
			perror("set high sched priority");
			return -1;
		}
		UtilDprintf("now policy %d, priority %d\n", SCHED_FIFO, maxPrio);
	}
	else
	{
		UtilDprintf("set normal priority\n");

		if( normalPrio == -1 || normalPolicy == -1)
			return 0;

		//システムの優先度の最大/最少を取得
		schedParam.sched_priority = normalPrio;
		if( sched_setscheduler(0, normalPolicy, &schedParam) != 0 )
		{
			perror("set normal sched priority");
			return -1;
		}

		normalPolicy = sched_getscheduler(0);
		if( normalPolicy == -1 )
		{
			perror("get priority");
			return -1;
		}
		normalPrio = getpriority(PRIO_PROCESS, 0);
		UtilDprintf("now policy %d, priority %d\n", normalPolicy, normalPrio);
	}

}
