
#ifndef GPIO_I2C_H
#define GPIO_I2C_H

#include <stdint.h>

#include "gpio.h"


//#define I2C_DEBUG
#ifdef I2C_DEBUG
	#define I2cDprintf	printf
#else
	#define I2cDprintf
#endif

/* データシートp28 BCM2835での詳細 */

#define I2C_0_BASE	( BCM2708_PERI_BASE + 0x00205000) //0x7E20 5000
#define I2C_1_BASE	( BCM2708_PERI_BASE + 0x00804000) //0x7E80 4000
#define I2C_2_BASE	( BCM2708_PERI_BASE + 0x00805000) //0x7E80 5000
#define I2C_C		0x0000					//BSCx + 0x0000	Control
#define I2C_S		0x0004/sizeof(uint32_t)	//BSCx + 0x0004	Status
#define I2C_DLEN	0x0008/sizeof(uint32_t)	//BSCx + 0x0008	Data Length
#define I2C_A		0x000C/sizeof(uint32_t)	//BSCx + 0x000C	Slave Address
#define I2C_FIFO	0x0010/sizeof(uint32_t)	//BSCx + 0x0010	Data FIFO
#define I2C_DIV		0x0014/sizeof(uint32_t)	//BSCx + 0x0014	Clock Divider
#define I2C_DEL		0x0018/sizeof(uint32_t)	//BSCx + 0x0018	Data Delay
#define I2C_CLKT	0x001C/sizeof(uint32_t)	//BSCx + 0x001C	Clock Stretch Timeout


//C Register Reserved 1～3, 11～14, 16～31
#define I2C_C_REGISTER_I2CEN	15		//I2CEN	I2C Enable 0->無効 1->有効
#define I2C_C_REGISTER_INTR		10		//interrupt on RX RXの状態によって割込みが発生 0->無効 1->有効 RXRが1の間発生
#define I2C_C_REGISTER_INTT		9		//interrupt on TX TXの状態によって割込みが発生 0->無効 1->有効 TXWが1の間発生
#define I2C_C_REGISTER_INTD		8		//interrupt on DONE DONEの状態によって割込みが発生 0->無効 1->有効 DONEが1の間発生
#define I2C_C_REGISTER_ST		7		//Start Transfer 0->何もしない 1->転送を開始し後0に
#define I2C_C_REGISTER_CLEAR	4		//FIFO Clear
	#define I2C_C_CLEAR_USE_BIT			2	//2bit 4-5
	#define I2C_C_CLEAR_NONE			0	//bin 00
	#define I2C_C_CLEAR_1				1	//bin 01
	#define I2C_C_CLEAR_2				2	//bin 10(互換性用?)
#define I2C_C_REGISTER_READ		0		//Read Transfer 0->書込みの転送 1->読込みの転送
	#define I2C_READ					1
	#define I2C_WRITE					0

//S Register Reserved 10～31
#define I2C_S_REGISTER_CLKT		9		//Clock Strech Timeout 0->エラーなし 1->スレーブ(クロックストレッチ)が長い
#define I2C_S_REGISTER_ERR		8		//ACK Error 0->エラーなし, 1->指定スレーブアドレスが存在しない
typedef enum _I2cError{
	NO_ERROR = 0,
	TIME_OUT = 1,
	NO_ADDR  = 2
}I2cError;

#define I2C_S_REGISTER_RXF		7		//FIFO Full 0->not full, 1->full,データの受信中なら読み出されるまでこれ以上受信しない
#define I2C_S_REGISTER_TXE		6		//FIFO Empty 0->not empty, 1->empty,データ送信中なら書き込まれるまで送信されない
#define I2C_S_REGISTER_RXD		5		//FIFO contains Data 0->empty 1->少なくても1byteのデータがある、読み出しでクリアされる
#define I2C_S_REGISTER_TXD		4		//FIFO can accept Data 0->full 1->少なくても1byteの空きがある
#define I2C_S_REGISTER_RXR		3		//FIFO needs Reading 1が立ったらFIFOから読み取りをして更新をする
#define I2C_S_REGISTER_TXW		2		//FIFO needs Writing 1が立ったらFIFOから書き込みをして更新をする
#define I2C_S_REGISTER_DONE		1		//Transfer Done 0->転送未完了 1->転送完了1を書き込むとクリア
#define I2C_S_REGISTER_TA		0		//Transfer Active 0->未転送 1->転送

//DLEN Register	Reserved 16～31
#define I2C_DLEN_REGISTER_DLEN		0		//Data Length
	#define I2C_DLEN_DLEN_USE_BIT		16		//16bit 0-15

//A Register	Reserved 7～31
#define I2C_A_REGISTER_ADDR		0		//Slave Address スレーブアドレスの指定
	#define I2C_A_ADDR_USE_BIT		7		//7bit 0-6

//FIFO Register	Reserved 8～31
#define I2C_FIFO_REGISTER_DATA		0		//転送するデータ
	#define I2C_FIFO_DATA_USE_BIT		8		//8bit 0-7		

//DIV Register	Reserved 16～31
#define I2C_DIV_REGISTER_CDIV		0		//Clock Divider SCL=core clock(150MHz)/CDIV
	#define I2C_DIV_CDIV_USE_BIT		16		//16bit 0-15	def 1500=100KHz

//DEL Register(よくわからん)
#define I2C_DEL_REGISTER_FEDL		16		//Falling Edge Delay
	#define I2C_DEL_FEDL_USE_BIT		16		//16bit 16-31
#define I2C_DEL_REGISTER_REDL		0		//Rising Edge Delay
	#define I2C_DEL_REDL_USE_BIT		16		//16bit 0-15
	
//CLKT Register	Reserved 16～31	
#define I2C_CLKT_REGISTER_TOUT		0		//Clock Stretch Timeout Value 反応がないと認識するクロック数
	#define I2C_CLKT_TOUT_USE_BIT		16		//16bit 0-15	

int InitI2c(RpiRevision rev);
int UnInitI2c();

void PrintI2cRegister();

unsigned int I2cSetClock(unsigned int speed);

int I2cSetDlen(uint16_t byteCount);
int I2cSetSlaveAddr(uint8_t addr);

void I2cClear();
I2cError I2cErrorCheck();
int I2cTransfer(uint8_t *tbuf, unsigned int tlen, uint8_t *rbuf, unsigned int rlen);
int I2cWrite(uint8_t *tbuf, unsigned int len);
int I2cRead(uint8_t *rbuf, unsigned int len);

int I2cSearch();
#endif
