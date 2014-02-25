
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
#include "gpio-i2c.h"

char *i2c_map;
volatile unsigned int *i2c;
int rpiRev;
int slaveAddr;

void PrintI2cRegister()
{
	PrintRegStatus(stdout, i2c, I2C_C,		"I2C_C           ",	1);
	PrintRegStatus(stdout, i2c, I2C_S,		"I2C_S           ",	0);
	PrintRegStatus(stdout, i2c, I2C_DLEN,	"I2C_DLEN        ",	0);
	PrintRegStatus(stdout, i2c, I2C_A,		"I2C_A           ",	0);
	PrintRegStatus(stdout, i2c, I2C_FIFO,	"I2C_FIFO        ",	0);
	PrintRegStatus(stdout, i2c, I2C_DIV,	"I2C_DIV         ",	0);
	PrintRegStatus(stdout, i2c, I2C_DEL,	"I2C_DEL         ",	0);
	PrintRegStatus(stdout, i2c, I2C_CLKT,	"I2C_CLKT        ",	0);
	return;
}
void PrintI2cRegister2()
{
	PrintRegStatus(stdout, i2c, I2C_C,		"   I2C_C        ",	1);
	PrintRegStatus(stdout, i2c, I2C_S,		"   I2C_S        ",	0);
	PrintRegStatus(stdout, i2c, I2C_DLEN,	"   I2C_DLEN     ",	0);
	PrintRegStatus(stdout, i2c, I2C_A,		"   I2C_A        ",	0);
	//PrintRegStatus(stdout, i2c, I2C_FIFO,	"   I2C_FIFO     ",	0);
	return;
}
int InitI2c(RpiRevision rev)
{
	int mem_fd, addr, sda, scl;

	if( gpio == NULL )
	{
		perror("gpio NULL");
		return -1;
	}

	if( (mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0 )
	{
		fprintf(stderr, "can't open /dev/mem: %s\n", strerror(errno));
		return -1;
	}

	rpiRev = rev;

	switch( rpiRev )
	{
		case REV_1:
			addr = I2C_0_BASE;
			break;
		case REV_2:
			addr = I2C_1_BASE;
			break;
	}
	i2c_map = (char *)mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr);
	if( i2c_map == MAP_FAILED )
	{
		fprintf(stderr, "mmap error %d\n", (int)i2c_map);
		return -1;
	}
	#ifdef I2C_DEBUG
	switch( rpiRev )
	{
		case REV_1:
			I2cDprintf("mmap I2C_0_BASE %s %X\n", strerror(errno), (int32_t)i2c_map);
			break;
		case REV_2:
			I2cDprintf("mmap I2C_1_BASE %s %X\n", strerror(errno), (int32_t)i2c_map);
			break;
	}
	#endif

	close(mem_fd);

	i2c = (volatile unsigned int *)i2c_map;

	/*
	 *  Rev1 ALT0                            | Rev2 ALT0
	 *       I2C0_SDA | GPIO_0  |3    4| 5v  |      I2C1_SDA | GPIO_2  |3    4| 5v
	 *       I2C0_SCL | GPIO_1  |5    6| GND |      I2C1_SCL | GPIO_3  |5    6| GND
	 */
	switch( rpiRev )
	{
		case REV_1:
			sda = GPIO_0;
			scl = GPIO_1;
			break;
		case REV_2:
			sda = GPIO_2;
			scl = GPIO_3;
			break;
	}
	InitPin(sda,	PIN_ALT0);
	InitPin(scl,	PIN_ALT0);

	//I2Cを有効にする
	SetRegisterBit(i2c+I2C_C, I2C_C_REGISTER_I2CEN, 1, 1);
	return 1;
}
int UnInitI2c()
{
	/*
	この辺実行するとi2cdetectとかが使えなくなるので中止
	int sda, scl;

	//I2Cを無効にする
	SetRegisterBit(i2c+I2C_C, I2C_C_REGISTER_I2CEN, 1, 0);

	switch( rpiRev )
	{
		case REV_1:
			sda = GPIO_0;
			scl = GPIO_1;
			break;
		case REV_2:
			sda = GPIO_2;
			scl = GPIO_3;
			break;
	}
	InitPin(sda,	PIN_INIT);
	InitPin(scl,	PIN_INIT);
	*/

	//取得したSPIの箇所の仮想アドレスの解放
	munmap(i2c_map, BLOCK_SIZE);

	return 1;
}

unsigned int I2cSetClock(unsigned int speed)
{
 	//core clock 150MHz システムクロック(250MHz)とは別?
	const unsigned int coreClock = 150000000;

	//単純に割るだけでいい SCL=core clock(150MHz)/CDIV
	unsigned int clockDivider = coreClock / speed;

	//0の場合は32768になるらしい
	//設定しない場合は100KHz=1500

	//多いときは-1を返す
	if( clockDivider < 1<<I2C_DIV_CDIV_USE_BIT )
		*(i2c+I2C_DIV) = clockDivider;
	else
		return -1;

	return 1;
}

int I2cSetDlen(uint16_t byteCount)
{
	//多いときは-1を返す
	if( byteCount < 1<<I2C_DLEN_DLEN_USE_BIT )
		*(i2c+I2C_DLEN) = byteCount;
	else
		return -1;

	return 1;
}
int I2cSetSlaveAddr(uint8_t addr)
{
	//アドレスが8bit以上は-1
	if( addr < 1<<I2C_A_ADDR_USE_BIT )
		*(i2c+I2C_A) = addr;
	else
		return -1;

	return 1;
}
void I2cClear()
{
	SetRegisterBit(i2c+I2C_C, I2C_C_REGISTER_CLEAR, I2C_C_CLEAR_USE_BIT, I2C_C_CLEAR_1);
	return;
}

I2cError I2cErrorCheck()
{
	if( GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_CLKT, 1) != 0 )
		return TIME_OUT;
	if( GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_ERR,  1) != 0 )
		return NO_ADDR;
		
	//fprintf(stderr, "i2c errors\n");
	return NO_ERROR;
}
int I2cTransfer(uint8_t *tbuf, unsigned int tlen, uint8_t *rbuf, unsigned int rlen)
{
	int result;
	
	if( tbuf != NULL )
	{
		result = I2cWrite(tbuf, tlen);
		if( result != 0 )
			return result;
	}
	
	if( rbuf != NULL )
	{
		result = I2cRead(rbuf, rlen);
		if( result != 0 )
			return result;
	}
	return 0;
}

int I2cWrite(uint8_t *tbuf, unsigned int len)
{
	int i, result;
	//I2cDprintf("send init\n");
	
	result = 0;
	//データのクリア
	I2cClear();
	SetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_CLKT, 1, 0);
	SetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_ERR, 1, 0);
	//I2cDprintf("%d\n", __LINE__); PrintI2cRegister2();

	//転送サイズの挿入
	I2cSetDlen(len);
	//I2cDprintf("%d\n", __LINE__); PrintI2cRegister2();

	//転送開始レジスタの設定(READとST)
	SetRegisterBit(i2c+I2C_C, I2C_C_REGISTER_READ, 1, I2C_WRITE);
	SetRegisterBit(i2c+I2C_C, I2C_C_REGISTER_ST, 1, 1);
	//S_TAが1なら送信中なので0になるまで待機(現状何か送信中でなければ0のはず)
	while( GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_TA, 1) == 0 )
	{
		result = I2cErrorCheck();
		if( result != 0 )
			return result;
	}

	i=0;
	//S_TAが1の場合はまだデータの転送が必要なので転送
	while( i<len && (GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_TA, 1)==1) )
	{
		//転送用のレジスタにデータの挿入 TXE(FIFOが空なら1)
		while( (GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_TXE, 1)==0) )
		{
			result = I2cErrorCheck();
			if( result != 0 )
				return result;
			//I2cDprintf("%d\n", __LINE__); PrintI2cRegister2();
		}
			
		*(i2c+I2C_FIFO) = tbuf[i];
		//I2cDprintf("send data 0x%x \n", tbuf[i]); PrintI2cRegister2();

		++i;
	}
	//相手先の応答をちょっと待つ(1ms)
	DelayMicroSecond(1000);
	
	//I2cDprintf("%d\n", __LINE__); PrintI2cRegister2();
	//S_TAがまだ1でここに来る場合は送信データが足りない
	if( GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_TA, 1)==1 )
		fprintf(stderr, "i2c send data short\n");

	return 0;
}
int I2cRead(uint8_t *rbuf, unsigned int len)
{
	int i, result;
	
	result = 0;
	
	I2cClear();
	SetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_CLKT, 1, 0);
	SetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_ERR, 1, 0);
	//I2cDprintf("%d\n", __LINE__); PrintI2cRegister2();

	//転送サイズの挿入
	I2cSetDlen(len);
	//I2cDprintf("%d\n", __LINE__); PrintI2cRegister2();

	//転送開始レジスタの設定
	SetRegisterBit(i2c+I2C_C, I2C_C_REGISTER_READ, 1, I2C_READ);
	SetRegisterBit(i2c+I2C_C, I2C_C_REGISTER_ST, 1, 1);
	//I2cDprintf("%d\n", __LINE__); PrintI2cRegister2();
	
	//S_TAが1なら送信中なので0になるまで待機(現状何か送信中でなければ0のはず)
	while( GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_TA, 1)==0 )
	{
		result = I2cErrorCheck();
		if( result != 0 )
			return result;
	}
	
	i=0;
	//S_TAが1の場合はまだデータの転送が必要なので転送
	while( i<len && (GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_TA, 1)==1) )
	{
		//転送用のレジスタにデータの挿入 RXD(FIFOが空なら0)
		while( GetRegisterBit(i2c+I2C_S, I2C_S_REGISTER_RXD, 1) ==0 )
		{
			result = I2cErrorCheck();
			if( result != 0 )
				return result;
			//I2cDprintf("%d\n", __LINE__); PrintI2cRegister2();
		}
		rbuf[i] = *(i2c+I2C_FIFO);
		//I2cDprintf("read 1byte 0x%x\n", rbuf[i] );
		
		++i;
	}
		
	return result;
}

int I2cSearch()
{
	int i, ret;
	uint8_t send, recive;
	
	send = 0;
	//7bitアドレス
	printf("    ");
	for(i=0; i<0x10; i++)
		printf("%2X ", i);
	for(i=0; i<1<<8; i++)
	{
		if( i%0x10 == 0 )
			printf("\n %2X ", i);
			
		I2cSetSlaveAddr(i);
		
		I2cWrite(&send, 1);
		ret = I2cRead(&recive, 1);
		if( ret != 0 )
		{
			printf("-- ");
			continue;
		}
		printf("%2x ", i);
		
		//for(i=0; i<1; i++)
		//	I2cDprintf("recive[%d]  %d\n", i, recive[i]);
		DelayMicroSecond(100);
	}
	printf("\n");
}