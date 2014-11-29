
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
#include <string.h>

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

//inturruptを使いたかったがカーネルモジュールじゃないとできなく
//raspberryはlinux-headersの取得が面倒なのでなし、linuxのデバイスドライバを使うように
//#include <linux/interrupt.h>
//#include <poll.h>
#include <sys/epoll.h> 

#include "gpio-timer.h"

void InitGpio()
{
	int mem_fd;
	//現在のメモリのファイルポインタの取得
	if( (mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0 )
	{
		Dgprintf("can't open /dev/mem: %s\n", strerror(errno));
		exit(-1);
	}

	//現在のメモリからGPIOの箇所の仮想アドレスを再取得
	//解放するときはmunmap()を使うプロセスが終了すれば自動で開放される
	gpio_map = (char *)mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);

	//mmapにてaddrを指定すると<0判定でもいいが
	//unsignedのため<0判定に引っかかるのでMAP_FAILEDにする if( (long)gpio_map < 0)
	if( gpio_map == MAP_FAILED )
	{
		Dgprintf("mmap error %d\n", (int)gpio_map);
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
	//Dgprintf("init %2d pin-> %d\n", pin, ioFlag);
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

//Interrupt
struct GpioInterrupt	gpioInterrupt;
int 					epollFd;
pthread_t				interruptThreadId;
int GpioInterruptStart()
{
	//コールバックがない場合はファイルの読込だけして次の書き込みまで再度待ち状態にする
	if( gpioInterrupt.callback == NULL )
	{
		//Dgprintf("no register callback\n");
		return INTERRUPT_NO_CALLBACK;
	}
	pthread_create(&interruptThreadId, NULL, InterruptThread, (void *)NULL);
	
	return INTERRUPT_START;
}
void GpioInterruptEnd()
{
	pthread_cancel(interruptThreadId);
	
	GpioInerruptUnInit();
}
void GpioInerruptUnInit()
{
	int  i, fd;
	char pinStr[10];
	
	//開いているファイルディスクリプタをすべて閉じてからGPIOをunexportする
	for(i=0; i<GPIO_PIN_COUNT; i++)
	{
		if( gpioInterrupt.fd[i] != 0 )
		{
			//Dgprintf("close file discript\n");
			
			//ファイルディスクリプタを閉じる
			close( gpioInterrupt.fd[i] );
			
			//gpioをunexport
			sprintf(pinStr, "%d", i);
			if( WriteSysGpio(SYS_GPIO "unexport", pinStr) == -1 )
				perror("open unexport");
				
			//全てのpinをINでpull_noneにする
			InitPin(i, PIN_IN);
			PullUpDown(i, PULL_NONE);
		}
	}
	
	//epollファイルディスクリプタを閉じる
	if( epollFd != -1 )
		close(epollFd);
}

void RegisterInterruptCallback(GpioInterruptCallback callback)
{
	gpioInterrupt.callback = callback;
}

int WriteSysGpio(char *path, char *writeData)
{
	int fd;
	//使用するgpioのpinの作成 echo Pin > /sys/class/gpio/export
	fd = open(path, O_WRONLY);
	if( fd == -1 )
	{
		perror(path);
		return -1;
	}
	write(fd, writeData, strlen(writeData));
	close(fd);
	
	return 1;
}
int RegisterInterruptPin(int pin, int edgeType)
{
	int  fd;
	char value;
	char sysfsPath[100];
	char buf[20];
	struct epoll_event ev;
	
	//linuxのデバイスドライバを使う レジスタを使うと割り込みが取得できない
	//http://vintagechips.wordpress.com/2013/09/09/linuxの流儀でgpioを監視する/
	
	//SetRegisterBitDebug(gpio+GPIO_REN_0, pin, 1, 1);
	//SetRegisterBitDebug(gpio+GPIO_FEN_0, pin, 1, 1);
	//SetRegisterBitDebug(gpio+GPIO_REN_0, pin, 1, 0);
	//SetRegisterBitDebug(gpio+GPIO_FEN_0, pin, 1, 0);
	
	//epollファイルディスクリプタの作成
	//epoll_create(int size) Linux 2.6.8でsizeは無視されるが0以上でないとエラー
	epollFd = epoll_create(1);
	if( epollFd == -1 )
	{
		perror("epoll_create");
		return REGISTER_INTERRUPT_EPOLL_CREATE_FAIL;
	}
	
	
	//使用するgpioのpinの作成 echo Pin > /sys/class/gpio/export
	sprintf(buf, "%d", pin);
	if( WriteSysGpio(SYS_GPIO "export", buf) == -1 )
	{
		perror("open export");
		return REGISTER_INTERRUPT_EXPORT_FAIL;
	}
	
	
	//pinのモード echo 'in' > /sys/class/gpio/gpio[Pin]/direction
	sprintf(sysfsPath, SYS_GPIO "gpio%d/direction", pin);
	if( WriteSysGpio(sysfsPath, "in") == -1 )
	{
		perror("open direction");
		return REGISTER_INTERRUPT_DIRECT_FAIL;
	}
	
	
	//検出モード echo edgeType > /sys/class/gpio/gpio[Pin]/edge
	switch(edgeType)
	{
		case EDGE_TYPE_RISE:
			strcpy(buf, "rising");
			break;
		case EDGE_TYPE_FALL:
			strcpy(buf, "falling");
			break;
		case EDGE_TYPE_BOTH:
		default:
			strcpy(buf, "both");
			break;
	}
	sprintf(sysfsPath, SYS_GPIO "gpio%d/edge", pin);
	if( WriteSysGpio(sysfsPath, buf) == -1 )
	{
		perror("open edge");
		return REGISTER_INTERRUPT_EDGE_FAIL;
	}
	
	
	//値 echo /sys/class/gpio/gpio[Pin]/value
	sprintf(sysfsPath, SYS_GPIO "gpio%d/value", pin);
	gpioInterrupt.fd[pin] = open(sysfsPath, O_RDONLY);
	if(gpioInterrupt.fd[pin] == -1 )
	{
		perror("open value");
		return REGISTER_INTERRUPT_VALUE_FAIL;
	}
	//読み込みが必要なイベントの発行をさせるために一度読み込む
	read(gpioInterrupt.fd[pin], &value, sizeof(char));
	//Dgprintf("pin:%d  fd:%d\n", pin, gpioInterrupt.fd[pin]);
	
	
	//event設定
	ev.events = EPOLLPRI; //操作が可能な緊急 (urgent) データがある。
	ev.data.fd = gpioInterrupt.fd[pin];
	//epollFdをコントロールへ登録
	if( epoll_ctl(epollFd, EPOLL_CTL_ADD, gpioInterrupt.fd[pin], &ev) == -1 )
	{
		perror("epoll_ctl");
		close(gpioInterrupt.fd[pin]);
		return REGISTER_INTERRUPT_EPOLL_CTL_FAIL;
	}
	return REGISTER_INTERRUPT_SUCCESS;
}
void* InterruptThread(void *param)
{
	int i, nfds, n, value, flag;
	char c;
	struct epoll_event events[GPIO_PIN_COUNT];
	
	flag = 1;
	while(flag)
	{
		//epollはpthread_cancelでキャンセルできる
		
		//epoll_waite(ファイルディスクリプタ, 実行されたeventが入る配列, event変数の数, タイムアウト[ms])
		nfds = epoll_wait(epollFd, events, GPIO_PIN_COUNT, -1);
		if( nfds == -1 )
		{
			perror("epoll_wait");
			break;
		}
		//変更があったファイルディスクリプタの一覧から登録している関数を呼び出す
		//Dgprintf("nfds = %d\n", nfds);
		for(n=0; n<nfds; n++)
		{
			//ファイルシークを先頭にして値を読み込む
			lseek(events[n].data.fd, 0, SEEK_SET);
			read(events[n].data.fd, &c, sizeof(char));
			//ファイルシークを先頭にして値を読み込む
			//lseek(gpioInterrupt.fd[i], 0, SEEK_SET);
			//read(gpioInterrupt.fd[i], &c, sizeof(char));
			
			//Dgprintf("n=%d  fd=%d \n", n, events[n].data.fd);
			
			//epollのイベントが来たファイルディスクリプタのpinを検索、コールバック実行
			for(i=0; i<GPIO_PIN_COUNT; i++)
			{
				if( gpioInterrupt.fd[i] != events[n].data.fd)
					continue;
					
				//読み込み値をintへ
				value = c-'0';
				//Dgprintf("call pin:%d callback value %d\n", i, value);
				
				//コールバック中1つでも戻り値がINTERRUPT_ENDだったらInterrupt終了
				if( gpioInterrupt.callback(i, value) == INTERRUPT_END )
					flag = 0;
				break;
			}
		}
	}
	GpioInerruptUnInit();
	pthread_exit(NULL);
}

char *pads_map = NULL;
volatile unsigned int *pads = NULL;
#define PADS_BASE	(BCM2708_PERI_BASE+0x00100000)	//0x 7e10 0000
#define PADS_1		0x002C/sizeof(uint32_t)
#define PADS_2		0x0030/sizeof(uint32_t)
#define PADS_3		0x0034/sizeof(uint32_t)
int InitPads()
{
	int mem_fd;

	if( pads != NULL )
		return 1;

	if( gpio == NULL )
	{
		perror("gpio NULL");
		return -1;
	}

	if( (mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0 )
	{
		Dgprintf("can't open /dev/mem: %s\n", strerror(errno));
		return -1;
	}

	pads_map = (char *)mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, PADS_BASE);
	if( pads_map == MAP_FAILED )
	{
		Dgprintf("mmap error %s %d\n", strerror(errno), (int)pads_map);
		return -1;
	}

	close(mem_fd);

	pads = (volatile unsigned int *)pads_map;

	
	Dgprintf("gpio base 0x%X\n", (int32_t)gpio);
	Dgprintf("mmap PADS_BASE %s 0x%X\n", strerror(errno), (int32_t)pads);

	PrintRegStatus(stdout, pads, PADS_1,  "0x7e10002c    ",	1);
	PrintRegStatus(stdout, pads, PADS_2,  "0x7e10002c    ",	0);
	PrintRegStatus(stdout, pads, PADS_3,  "0x7e10002c    ",	0);

	return 1;

	
}