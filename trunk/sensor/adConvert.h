
#ifndef ADCONVERT_H
#define ADCONVERT_H

//計算時のデータ
#define AD_RESOLUTION	12		//ICの分解能
#define AD_LSB			2		//ICの分解能で精度の問題で捨てるbit数
#define VREF			3300	//基準ボルト(mV)

//通常のspiでの送受信
#define AD_PIN_GAP		100	//データ送信からADコンバートが終了するまでの時間(0.1us単位)

//短縮した送受信(レジスタを使ったSPIで必要ないパディングを削除したとき)
#define AD_NO_PADDING_GAP		73	//7.3us

int InitAD();
int UnInitAD();

//パディングを使ったデータ送受信かどうか
#define PADDING_USE	0
#define PADDING_NO	1
void SetSPIData(int ch, int noPad);


//捨てるlsbを設定
static inline float AtoDmV(unsigned int ad, int lsbBit)
{
	//下位8bitのみと次のデータを使用(取得データの下位bitは精度の問題で捨てる)
	unsigned int lsbAd, maxAd;
	lsbAd = ad>>lsbBit;
	maxAd = 1<<(AD_RESOLUTION-lsbBit);
	return  lsbAd * ((float)VREF / maxAd);
}

//AD変換に失敗した場合(恐らく大体spiでエラー)
#define AD_ERROR	-1

unsigned int GetAD(int ch);
unsigned int GetADpin(int pin, int ch, unsigned long sleepTime);

unsigned int GetADNoPad(int ch);
unsigned int GetADNoPadPin(int pin, int ch, unsigned int sleepTime);
unsigned int GetADmcp3204(int pin, int ch, unsigned long sleepTime);
#endif