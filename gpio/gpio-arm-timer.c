
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
#include "gpio-arm-timer.h"

char *armTimer_map = NULL;
volatile unsigned int *armTimer = NULL;

//分解能が入る 1なら詳細 0なら通常
unsigned int armTimerResolution;
unsigned int sleepLimit1;
unsigned int sleepLimit2;
unsigned int sleepLimit3;

//単位が1usか0.1us、0.1usのときはどうしても2counter分実際とずれる
//while( *(armTimer+ARM_TIMER_COUNTER) < over )が2counter分使う?ので使用するときは-2を忘れないようにする
void DelayArmTimerCounter(unsigned int delayCount)
{
	unsigned int over;

	if( armTimer == NULL || delayCount == 0)
		return;
		
	//unsigned int test;
	//test = *(armTimer+ARM_TIMER_COUNTER);
		
	//ある程度大きい数をカウンタのループで監視するとcpuの処理を使いすぎるので
	//sleepを混ぜ、その間に進んだカウンタ分を計測し、
	//元のカウンタからスリープ中に進んだ分を差し引きして1ms以下になったら監視を開始する
	over = *(armTimer+ARM_TIMER_COUNTER)+delayCount;
	
	if( delayCount > sleepLimit1 )
	{
		int waitCountStart;
		//over 1s
		while( delayCount >= sleepLimit3 )
		{
			waitCountStart	= *(armTimer+ARM_TIMER_COUNTER);
			usleep(SLEEP_WAIT_3);
			delayCount		-= *(armTimer+ARM_TIMER_COUNTER) - waitCountStart;
		}
		//over 100ms
		while( delayCount >= sleepLimit2 )
		{
			waitCountStart	= *(armTimer+ARM_TIMER_COUNTER);
			usleep(SLEEP_WAIT_2);
			delayCount		-= *(armTimer+ARM_TIMER_COUNTER) - waitCountStart;
		}
		//over 1ms
		while( delayCount >= sleepLimit1 )
		{
			waitCountStart	= *(armTimer+ARM_TIMER_COUNTER);
			usleep(SLEEP_WAIT_1);
			delayCount		-= *(armTimer+ARM_TIMER_COUNTER) - waitCountStart;
		}
	}
	while( *(armTimer+ARM_TIMER_COUNTER) < over )
		;
	
	//printf("diff %u\n", *(armTimer+ARM_TIMER_COUNTER) - test);
	return;
}
unsigned int GetArmTimer()
{
	return *(armTimer+ARM_TIMER_COUNTER);
}
unsigned int ArmTimerSetFreeScale(unsigned int divider)
{
	//datasheet p197
	//sys_clk/scale+1 sys_clk=250MHz?
	
	//int lim = 1;
	//int over;
	//for(i=0; i<5; i++){
	//	ArmTimerDprintf("Arm Timer Precision Test\n");
	//	lim = 100;
	//	clock_gettime(CLOCK_MONOTONIC, &startTs);
	//	
	//	tmp = *(armTimer+ARM_TIMER_COUNTER);
	//	over = tmp+lim;
	//	while( *(armTimer+ARM_TIMER_COUNTER) < over )
	//		;
	//	diff = *(armTimer+ARM_TIMER_COUNTER);
	//	
	//	clock_gettime(CLOCK_MONOTONIC, &endTs);
	//	TimeDiff(&startTs, &endTs, 0);
	//	ArmTimerDprintf("%u\t\t%u\n", tmp, diff);
	//}
	//で試したら 249->およそ1ms だったので 
	// 250MHz / ( divider+1 ) の計算式っぽい
	
	// s=(divider+1)/250MHz, divider=250*s-1
	// 使用bitが8bitのためmax 255
	// 端数が出て使いづらいので 1msか0.1msのどちらかにする
	switch( divider )
	{
		case 1:
			//詳細の時には0.1ms
			SetRegisterBit(armTimer+ARM_TIMER_C, ARM_TIMER_C_REGISTER_FREE_SCALER, ARM_TIMER_C_FREE_SCALER_USE_BIT, 24);
			sleepLimit1	= SLEEP_LIMIT_1*10;
			sleepLimit2	= SLEEP_LIMIT_2*10;
			sleepLimit3	= SLEEP_LIMIT_3*10;
			break;
		case 0:
		default:
			//標準では1msにする
			SetRegisterBit(armTimer+ARM_TIMER_C, ARM_TIMER_C_REGISTER_FREE_SCALER, ARM_TIMER_C_FREE_SCALER_USE_BIT, 249);
			sleepLimit1	= SLEEP_LIMIT_1;
			sleepLimit2	= SLEEP_LIMIT_2;
			sleepLimit3	= SLEEP_LIMIT_3;
			break;
	}
	return 1;
}
void PrintArmTimerRegister()
{
	printf("%u\n", *armTimer);
	//桁を合わせるための調整
	printf("      ");
	PrintRegStatus(stdout, armTimer, ARM_TIMER_LOAD    , "ARM_TIMER_LOAD        ",	1);
	PrintRegStatus(stdout, armTimer, ARM_TIMER_VALUE   , "ARM_TIMER_VALUE       ",	0);
	PrintRegStatus(stdout, armTimer, ARM_TIMER_C       , "ARM_TIMER_C           ",	0);
	PrintRegStatus(stdout, armTimer, ARM_TIMER_IRQ     , "ARM_TIMER_IRQ         ",	0);
	PrintRegStatus(stdout, armTimer, ARM_TIMER_RAW_IRQ , "ARM_TIMER_RAW_IRQ     ",	0);
	PrintRegStatus(stdout, armTimer, ARM_TIMER_MASK_IRQ, "ARM_TIMER_MASK_IRQ    ",	0);
	PrintRegStatus(stdout, armTimer, ARM_TIMER_RELOAD  , "ARM_TIMER_RELOAD      ",	0);
	PrintRegStatus(stdout, armTimer, ARM_TIMER_DIVIDER , "ARM_TIMER_DIVIDER     ",	0);
	PrintRegStatus(stdout, armTimer, ARM_TIMER_COUNTER , "ARM_TIMER_COUNTER     ",	0);

	return;
}
void PrintArmTimerCounter()
{
	//桁を合わせるための調整
	printf("      ");
	PrintRegStatus(stdout, armTimer, ARM_TIMER_COUNTER , "ARM_TIMER_COUNTER     ",	1);
	return;
}
int InitArmTimer(ArmTimerRes res)
{
	int mem_fd;

	if( armTimer != NULL )
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

	armTimer_map = (char *)mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, ARM_TIMER_BASE);
	if( armTimer_map == MAP_FAILED )
	{
		printf("mmap error %d\n", (int)armTimer_map);
		return -1;
	}
	ArmTimerDprintf("mmap ARM_TIMER_BASE %s %X\n", strerror(errno), (int32_t)armTimer_map);

	close(mem_fd);

	armTimer = (volatile unsigned int *)armTimer_map;
	
	//ここでタイマの分解能を1msにするか0.1msにするか選択
	ArmTimerSetFreeScale(res);
	armTimerResolution = 1;
	SetRegisterBit(armTimer+ARM_TIMER_C, ARM_TIMER_C_REGISTER_FREE, 1, 1);
	
	//PrintArmTimerRegister();
	return 1;
}
int UnInitArmTimer()
{
	SetRegisterBit(armTimer+ARM_TIMER_C, ARM_TIMER_C_REGISTER_FREE, 1, 0);
	//分解能をクリア
	ArmTimerSetFreeScale(0);
	
	//PrintArmTimerRegister();
	
	//取得したSPIの箇所の仮想アドレスの解放
	munmap(armTimer_map, BLOCK_SIZE);

	armTimer_map = NULL;
	armTimer = NULL;

	return 1;
}

#ifdef ARM_TIMER_DEBUG
void ArmTimerPrecisionTest()
{
	uint32_t tmp, diff, i, avg;
	struct timespec startTs, endTs, sleepTs;
	
    sleepTs.tv_sec = 0;
    sleepTs.tv_nsec = 1000;
	/*clock_nanosleep(CLOCK_MONOTONIC, 0, &sleepTs, NULL);*/
	
	//clock_gettime(CLOCK_MONOTONIC, &startTs);
	//ArmTimerDprintf("armtem timer %u\n", *(armTimer+ARM_TIMER_COUNTER) );
	//for(i=0; i<100; i++)
	//{
	//	//ArmTimerDprintf("armtem timer %u\n", *(armTimer+ARM_TIMER_COUNTER) );
	//    tmp = *(armTimer+ARM_TIMER_COUNTER);
	//    clock_nanosleep(CLOCK_MONOTONIC, 0, &sleepTs, NULL);
	//    diff = *(armTimer+ARM_TIMER_COUNTER) - tmp;
	//	//ArmTimerDprintf("armtem timer diff tmp %u\n", diff);
	//	avg += diff;
	//}
	//clock_gettime(CLOCK_MONOTONIC, &endTs);
	//TimeDiff(&startTs, &endTs, 0);
	
	int lim = 1;
	int over;
	long sum;
	//ArmTimerDprintf("Arm Timer Precision Test\n");
	for(i=0; i<1000; i++){
		lim = 10;
		clock_gettime(CLOCK_MONOTONIC, &startTs);
		
		tmp = *(armTimer+ARM_TIMER_COUNTER);
		over = tmp+lim;
		while( *(armTimer+ARM_TIMER_COUNTER) < over )
			;
		diff = *(armTimer+ARM_TIMER_COUNTER);
		
		clock_gettime(CLOCK_MONOTONIC, &endTs);
		//TimeDiff(&startTs, &endTs, 0);
		sum += endTs.tv_nsec - startTs.tv_nsec;
		//ArmTimerDprintf("%u\t\t%u\n", tmp, diff);
	}
	ArmTimerDprintf("Arm Timer Test\t\tsum %ld\n", sum);
	//ArmTimerDprintf("armtem timer diff add %u avg %u\n", avg, avg/(i-1) );
	
	return;
}
#endif
		