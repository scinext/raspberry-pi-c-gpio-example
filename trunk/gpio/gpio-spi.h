
#ifndef GPIO_SPI_H
#define GPIO_SPI_H

#include <stdint.h>

#include "gpio.h"


//#define SPI_DEBUG
#ifdef SPI_DEBUG
	#define SpiDprintf	printf
#else
	#define SpiDprintf
#endif
/* データシートp148 BCM2835での詳細 */

#define SPI_BASE	( BCM2708_PERI_BASE + 0x00204000) //0x7E20 4000
#define SPI_CS		0x0000					//0x7E20 4000(32bit)	CS SPI Master Control and Status
#define SPI_FIFO	0x0004/sizeof(uint32_t)	//0x7E20 4004(32bit)	FIFO SPI Master TX and RX FIFOs
#define SPI_CLOCK	0x0008/sizeof(uint32_t)	//0x7E20 4008(32bit)	CLK SPI Master Clock Divider
#define SPI_DLEN	0x000C/sizeof(uint32_t)	//0x7E20 400C(32bit)	DLEN SPI Master Data Length
#define SPI_LTOH	0x0010/sizeof(uint32_t)	//たぶん使わない 0x7E20 4010(32bit)	LTOH SPI LOSSI mode TOH
#define SPI_DC		0x0014/sizeof(uint32_t)	//たぶん使わない 0x7E20 4014(32bit)	DC SPI DMA DREQ Controls

//CS Register
#define SPI_CS_REGISTER_LEN_LONG	25	//DMAmodeの時に1byteか32bitのデータを使うか 0->1byte 1->32bit 
#define SPI_CS_REGISTER_DMA_LEN		24	//Enable DMA mode in Lossi mode
#define SPI_CS_REGISTER_CSPOL2		23	//cs2をアクティブにするhigh,low選択(DMAの時は手動で通常は自動?) 0->low 1->high
#define SPI_CS_REGISTER_CSPOL1		22	//cs1 同上
#define SPI_CS_REGISTER_CSPOL0		21	//cs0 同上
#define SPI_CS_REGISTER_RXF			20	//RX Full 0->not full 1->full(読込されないとこれ以上は送受信がされない)
#define SPI_CS_REGISTER_RXR			19	//RX needs Reading 0->not full又はTA=0でない 1->full 読込むかTA=0でクリアされる
#define SPI_CS_REGISTER_TXD			18	//TX can accept Data 0->full(これ以上入れれない) 1->not full(最低1byteは空きがある)
#define SPI_CS_REGISTER_RXD			17	//RX contains Data 0->空 1->1byte以上は入ってる
#define SPI_CS_REGISTER_DONE		16	//transfer Done 転送の状態 0->進捗中又はTA=0 1->完了 TXにデータを書込むかTA=0でクリア
#define SPI_CS_REGISTER_TE_EN		15	//未使用
#define SPI_CS_REGISTER_LMONO		14	//未使用
#define SPI_CS_REGISTER_LEN			13	//LoSSi enable LoSSiの有効化 0->SPI master 1->LoSSi master
#define SPI_CS_REGISTER_REN			12	//Read Enable 読込と書込のどちらか default 1
	#define SPI_CS_REN_READ					1	//1->Read
	#define SPI_CS_REN_WRITE				0	//0->Write
#define SPI_CS_REGISTER_ADCS		11	//Automatically Deassert Chip Select DMAの時に自動でチップを選ぶか 0->手動 1->自動
#define SPI_CS_REGISTER_INTR		10	//Interrupt on RXR RXRの状態で割込みさせるか 0->させない 1->させる(RXRが1の間)
#define SPI_CS_REGISTER_INTD		9	//Interrupt on Done 転送完了時の割込み 0->させない 1->させる(DONEが1の間)
#define SPI_CS_REGISTER_DMAEN		8	//DMA Enable DMAの有効 0->無効 1->有効(詳細不明 データシートp155)
#define SPI_CS_REGISTER_TA			7	//Transfer Active 0->未転送	1->転送	
#define SPI_CS_REGISTER_CSPOL		6	//Chip Select Polarity 選択したCSのラインの有効化? 0->low 1->high
#define SPI_CS_REGISTER_CLEAR		4	//2bit //FIFO Clear 00->No action x1->TX FIFOの初期化 1x->RX FIFOの初期化
	#define SPI_CS_CLEAR_USE_BIT			2	//2bit 
	typedef enum _SpiClearMode{
		SPI_CLR_NO=0x00,	//bin 0000
		SPI_CLR_TX=0x01,	//bin 0001
		SPI_CLR_RX=0x02,	//bin 0010
		SPI_CLR_ALL=0x03,	//bin 0011 ( CLR_TX | CLR_RX )
	} SpiClearMode;
	//#define SPI_CS_REGISTER_CLEAR_TX	0x01	//bin 01
	//#define SPI_CS_REGISTER_CLEAR_RX	0x02	//bin 10
#define SPI_CS_REGISTER_CPOL		3	//Clock Polarity  クロックの休止状態 0->low 1->high
#define SPI_CS_REGISTER_CPHA		2	//Clock Phase データの転送を波形のどのタイミングで行うか 0->途中 1->立上り
#define SPI_CS_REGISTER_CS			0	//Chip Select 00->chip0, 01->chip1, 10->chip2, 11->Reserved
	#define SPI_CS_CS_USE_BIT				2	//2bit 
	typedef enum _SpiCsSelect{
		SPI_CS_0=0x00,	//bin 0000
		SPI_CS_1=0x01,	//bin 0001 
		SPI_CS_2=0x02	//bin 0002
	} SpiCsSelect;
	//#define SPI_CS_REGISTER_CS_CS0		0x00	//bin 00
	//#define SPI_CS_REGISTER_CS_CS1		0x01	//bin 01
	//#define SPI_CS_REGISTER_CS_CS2		0x02	//bin 10

typedef enum _SpiMode{
	SPI_MODE_0=0x00,	//bin 0000 CPOL=0, CPHA=0
	SPI_MODE_1=0x01,	//bin 0001 CPOL=0, CPHA=1
	SPI_MODE_2=0x02,	//bin 0010 CPOL=1, CPHA=0
	SPI_MODE_3=0x03 	//bin 0011 CPOL=1, CPHA=1
}SpiMode;

//CLK Register
//データシート p21 計算式はここ SPIx_CLK = system_clock / 2*(speed_filed+1)
#define SPI_CLK_REGISTER_CDIV	0	//転送用のクロックの設定 SCLK=CoreClock/CDIVになる 0-65536の間
	#define SPI_CLK_CDIV_USE_BIT	16

/*
ALT0                                 |ALT0        
SPI0 MOSI | GPIO_10 |19  20| GND     |          
SPI0 MISO | GPIO_9  |21  22| GPIO_25 |          
SPI0 SCLK | GPIO_11 |23  24| GPIO_8  |SPI0 CE0_N
          | GND     |25  26| GPIO_7  |SPIO CE1_N
*/
#define SPI_MOSI	P1_19
#define SPI_MISO	P1_21
#define SPI_SCLK	P1_23
#define SPI_CE0		P1_24
#define SPI_CE1		P1_26

int InitSpi();
int UnInitSpi();

void PrintSpiRegister();
void PrintSpiRegisterNoFIFO(int log);

void SpiSetCS(SpiCsSelect cs);
void SpiSetMode(SpiMode mode);
void SpiSetCSPolarity(SpiCsSelect cs, LineSignal activeLevel);
void SpiClear(SpiClearMode mode);
unsigned int SpiSetClock(unsigned int speed);

void SpiSetWriteMode();
void SpiSetReadMode();

//Polled(ポーリングでの転送)
uint32_t SpiTransfer(uint8_t td);
int SpiTransferMulitple(uint8_t *td, uint8_t *rd, unsigned int len);

//AD変換を行う時にできる限り無駄をなくすために直接行う
int SpiTransferMulitpleAndPinHighLow(uint8_t *td, uint8_t *rd, unsigned int len, int pin, unsigned int sleepTime);


#endif
