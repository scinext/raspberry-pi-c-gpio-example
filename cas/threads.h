
#ifndef THREADS_H
#define THREADS_H


#define THREAD_MSG_EXIT		SIGRTMIN+1

//表示するスレッド
void* DispDataThread(void *param);

int SetSignalBlock(sigset_t *ss);
//センサからデータを取得
void* SensorDataThread(void *param);
void SensorLog();


//logのバックアップ
void LogBukup();
//logの復元
void LogOpen();



#endif
