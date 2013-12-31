
#include <stdio.h>

//open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//mmap
#include <sys/mman.h>
//errno
#include <errno.h>

//clock_gettime gccで -lrtをつけないとコンパイルエラー
#include <time.h>

#include "gpio.h"
#include "gpio-util.h"
#include "gpio-spi.h"
#include "gpio-timer.h"

char *spi_map = NULL;
volatile unsigned int *spi = NULL;

void PrintSpiRegister()
{
	PrintRegStatus(stdout, spi, SPI_CS,		"SPI_CS          ",	1);
	PrintRegStatus(stdout, spi, SPI_FIFO,	"SPI_FIFO        ",	0);
	PrintRegStatus(stdout, spi, SPI_CLOCK,	"SPI_CLOCK       ",	0);
	PrintRegStatus(stdout, spi, SPI_DLEN,	"SPI_DLEN        ",	0);
}
void PrintSpiRegisterNoFIFO(int log)
{
	FILE *fp;
	if( log == 0 )
		fp = stdout;
	else
		fp = NULL;
		
	PrintRegStatus(fp, spi, SPI_CS,		"SPI_CS          ",	1);
	PrintRegStatus(fp, spi, SPI_CLOCK,	"SPI_CLOCK       ",	0);
	PrintRegStatus(fp, spi, SPI_DLEN,	"SPI_DLEN        ",	0);
}
int InitSpi()
{
	int mem_fd;
	
	if( spi != NULL )
		return 1;

	if( gpio == NULL )
	{
		perror("gpio NULL");
		return -1;
	}

	if( (mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0 )
	{
		printf("can't open /dev/mem: %s\n", strerror(errno));
		return -1;
	}

	spi_map = (char *)mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, SPI_BASE);
	if( spi_map == MAP_FAILED )
	{
		printf("mmap error %d\n", (int)spi_map);
		return -1;
	}
	SpiDprintf("mmap SPI_BASE %s %X\n", strerror(errno), (int32_t)spi_map);

	close(mem_fd);

	spi = (volatile unsigned int *)spi_map;

	InitPin(SPI_MOSI,	PIN_ALT0);
	InitPin(SPI_MISO,	PIN_ALT0);
	InitPin(SPI_SCLK,	PIN_ALT0);
	InitPin(SPI_CE0,	PIN_ALT0);
	InitPin(SPI_CE1,	PIN_ALT0);

	//PrintSpiRegister();
	//TAを0にして未転送状態へ
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 0);

	return 1;
}
int UnInitSpi()
{
	InitPin(SPI_MOSI,	PIN_INIT);
	InitPin(SPI_MISO,	PIN_INIT);
	InitPin(SPI_SCLK,	PIN_INIT);
	InitPin(SPI_CE0,	PIN_INIT);
	InitPin(SPI_CE1,	PIN_INIT);

	//取得したSPIの箇所の仮想アドレスの解放
	munmap(spi_map, BLOCK_SIZE);

	return 1;
}
void SpiSetCS(SpiCsSelect cs)
{
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_CS, SPI_CS_CS_USE_BIT, cs);
}
void SpiSetMode(SpiMode mode)
{
	//mode0=0x00	CPOL=0, CPHA=0
	//mode1=0x01	CPOL=0, CPHA=1
	//mode2=0x10	CPOL=1, CPHA=0
	//mode3=0x11	CPOL=1, CPHA=1

	//1bit目が1ならCPHAを立てる
	if( mode & 0x01 != 0 )
		SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_CPHA, 1, 1);

	//2bit目が1ならCPOLを立てる
	if( mode & 0x02 != 0 )
		SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_CPOL, 1, 1);
	return;
}
void SpiSetCSPolarity(SpiCsSelect cs, LineSignal activeLevel)
{
	//#define SPI_CS_REGISTER_CSPOL2		23	//cs2のアクティブ時のhigh,lowの選択 0->low 1->high
	//#define SPI_CS_REGISTER_CSPOL1		22	//cs1 同上
	//#define SPI_CS_REGISTER_CSPOL0		21	//cs0 同上
	switch(cs)
	{
		case SPI_CS_0:
			SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_CSPOL0, 1, (unsigned int)activeLevel);
			break;
		case SPI_CS_1:
			SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_CSPOL1, 1, (unsigned int)activeLevel);
			break;
		case SPI_CS_2:
			SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_CSPOL2, 1, (unsigned int)activeLevel);
			break;
	}
	return;
}
unsigned int SpiSetClock(unsigned int speed)
{
	//レジスタアドレス
	//#define SPI_CLOCK	0x0008	//0x7E20 4008  CLK SPI Master Clock Divider

	//計算式
	//データシート p21
	//SPIx_CLK		= system_clock / 2*(speed_filed+1)
	//system_clock	= 250MHz
	//speed_filed	= 0 -> 125MHz

	//変形
	//speed_filed = ((system_clock/2)/SPIx_CLK) - 1

	/*
	 *	CDIV = 0 -> 125MHz
	 *	0 		125.00MHz	8ns(MAX Speed)
	 *	2 		41.67MHz 	25ns
	 *	4 		25.00MHz 	40ns
	 *	8 		13.89MHz 	72ns
	 *	16 		7.35MHz 	126ns
	 *	32 		3.79Mhz 	264ns
	 *	64 		1.92MHz 	520ns
	 *	128 	968.99KHz	1.03us
	 *	256 	486.38KHz	2.06us
	 *	512 	243.67KHz	4.10us
	 *	1024 	121.95KHz	8.20us
	 *	2048 	61.01KHz	16.39us
	 *	4096 	30.51KHz	32.78us
	 *	8192 	15.26KHz	65.54us
	 *	16384 	7.63KHz 	121.08us
	 *	32768 	3.82KHz 	262.15us
	 *	65536 	1.91KHz 	524.30us(MIN speed)
	 *
	 *	MCP3204
	 *		clock 5v->2MHz,	3v->1MHz
	 *			3.3-> (2-1)MHz/(5-2.7)v*(3.3v-2.7v)+1MHz ->1.26MHzまでいける?
	 *		1.65MHz
	 *			CDIV->75 CLK->1.64MHz(610ns)
	 *
	 */

	unsigned int clockDivider, clkClock;

	//初期化
	SetRegisterBit(spi+SPI_CLOCK, SPI_CLK_REGISTER_CDIV, SPI_CLK_CDIV_USE_BIT, 0);

	clockDivider = ((SYS_CLOCK/2)/speed) - 1;

	//MAX 65535
	//65535 = ( 1<<CLK_CDIV_USE_BIT ) - 1;
	clockDivider = clockDivider > 65535 ? 65535 : clockDivider;

	clkClock = SYS_CLOCK/(2*(clockDivider+1));
	//もし計算上指定した数より周波数が高くなってたらDividerを一つ上げる
	if( clkClock > speed && clockDivider < 65535 )
	{
		SpiDprintf("clock %u, divider %u -> Clock %u\n", speed, clockDivider, clkClock);
		++clockDivider;
		clkClock = SYS_CLOCK/(2*(clockDivider+1));
	}
	SpiDprintf("clock %u, divider %u -> Clock %u\n", speed, clockDivider, clkClock);

	SetRegisterBit(spi+SPI_CLOCK, SPI_CLK_REGISTER_CDIV, SPI_CLK_CDIV_USE_BIT, clockDivider);
	return clkClock;
}

void SpiClear(SpiClearMode mode)
{
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_CLEAR, SPI_CS_CLEAR_USE_BIT, mode);
	return;
}

void SpiSetWriteMode()
{
	//書き込み状態にする
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_REN, 1, SPI_CS_REN_WRITE);
	return;
}
void SpiSetReadMode()
{
	//読み込み状態にする
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_REN, 1, SPI_CS_REN_READ);
	return;
}

uint32_t SpiTransfer(uint8_t td)
{
	//Polled(ポーリング bcm2835はこの方法っぽい)
	//	1.CS, CPOL, CPHA(チップとSPIモードの設定)をしてTAを1にする
	//	2.TXDにデータを書込んだりRXDからデータを読み取る(FIFOレジスタが送信時-TX、受信時-RXになる)
	//	3.DONEが1になるまで待機
	//	4.TAを0にして完了

	//	#define SPI_CS		0x0000					//0x7E20 4000(32bit)
	//	#define SPI_FIFO	0x0004/sizeof(uint32_t)	//0x7E20 4004(32bit)
	//
	//	#define SPI_CS_REGISTER_TXD			18	//TX can accept Data
	//	#define SPI_CS_REGISTER_DONE		16	//transfer Done

	uint32_t txd;

	SpiDprintf("spi transfer\n");
	//TXとRXのクリア
	SpiClear(SPI_CLR_ALL);

	//TAを1に
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 1);

	//TXDフラグが1になるまで(0の間は待機)
	SpiDprintf("send td\n");
	SpiDprintf("\twait TXD(bit-%d)->1\n", SPI_CS_REGISTER_TXD);
	while( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TXD, 1)==0 )
		;

	//FIFOに書き込む
	SpiDprintf("\twrite SPI_FIFO 0x%x td %d\n", (spi+SPI_FIFO), td);
	*(spi+SPI_FIFO) = td;

	//DONEフラグが1になるまで(0の間は待機)
	SpiDprintf("\twait DONE(bit-%d)->1\n", SPI_CS_REGISTER_DONE);
	while( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_DONE, 1)==0 )
		;

	//FIFOから読み込む
	txd = *(spi+SPI_FIFO);

	//TAを0にして完了
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 0);

	return txd;
}

int SpiTransferMulitple(uint8_t *td, uint8_t *rd, unsigned int len)
{
	//連続データの送信
	//Polled(ポーリング bcm2835はこの方法っぽい)
	//	1.CS, CPOL, CPHA(チップとSPIモードの設定)をしてTAを1にする
	//	2.TXDにデータを書込、RXDからデータを読取
	//	3.書き込みデータがあるなら繰り返す2をDONEが1になるまで待機
	//	4.TAを0にして完了
	//	#define SPI_CS_REGISTER_TXD			18	//TX can accept Data
	//	#define SPI_CS_REGISTER_RXD			17	//RX contains Data 0->空 1->1byte以上は入ってる
	//	#define SPI_CS_REGISTER_DONE		16	//transfer Done
	unsigned int i;

	if( len < 0 )
	{
		perror("not data");
		return;
	}
	SpiDprintf("spi transfer length %d\n", len);
	//TXとRXのクリア
	SpiClear(SPI_CLR_ALL);

	//TAを1に
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 1);

	for(i=0; i<len; i++)
	{
		SpiDprintf("\tsend td[%d]\n");

		SpiDprintf("\t\twait TXD(bit-%d)->1\n", i, SPI_CS_REGISTER_TXD);
		//TXDフラグが1になるまで(0の間は待機)
		while( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TXD, 1)==0 )
			;

		//FIFOに書き込む
		SpiDprintf("\t\twrite SPI_FIFO 0x%x td %d\n", (spi+SPI_FIFO), td[i]);
		*(spi+SPI_FIFO) = td[i];

		//PrintSpiRegisterNoFIFO(0);

		//_RXDフラグが1になるまで(0の間は待機)
		SpiDprintf("\t\twait RXD(bit-%d)->1\n", SPI_CS_REGISTER_RXD);
		while( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_RXD, 1) == 0 )
		{
			//RXDを見てからDONEを見るとその間にRXDが立つことがあるので先にDONEを見てRXDを見る
			//受信できていないのにDONEフラグが立った時
			if( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_DONE, 1) == 1 &&
				GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_RXD, 1) == 0
			)
			{
				time_t 		t;
				struct tm 	*ts;
				char		dateBuf[20];
				
				t  = time(NULL);
				ts = localtime(&t);
				strftime(dateBuf, sizeof(dateBuf), "%F %T", ts);
				
				PrintLog(NULL, "%s", dateBuf);
				PrintLog(NULL, "  spi RXD polling miss RXD(bit-%d) DONE(bit-%d)\n",
					SPI_CS_REGISTER_RXD, SPI_CS_REGISTER_DONE);
				PrintSpiRegisterNoFIFO(1);
				
				//TAを0にして完了
				SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 0);
				return -1;
			}
		}

		//FIFOから読み込む
		rd[i] = *(spi+SPI_FIFO);
		SpiDprintf("\trecive rd[%d]=%d\n\n", i, rd[i]);
	}
	SpiDprintf("\tsend complete\n");
	//DONEフラグが1になるまで(0の間は待機)
	SpiDprintf("\t\twait DONE(bit-%d)->1\n", SPI_CS_REGISTER_DONE);
	while( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_DONE, 1)==0 )
		;

	//TAを0にして完了
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 0);

	SpiDprintf("complete transfer\n");
	return 0;
}

int SpiTransferMulitpleAndPinHighLow(uint8_t *td, uint8_t *rd, unsigned int len, int pin, unsigned int sleepTime)
{
	unsigned int i;

	if( len < 0 )
	{
		perror("not data");
		return;
	}
	SpiDprintf("spi transfer length %d\n", len);

	//TXとRXのクリア
	SpiClear(SPI_CLR_ALL);

	//TAを1に
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 1);

	//pinの初期化
	InitPin(pin, PIN_OUT);

	//struct timespec startTs, endTs;
	//clock_gettime(CLOCK_MONOTONIC, &startTs);
	//pinをHighへ
	GPIO_SET(pin);
	DelayArmTimerCounter(sleepTime);

	for(i=0; i<len; i++)
	{
		SpiDprintf("\tsend td[%d]\n");

		SpiDprintf("\t\twait TXD(bit-%d)->1\n", i, SPI_CS_REGISTER_TXD);
		//TXDフラグが1になるまで(0の間は待機)
		while( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TXD, 1)==0 )
			;

		//FIFOに書き込む
		SpiDprintf("\t\twrite SPI_FIFO 0x%x td %d\n", (spi+SPI_FIFO), td[i]);
		*(spi+SPI_FIFO) = td[i];

		//コンソールへ出力をするとRXDのポーリングが間に合わず
		//DONEフラグが立ってRXDが立たず無限ループに陥る
		//PrintSpiRegisterNoFIFO();

		//_RXDフラグが1になるまで(0の間は待機)
		SpiDprintf("\t\twait RXD(bit-%d)->1\n", SPI_CS_REGISTER_RXD);
		while( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_RXD, 1) == 0 )
		{
			//RXDを見てからDONEを見るとその間にRXDが立つことがあるので先にDONEを見てRXDを見る
			//受信できていないのにDONEフラグが立った時
			if( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_DONE, 1) == 1 &&
				GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_RXD, 1) == 0
			)
			{
				time_t 		t;
				struct tm 	*ts;
				char		dateBuf[20];
				
				t  = time(NULL);
				ts = localtime(&t);
				strftime(dateBuf, sizeof(dateBuf), "%F %T", ts);
				
				PrintLog(NULL, "%s", dateBuf);
				PrintLog(NULL, "  spi RXD polling miss RXD(bit-%d) DONE(bit-%d)\n",
					SPI_CS_REGISTER_RXD, SPI_CS_REGISTER_DONE);
				PrintSpiRegisterNoFIFO(1);
				
				//TAを0にして完了
				SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 0);
				return -1;
			}
		}

		//FIFOから読み込む
		rd[i] = *(spi+SPI_FIFO);
		SpiDprintf("\trecive rd[%d]=%d\n\n", i, rd[i]);
	}
	
	//pinをLowへ
	GPIO_CLR(pin);
	//clock_gettime(CLOCK_MONOTONIC, &endTs);
	//TimeDiff(&startTs, &endTs, sleepTime);

	SpiDprintf("\tsend complete\n");
	//DONEフラグが1になるまで(0の間は待機)
	SpiDprintf("\t\twait DONE(bit-%d)->1\n", SPI_CS_REGISTER_DONE);
	while( GetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_DONE, 1)==0 )
		;

	//TAを0にして完了
	SetRegisterBit(spi+SPI_CS, SPI_CS_REGISTER_TA, 1, 0);

	SpiDprintf("complete transfer\n");

	return 0;
}
