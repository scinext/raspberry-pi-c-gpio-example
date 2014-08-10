
#include <stdio.h>
#include <string.h>
#include <time.h>

//syslog
#include <syslog.h>

#include "main.h"
#include "mode.h"
#include "shiftRegister.h"

//ARRAY_SIZE
#include "../gpio/gpio-util.h"


#define MODE_START		1

#define MAX_LOOP		3000
#define CLOCK_DP_BLINK  200
#define YEAR_SWITCH		300
#define MODE_NAME		2500


extern char		g_scrollBuf[SCROLL_BUF];

extern float	g_press;
extern float	g_temp;
extern float	g_hum;
extern float	g_lux;
extern float	g_coreTemp;
extern int		g_dataInterval;
extern struct timespec	g_waitLog;

//古いモード
int g_oldMode;
//各種モードでの古いfloat値
float oldFloatData;

//年、日付、時間で共通利用する
struct tm *g_ts;

//各モードでブリンクなどに使用
int g_loopCounter;

//すべてのモードで同じように使う
char g_segBuf[SEG_COUNT+1];

int DispModeName(int mode)
{
	switch(mode)
	{
		case MODE_PRESS:
			MySysLog(LOG_DEBUG, "  mode : pressure [ID - %x]\n", mode);
			break;
		case MODE_TEMP:
			MySysLog(LOG_DEBUG, "  mode : temperature [ID - %x]\n", mode);
			break;
		case MODE_LUX:
			MySysLog(LOG_DEBUG, "  mode : Lux [ID - %x]\n", mode);
			break;
		case MODE_HUMIDITY:
			MySysLog(LOG_DEBUG, "  mode : humidity [ID - %x]\n", mode);
			break;
		case MODE_CORE_TEMP:
			MySysLog(LOG_DEBUG, "  mode : core temperature [ID - %x]\n", mode);
			break;
		case MODE_QUIT:
			MySysLog(LOG_DEBUG, "  quit\n");
			break;
		case MODE_YEAR:
			MySysLog(LOG_DEBUG, "  mode : year [ID - %x]\n", mode);
			break;
		case MODE_DATE:
			MySysLog(LOG_DEBUG, "  mode : date [ID - %x]\n", mode);
			break;
		case MODE_WAIT_LOG:
			MySysLog(LOG_DEBUG, "  mode : wait log time [ID - %x]\n", mode);
			break;
		case MODE_CLOCK:
			MySysLog(LOG_DEBUG, "  mode : clock [ID - %x]\n", mode);
			break;
		case MODE_ANI_0:
		case MODE_ANI_1:
		case MODE_ANI_2:
			MySysLog(LOG_DEBUG, "  mode : ANIMATION %d [ID - %x]\n", mode - MODE_ANI_0, mode);
			break;
		default:
			MySysLog(LOG_DEBUG, "  mode : default [ID - %x]\n", mode);
			return -1;
	}
}

void DispModeData(int mode)
{
	int init;

	//ループ内でのカウンタ、ブリンクなどに使用
	if( g_oldMode != mode )
	{
		//状況によってモードごとにモードが変わったのを知るために使用
		init = MODE_START;
		//カウンタはモードが変わると初期化
		g_loopCounter = 0;

		g_oldMode = mode;
	}
	else
	{
		init = 0;
		++g_loopCounter;
		//ある程度立つと強制的に初期化に
		if( g_loopCounter >= MAX_LOOP )
		{
			init = MODE_START;
			g_loopCounter = 0;
		}
	}
	//printf("%d  ", g_loopCounter);

	switch(mode)
	{
		case MODE_PRESS:
			PressMode(init);
			break;
		case MODE_TEMP:
			TempMode(init);
			break;
		case MODE_LUX:
			LuxMode(init);
			break;
		case MODE_HUMIDITY:
			HumMode(init);
			break;
		case MODE_CORE_TEMP:
			CoreTemp(init);
			break;
		case MODE_QUIT:
			break;
		case MODE_YEAR:
			YearMode(init);
			break;
		case MODE_DATE:
			DateMode(init);
			break;
		case MODE_WAIT_LOG:
			WaitLogTime(init);
			break;
		case MODE_OUTPUT:
			ScrollOutput(init);
			break;
		case MODE_CLOCK:
		default:
			ClockMode( init );
			break;
	}
	return;
}
void BlinkDp(int digit, int blinkInterval)
{
	static char blinkDpToggle;
	//static int old_loopCounter;

	//if( old_loopCounter != g_loopCounter)
	//{
	//	old_loopCounter = g_loopCounter;
	//	if( (g_loopCounter % blinkInterval) == 0)
	//		blinkDpToggle ^= 0x1;
	//}
	if( (g_loopCounter % blinkInterval) == 0)
		blinkDpToggle ^= 0x1;
			
	if( (blinkDpToggle & 0x1) == 1 )
		SetDP(digit, DP_ON);
	else
		SetDP(digit, DP_OFF);
	
}
void DispWait()
{
	snprintf(g_segBuf, sizeof(g_segBuf), "Wait");
	Insert7segData(g_segBuf);
	return;
}
void PressMode(int init)
{	
	//1000オーバーは一つの関数で初期化されMODE_START状態になるのを利用
	if( g_loopCounter > MODE_NAME )
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "Pres");
		Insert7segData(g_segBuf);
		return;
	}
	
	if( g_press == INIT_SENSOR )
	{
		//g_press = GetPress();
		DispWait();
		return;
	}

	if( oldFloatData != g_press || init == MODE_START)
	{
		if( g_press < 1000 )
			snprintf(g_segBuf, sizeof(g_segBuf), "%4d", (int)(g_press*10));
		else
			snprintf(g_segBuf, sizeof(g_segBuf), "%4d", (int)g_press );

		Insert7segData(g_segBuf);

		oldFloatData = g_press;
	}

	if( g_press < 1000 )
		BlinkDp(1, CLOCK_DP_BLINK);
	
}
void TempMode(int init)
{
	if( g_temp == INIT_SENSOR )
	{
		//g_temp = GetTemp();
		DispWait();
		return;
	}

	if( oldFloatData != g_temp || init == MODE_START)
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "%3dC", (int)(g_temp*10));
		Insert7segData(g_segBuf);

		oldFloatData = g_temp;
	}

	BlinkDp(2, CLOCK_DP_BLINK);
}
void LuxMode(int init)
{
	static int dpDigit;

	if( g_loopCounter > MODE_NAME )
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "Lux");
		Insert7segData(g_segBuf);
		return;
	}
	
	if( g_lux == INIT_SENSOR )
	{
		DispWait();
		return;
	}

	if( oldFloatData != g_lux || init == MODE_START)
	{
		if( g_lux > 1000 )
		{
			snprintf(g_segBuf, sizeof(g_segBuf), "%4d", (int)g_lux);
		}
		else if( g_lux < 0.01 )
		{
			snprintf(g_segBuf, sizeof(g_segBuf), "%04d", (int)(g_lux*1000) );
			dpDigit = 3;
		}
		else if( g_lux < 0.1 )
		{
			snprintf(g_segBuf, sizeof(g_segBuf), " %03d", (int)(g_lux*100) );
			dpDigit = 2;
		}
		else if( g_lux < 1 )
		{
			snprintf(g_segBuf, sizeof(g_segBuf), "  %02d", (int)(g_lux*10) );
			dpDigit = 1;
		}
		else
		{
			snprintf(g_segBuf, sizeof(g_segBuf), "%4d", (int)(g_lux*10) );
			dpDigit = 1;
		}
		Insert7segData(g_segBuf);

		oldFloatData = g_lux;
	}
	if( g_lux < 1000 )
		BlinkDp(dpDigit, CLOCK_DP_BLINK);

}
void HumMode(int init)
{
	if( g_loopCounter > MODE_NAME )
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "Hum");
		Insert7segData(g_segBuf);
		return;
	}
	
	if( g_hum == INIT_SENSOR )
	{
		//g_hum = GetHumidity();
		DispWait();
		return;
	}
	
	if( oldFloatData != g_hum || init == MODE_START)
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "%4d", (int)(g_hum*10));
		Insert7segData(g_segBuf);

		oldFloatData == g_hum;
	}
	BlinkDp(1, CLOCK_DP_BLINK);


}
void CoreTemp(int init)
{
	if( g_loopCounter > MODE_NAME )
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "Ctmp");
		Insert7segData(g_segBuf);
		return;
	}
	
	if( g_coreTemp == INIT_SENSOR )
	{
		DispWait();
		return;
	}
	
	if( oldFloatData != g_coreTemp || init == MODE_START)
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "%3dC", (int)(g_coreTemp*10));
		Insert7segData(g_segBuf);

		oldFloatData = g_coreTemp;
	}
	BlinkDp(2, CLOCK_DP_BLINK);
}

void YearMode(int init)
{
	if( g_ts == 0 )
	{
		time_t t;
		t = time(NULL);
		g_ts = localtime(&t);
	}

	strftime(g_segBuf, sizeof(g_segBuf), "%Y", g_ts);

	//和暦の表示
	if( g_loopCounter > YEAR_SWITCH*2 )
	{
		g_loopCounter = 0;
	}
	else if( g_loopCounter > YEAR_SWITCH )
	{
		int y;
		//現在の西暦から1988を引けば平成の和暦に
		y = atoi(g_segBuf) - 1988;
		snprintf(g_segBuf, sizeof(g_segBuf), " H%d", y);
		//printf("%s\n", g_segBuf);
	}

	Insert7segData(g_segBuf);
}
void DateMode(int init)
{
	static int month, day;

	if( g_loopCounter > MODE_NAME )
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "Date");
		Insert7segData(g_segBuf);
		return;
	}
	
	if( g_ts == 0 || init == MODE_START)
	{
		time_t t;
		t = time(NULL);
		g_ts = localtime(&t);

		strftime(g_segBuf, sizeof(g_segBuf), "%m", g_ts);
		month = atoi(g_segBuf);

		strftime(g_segBuf, sizeof(g_segBuf), "%d", g_ts);
		day	= atoi(g_segBuf);

		snprintf(g_segBuf, sizeof(g_segBuf), "%2d%2d", month, day);
		
		Insert7segData(g_segBuf);
	}
	
	//static int dpDigit = SEG_COUNT;
	//int i;
	
	//月と日の部分を点滅
	BlinkDp(0, CLOCK_DP_BLINK);
	BlinkDp(2, CLOCK_DP_BLINK);
	
	////すべてのDPを点滅させる
	//for(i=0; i<SEG_COUNT; i++)
	//	BlinkDp(i, CLOCK_DP_BLINK);

	//左から右へ移動
	//g_dispData[dpDigit] |= SEG_DP;
	//
	//if( g_loopCounter % CLOCK_DP_BLINK == 0 )
	//{
	//	if( dpDigit <= 0 )
	//		dpDigit = SEG_COUNT;
	//	else
	//		--dpDigit;
	//}
}
void ClockMode(int init)
{
	time_t t;
	static time_t oldT;
	int i;

	//2桁目のドットの点滅は計測を行う
	BlinkDp(2, CLOCK_DP_BLINK);

	//20秒経ってなかったら更新しない
	t  = time(NULL);
	if( oldT+20 > t && init != MODE_START)
		return;

	oldT = t;
	g_ts = localtime(&t);

	strftime(g_segBuf, sizeof(g_segBuf), "%k%M", g_ts);

	Insert7segData(g_segBuf);
}

void ScrollOutputInit(char *buf)
{
	int length;
	
	memset(g_scrollBuf, 0, sizeof(char)*SCROLL_BUF);
	
	//先頭のスペース3文字分を省いた文字以下なら処理
	length = strlen(buf);
	if( length <= 4 )
		sprintf(g_scrollBuf, "%s", buf);
	else if( length <= SCROLL_BUF-SPACE_LENGTH )
		sprintf(g_scrollBuf, SPACE "%s", buf);
	else
		strcpy(g_scrollBuf, "char over");
	
}
void ScrollOutput(int init)
{
	static int i;
	int length;
	
	length = strlen(g_scrollBuf);
	if( length <= 4 )
	{
		i = 0;
	}
	else
	{		
		if( g_loopCounter > 70 )
		{
			g_loopCounter = 0;
			if( ++i >= length )
				i = 0;
			//printf("  i: %d, string: %s\n", i, &(g_scrollBuf[i]) );
		}
		else if( init == MODE_START )
		{
			//最初の場合、文字を左端に表示させるためにスペース分は飛ばす
			//直ぐに飛ばされるのでわざと1文字分スペースを空ける
			i = SPACE_LENGTH-1;
		}
	}
	Insert7segData( &(g_scrollBuf[i]) );
}

#define N_TO_NANO			1e+9
#define NANO_TO_100MILLI	1e+8	//1000[n]*1000[u]*100[milli]
void WaitLogTime(int init)
{
	static unsigned long oldElapsedTime;
	struct timespec nTime, diff;
	unsigned long sDiff, mDiff;
	unsigned long elapsedTime;
	long waitTime;

	//まだ1回目のログを取得してない場合
	if( g_waitLog.tv_sec == 0 )
	{
		DispWait();
		return;
	}

	//状態名の表示
	if( g_loopCounter > MODE_NAME )
	{
		snprintf(g_segBuf, sizeof(g_segBuf), "next");
		Insert7segData(g_segBuf);
		return;
	}

	//開始
	clock_gettime(CLOCK_MONOTONIC, &nTime);
	//if( nTime.tv_nsec >= g_waitLog.tv_nsec )
	//{
	//	sDiff = nTime.tv_sec - g_waitLog.tv_sec;
	//	mDiff = ( nTime.tv_nsec - g_waitLog.tv_nsec ) /NANO_TO_100MILLI;
	//}
	//else
	//{
	//	sDiff = nTime.tv_sec - 1 - g_waitLog.tv_sec;
	//	mDiff = ( (unsigned long)N_TO_NANO - g_waitLog.tv_nsec + nTime.tv_nsec ) /NANO_TO_100MILLI;
	//}
	TimespecDiff(&g_waitLog, &nTime, &diff);
	sDiff = diff.tv_sec;
	mDiff = diff.tv_nsec/NANO_TO_100MILLI;
	//printf("sec %lu . milli %lu \t", g_waitLog.tv_sec, g_waitLog.tv_nsec);
	//printf("sec %lu . milli %lu \t", nTime.tv_sec, nTime.tv_nsec);
	//printf("sec %lu . %lu *100milli\n", sDiff, mDiff);

	elapsedTime = sDiff*10 + mDiff;
	if( oldElapsedTime != elapsedTime )
	{
		waitTime = g_dataInterval*10 - elapsedTime;
		//printf("wait time %d\n", waitTime);
		if( waitTime <= 0 )
		{
			DispWait();
			return;
		}

		snprintf(g_segBuf, sizeof(g_segBuf), "%4d", waitTime);
		Insert7segData(g_segBuf);
		oldElapsedTime = elapsedTime;
	}
	if( waitTime < 10000 )
		SetDP(1, DP_ON); //g_dispData[1] |= SEG_DP;
}


/*
 *                        DIGIT_1   SEG_A
 *         SEG_A          DIGIT_2   SEG_B
 *        --------        DIGIT_3   SEG_C
 *       |        |       DIGIT_4   SEG_D
 *  SEG_F|        |SEG_B            SEG_E
 *       | SEG_G  |                 SEG_F
 *        --------                  SEG_G
 *       |        |                 SEG_DP
 *  SEG_E|        |SEG_C  _
 *       | SEG_D  |      | | SEG_DP(RDP) LDPは配線していない
 *        --------        -
 */
unsigned int ani0[] = {
	DIGIT_1|SEG_A, DIGIT_2|SEG_A, DIGIT_3|SEG_A, DIGIT_4|SEG_A, DIGIT_4|SEG_F,
	DIGIT_4|SEG_G, DIGIT_3|SEG_G, DIGIT_2|SEG_G, DIGIT_1|SEG_G, DIGIT_1|SEG_C,
	DIGIT_1|SEG_D, DIGIT_2|SEG_D, DIGIT_3|SEG_D, DIGIT_4|SEG_D, DIGIT_4|SEG_E,
	DIGIT_4|SEG_G, DIGIT_3|SEG_G, DIGIT_2|SEG_G, DIGIT_1|SEG_G, DIGIT_1|SEG_B
};
unsigned int ani1[][4] = {
	{ DIGIT_1|SEG_A,	0x0000,			0x0000, 		DIGIT_4|SEG_D},
	{ 0x0000,			DIGIT_2|SEG_A,	DIGIT_3|SEG_D, 	0x0000},
	{ 0x0000,			DIGIT_2|SEG_D,	DIGIT_3|SEG_A, 	0x0000},
	{ DIGIT_1|SEG_C,	0x0000,			0x0000, 		DIGIT_4|SEG_F},
	{ DIGIT_1|SEG_B,	0x0000,			0x0000, 		DIGIT_4|SEG_E}
};
unsigned int ani3[] = {
	DIGIT_1|SEG_A|DIGIT_4|SEG_D, DIGIT_2|SEG_A|DIGIT_3|SEG_D,
	DIGIT_3|SEG_A|DIGIT_2|SEG_D, DIGIT_4|SEG_A|DIGIT_1|SEG_D,
	DIGIT_4|SEG_F|DIGIT_1|SEG_C, DIGIT_4|SEG_E|DIGIT_1|SEG_B,
};

int g_aniSpeed;
void AnimationMode(int mode)
{
	static unsigned int digitCounter;
	static unsigned int animationCounter;
	//ループ内でのカウンタ、ブリンクなどに使用
	if( g_oldMode != mode )
	{
		//カウンタはモードが変わると初期化
		g_loopCounter		= 0;
		digitCounter		= 0;
		animationCounter	= 0;
		g_oldMode 			= mode;
		switch( mode )
		{
			case MODE_ANI_1:
				g_aniSpeed = 6000;
				break;
			case MODE_ANI_2:
			case MODE_ANI_0:
			default:
				g_aniSpeed = 170000;
				break;
		}
	}
	switch( mode )
	{
		case MODE_ANI_2:
			{
				Send7Seg(ani3[g_loopCounter], g_aniSpeed);
				if( ++g_loopCounter >= sizeof(ani3)/sizeof(ani3[0]) )
					g_loopCounter = 0;
			}
			break;
		case MODE_ANI_1:
			{
				//BoundSpeed(2500, 7000, 5);
				Send7Seg( ani1[animationCounter][digitCounter], g_aniSpeed);
				if( ++digitCounter >= ARRAY_SIZE(ani1[0]) )
				{
					++g_loopCounter;
					digitCounter = 0;
				}
				if( g_loopCounter >= 5 )
				{
					++animationCounter;
					g_loopCounter = 0;
				}
				if( animationCounter >= ARRAY_SIZE(ani1) )
					animationCounter = 0;
			}
			break;
		case MODE_ANI_0:
		default:
			{
				//BoundSpeed(30000, 100000, 800);
				Send7Seg( ani0[g_loopCounter], g_aniSpeed);
				if( ++g_loopCounter >= sizeof(ani0)/sizeof(ani0[0]) )
					g_loopCounter = 0;
			}
			break;
	}
}
void BoundSpeed(int min, int max, int inc)
{
	static int incToggle = 1;
	if( g_aniSpeed <= min && incToggle != 1 )
		incToggle = 1;

	if( g_aniSpeed >= max && incToggle != -1 )
		incToggle = -1;

	g_aniSpeed += incToggle * inc;
	//printf("toggle %d  speed %d\n", incToggle, g_aniSpeed);
}