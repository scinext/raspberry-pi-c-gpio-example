
#ifndef MODE_H
#define MODE_H

int DispModeName(int mode);
void DispModeData(int mode);


void BlinkDp(int digit, int blinkInterval);
void DispWait();

void PressMode(int init);
void TempMode(int init);
void LuxMode(int init);
void HumMode(int init);
void CoreTemp(int init);

void ScrollOutputInit(char *buf);
void ScrollOutput(int init);

void YearMode(int init);
void DateMode(int init);
void ClockMode(int init);
void WaitLogTime(int init);

void AnimationMode(int mode);
void BoundSpeed(int min, int max, int inc);

#endif