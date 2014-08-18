
//syslog
#include <syslog.h>

#include "../gpio/gpio.h"

#include "main.h"
#include "shiftRegister.h"


//1. STを下げて入力モードへ
//2. SHを下げてdata storeの入力待ち
//3. DS入力(0ならそのまま)
//4. SHをあげて入力の保存
//5. 2-4を繰り返す
//6. STを挙げて出力
const int ds = 18;	//data store
const int st = 15;	//st(strage register)
const int sh = 14;	//shift register

int	dispData[SEG_COUNT];

//明るさによって7segの点灯時間を変える
int lightCount = DIGIT_SWITCH;

void InitShiftRegister()
{
	//pinを出力モードへ //PIN_INIT
	InitPin(ds, PIN_OUT);
	InitPin(st, PIN_OUT);
	InitPin(sh, PIN_OUT);
}
void UnInitShiftRegister()
{
	//消灯
	SendShiftRegister( 0x0000 );

	//pinを出力モードへ //PIN_INIT
	InitPin(ds, PIN_INIT);
	InitPin(st, PIN_INIT);
	InitPin(sh, PIN_INIT);
}
void SendShiftRegister(unsigned int dispNum)
{
	int i;
	
	GPIO_CLR(st);

	for(i=15; i>=0; i--)
	{
		GPIO_CLR(sh);
		if( (dispNum & (1<<i) ) != 0 )
		{
			//printf("1 ");
			GPIO_SET(ds);
		}
		else
		{
			//printf("0 ");
			GPIO_CLR(ds);
		}
		GPIO_SET(sh);
	}
	//printf("\n");

	GPIO_SET(st);
}

void Send7Seg(unsigned int disp, unsigned int digitSwitch)
{
	int restSwitchTime;
	do{
		//点灯
		SendShiftRegister( disp );
		
		//残りの切り替え時間と既定の切り替え時間の小さいほうを
		restSwitchTime = digitSwitch < DIGIT_SWITCH ? digitSwitch : DIGIT_SWITCH;
		digitSwitch -= DIGIT_SWITCH;
		
		//lightCount = 2500;
		//点灯時間の設定(lux値によって変化) digitSwitch以下ならその時間だけ点灯
		if( lightCount < restSwitchTime )
		{
			//残り時間の計算
			restSwitchTime -= lightCount;
			
			//決められた時間だけ点灯
			usleep(lightCount);
			//消灯
			SendShiftRegister( 0x0 );
		}
		usleep(restSwitchTime);
	
	}while( (int)digitSwitch > 0 );
	
	//SendShiftRegister( disp );
	//usleep( digitSwitch );
}
void Dips7segData()
{
	static unsigned int digit;
	
	Send7Seg(dispData[digit], DIGIT_SWITCH);
	
	if( ++digit > 3 )
		digit = 0;
}

void Insert7segData(char *buf)
{
	int i;
	for(i=0; i<SEG_COUNT; i++)
	{
		//一番下から
		switch( buf[3-i] )
		{
			//数字
			case '0': dispData[i] = SEG_NUM_0;	break;
			case '1': dispData[i] = SEG_NUM_1;	break;
			case '2': dispData[i] = SEG_NUM_2;	break;
			case '3': dispData[i] = SEG_NUM_3;	break;
			case '4': dispData[i] = SEG_NUM_4;	break;
			case '5': dispData[i] = SEG_NUM_5;	break;
			case '6': dispData[i] = SEG_NUM_6;	break;
			case '7': dispData[i] = SEG_NUM_7;	break;
			case '8': dispData[i] = SEG_NUM_8;	break;
			case '9': dispData[i] = SEG_NUM_9;	break;
			//アルファベット
			case 'a':
			case 'A': dispData[i] = SEG_ALPHA_A;	break;
			case 'b':
			case 'B': dispData[i] = SEG_ALPHA_B;	break;
			case 'c':
			case 'C': dispData[i] = SEG_ALPHA_C;	break;
			case 'd':
			case 'D': dispData[i] = SEG_ALPHA_D;	break;
			case 'e':
			case 'E': dispData[i] = SEG_ALPHA_E;	break;
			case 'f':
			case 'F': dispData[i] = SEG_ALPHA_F;	break;
			case 'g':
			case 'G': dispData[i] = SEG_ALPHA_G;	break;
			case 'h':
			case 'H': dispData[i] = SEG_ALPHA_H;	break;
			case 'i':
			case 'I': dispData[i] = SEG_ALPHA_I;	break;
			case 'j':
			case 'J': dispData[i] = SEG_ALPHA_J;	break;
			case 'k':
			case 'K': dispData[i] = SEG_ALPHA_K;	break;
			case 'l':
			case 'L': dispData[i] = SEG_ALPHA_L;	break;
			case 'm':
			case 'M': dispData[i] = SEG_ALPHA_M;	break;
			case 'n':
			case 'N': dispData[i] = SEG_ALPHA_N;	break;
			case 'o':
			case 'O': dispData[i] = SEG_ALPHA_O;	break;
			case 'p':
			case 'P': dispData[i] = SEG_ALPHA_P;	break;
			case 'q':
			case 'Q': dispData[i] = SEG_ALPHA_Q;	break;
			case 'r':
			case 'R': dispData[i] = SEG_ALPHA_R;	break;
			case 's':
			case 'S': dispData[i] = SEG_ALPHA_S;	break;
			case 't':
			case 'T': dispData[i] = SEG_ALPHA_T;	break;
			case 'u':
			case 'U': dispData[i] = SEG_ALPHA_U;	break;
			case 'v':
			case 'V': dispData[i] = SEG_ALPHA_V;	break;
			case 'w':
			case 'W': dispData[i] = SEG_ALPHA_W;	break;
			case 'x':
			case 'X': dispData[i] = SEG_ALPHA_X;	break;
			case 'y':
			case 'Y': dispData[i] = SEG_ALPHA_Y;	break;
			case 'z':
			case 'Z': dispData[i] = SEG_ALPHA_Z;	break;
			//記号
			case '-': dispData[i] = SEG_SYMBOL_HYPHEN;	break;
			case '_': dispData[i] = SEG_SYMBOL_UNDER_BAR;	break;
			default: dispData[i] = 0;
		}
		dispData[i] |= (DIGIT_1>>i);
	}
	return;
}

void Set7segLightControl(float lux)
{
	/*
	 * 明るさによって7segの点灯時間の変更
	 * ノーマル   削りあり
	 * 5.369086 - 6.422781   蛍光灯1
	 * 0.040268 - 0.040142   蛍光灯2
	 * 0.020134 - 0.030107   豆電球
	 * 0.005369 - 0.005352   モニタあり
	 * 0.004027 - 0.004014   モニタなし
	 *
	 * MAX(10以上) > 1以上 > 0.1以上 > 0.1以下
	 */

	//昼間
	if( lux >= 10.0f )
		lightCount = DIGIT_SWITCH;
	//蛍光灯
	else if( lux > 1.0f )
		lightCount = 2500;
	//特にない
	else if( lux > 0.1f )
		lightCount = 1850; //1250;
	//豆電球 真っ暗
	else
		lightCount = 1200; //600;
	
	////昼間
	//if( lux >= 10.0f )
	//	lightCount = 100;
	////蛍光灯
	//else if( lux > 1.0f )
	//	lightCount = 50; //2500;
	////特にない
	//else if( lux > 0.1f )
	//	lightCount = 25; //1250;
	////豆電球 真っ暗
	//else
	//	lightCount = 12; //600;
		
	//MySysLog(LOG_DEBUG, "7seg light count %d\n", lightCount);
	
	return;
}

void SetDP(int digit, int on)
{
	if( on == DP_ON )
		dispData[digit] |= SEG_DP;
	else
		dispData[digit] &= ~(SEG_DP);
}