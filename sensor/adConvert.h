
#ifndef ADCONVERT_H
#define ADCONVERT_H

int InitAD();
int UnInitAD();

int PinGetADCH(int pin, int ch, float *outADVolt, unsigned long sleepTime);

int GetADCH(int CH, float *outADVolt);

#endif