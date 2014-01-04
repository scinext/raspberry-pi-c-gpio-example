
#ifndef ADCONVERT_H
#define ADCONVERT_H

//計算時のデータ
#define AD_RESOLUTION	12		//ICの分解能
#define AD_LSB			2		//ICの分解能で精度の問題で捨てるbit数
#define VREF			3300	//基準ボルト(mV)


int InitAD();
int UnInitAD();


//捨てるlsbを設定
int PinGetADCH(int pin, int ch, float *outADVolt, unsigned long sleepTime, unsigned int lsbBit);
int GetADCH(int CH, float *outADVolt, unsigned int lsbBit);

#endif