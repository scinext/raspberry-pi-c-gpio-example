
#ifndef MAIN_H
#define MAIN_H

#define DEBUG

#ifndef DEBUG
	#define Dprintf
#else
	#define Dprintf printf
#endif

void I2c(unsigned int slaveAddress, unsigned int addr, unsigned int data, unsigned int rw);
void GpioTest();

//interrupt
	int GpioInterruptCallbackFunc(int pin, int value);
	void InterruptTest();

//spi
	#define START_BIT		1<<2	//S
	#define AD_SINGLE		1<<1	//シングルエンドモードでAD変換
	#define AD_DIFF			0<<1	//差動モードでAD変換
	#define SEND_CH_SHIFT	6		//チャンネルを設定するときに上位へシフトする数
	void SpiTest();

//system timer
	void SysTimerTest();

//i2c
	#define I2C_ADDR_LPS331 			0x5d
	
	#define LPS331_MULTI_DATA(addr)		(addr | 0x80)
	#define LPS331_SINGLE_DATA(addr)	(addr)
	
	#define LPS331_WHO_AM_I				0x0F
	#define LPS331_PRESS				0x28
	#define LPS331_TEMP					0x2B
	#define LPS331_ACTIVE				0x20
		#define LPS331_ACTIVE_ON			0x90
		#define LPS331_ACTIVE_OFF			0x00
	void I2cTest();

//arm timer
	void ArmTimerTest();


void MeasureSleepTime(int pin, long sleepTime, int loop);

#endif
