
#ifndef THREADS_H
#define THREADS_H


#define THREAD_MSG_EXIT			SIGRTMIN+1

void InitShiftRegister();
void UninitShiftRegister();
//16bit(8bitシフトレジスタを2つ分)使用して7segをコントロール
//下位8bitが数字, 上位8bitが桁(同じ数字なら複数桁可能
void SendShiftRegister(unsigned int dispNum);

//表示するスレッド
void* DispDataThread(void *param);

int SetSignalBlock(sigset_t *ss);
//センサからデータを取得
void* SensorDataThread(void *param);

void SensorLog();

////センサのデータをロギングする
//void* SensorLoggerThread(void *param);

#endif
