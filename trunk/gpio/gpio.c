
/*	GPIOを操作するのにARMのレジスタを操作する必要がある
 *	BCM2708_PERI_BASEはここからARMのPeripheralsの物理的(Physical)なアドレスで
 *	カーネルの共通仮想アドレスだと0xF200 0000に当たる
 *	GPIOのレジスタはそこから0x0020 0000にある
 *	プログラムは使用アドレス範囲が(0x0000 0000-0xBFFF FFFF)のため0xF200 0000にアクセスできないため
 *	BCM2708_PERI_BASEはプログラムベースの仮想アドレス0x7Exx xxxxにマップされる
 *	GPIOの使用範囲は0x7E20 0000 - 0x7E20 00B0
 *	仮想アドレスにアクセスするには /dev/kmem を解することで行えるがセキュリティ上よくないらしい
 *	http://www.raspberrypi.org/phpBB3/viewtopic.php?t=8476&p=101994
 */


#include <stdio.h>
#include <stdlib.h>
//open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//mmap
#include <sys/mman.h>
//errno
#include <errno.h>

#define DEF_GPIO
#include "gpio.h"
char *gpio_map;


#include "gpio-timer.h"

void InitGpio()
{
	int mem_fd;
	//現在のメモリのファイルポインタの取得
	if( (mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0 )
	{
		printf("can't open /dev/mem: %s\n", strerror(errno));
		exit(-1);
	}

	//現在のメモリからGPIOの箇所の仮想アドレスを再取得
	//解放するときはmunmap()を使うプロセスが終了すれば自動で開放される
	gpio_map = (char *)mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);

	//mmapにてaddrを指定すると<0判定でもいいが
	//unsignedのため<0判定に引っかかるのでMAP_FAILEDにする if( (long)gpio_map < 0)
	if( gpio_map == MAP_FAILED )
	{
		printf("mmap error %d\n", (int)gpio_map);
		exit(-1);
	}
	//Dgprintf("mmap GPIO_BASE %s %X\n", strerror(errno), (int32_t)gpio_map);

	//メモリのクローズ
	close(mem_fd);

	gpio = (volatile unsigned int *)gpio_map;


	InitSysTimer();
}

void InitPin(unsigned int pin, int ioFlag)
{
	//printf("init %2d pin-> %d\n", pin, ioFlag);
	//初期化(実際はIN_GPIOと同じ)
	INIT_GPIO(pin);
	switch( ioFlag )
	{
		//case PIN_INIT: 実際はPIN_INと同じ 0が来る
		case PIN_IN:
			//実際は何もしない
			IN_GPIO(pin);
			break;
		case PIN_OUT:
			OUT_GPIO(pin);
			break;
		case PIN_ALT0:
		case PIN_ALT1:
		case PIN_ALT2:
		case PIN_ALT3:
		case PIN_ALT4:
		case PIN_ALT5:
			ALT_GPIO(pin, ioFlag);
			break;
	}
}

void PullUpDown(unsigned int pin, int upDown)
{
	//データシート p101だと150cycle待機
	//データシート p105だと一番遅いものだと14.29MHzで約0.07us
	//150cycleで約10.5なのでおそらく11us待機すれば大丈夫だと思われる余裕をもって30us
	//0というより待機なしでも動くっぽい…
	const unsigned int setupCycle = 30;

	SetRegisterBit(gpio+GPIO_PUD, GPIO_PUD_REGISTER_PUD, GPIO_PUD_USE_BIT, upDown);


	DelayMicroSecond(setupCycle);

	if( pin <= 31 )
		//SetRegisterBit(gpio+GPIO_PUD_CLK_0, pin, 1, 1);
		*(gpio+GPIO_PUD_CLK_0) = 1<<pin;
	else
		//SetRegisterBit(gpio+GPIO_PUD_CLK_1, pin-32, 1, 1);
		*(gpio+GPIO_PUD_CLK_1) = 1<<(pin - 32);

	DelayMicroSecond(setupCycle);


	SetRegisterBit(gpio+GPIO_PUD, GPIO_PUD_REGISTER_PUD, GPIO_PUD_USE_BIT, PULL_NONE);
	if( pin <= 31 )
		//SetRegisterBit(gpio+GPIO_PUD_CLK_0, pin, 1, 0);
		*(gpio+GPIO_PUD_CLK_0) = 0;
	else
		//SetRegisterBit(gpio+GPIO_PUD_CLK_1, pin-32, 1, 0);
		*(gpio+GPIO_PUD_CLK_1) = 0;

}

//reg レジスタのアドレス, bit 何ビット目を変更するか, useBit 変更するビット数, value 値
unsigned int SetRegisterBit(volatile unsigned int *reg, unsigned int bit, unsigned int useBit, unsigned int value)
{
	unsigned int mask, tmp;

	if( useBit <= 0 )
		useBit = 1;
	//指定した数分の1bitを指定したbitまでシフト
	mask = ( (1<<useBit)-1 ) << bit;

	//初期化(否定をすることで指定した数分の1bit以外1に該当部は0)
	tmp = *reg & ~mask;

	//指定したbitまでシフトしてORで値を設定
	tmp |= value << bit;

	*reg = tmp;

	return tmp;
}
//debug用
unsigned int SetRegisterBitDebug(volatile unsigned int *reg, unsigned int bit, unsigned int useBit, unsigned int value)
{
	unsigned int mask, tmp;

	if( useBit <= 0 )
		useBit = 1;

	printf("value\n");
	PrintUintDelimiter(stdout, value, 4);
	printf("shift value\n");
	PrintUintDelimiter(stdout, value<<bit, 4);
	printf("reg\n");
	PrintUintDelimiter(stdout, *reg, 4);
	printf("\n");

	//指定した数分の1bitを指定したbitまでシフト
	mask = ( (1<<useBit)-1 ) << bit;
	PrintUintDelimiter(stdout, mask, 4);

	//初期化(否定をすることで指定した数分の1bit以外1に該当部は0)
	tmp = *reg & ~mask;
	PrintUintDelimiter(stdout, tmp, 4);

	//指定したbitまでシフトしてORで値を設定
	tmp |= value << bit;
	PrintUintDelimiter(stdout, tmp, 4);

	//データの更新
	*reg = tmp;
	PrintUintDelimiter(stdout, *reg, 4);

	printf("\n");
	return tmp;
}


//reg レジスタのアドレス, bit 何ビット目を変更するか, useBit 変更するビット数
unsigned int GetRegisterBit(volatile unsigned int *reg, unsigned int bit, unsigned int useBit)
{
	unsigned int mask, tmp;

	if( useBit <= 0 )
		useBit = 1;
	//指定した数分の1bitを指定したbitまでシフト
	mask = ( (1<<useBit)-1 ) << bit;

	//マスク部のみ取得して右シフトして桁を下げる
	tmp = (*reg & mask)>>bit;

	return tmp;
}
//debug用
unsigned int GetRegisterBitDebug(volatile unsigned int *reg, unsigned int bit, unsigned int useBit)
{
	unsigned int mask, tmp;

	if( useBit <= 0 )
		useBit = 1;

	printf("reg\n");
	PrintUintDelimiter(stdout, *reg, 4);
	printf("\n");

	//指定した数分の1bitを指定したbitまでシフト
	mask = ( (1<<useBit)-1 ) << bit;
	PrintUintDelimiter(stdout, mask, 4);

	//マスク部のみ取得
	tmp = *reg & mask;
	PrintUintDelimiter(stdout, tmp, 4);

	//データを右シフトして該当部の桁を下げる
	tmp = tmp>> bit;
	PrintUintDelimiter(stdout, tmp, 4);

	printf("\n");
	return tmp;
}