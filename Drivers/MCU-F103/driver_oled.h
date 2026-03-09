#ifndef __OLED_H
#define __OLED_H
#include <stdint.h>
extern const uint8_t Yeqi[][16];
extern const uint8_t Yeqi2[][16];

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line,uint8_t Column,char *String);
void OLED_ShowNum(uint8_t Line,uint8_t Column,uint32_t Num,uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line,uint8_t Column,int32_t Num,uint8_t Length);
void OLED_ShowHexNum(uint8_t Line,uint8_t Column,uint32_t HexNum,uint8_t Length);
void OLED_ShowBinNum(uint8_t Line,uint8_t Column,uint32_t BinNum,uint8_t Length);
void OLED_ShowBMP(const uint8_t bmp[][16]);

#endif
