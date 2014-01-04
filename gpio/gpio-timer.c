 
#include <stdio.h>
#include <stdlib.h>
//open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//mmap
#include <sys/mman.h>
//errno
#include <errno.h>

//clock_gettime gccで -lrtをつけないとコンパイルエラー
#include <time.h>

#include <stdint.h>

#include "gpio.h"
#include "gpio-util.h"
#include "gpio-timer.h"

char *sysTimer_map = NULL;
volatile unsigned int *sysTimer = NULL;

//10ms以上をカウンタで監視すると7segがちらつく
//ある程度大きい数ならclock_nanosleepにする
#define SYS_SLEEP	1000
void DelayMicroSecond(unsigned int delayMicroSecond)
{
	uint32_t delayedTime, start, hiCounter;
	
	if( sysTimer == NULL || delayMicroSecond == 0)
		return;
	
	//念のため先にハイカウンタを取得
	hiCounter   = *(sysTimer+SYS_TIMER_CHI);
	
	start       = *(sysTimer+SYS_TIMER_CLO);
	delayedTime = start + delayMicroSecond;
	SysTimerDprintf("start time %u,  delay %u,  delayed time %u\n", start, delayMicroSecond, delayedTime );
	
	//単位はマイクロ?
	if( delayMicroSecond > SYS_SLEEP )
	{
		int waitCountStart;
		while( delayMicroSecond >= SYS_SLEEP )
		{
			waitCountStart		= *(sysTimer+SYS_TIMER_CLO);
			usleep(250);				
			delayMicroSecond	-= *(sysTimer+SYS_TIMER_CLO) - waitCountStart;
		}
	}
	//ディレイした数字が開始時より少ない場合は桁あふれをしてるのでハイカウンタがカウントされるのを待つ
	if( delayedTime < start )
	{
		//ハイカウンタがカウントされるまで待機
		while( *(sysTimer+SYS_TIMER_CHI) > hiCounter )
			;
	}
	while( *(sysTimer+SYS_TIMER_CLO) < delayedTime )
		;
	
	//printf("diff %u\n", *(sysTimer+SYS_TIMER_CLO)-start);
	
	return;
}
void PrintSysTimerRegister()
{
	PrintRegStatus(stdout, sysTimer, SYS_TIMER_CS,  "SYS_TIMER_CS    ",	1);
	PrintRegStatus(stdout, sysTimer, SYS_TIMER_CLO,	"SYS_TIMER_CLO   ",	0);
	PrintRegStatus(stdout, sysTimer, SYS_TIMER_CHI,	"SYS_TIMER_CHI   ",	0);
	PrintRegStatus(stdout, sysTimer, SYS_TIMER_C0,	"SYS_TIMER_C0    ",	0);
	PrintRegStatus(stdout, sysTimer, SYS_TIMER_C1,	"SYS_TIMER_C1    ",	0);
	PrintRegStatus(stdout, sysTimer, SYS_TIMER_C2,	"SYS_TIMER_C2    ",	0);
	PrintRegStatus(stdout, sysTimer, SYS_TIMER_C3,	"SYS_TIMER_C3    ",	0);
	
	return;
}
int InitSysTimer()
{
	int mem_fd;
	
	if( sysTimer != NULL )
		return 1;

	if( gpio == NULL )
	{
		perror("gpio NULL");
		return -1;
	}

	if( (mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0 )
	{
		printf("can't open /dev/mem: %s\n", strerror(errno));
		return -1;
	}

	sysTimer_map = (char *)mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, SYS_TIMER_BASE);
	if( sysTimer_map == MAP_FAILED )
	{
		printf("mmap error %d\n", (int)sysTimer_map);
		return -1;
	}
	SysTimerDprintf("mmap SYS_TIMER_BASE %s %X\n", strerror(errno), (int32_t)sysTimer_map);

	close(mem_fd);

	sysTimer = (volatile unsigned int *)sysTimer_map;

	return 1;
}
int UnInitSysTimer()
{
	//取得したSPIの箇所の仮想アドレスの解放
	munmap(sysTimer_map, BLOCK_SIZE);
	
	sysTimer_map = NULL;
	sysTimer = NULL;

	return 1;
}

#ifdef SYS_TIMER_DEBUG
void SysTimerPrecisionTest()
{
	uint32_t tmp, diff, i, avg;
	struct timespec startTs, endTs, sleepTs;
    sleepTs.tv_sec = 0;
    sleepTs.tv_nsec = 1000;
	//clock_nanosleep(CLOCK_MONOTONIC, 0, &sleepTs, NULL);
	
	//これでやったら平均17、合計1754、時間1772933になった
	//時間がnsなのでusに直すと1772.9
	//1カウンタ1usでよさげ
	//clock_gettime(CLOCK_MONOTONIC, &startTs);
	//for(i=0; i<100; i++)
	//{
	//    tmp = *(sysTimer+SYS_TIMER_CLO);
	//    clock_nanosleep(CLOCK_MONOTONIC, 0, &sleepTs, NULL);
	//    diff = *(sysTimer+SYS_TIMER_CLO) - tmp;
	//	//SysTimerDprintf("system timer diff tmp %u\n", diff);
	//	avg += diff;
	//}
	//clock_gettime(CLOCK_MONOTONIC, &endTs);
	//TimeDiff(&startTs, &endTs, 0);
	
	int lim = 1;
	int over;
	long sum;
	//SysTimerDprintf("System Timer Precision Test\n");
	for(i=0; i<1000; i++){
		lim = 10;
		clock_gettime(CLOCK_MONOTONIC, &startTs);
		
		tmp = *(sysTimer+SYS_TIMER_CLO);
		over = tmp+lim;
		while( *(sysTimer+SYS_TIMER_CLO) < over )
			;
		diff = *(sysTimer+SYS_TIMER_CLO);
		
		clock_gettime(CLOCK_MONOTONIC, &endTs);
		//TimeDiff(&startTs, &endTs, 0);
		sum += endTs.tv_nsec - startTs.tv_nsec;
		//SysTimerDprintf("%u\t\t%u\n", tmp, diff);
	}
	SysTimerDprintf("system Timer Test\t\tsum %ld\n", sum);
	//SysTimerDprintf("system timer diff add %u avg %u\n", avg, avg/(i-1) );
	
	return;	
}
#endif
		