
#include <stdio.h>
//uint8_t,16_t
#include <stdint.h>
//syslog
#include <syslog.h>

#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"
#include "../gpio/gpio-i2c.h"
#include "lps331.h"

int InitLps331()
{
	InitI2c(REV_2);
	I2cSetSlaveAddr(I2C_ADDR_LPS331);

	//uint8_t send[2]   = {0,};
	//send[0] = LPS331_SINGLE_DATA( LPS331_ACTIVE );
	//send[1] = LPS331_ACTIVE_ON;
	//I2cWrite(send, 2);
	
	uint8_t send[2];
	send[0] = LPS331_SINGLE_DATA( LPS331_CTRL1 );
	send[1] = LPS331_POWER_ON<<LPS331_POWER;
	I2cWrite(send, 2);
}
int UnInitLps331()
{
	//UnInitI2c();
}
void DispLps331Register()
{
	uint8_t send;
	uint8_t recive[8];
	int i;
	
	send = LPS331_MULTI_DATA( LPS331_WHO_AM_I );
	I2cWrite(&send, 1);
	I2cRead(recive, 2);
	for(i=0; i<2; i++)
		printf("addr 0x%02X : 0x%02X\n", LPS331_WHO_AM_I+i, recive[i]);
	
	send = LPS331_MULTI_DATA( LPS331_CTRL1 );
	I2cWrite(&send, 1);
	I2cRead(recive, 8);
	for(i=0; i<8; i++)
		printf("addr 0x%02X : 0x%02X\n", LPS331_CTRL1+i, recive[i]);
}
void DispLps331Data()
{
	uint8_t send;
	uint8_t recive[LPS331_PRESS_REG_COUNT];
	int i;
	
	send = LPS331_MULTI_DATA( LPS331_PRESS );
	I2cWrite(&send, 1);
	I2cRead(recive, LPS331_PRESS_REG_COUNT);
	printf("addr 0x%02X : ", LPS331_PRESS);
	for(i=0; i<LPS331_PRESS_REG_COUNT; i++)
		printf("0x%02X ", recive[i]);
	printf("\n");
		
	send = LPS331_MULTI_DATA( LPS331_TEMP );
	I2cWrite(&send, 1);
	I2cRead(recive, LPS331_TEMP_REG_COUNT);
	printf("addr 0x%02X : ", LPS331_TEMP);
	for(i=0; i<LPS331_TEMP_REG_COUNT; i++)
		printf("0x%02X ", recive[i]);
	printf("\n");
}
int ClearLps331()
{
	int i;
	const int readPressTemp = LPS331_PRESS_REG_COUNT+LPS331_TEMP_REG_COUNT;
	uint8_t send[2];
	uint8_t recive[readPressTemp];
	//statusレジストリとone_shotフラグを初期化
	//printf("clear lps331\n");
	
	//one_shotフラグの初期化
	send[0] = LPS331_SINGLE_DATA( LPS331_CTRL2 );
	send[1] = 0<<LPS331_ONE_SHOT;
	I2cWrite(send, 2);
	
	//statusレジストリの初期化
	//読み込み専用なのでフラグを下げるためにpress(H,M,L)とtmp(H,L)を読み込む
	send[0] = LPS331_MULTI_DATA( LPS331_PRESS );
	I2cWrite(send, 1);
	I2cRead(recive, readPressTemp);
	
	//DispLps331Register();	
	//printf("addr 0x%02X : ", LPS331_PRESS);
	//for(i=0; i<LPS331_PRESS_REG_COUNT; i++)
	//	printf("0x%02X ", recive[i]);
	//printf("\n");
	//printf("addr 0x%02X : ", LPS331_TEMP);
	//for(i=LPS331_PRESS_REG_COUNT; i<readPressTemp; i++)
	//	printf("0x%02X ", recive[i]);
	//printf("\n");
}
int WakeUpLps331()
{
	uint8_t send[3];
	uint8_t recive;
	int i;
	int endFlag =  1<<LPS331_P_DA | 1<<LPS331_T_DA;
	
	ClearLps331();
	printf("endFlag 0x%X\n", endFlag);
	
	//送信データ(power onとone shotフラグ)のレジスタが
	//連続している( LPS331_CTRL1(0x20),CTEL2(0x21) )ので連続で送る
	send[0] = LPS331_MULTI_DATA( LPS331_CTRL1 );
	send[1] = LPS331_POWER_ON<<LPS331_POWER;
	send[2] = 1<<LPS331_ONE_SHOT;
	I2cWrite(send, 3);
	
	//printf("now sample\n");
	//DispLps331Register();
	//DispLps331Data();
	//one_shotフラグを立て、変換が完了すると下がるので下がったら次の処理へ
	//若しくはone_shotフラグでなくstatusレジストリを見る
	//one_shotのフラグだとうまく動かなかったのでstatusレジスタに変更
	//現状RES_CONFが0x7Aの時30msでフラグが立ってて40で立ってなかったので40で
	i=0;
	do{
		usleep(40000);
		//send[0] = LPS331_SINGLE_DATA( LPS331_CTRL2 );
		send[0] = LPS331_SINGLE_DATA( LPS331_STATUS );
		I2cWrite(send, 1);
		I2cRead(&recive, 1);
		++i;
		//基本的に一度で通過するはずだが
		//たまにデバイスがまともに動かない?のかフラグが立たない or 消えないので
		//30以上(40m*30=1.2s)チェックして動かないようなら戻る
		if( i>30 )
		{
			syslog(LOG_WARNING, "lps331 one shot flag error\n");
			syslog(LOG_WARNING, "    LPS331_STATUS addr 0x%02X : 0x%X  check count %d  endflag 0x%X\n",
				LPS331_STATUS, recive, i, endFlag);
				
			send[0] = LPS331_SINGLE_DATA( LPS331_CTRL2 );
			I2cWrite(send, 1);
			I2cRead(&recive, 1);
			syslog(LOG_WARNING, "    LPS331_CTRL2 addr 0x%02X : 0x%X\n",	LPS331_CTRL2, recive);
			
			return -1;
		}
	//LPS331_P_DA(気圧) LPS331_T_DA(気温)の測定完了フラグが立つまで待機;
	}while( (recive&endFlag) != endFlag );
	//printf("addr 0x%02X : 0x%X  check count %d\n", LPS331_STATUS, recive, i);
	//printf("sample ok\n");
	//DispLps331Data();
	
	//データの変換が完了したらpower downモードへ
	//printf("power off\n");
	send[0] = LPS331_SINGLE_DATA( LPS331_CTRL1 );
	send[1] = LPS331_POWER_OFF<<LPS331_POWER;
	I2cWrite(send, 2);
	
	//power off後にデータを読んでも同じ数字が返ってくる(clearされていない)
	//DispLps331Register();
	//DispLps331Data();
}
