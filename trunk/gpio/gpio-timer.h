
#ifndef GPIO_TIMER_H
#define GPIO_TIMER_H

#include "gpio.h"

//#define SYS_TIMER_DEBUG
#if defined(SYS_TIMER_DEBUG) || defined(GPIO_DEBUG)
	#define SysTimerDprintf	printf
#else
	#define SysTimerDprintf
#endif

#define SYS_TIMER_BASE	( BCM2708_PERI_BASE + 0x00003000) //=0x7E00 3000
#define SYS_TIMER_CS    0x0000                   //System Timer Control/Status
#define SYS_TIMER_CLO   0x0004/sizeof(uint32_t)  //System Timer Counter Lower 32bit
#define SYS_TIMER_CHI   0x0008/sizeof(uint32_t)  //System Timer Counter Higher 32bit
#define SYS_TIMER_C0    0x000C/sizeof(uint32_t)  //System Timer Compare 0
#define SYS_TIMER_C1    0x0010/sizeof(uint32_t)  //System Timer Compare 1
#define SYS_TIMER_C2    0x0014/sizeof(uint32_t)  //System Timer Compare 2
#define SYS_TIMER_C3    0x0018/sizeof(uint32_t)  //System Timer Compare 3

//CS Register 4-31 Reserved
#define SYS_TIMER_REGISTER_M3  //System Timer Match 0->最後に0にしてから一致してない 1->一致
#define SYS_TIMER_REGISTER_M2  //同上(timer 2)
#define SYS_TIMER_REGISTER_M1  //同上(timer 1)
#define SYS_TIMER_REGISTER_M0  //同上(timer 0)

/*
 *  SYS_TIMER_CLOとSYS_TIMER_CHIにシステムタイマー値が入っていてそれを読み取れば良い
 *  SYS_TIMER_Cxを1にして C0-C3のレジスタに下位32bitの値を入れシステムタイマー値と一致すると
 *  割り込みが発生?
 */


void DelayMicroSecond(unsigned int delay);
unsigned int GetSysCounter();
void PrintSysTimerRegister();
int InitSysTimer();
int UnInitSysTimer();

#ifdef SYS_TIMER_DEBUG
void SysTimerPrecisionTest();
#endif


#endif