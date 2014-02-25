
#include <stdio.h>
//getopt
#include <unistd.h>

#include "gpio.h"
#include "gpio-util.h"
#include "gpio-spi.h"

#define SYS_TIMER_DEBUG
#include "gpio-timer.h"
#include "gpio-i2c.h"
#include "gpio-arm-timer.h"

#include "main.h"

struct timespec startTs, endTs;

#define SLEEP_TEST	1
#define SLEEP_LOOP	10
#define SLEEP_LIMIT	50000

#define M_TEST	0
#define M_I2C	1

#define M_WRITE 0
#define M_READ	1
int main(int argc, char** argv)
{	
	int mode = M_TEST;
	unsigned int data	= 0;
	unsigned int addr	= 0;
	unsigned int rw		= 0;
	unsigned int slaveAddress = 0;
	//コマンドラインを解析
	int opt;
	extern char *optarg;	
	/*
	 *  -i i2c -s SlaveAddres -a address -r read -w write
	 *  -t other test
	 */
	while( (opt = getopt(argc, argv, "iv:s:a:rw:t")) != -1 )
	{
		switch( opt )
		{
			case 'i':
				mode = M_I2C;
				break;
			case 's':
				slaveAddress = strtol(optarg, NULL, 16);
				break;
			case 'a':
				addr = strtol(optarg, NULL, 16);
				break;
			case 'r':
				data	= 0;
				rw		= M_READ;
				break;
			case 'w':
				data	= strtol(optarg, NULL, 16);
				rw		= M_WRITE;
				break;
			case 't':
				mode = M_TEST;
				break;
		}
	}
	
	InitGpio();	
	SetPriority(HIGH_PRIO);
	
	switch(mode)
	{
		case M_I2C:		I2c(slaveAddress, addr, data, rw);	break;
		case M_TEST:	GpioTest();							break;
	}
	return 0;
}
void I2c(unsigned int slaveAddress, unsigned int addr, unsigned int data, unsigned int rw)
{
	uint8_t send[2];
	
	InitI2c(REV_2);
	//slaveaddresが0の時はアドレスを検索する
	if( slaveAddress == 0 )
	{
		I2cSearch();
		return;
	}
	printf("slave address 0x%X, addr 0x%X, data 0x%X, read/write %s\n",
		slaveAddress, addr, data, rw==M_WRITE?"write":"read" );
	
	I2cSetSlaveAddr(slaveAddress);
	
	send[0] = (uint8_t)addr;
	if( rw == M_WRITE )
	{
		send[1] = (uint8_t)data;
		I2cWrite(send, 2);
	}
	else
	{
		int i;
		uint8_t recive;
		I2cWrite(send, 1);
		I2cRead(&recive, 1);
		printf("0x%X\n", recive);
	}
}
void GpioTest()
{
			
	//SpiTest();
	//I2cTest();
	
	//int i;
	//printf("arm\n");
	////for(i=0; i<20; i++)
	//{
	//ArmTimerTest();
	//}
	//
	//printf("sys\n");
	////for(i=0; i<20; i++)
	//{
	//SysTimerTest();
	//}
	
	//InitSysTimer();
	//SysTimerPrecisionTest();
	//UnInitSysTimer();
	//
	//InitArmTimer(0);
	//ArmTimerPrecisionTest();
	//UnInitArmTimer();
	
	//InitPads();
	
	//PrintGpioStatus(gpio);
	int drainPin = 23;
	InitPin(drainPin, PIN_OUT);
	GPIO_CLR(drainPin);
	//PullUpDown(drainPin, PULL_NONE);
	
	PrintGpioPinMode(gpio);
	PrintGpioLevStatus(gpio);
	return;
}
void ArmTimerTest()
{
	struct timespec startTs, endTs;
	int i, sleep;
	long sum;
	    
	InitArmTimer(0);
	
	sum = 0;
	//sleep = SLEEP_TEST*10-2;
	sleep = SLEEP_TEST;
	//Dprintf("Arm Timer Test\n");
	for(i=0; i<SLEEP_LOOP; i++){
		clock_gettime(CLOCK_MONOTONIC, &startTs);
		
		DelayArmTimerCounter(sleep);
		
		clock_gettime(CLOCK_MONOTONIC, &endTs);
		//TimeDiff(&startTs, &endTs, 0);
		if( endTs.tv_sec == startTs.tv_sec)
			sum += endTs.tv_nsec - startTs.tv_nsec;
		else
			sum += (unsigned long)1e+9 - startTs.tv_nsec + endTs.tv_nsec;
		printf("\t%lu\n", sum);
		sum = 0;
		//printf("\t%ld\n", endTs.tv_nsec - startTs.tv_nsec);
		//PrintArmTimerRegister();
	}
	//if( sum > SLEEP_TEST*SLEEP_LOOP*1000+SLEEP_LIMIT )
	//	printf("Arm Timer Test\t\tsum %ld\n", sum);
	
	//PrintArmTimerRegister();
	
	//ArmTimerPrecisionTest();
	UnInitArmTimer();
	
}
void I2cTest()
{
	uint8_t send[3]   = {0,};
	uint8_t recive[3] = {0,};
	int i, ret;
	
	//PrintGpioPinMode(gpio);
	InitI2c(REV_2);
	//PrintGpioPinMode(gpio);
	
	//PrintI2cRegister();
	I2cSearch();
	
	I2cSetSlaveAddr(I2C_ADDR_LPS331);
	
	send[0] = LPS331_MULTI_DATA( LPS331_ACTIVE );
	I2cTransfer(send, 1, recive, 3);
	for(i=0; i<3; i++)
		I2cDprintf("recive[%d]  %d\n", i, recive[i]);
	DelayMicroSecond(1000);
	/*
	I2cSetSlaveAddr(I2C_ADDR_LPS331);
	I2cDprintf("%d\n", __LINE__);
	send[0] = LPS331_SINGLE_DATA( LPS331_ACTIVE );
	send[1] = LPS331_ACTIVE_OFF;
	I2cWrite(send, 2);
	DelayMicroSecond(1000);
	
	I2cDprintf("%d\n", __LINE__);
	send[0] = LPS331_MULTI_DATA( LPS331_ACTIVE );
	I2cWrite(send, 1);
	I2cRead(recive, 3);
	for(i=0; i<3; i++)
		I2cDprintf("recive[%d]  %d\n", i, recive[i]);
	DelayMicroSecond(1000);
	
	I2cDprintf("%d\n", __LINE__);
	send[0] = LPS331_SINGLE_DATA( LPS331_ACTIVE );
	send[1] = LPS331_ACTIVE_ON;
	I2cWrite(send, 2);
	DelayMicroSecond(1000);
	
	I2cDprintf("%d\n", __LINE__);
	send[0] = LPS331_MULTI_DATA( LPS331_ACTIVE );
	I2cWrite(send, 1);
	I2cRead(recive, 3);
	for(i=0; i<3; i++)
		I2cDprintf("recive[%d]  %d\n", i, recive[i]);
	DelayMicroSecond(1000);
	
	I2cDprintf("%d\n", __LINE__);
	send[0] = LPS331_MULTI_DATA( LPS331_TEMP );
	I2cWrite(send, 1);
	I2cRead(recive, 2);
	for(i=0; i<2; i++)
		I2cDprintf("recive[%d]  %d\n", i, recive[i]);
	DelayMicroSecond(1000);
	
	send[0] = LPS331_MULTI_DATA( LPS331_PRESS );
	I2cWrite(send, 1);
	I2cRead(recive, 3);
	for(i=0; i<3; i++)
		I2cDprintf("recive[%d]  %d\n", i, recive[i]);
	DelayMicroSecond(1000);
	*/
	//UnInitI2c();
	//PrintGpioPinMode(gpio);
}

void SysTimerTest()
{	
	struct timespec startTs, endTs;
	int i, sleep;
	long sum;
	
	InitSysTimer();
	
	sum = 0;
	//PrintSysTimerRegister();
	sleep = SLEEP_TEST;
	//Dprintf("System Timer Test\n");
	for(i=0; i<SLEEP_LOOP; i++){
		clock_gettime(CLOCK_MONOTONIC, &startTs);
		
		DelayMicroSecond(sleep);
		
		clock_gettime(CLOCK_MONOTONIC, &endTs);
		//TimeDiff(&startTs, &endTs, 0);
		if( endTs.tv_sec == startTs.tv_sec)
			sum += endTs.tv_nsec - startTs.tv_nsec;
		else
			sum += (unsigned long)1e+9 - startTs.tv_nsec + endTs.tv_nsec;
		printf("\t%lu\n", sum);
		sum = 0;
		//printf("\t%ld\n", endTs.tv_nsec - startTs.tv_nsec);
	}
	//if( sum > SLEEP_TEST*SLEEP_LOOP*1000+SLEEP_LIMIT )
	//	printf("System Timer Test\tsum %ld\n", sum);
	
	UnInitSysTimer();
}

void SpiTest()
{
	long sleepTime;
	const unsigned int pin = 24;
	
	int ch, i, j;
	uint8_t tbuf[3];
	uint8_t rbuf[3];
	
	InitPin(pin, PIN_OUT);
	
	InitSpi();
	//PrintGpioPinMode(gpio);
	
	//PrintSpiRegister();
	
	SpiSetCS(SPI_CS_0);
	SpiSetMode(SPI_MODE_0);
	SpiClear(SPI_CLR_ALL);
	SpiSetCSPolarity(SPI_CS_0, LOW);
	SpiSetClock(1200000);
	SpiSetWriteMode();
	
	PrintSpiRegister();
	
	
	ch = 0;
	printf("ch %d\n", ch);
	tbuf[0] = START_BIT | AD_SINGLE;
	//4ch以上の場合、1つ前の送信バイトに1を入れる
	tbuf[0] |= ch>=4 ? 1 : 0;
	ch = ch << SEND_CH_SHIFT;
	tbuf[1] = ch;
	tbuf[2] = 0;
	
	for(j=0; j<1000; j++)
	{
		//printf("tbuf data\t: ");
		//for(i=0; i<3; i++)
		//	printf(" %02X", tbuf[i]);
		//printf("\n");
		
		//clock_gettime(CLOCK_MONOTONIC, &startTs);
		
		SpiTransferMulitple(tbuf, rbuf, 3);
		
		//clock_gettime(CLOCK_MONOTONIC, &endTs);
		//TimeDiff(&startTs, &endTs, 0);
		
		//printf("rbuf data\t: ");
		//for(i=0; i<3; i++)
		//	printf(" %02X", rbuf[i]);
		//printf("\n\n");
	}
	
	//rbuf[0] = 0;
	//rbuf[0] = SpiTransfer(tbuf[0]);
	//printf("rbuf[0] %d\n", rbuf[0] );
	//rbuf[1] = 0;
	//rbuf[1] = SpiTransfer(tbuf[1]);
	//printf("rbuf[0] %d\n", rbuf[1] );
	
	
	//UnInitSpi();
	//PrintGpioPinMode(gpio);
	//
	//InitPin(pin, PIN_IN);
	//PrintGpioPinMode(gpio);
	

	//SetPriority(HIGH_PRIO);

	//int loop;
	//loop		= 20;
	//sleepTime	= 10000; //1000ns -> 1us
	//MeasureSleepTime(pin, sleepTime, loop);
	//sleepTime	*= 10;
	//MeasureSleepTime(pin, sleepTime, loop);
	//sleepTime	*= 10;
	//MeasureSleepTime(pin, sleepTime, loop);
	//
	//SetPriority(NOMAL_PRIO);
	//sleepTime	= 10000; //1000ns -> 1us
	//MeasureSleepTime(pin, sleepTime, loop);
	//sleepTime	*= 10;
	//MeasureSleepTime(pin, sleepTime, loop);
	//sleepTime	*= 10;
	//MeasureSleepTime(pin, sleepTime, loop);
	
	/*
	sleepTime = 10000;
	InitPin(pin, PIN_OUT);
	PrintGpioLevStatus(gpio);
	clock_gettime(CLOCK_MONOTONIC, &startTs);
	
	GPIO_SET(pin);
	PrintGpioLevStatus(gpio);
	
	DelayNanoSecond(sleepTime);
	GPIO_CLR(pin);
	PrintGpioLevStatus(gpio);
	
	clock_gettime(CLOCK_MONOTONIC, &endTs);
	
	TimeDiff(&startTs, &endTs, sleepTime);
	*/
}