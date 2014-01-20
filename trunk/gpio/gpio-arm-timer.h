
#ifndef GPIO_ARM_TIMER_H
#define GPIO_ARM_TIMER_H

#include "gpio.h"

//#define ARM_TIMER_DEBUG
#ifdef ARM_TIMER_DEBUG
	#define ArmTimerDprintf	printf
#else
	#define ArmTimerDprintf
#endif

/* datasheet p196 */

#define ARM_TIMER_BASE	    ( BCM2708_PERI_BASE + 0x0000B000) //=0x7E00 B000
#define ARM_TIMER_LOAD      0x0400/sizeof(uint32_t)  //Timer Load register
#define ARM_TIMER_VALUE     0x0404/sizeof(uint32_t)  //Timer Value register Read Only
#define ARM_TIMER_C         0x0408/sizeof(uint32_t)  //Control
#define ARM_TIMER_IRQ       0x040C/sizeof(uint32_t)  //IRQ Clear/Ack Write only
#define ARM_TIMER_RAW_IRQ   0x0410/sizeof(uint32_t)  //Raw IRQ Read Only
#define ARM_TIMER_MASK_IRQ  0x0414/sizeof(uint32_t)  //Masked IRQRead Only
#define ARM_TIMER_RELOAD    0x0418/sizeof(uint32_t)  //Reload
#define ARM_TIMER_DIVIDER   0x041C/sizeof(uint32_t)  //Pre divider
#define ARM_TIMER_COUNTER   0x0420/sizeof(uint32_t)  //Free running counter

//Load Register
//ここに値を入れると自動でカウントダウンが始まる?
//Value Register
//Load registerに入れられた値からスタートした現在のカウントダウンされたタイマ値が入っている?

//Control register 24-31 unused, 10-15 unused, 4 unused
#define ARM_TIMER_C_REGISTER_FREE_SCALER    16	//フリーカウンタのスケール?( Freq= sys_clk/scale+1 )
	#define ARM_TIMER_C_FREE_SCALER_USE_BIT		8
	typedef enum _ArmTimerRes{
		ARM_TIMER_DEFAULT = 0,		//通常の分解能のタイマ(divider=249 -> 1/(250/(249+1)) -> 1ms
		ARM_TIMER_HI_RES = 1		//高性能の分解能のタイマ(divider=24 -> 1/(250/(24+1)) -> 0.1ms
	} ArmTimerRes;
#define ARM_TIMER_C_REGISTER_FREE           9	//フリーカウンタの有効 0->Disable 1->Enable
#define ARM_TIMER_C_REGISTER_HALT           8	//debug halted modeの時の動き 0->keep 1->halt
#define ARM_TIMER_C_REGISTER_ACTIVE         7	//タイマの有効 0->Disable 1->Enable
#define ARM_TIMER_C_REGISTER_MODE           6	//not use, runningモードの設定だが常にフリーモードらしい
#define ARM_TIMER_C_REGISTER_INT            5	//タイマー割り込みの有効 0->Disable 1->Enable
#define ARM_TIMER_C_REGISTER_SCALE          3	//pre-scale
	#define ARM_TIMER_C_SCALE_USE_BIT			2
	typedef enum _ArmTimerScale{
		ARM_TIMER_SCALE_NO=0x00,    //bin 0000 no pre scale
		ARM_TIMER_SCALE_16=0x01,	//bin 0001 clock / 16
		ARM_TIMER_SCALE_256=0x02,	//bin 0010 clock / 256
		ARM_TIMER_SCALE_1_2=0x03,	//bin 0011 clock / 1(SP804ではない機能)
	} ArmTimerScale;
#define ARM_TIMER_C_REGISTER_BITS           1	//カウンタのbit数 0->16bit 1->23bit
#define ARM_TIMER_C_REGISTER_WRAP           0	//not use, wrappin modeの設定常にwrapping modeらしい

//Raw IRQ register 1-31 unused Read only
#define ARM_TIMER_RAW_IRQ_REGISTER_PEND     0   //保留の割り込みがあるかどうか
//Masked IRQ 1-31 unused Read only
#define ARM_TIMER_MASK_REGISTER_IRQ         0   //割り込みラインが有効かどうか?

//Timer Reload
//Loadレジスタのコピー、通常ここには書き込まない?、valueレジスタのカウントが0になるとここが読み込まれる

//pre-divider 10-31 unused
#define ARM_TIMER_DIVIDER_REGISTER_VALUE    0   // clock = apb_clock /(pre divider+1) 0x7Dのときは126になる
	#define ARM_TIMER_DIVIDER_USE_BIT            9

//Free running counter 0-31 data

void DelayArmTimerCounter(unsigned int delayCount);
unsigned int GetArmTimer();

unsigned int ArmTimerSetFreeScale(unsigned int divider);
void PrintArmTimerRegister();
void PrintArmTimerCounter();
int InitArmTimer(ArmTimerRes res);
int UnInitArmTimer();

#ifdef ARM_TIMER_DEBUG
void ArmTimerPrecisionTest();
#endif


#endif