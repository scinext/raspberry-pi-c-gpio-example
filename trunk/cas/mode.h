﻿
#ifndef MODE_H
#define MODE_H

//終了文字分
#define SEG_COUNT  4

//配線上左から1,2,3,4にしちゃったのでプログラム上で変更
#define DIGIT_1    1<<11
#define DIGIT_2    1<<10
#define DIGIT_3    1<<9
#define DIGIT_4    1<<8

#define DIGIT_SWITCH    6000
//#define DIGIT_SWITCH    600000


//配線のピン番号 A 1    B 14    C 12    D 10    E 3    F 2    G 13    RDP 9
/*
 *         SEG_A
 *        --------
 *       |        |
 *  SEG_F|        |SEG_B
 *       | SEG_G  |
 *        --------
 *       |        |
 *  SEG_E|        |SEG_C  _
 *       | SEG_D  |      | | SEG_DP(RDP) LDPは配線していない
 *        --------        -
 *
 */
#define SEG_A  (1)
#define SEG_B  (1<<7)
#define SEG_C  (1<<5)
#define SEG_D  (1<<4)
#define SEG_E  (1<<2)
#define SEG_F  (1<<1)
#define SEG_G  (1<<6)
#define SEG_DP (1<<3)

// | SEG_
#define SEG_NUM_0		SEG_A | SEG_F | SEG_E | SEG_D | SEG_C | SEG_B
#define SEG_NUM_1		SEG_B | SEG_C
#define SEG_NUM_2		SEG_A | SEG_B | SEG_G | SEG_E | SEG_D
#define SEG_NUM_3		SEG_A | SEG_B | SEG_G | SEG_C | SEG_D
#define SEG_NUM_4		SEG_F | SEG_G | SEG_B | SEG_C
#define SEG_NUM_5		SEG_A | SEG_F | SEG_G | SEG_C | SEG_D
#define SEG_NUM_6		SEG_A | SEG_F | SEG_E | SEG_D | SEG_C | SEG_G
#define SEG_NUM_7		SEG_A | SEG_B | SEG_C
#define SEG_NUM_8		SEG_A | SEG_F | SEG_E | SEG_G | SEG_C | SEG_D | SEG_E | SEG_B
#define SEG_NUM_9		SEG_A | SEG_F | SEG_G | SEG_B | SEG_C

#define SEG_ALPHA_A		SEG_F | SEG_E | SEG_A | SEG_B | SEG_C | SEG_G
#define SEG_ALPHA_B		SEG_F | SEG_E | SEG_G | SEG_C | SEG_D
#define SEG_ALPHA_C		SEG_A | SEG_F | SEG_E | SEG_D
#define SEG_ALPHA_D		SEG_B | SEG_C | SEG_G | SEG_E | SEG_D
#define SEG_ALPHA_E		SEG_A | SEG_F | SEG_E | SEG_G | SEG_D
#define SEG_ALPHA_F		SEG_A | SEG_F | SEG_E | SEG_G
#define SEG_ALPHA_G		SEG_A | SEG_F | SEG_E | SEG_D | SEG_C | SEG_G
#define SEG_ALPHA_H		SEG_F | SEG_E | SEG_G | SEG_B | SEG_C
#define SEG_ALPHA_I		SEG_C
#define SEG_ALPHA_J		SEG_B | SEG_C | SEG_D | SEG_E
#define SEG_ALPHA_K		SEG_A | SEG_F | SEG_E | SEG_G | SEG_C
#define SEG_ALPHA_L		SEG_F | SEG_E | SEG_D | SEG_E
#define SEG_ALPHA_M		SEG_F | SEG_E | SEG_A | SEG_B | SEG_C
#define SEG_ALPHA_N		SEG_E | SEG_G | SEG_C
#define SEG_ALPHA_O		SEG_G | SEG_E | SEG_D | SEG_C
#define SEG_ALPHA_P		SEG_F | SEG_E | SEG_A | SEG_B | SEG_G
#define SEG_ALPHA_Q		SEG_G | SEG_B | SEG_A | SEG_F | SEG_E | SEG_D
#define SEG_ALPHA_R		SEG_E | SEG_G
#define SEG_ALPHA_S		SEG_F | SEG_G | SEG_C | SEG_D
#define SEG_ALPHA_T		SEG_F | SEG_G | SEG_E | SEG_D
#define SEG_ALPHA_U		SEG_E | SEG_D | SEG_C
#define SEG_ALPHA_V		SEG_F | SEG_E | SEG_D | SEG_C | SEG_B
#define SEG_ALPHA_W		SEG_F | SEG_E | SEG_D | SEG_C | SEG_B | SEG_G
#define SEG_ALPHA_X		SEG_F | SEG_E | SEG_G | SEG_B | SEG_C
#define SEG_ALPHA_Y		SEG_F | SEG_G | SEG_B | SEG_C | SEG_D
#define SEG_ALPHA_Z		SEG_A | SEG_B | SEG_E | SEG_D


int DispModeName(int mode);
void DispModeData(int mode);


void ReverseInsert(char *buf);
void BlinkDp(int digit, int blinkInterval);
void DispWait();

void PressMode(int init);
void TempMode(int init);
void LuxMode(int init);
void HumMode(int init);
void YearMode(int init);
void DateMode(int init);
void ClockMode(int init);

void AnimationMode(int mode);
void BoundSpeed(int min, int max, int inc);

#endif