
#include <stdio.h>

//clock_gettime gccで -lrtをつけないとコンパイルエラー
#include <time.h>

#include "../gpio/gpio.h"
#include "../gpio/gpio-util.h"
#include "../gpio/gpio-spi.h"
#include "../gpio/gpio-timer.h"
#include "../gpio/gpio-arm-timer.h"
#include "adConvert.h"

//送信時のデータ
#define START_BIT		1<<2	//S
#define AD_SINGLE		1<<1	//シングルエンドモードでAD変換
#define AD_DIFF			0<<1	//差動モードでAD変換
#define SEND_CH_SHIFT	6		//チャンネルを設定するときに上位へシフトする数

//5v->2MHz 2.7v->1MHz 3.3v->1.26MHzまで行ける?
#define SPI_CLK			1000000	//1MHz
//#define SPI_CLK			1300000	//
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

//外向けじゃないのでここで
void SetSPIData(int ch);

int InitAD()
{
	InitSpi();
	
	//SPIの設定 MCP3204は3.3v時だとClock 1.6MHzが限界っぽい
	SpiSetCS(SPI_CS_0);
	SpiSetMode(SPI_MODE_0);
	SpiClear(SPI_CLR_ALL);
	SpiSetCSPolarity(SPI_CS_0, LOW);
	SpiSetClock(SPI_CLK);
}
int UnInitAD()
{
	//特に行わなくても大丈夫だと思われる
	//UnInitSpi();
}

void SetSPIData(int ch)
{
	//一つ目の送信バイトの設定
	tbuf[0] = START_BIT | AD_SINGLE;
	//chの設定
	//ch0 000 0 0000
	//ch1 001 0 0000
	//ch2 010 0 0000
	//ch3 011 0 0000
	//ch4 100 0 0000
	//ch5 101 0 0000
	//ch6 110 0 0000
	//ch7 111 0 0000
	//4ch以上の場合、1つ前の送信バイトに1を入れる
	tbuf[0] |= ch>=4 ? 1 : 0;
	tbuf[1] = ch << SEND_CH_SHIFT;
	tbuf[2] = 0;
	
	//int i;
	//printf("ch %d\n", ch);
	//printf("tbuf data  : ");
	//for(i=0; i<ARRAY_SIZE(tbuf); i++)
	//	printf(" %02X", tbuf[i]);
	//printf("\n");

	//SPIを書き込みモードにする
	//相手にデータを送る場合?よくわからないがMCP3204の時はこのモードにしないとデータがもらえない)
	SpiSetWriteMode();
	
	return;
}

unsigned int GetAD(int ch)
{	
	SetSPIData(ch);
		
	SpiTransferMulitple(tbuf, rbuf, 3);
	
	//printf("rbuf data  : ");
	//for(i=0; i<ARRAY_SIZE(rbuf); i++)
	//	printf(" %02X", rbuf[i]);
	//printf("\n");	
	return (unsigned int)( ((rbuf[1]&0x0F)<<8) | rbuf[2] );
}

unsigned int GetADpin(int pin, int ch, unsigned long sleepTime)
{
	int i;
	SetSPIData(ch);
	
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
	sleepTime *= 10;
	if( sleepTime > AD_PIN_GAP )
		sleepTime -= AD_PIN_GAP;
	else
		sleepTime = 0;
		
	SpiTransferMulitpleAndPinHighLow(tbuf, rbuf, ARRAY_SIZE(tbuf), pin, sleepTime);
	
	return (unsigned int)( ((rbuf[1]&0x0F)<<8) | rbuf[2] );
}


unsigned int GetADNoPad(int ch)
{
	int i;
	unsigned int b;
		
	//送受信バッファの初期化
	for(i=0; i<ARRAY_SIZE(tbuf); i++)
	{
		tbuf[i] = 0;
		rbuf[i] = 0;
	}
	
	//送信データの作成 0b( 11XX X000 )
	tbuf[0] = 0xC0 | (ch<<3);
	
	SpiTransferMulitple(tbuf, rbuf, 3);
	
	b = rbuf[0]<<16 | rbuf[1]<<8 | rbuf[2];
	
	//受信データの編集
	b =	( b & 0x1FFE0 ) >> 5;
	return b;
}
unsigned int GetADNoPadPin(int pin, int ch, unsigned int sleepTime)
{
	int i;
	unsigned int b;
	int useTime;
	
	
	//データシートだと受信したデータを操作しやすい位置に来させるため
	//5bitのパディングをしているがそうすると1MHz時に
	//データの送信に9bit、変換に1bitで計10clock必要となり時間が足りないので
	//受信データはプログラムでどうとでもできるので
	//送信データをパディングなしで送るようにする
	//
	//送信データ mcp3204だと 開始bit, SGL/DIFF, chD0, chD1, chD2 の5bit
	//送信したい8bit	0b(11XX X000)	XXX->ch(0x0～0x7) mcp3204は0x3まで
	//0xC0 | (ch<<3)でOKのはず
	
	//送受信データ
	//送     0b 11XX X000 ---- ---- ---- ----
	//受     0b ---- -SN1 1111 1111 1110 0000
	//MASK   0x    0    1    F    F    E    0
	//SHIFT  >>5
	//S ->サンプリングに必要なクロック
	//N ->NULL bit
	//- ->無視(ゴミデータ)
	//
	//テストしたらこうなった
	//          ASDD D
	//0000 0000 1100 0 0 00 0000 0000 0000 0000
	//0000 0000 1111 1 1 00 1001 1110 0111 0011
	//          ---- - S NB BBBB BBBB BBBx xxxx
	//A ->開始ビット    S ->SGL/DIFF    D ->chD0-D2
	//S ->サンプリングクロック    N ->NULL bit    B ->データbit
	//- ->無視(ゴミデータ)
	//
	//
	//データシートより p15
	//          ASDD D
	//0000 0000 1100 0 0 -無視-------------------------------------------------------------------
	//0000 0000 1111 1 1 0   0   1   0   0   1   1   1   1   0   0   1   1  | 1   0   0   1   1
	//                 S N   B11 B10 B9  B8  B7  B6  B5  B4  B3  B2  B1  B0 | B1  B2  B3  B4  B5
	//B0以降はB1から逆にデータが入る
	//
	//
	//
	//送信データ5bit + サンプリング時間1clock  合計6クロックの時間が最低限掛かるはず
	//計算上はfreqが1MHzのとき1clock->1usなので6usのはず
	//実際はそれに待機用の関数呼び出しのオーバヘッドなどが加わるので多少の+aが出るはず
	//待機用の関数
	//	なし 
	//	    ch sleepTime 10     0    adc      836    volt 2694.140625mV
	//	    Vcc(V) 3.3,  C(uF) 0.022,  T(us)  10,  Et(mV)  2.694141 
	//	    R =   268.163696  Et =    2.694140
	//	    R==150 -> T =    5.593598
    //	あり(0)
	//	    ch sleepTime 10     0    adc     1162    volt 936.181641mV
	//	    Vcc(V) 3.3,  C(uF) 0.022,  T(us)  10,  Et(mV)  0.936182
	//	    R =   1362.365723  Et =    0.936182
	//	    R==1000 -> T =    7.340173
	//	    Rs = 214.615738
	//理想では73+27(0.1us単位)でちょうど10usになるはず
	//
	//平均的にarm counterが270前後を出す
	//8bit*1us*3 = 24us+オーバーヘッド3us で計算してよさげ
	//
	//
	
	//150	-> 73 R==150		-> T=8.372174	Rs=249.857361
	//1k	-> 73 R==1000		-> T=9.998987   Rs=219.985718
    //100k	-> 73 R==100000		-> T=94.378563	Rs=138.982422
	//1M	-> 73 R==1000000	-> T=943.785583 Rs=98.982414

	sleepTime *= 10;
	//待機時間の処理
	//spiでsleep直前からデータ送信までに約14(0.1us)使う
	//データが5byte+1byte(データ変換時間)で約6us(1MHz)->60
	//計74余分にかかるのでその分を引く
	const int measureUseTime = 74; //AD_NO_PADDING_GAP;
	if( sleepTime > measureUseTime )
		sleepTime -= measureUseTime;
	else
		sleepTime = 0;
	//printf("sleepTime - measureUseTime %d\n", sleepTime);

	
	//送受信バッファの初期化
	for(i=0; i<ARRAY_SIZE(tbuf); i++)
	{
		tbuf[i] = 0;
		rbuf[i] = 0;
	}
	
	//送信データの作成 0b( 11XX X000 )
	tbuf[0] = 0xC0 | (ch<<3);
	
	////送信テスト表示
	//printf("tbuf data  : ");
	//b = tbuf[0]<<16 | tbuf[1]<<8 | tbuf[2];
	//PrintUintDelimiter(stdout, b, 4);
	
	SpiTransferMulitpleAndPinHighLow(tbuf, rbuf, ARRAY_SIZE(tbuf), pin, sleepTime);
	//printf("use time %d\n", useTime);
	
	b = rbuf[0]<<16 | rbuf[1]<<8 | rbuf[2];
	////受信テスト表示	
	//printf("rbuf data  : ");
	//PrintUintDelimiter(stdout, b, 4);
	
	//受信データの編集
	b =	( b & 0x1FFE0 ) >> 5;
	////受信編集テスト表示	
	//printf("rbuf data  : ");
	//PrintUintDelimiter(stdout, b, 4);
	
	//テスト時コード
	//for(j=0; j<4; j++)
	//{
	//	//テスト時送信1byte 受信2byteでできるかテスト
	//	//0b 0001 1000 
	//	//             0011 1111 1111 1100
	//	//             0x3  F    F    C
	//	tbuf[0] = 0x18<<j;
	//	tbuf[1] = 0;
	//	tbuf[2] = 0;
	//	rbuf[0] = 0;
	//	rbuf[1] = 0;
	//	rbuf[2] = 0;
	//	SpiTransferMulitpleAndPinHighLow(tbuf, rbuf, ARRAY_SIZE(tbuf), pin, sleepTime);
	//	
	//	printf("tbuf data  : ");
	//	b = tbuf[0]<<16 | tbuf[1]<<8 | tbuf[2];
	//	PrintUintDelimiter(stdout, b, 4);
	//		
	//	printf("rbuf data  : ");
	//	b = rbuf[0]<<16 | rbuf[1]<<8 | rbuf[2];
	//	PrintUintDelimiter(stdout, b, 4);
	//	
	//	printf("rbuf data  : ");
	//	b = (rbuf[0]<<16 | rbuf[1]<<8 | rbuf[2]) & (0x3FFC<<j);
	//	b >>= (2+j);
	//	PrintUintDelimiter(stdout, b, 4);
	//	
	//	for(i=0; i<ARRAY_SIZE(rbuf); i++)
	//		printf(" %02X", rbuf[i]);
	//	printf("\n");
	//	usleep(100000);
	//}
	return b;
}

#define WAIT_CLOCK_ARM_COUNTER	4
static inline void WaitClk(unsigned int start)
{
	while( (start+WAIT_CLOCK_ARM_COUNTER) > GetArmTimer() )
		;
	//printf("clk %u\n", GetArmTimer() - start);
	return ;
}
//自力実装spi gpioのオンオフを自力
unsigned int GetADmcp3204(int pin, int ch, unsigned long sleepTime)
{
	//#define SPI_MOSI	P1_19	 GPIO_10	serial in 
	//#define SPI_MISO	P1_21	 GPIO_9 	serial out
	//#define SPI_SCLK	P1_23	 GPIO_11	clk
	//#define SPI_CE0	P1_24	 GPIO_8 	cs
	//#define SPI_CE1	P1_26	 GPIO_7 	cs
	unsigned int start, diff;
	unsigned int clkUpStart, clkDownStart;
	unsigned int send, recive, bit;
	int i;
	
	
	//クロックのダウン中にDinの上げ下げ or Doutの読み込みを行いその後クロックを上げる
	//下がっている時間0.5us, 上がっている時間0.5usの計1usで1MHzになる
	//計測は0.1単位で行うためart timerの高分解能で行う
	//単純なpinの上げ下げで0.1ms使用するので
	//WaitClkでpinの上げ下げ分を除くと4待機させるとほぼ0.5usのはず
	//
	//
	//送受信bit
	//送 	0b 11XX X
	//受    0b       SN 1111 1111 1111
	//MASK	0x        0    F    F    F
	//S->サンプリングに必要なクロック
	//N->NULL bit
	//
	//
	//送信データ5bit + サンプリング時間1clock  合計6クロックの時間が最低限掛かるはず
	//実際は下記だったので 68+32でちょうど10usになるはず
	//待機関数
	//	なし
	//		WAIT_CLOCK_ARM_COUNTER = 5
	//			ch sleepTime 10     0    adc     1199    volt 965.991211mV
	//			Vcc(V) 3.3,  C(uF) 0.022,  T(us)  10,  Et(mV)  0.965991
	//			R =   1312.443848  Et =    0.965991
	//			R==1000 -> T =    7.619374
	//		WAIT_CLOCK_ARM_COUNTER = 4
	//			ch sleepTime 10     0    adc     1043    volt 840.307617mV
	//			Vcc(V) 3.3,  C(uF) 0.022,  T(us)  10,  Et(mV)  0.840308
	//			R =   1546.671875  Et =    0.840308
	//			R==1000 -> T =    6.465496
	//
	//  	5 150ohm
	//			ch sleepTime 10     0    adc      819    volt 2639.355469mV
	//			Vcc(V) 3.3,  C(uF) 0.022,  T(us)  10,  Et(mV)  2.639355 
	//			R =   282.596375  Et =    2.639355
	//			R==150 -> T =    5.307924
	//	あり(0)
	//		WAIT_CLOCK_ARM_COUNTER = 5	+20で約10usに ->必要な時間 80
	//			ch sleepTime 10     0    adc     1236    volt 995.800781mV
	//			Vcc(V) 3.3,  C(uF) 0.022,  T(us)  10,  Et(mV)  0.995801
	//			R =   1265.476196  Et =    0.995801
	//			R==1000 -> T =    7.902164
	//		WAIT_CLOCK_ARM_COUNTER = 4  +32で約10usに ->必要な時間 68
	//			ch sleepTime 10     0    adc     1077    volt 867.700195mV
	//			Vcc(V) 3.3,  C(uF) 0.022,  T(us)  10,  Et(mV)  0.867700
	//			R =   1489.896484  Et =    0.867700
	//			R==1000 -> T =    6.711875
	//
	
	
	//pinの初期化 受信するMISOはプルアップ
	InitPin(SPI_MOSI,	PIN_OUT);
	InitPin(SPI_MISO,	PIN_IN);
	PullUpDown(SPI_MISO, PULL_UP);
	InitPin(SPI_SCLK,	PIN_OUT);
	InitPin(SPI_CE0,	PIN_OUT);
	InitPin(SPI_CE1,	PIN_OUT);
	
	//通信の初期化(通信先のcsをあげる)
	GPIO_SET(SPI_CE0);
	
	//送信データの作成
	//開始bit	  SGL/DIFF	chの設定				(シングルエンド)
	//	1			1/0		000			ch0 	->  11000
	//	1			1/0		001			ch1 	->	11001
	//	1			1/0		010			ch2 	->	11010 
	//	1			1/0		011			ch3  	->	11011
	//	1			1/0		100			ch4  	->	11100
	//	1			1/0		101			ch5  	->	11101
	//	1			1/0		110			ch6  	->	11110
	//	1			1/0		111			ch7  	->	11111
	// STARTBIT | AD_SINGLE -> 0x03 0b11 
	send	= (0x3<<3) | ch;
	recive	= 0;
	
	//PrintGpioLevStatus(gpio);
	
	//待機時間の処理
	//データ送信時間 5byte 約5us(1MHz) -> 実測55(0.1us)
	//DelayArmTimerCounter, GPIO_CLR, GPIO_SETにかかる時間 約8(0.1)
	//データ変換時間 1us -> 10(0.1us)
	//計 7.3usが余分に掛かる時間 余分に+2しておく
	sleepTime *= 10;
	const int measureUseTime = 75;
	if( sleepTime > measureUseTime )
		sleepTime -= measureUseTime;
	else
		sleepTime = 0;
		
	//start = GetArmTimer();
	//指定されたpinを出力
	GPIO_SET(pin);
	DelayArmTimerCounter(sleepTime);
	//usleep(sleepTime);
	
	//通信の開始(通信先のcsを下げる)
	GPIO_CLR(SPI_CE0);
	
	//SCLKがlowから開始しているのでmode0,0のはず
	
	//printf("send start 0x%x\n", send);
	for(i=4; i>=0; i--)
	{
		//クロックが下がっている時 時間0.5us
			clkDownStart = GetArmTimer();
			//作業部
			bit = ( (send>>i)&0x01 );
			if( bit == 1 )
				GPIO_SET(SPI_MOSI);
			else
				GPIO_CLR(SPI_MOSI);
			//printf("%d ", bit);
			
		//クロックを上げる時間まで待つ
		WaitClk( clkDownStart );
		//クロックを上げる
		GPIO_SET(SPI_SCLK);
		//クロックが上がっている時 時間0.5us
			clkUpStart = GetArmTimer();
			//作業部
			//
			
			
		//クロックを下げる時間まで待つ
		WaitClk(clkUpStart);
		//クロックを下げる
		GPIO_CLR(SPI_SCLK);
	}
	//diff = GetArmTimer() - start;
	
	//ここで受信できないが1クロック分余分に回して
	//サンプリング用のクロックを消費
	//printf("recive start\n");
	for(i=13; i>=0; i--)
	{
		//クロックが下がっている時 時間0.5us
			clkDownStart = GetArmTimer();
			//作業部
			bit = GPIO_GET(SPI_MISO);
			recive |= bit<<i;
			//printf("%d ", bit);
			
		//クロックを上げる時間まで待つ
		WaitClk( clkDownStart );
		//クロックを上げる
		GPIO_SET(SPI_SCLK);
			//クロックが上がっている時 時間0.5us
			clkUpStart = GetArmTimer();
			//作業部
			//
		
		
		//クロックを下げる時間まで待つ
		WaitClk(clkUpStart);
		//クロックを下げる
		GPIO_CLR(SPI_SCLK);
	}
	
	//指定されたpinを初期化
	GPIO_CLR(pin);
	
	//通信の終了(通信先のcsをあげる)
	GPIO_SET(SPI_CE0);
	
	//printf("diff end - start %d\n", diff);
	
	//最初の2bitは不要(サンプリング時のもの+null bit)
	recive &= 0xFFF; //(0b 1111 1111 1111)
	//printf("recive 0x%X\n", recive);
	
	return recive;
}