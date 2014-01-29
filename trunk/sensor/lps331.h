
#ifndef LPS331_H
#define LPS331_H

//取得するi2cのアドレス 0x5d->LPS331
#define I2C_ADDR_LPS331 0x5d
//i2cのアドレスを7bitモードと10bitモードのどちらにするか 1->10bit 0->7bit
#define I2C_ADDR_BIT	0

//LPS331の分解能 データシートより
#define LPS331_PRESS_RES			4096
#define LPS331_TEMP_RES				480
//LPS331の温度のオフセット
#define LPS331_TEMP_OFFSET			42.5

//i2cにデータを送るときのread write指定
#define I2C_READ					1
#define I2C_WRITE					0
#define LPS331_MULTI_DATA(addr)		(addr | 0x80)
#define LPS331_SINGLE_DATA(addr)	(addr)

//register
#define LPS331_REF_P				0x08	//0x08-0x0A P_XL, P_L, P_H, LPS331_AUTO_ZEROで使う

#define LPS331_WHO_AM_I				0x0F	//dummy register

#define LPS331_RES_CONF				0x10	//おそらく取得する分解能設定 RMS noise
	#define LPS331_RFU					7
												//0x7Aが最高で0.02mbar[hPa]だが取得間隔が25Hzだと取れないので
												//6Aにする必要がある(温度側の速度が遅いから?)
	#define LPS331_AVGT					4		//温度の分解能? max 0x7(0b0111) use 3bit
	#define LPS331_AVGP					0		//気圧の分解能? max 0xA(0b1010) use 4bit
												//値をシフトさせる 1<<LPS331_AVGT | 1<<LPS331_AVGPにする

//#define LPS331_ACTIVE				0x20
#define LPS331_CTRL1				0x20	//コントロールレジスタ
	//#define LPS331_ACTIVE_ON			0x90 //0b1001 0000
	//#define LPS331_ACTIVE_OFF			0x00
	#define LPS331_POWER				7			//power mode
		#define LPS331_POWER_OFF			0x00		//0b0000 0000 //downモード センサの値取得不可
		#define LPS331_POWER_ON				0x01		//0b1000 0000 //active モード センサの値取得可能
	#define LPS331_ODR					4			//ODR2-0は取得間隔の設定 max 0x7(0b111) use 3bit
		/*  設定できる取得間隔を最低1sにするので現状ではone shotのハードコーディングで行く
		 *	ODR*  2 1 0 | press    | tmp
		 *  -----------------------|----------
		 *        0 0 0 | One shot | One shot
		 *        0 0 1 | 1 Hz     | 1 Hz     | 1Hz    1s
		 *        0 1 0 | 7 Hz     | 1 Hz     | 7Hz    0.14s
		 *        0 1 1 | 12.5 Hz  | 1 Hz     | 12.5Hz 0.08s
		 *        1 0 0 | 25 Hz    | 1 Hz     | 25Hz   0.04s
		 *        1 0 1 | 7 Hz     | 7 Hz     |
		 *        1 1 0 | 12.5 Hz  | 12.5 Hz  |
		 *        1 1 1 | 25 Hz    | 25 Hz    |
		 */
	#define LPS331_DIFF					3	 	//THS_P_L/H レジスタとの差分で割り込みを発生させる
	#define LPS331_BDU					2	 	//block data update
														//1->MSBとLSBの更新を読み込みが終了するまで止める
														//0->常に更新するがあまりにも読み込まれないと更新が止まる?
														//   ので取得間隔より早く読み取る必要がある
	#define LPS331_DELTA				1 		//delta pressuerの使用(delta_pressレジスタ(3C-E)を校正用に使う?)
	#define LPS331_SIM					0			//SPIの3線と4線の選択

#define LPS331_CTRL2				0x21	//コントロールレジスタ2
	#define LPS331_BOOT					7			//メモリ内容の初期化させる
	//		reserved					6-3
	#define LPS331_SWRESET				2			//bootと一緒に1にした場合ソフトウェアリセットされる
	#define LPS331_AUTO_ZERO			1			//出力をREF_P_XXにコピーしてPRES_XXはREF_Pとの差分を保持する
	#define LPS331_ONE_SHOT				0			//一度のみセンサーを動かす

#define LPS331_CTRL3				0x22	//コントロールレジスタ3 割り込み
	#define LPS331_INT_H_L				7			//割り込みの有効化 1->active low, 0->active high
	#define LPS331_PP_OD				6			//push-pull/open drain 1->push-pull, 0->open drain
	#define LPS331_INT2_S				3			//max 0x7(0b111) use 3bit
	#define LPS331_INT1_S				0			//max 0x7(0b111) use 3bit
		/*
		 *  intX_S3 | intX_S3 | intX_S3 | intX ピン(highになる条件?)
		 *     0    |    0    |    0    |   GND
		 *     0    |    0    |    1    |   Pressure high(P_high)
		 *     0    |    1    |    0    |   Pressure low(P_low)
		 *     0    |    1    |    1    |   P_low or P_high
		 *     1    |    0    |    0    |   Data ready
		 *     1    |    0    |    1    |   reserved
		 *     1    |    1    |    0    |   reserved
		 *     1    |    1    |    1    |   Tri-state
		 */

#define LPS331_INT_CFG				0x23	//割り込みの設定
	//		reserved					7-3
	#define LPS331_LIR					2			//割り込みのラッチ(保管?) 0->not, 1->latche
	#define LPS331_PL					1			//low時の割り込み  0->disable, 1->enable
	#define LPS331_PH					0			//high時の割り込み 0->disable, 1->enable

#define LPS331_INT_SOURCE			0x24	//割り込みの状態(LPS331_LIRが1で有効) INT_ACKレジスタの読み込みでクリア
	//		reserved					7-3
	#define LPS331_IA								//割り込みの状態 0->割り込み未発生, 1->割り込み発生
	#define LPS331_SRC_PL							//lowでの割り込みの状態  0->割り込み未発生, 1->割り込み発生
	#define LPS331_SRC_PH							//highでの割り込みの状態 0->割り込み未発生, 1->割り込み発生

#define LPS331_THS_P_L				0x25	//割り込みで使う
#define LPS331_THS_P_H				0x26	//割り込みで使う 基準気圧( THS_P_H & THS_P_L )[dec]/16

#define LPS331_STATUS				0x27	//ステータス
	//		reserved					7-6
	#define LPS331_P_OR					5			//前回気圧が読み込まれず新しい気圧で上書きされた,
													//気圧のレジスタを読み込むと0に
	#define LPS331_T_OR					4			//温度で同じ
	//		reserved					3-2
	#define LPS331_P_DA					1			//気圧がレジスタにセットされた
													//気圧のレジスタを読み込むと0に
	#define LPS331_T_DA					0			//気温で同じ

#define LPS331_PRESS				0x28	//気圧レジスタ 0x28-0x2A
	#define LPS331_PRESS_REG_COUNT		3	//使用するレジスタの数

#define LPS331_TEMP					0x2B	//温度レジスタ 0x2B-0x2C
	#define LPS331_TEMP_REG_COUNT		2	//使用するレジスタの数

#define LPS331_AMP_CTRL				0x22	//オペアンプの電流セレクタ(googleより)
	//		reserved					7-1
	#define LPS331_SELMAIN				0			//1->常に高電流, 0->気圧は高電流,温度は低電流
#define LPS331_DELTA_PRESS			0x3C	//0x3C-0x3E LPS331_DELTAで使用 使い方は分からん 

int InitLps331();
int UnInitLps331();
void DispLps331Register();
void DispLps331Data();
int ClearLps331();
int WakeUpLps331();

#endif
