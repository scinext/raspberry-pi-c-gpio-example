
#ifndef GPIO_H
#define GPIO_H

#define DEBUG

#ifndef DEBUG
	#define Dgprintf
#else
	#define Dgprintf printf
#endif

//型を何もつけないとintになるっぽいsizeofが4でintと同じだった
#ifdef DEF_GPIO
	#define EXTERN 
#else
	#define EXTERN extern
#endif
EXTERN volatile unsigned int *gpio;

/* データシートp89 BCM2835での詳細 */

#define BCM2708_PERI_BASE 0x20000000 //0x2000 0000 ->0x7E00 0000になる
#define GPIO_BASE ( BCM2708_PERI_BASE + 0x00200000) //0x7E20 0000

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

//system clock 250MHz
#define SYS_CLOCK	250000000

//信号レベル
typedef enum _LineSignal{
	LOW=0,
	HIGH=1
}LineSignal;

//Rpiのリビジョン
typedef enum _RpiRevision{
	REV_1=1,
	REV_2=2
}RpiRevision;

//とりあえず下記はrev2

//P1ヘッダー
#define V3
#define V5
#define GND
//rev1
#define GPIO_0 	0
#define GPIO_1 	1
//rev2
#define GPIO_2 	2
#define GPIO_3 	3

#define GPIO_4 	4
#define GPIO_17	17
#define GPIO_27	27
#define GPIO_22	22
#define GPIO_10	10
#define GPIO_9 	9
#define GPIO_11	11
#define GPIO_14	14
#define GPIO_15	15
#define GPIO_18	18
#define GPIO_23	23
#define GPIO_24	24
#define GPIO_25	25
#define GPIO_8 	8
#define GPIO_7 	7
//P1ヘッダー
#define P1_1	V3
//rev1
//#define P1_3	GPIO_0
//#define P1_5	GPIO_1
//rev2
#define P1_3	GPIO_2
#define P1_5	GPIO_3
#define P1_7	GPIO_4
#define P1_9	GND
#define P1_11	GPIO_17
#define P1_13	GPIO_27
#define P1_15	GPIO_22
#define P1_17	V3
#define P1_19	GPIO_10
#define P1_21	GPIO_9
#define P1_23	GPIO_11
#define P1_25	GND

#define P1_2	V5
#define P1_4	V5
#define P1_6	GND
#define P1_8	GPIO_14
#define P1_10	GPIO_15
#define P1_12	GPIO_18
#define P1_14	GND
#define P1_16	GPIO_23
#define P1_18	GPIO_24
#define P1_20	GND
#define P1_22	GPIO_25
#define P1_24	GPIO_8
#define P1_26	GPIO_7

#define PIN_INIT	0x0 //初期化用
#define PIN_IN		0x0 //000
#define PIN_OUT		0x1 //001
#define PIN_ALT0	0x4 //100
#define PIN_ALT1	0x5 //101
#define PIN_ALT2	0x6 //110
#define PIN_ALT3	0x7 //111
#define PIN_ALT4	0x3 //011
#define PIN_ALT5	0x2 //010
void InitPin(unsigned int pin, int ioFlag);

void InitGpio();

//指定pin以外に1を立て、指定pinを0にしてANDをして初期化する
//#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
//#define IN_GPIO(g) *(gpio+((g)/10)) &= ~( 0x07<<( 3 * ((g)%10) ) )
//行う内容はIN_GPIOと全く同じだがわざと初期化を区別するため別に宣言してINの実体を無くした
#define INIT_GPIO(g) *(gpio+((g)/10)) &= ~( 0x07<<( ((g)%10) * 3 ) )
#define IN_GPIO

//初期化したpinに1を立ててoutputにする
//#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  ( 0x01<<( ((g)%10) * 3 ) )

//初期化後(0クリア)にALTのフラグを立てる
#define ALT_GPIO(g, alt) *(gpio+((g)/10)) |=  ( alt<<( 3 * ((g)%10) ) )


////0x20001C	gpio(0x200000) + 0x1C(sizeof(int)*7)
////#define GPIO_SET2 *(gpio+7)	//GPIO_SET2 = 1<<pin;
#define GPIO_SET(pin) *(gpio+7) = 1<<pin
#define GPIO_SETo32(pin) *(gpio+8) = 1<<(pin-32)

////0x20001C	gpio(0x200000) + 0x1C(sizeof(int)*7)
////#define GPIO_CLR2 *(gpio+10)	//GPIO_CLR2 = 1<<pin;
#define GPIO_CLR(pin) *(gpio+10) = 1<<pin
#define GPIO_CLRo32(pin) *(gpio+11) = 1<<(pin-32)

unsigned int SetRegisterBit(volatile unsigned int *reg, unsigned int bit, unsigned int useBit, unsigned int value);
unsigned int SetRegisterBitDebug(volatile unsigned int *reg, unsigned int bit, unsigned int useBit, unsigned int value);
unsigned int GetRegisterBit(volatile unsigned int *reg, unsigned int bit, unsigned int useBit);
unsigned int GetRegisterBitDebug(volatile unsigned int *reg, unsigned int bit, unsigned int useBit);
#endif     