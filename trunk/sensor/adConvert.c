
#include <stdio.h>

//clock_gettime gccで -lrtをつけないとコンパイルエラー
#include <time.h>

#include "./gpio/gpio.h"
#include "./gpio/gpio-util.h"
#include "./gpio/gpio-spi.h"
#include "./gpio/gpio-timer.h"
#include "./gpio/gpio-arm-timer.h"
#include "adConvert.h"


//送信時のデータ
#define START_BIT		1<<2	//S
#define AD_SINGLE		1<<1	//シングルエンドモードでAD変換
#define AD_DIFF			0<<1	//差動モードでAD変換
#define SEND_CH_SHIFT	6		//チャンネルを設定するときに上位へシフトする数

//計算時のデータ
//#define AD_RESOLUTION	12		//ICの分解能
#define AD_RESOLUTION	10		//ICの分解能精度の問題で下位2bitは捨てる
#define VREF			3300	//基準ボルト(mV)

#define SPI_CLK			1000000
/*
┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐| ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐|
┘1 └-┘2 └-┘3 └-┘4 └-┘5 └-┘6 └-┘7 └-┘8 └|-┘9 └-┘10└-┘11└-┘12└-┘13└-┘14└-┘15└-┘16└|
            			          ┌─-┐┌─-┐┌─-┐|┌─-┐┌─-┐┌----------------------------------------|
送信------------------------------┘S  └┘SD └┘D2 └|┘D1 └┘D0 └┘無視								    |
													   |					 ┌─-┐┌─-┐┌─-┐┌─-┐┌─-┐|
-------------------------------------------------------|---------------------┘N  └┘B11└┘B10└┘B9 └┘B8 └|
受信

送信後12bit分受信するトータルで3byte 受信時は最後のbit(b11)から
S	-> StartBit
SD	-> SGL/DIFF
N	-> 0が入る
*/

//送信用バッファ
uint8_t tbuf[3] = {0, };
//受信用バッファ
uint8_t rbuf[3] = {0, };


int InitAD()
{
	InitSpi();
}
int UnInitAD()
{
	//特に行わなくても大丈夫だと思われる
	//UnInitSpi();
}


//6.5-7.1us前後
int PinGetADCH(int pin, int ch, float *outADVolt, unsigned long sleepTime)
{
	int i, adVolt;
	float res;
	
	//一つ目の送信バイトの設定
	tbuf[0] = START_BIT | AD_SINGLE;
	//chの設定
	//4ch以上の場合、1つ前の送信バイトに1を入れる
	tbuf[0] |= ch>=4 ? 1 : 0;
	tbuf[1] = ch << SEND_CH_SHIFT;
	tbuf[2] = 0;
	
	//printf("ch %d\n", ch);
	//printf("tbuf data  : ");
	//for(i=0; i<ARRAY_SIZE(tbuf); i++)
	//	printf(" %02X", tbuf[i]);
	//printf("\n");

	//SPIの設定 MCP3204は3.3v時だとClock 1.6MHzが限界っぽい
	SpiSetCS(SPI_CS_0);
	SpiSetMode(SPI_MODE_0);
	SpiClear(SPI_CLR_ALL);
	SpiSetCSPolarity(SPI_CS_0, LOW);
	SpiSetClock(SPI_CLK);
	//SPIを書き込みモードにする
	//相手にデータを送る場合?よくわからないがMCP3204の時はこのモードにしないとデータがもらえない)
	SpiSetWriteMode();
	
	/*
	 * SpiTransferMulitpleAndPinHighLowでの計測
	 * MCP3204(1.5MHz)の時150オームでのコンデンサの充電電圧から
	 * 時間を求めると約4.59usになる(実際に5usディレイさせたら計算値と実測が一致)
	 * おそらくHighにしてからADコンバートを始めるまでに掛かる秒数だと思われる
	 * (2バイトデータの受信直後?)
	 * 
	 * MCP3204    2.7v定格 1MHz    5v定格 2MHz
	 * 計算だと3.3は1.2MHz位ならいけそう
	 * 秒数(電圧から計算)
	 *     1.5MHz  -> 4.59us    1.5MHz 1clock 0.6us
	 *     1MHz    -> 6.29us    1MHz   1clock 1us
	 * 
	 * clock_gettimeでの計測
	 *     1.5MHz 15us
	 *     1MHz   20us
	 * 
	 * 1.5MHzだとポーリングにミスることがある
	 */
	 
	//内部でpinの初期化とHigh,Lowを行っている
	//ここでspi転送にかかる5-6usをあらかじめ待機時間から引いとく
	//高分解能のため*10とずれる2count分を差し引く
	if( sleepTime > 52 )
		sleepTime -= 52;
	else
		sleepTime = 0;
	//printf("sleep %u\n", sleepTime);
	SpiTransferMulitpleAndPinHighLow(tbuf, rbuf, ARRAY_SIZE(tbuf), pin, sleepTime);
	
	//下位8bitのみと次のデータを使用(取得データの下位2bitは精度の問題で捨てる)
	adVolt = ((rbuf[1] & 0xF) << 8 | rbuf[2] ) >>2;
	
	if( outADVolt != NULL )
	{
		//べき乗が演算子にないので 1を分解能分シフトして取得
		res			= (float)VREF / (1<<AD_RESOLUTION);
		*outADVolt	= adVolt * res;
	}
	return adVolt;
	
}

int GetADCH(int ch, float *outADVolt)
{
	int i, adVolt;
	float res;
	struct timespec startTs, endTs;
	
	//一つ目の送信バイトの設定
	tbuf[0] = START_BIT | AD_SINGLE;
	//chの設定
	//チャンネル0 000 000000
	//チャンネル1 001 000000
	//チャンネル2 010 000000
	//チャンネル3 011 000000
	//チャンネル4 100 000000
	//チャンネル5 101 000000
	//チャンネル6 110 000000
	//チャンネル7 111 000000
	//4ch以上の場合、1つ前の送信バイトに1を入れる
	tbuf[0] |= ch>=4 ? 1 : 0;
	tbuf[1] = ch << SEND_CH_SHIFT;
	tbuf[2] = 0;
	
	//printf("ch %d\n", ch);
	//printf("tbuf data  : ");
	//for(i=0; i<ARRAY_SIZE(tbuf); i++)
	//	printf(" %02X", tbuf[i]);
	//printf("\n");

	SpiSetCS(SPI_CS_0);
	SpiSetMode(SPI_MODE_0);
	SpiClear(SPI_CLR_ALL);
	SpiSetCSPolarity(SPI_CS_0, LOW);
	SpiSetClock(SPI_CLK);
	SpiSetWriteMode();
		
	SpiTransferMulitple(tbuf, rbuf, 3);
	
	//下位8bitのみと次のデータを使用(取得データの下位2bitは精度の問題で捨てる)
	adVolt = ((rbuf[1] & 0xF) << 8 | rbuf[2] ) >>2;
	//adVolt = ((rbuf[1] & 0xF) << 8 | rbuf[2] );
	
	if( outADVolt != NULL )
	{
		//べき乗が演算子にないので 1を分解能分シフトして取得
		res			= (float)VREF / (1<<AD_RESOLUTION);
		*outADVolt	= adVolt * res;
	}
	return adVolt;
}