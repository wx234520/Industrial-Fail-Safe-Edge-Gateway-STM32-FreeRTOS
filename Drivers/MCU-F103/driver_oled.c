#include "stm32f1xx_hal.h"
#include <string.h>
#include "driver_oled_font.h"

extern I2C_HandleTypeDef hi2c1;

void OLED_WriteCommand(uint8_t Command)
{
    uint8_t Trans_Array[] = {0x00,Command};
    HAL_I2C_Master_Transmit(&hi2c1,0x78,Trans_Array,2,HAL_MAX_DELAY);
}

void OLED_WriteData(uint8_t Data)
{
    uint8_t Trans_Array[] = {0x40,Data};
    HAL_I2C_Master_Transmit(&hi2c1,0x78,Trans_Array,2,HAL_MAX_DELAY);
}

void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

void OLED_Clear(void)
{
    uint8_t i,j;
    for(j = 0;j < 8;j ++)
    {
        OLED_SetCursor(j,0);
        for(i = 0;i < 128;i ++)
        {
            OLED_WriteData(0x00);
        }
    }
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

void OLED_ShowString(uint8_t Line,uint8_t Column,char *String)
{
	uint8_t i;
	for(i = 0;String[i] != '\0';i++)
	{
		OLED_ShowChar(Line,Column + i,String[i]);
	}
}

uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

void OLED_ShowNum(uint8_t Line,uint8_t Column,uint32_t Num,uint8_t Length)
{	
	for(uint8_t i =0;i < Length;i ++)
	{
		OLED_ShowChar(Line,Column + i,Num / OLED_Pow(10,Length - i - 1) % 10 + '0');
	}
}

void OLED_ShowSignedNum(uint8_t Line,uint8_t Column,int32_t SignedNum,uint8_t Length)
{
	uint32_t uNum;
	if(SignedNum >= 0)
	{
		uNum = SignedNum;
		OLED_ShowChar(Line,Column,'+');
	}
	else
	{
		uNum = -SignedNum;
		OLED_ShowChar(Line,Column,'-');
	}

	for(uint8_t i = 0;i < Length;i ++)
	{
		OLED_ShowNum(Line,Column + 1 + i,uNum / OLED_Pow(10,Length - i - 1) % 10,1);
	}
}

void OLED_ShowHexNum(uint8_t Line,uint8_t Column,uint32_t HexNum,uint8_t Length)
{
	uint8_t SingleNum;

	for(uint8_t i =0;i < Length;i ++)
	{
		SingleNum = HexNum / OLED_Pow(16,Length - i - 1) % 16;
		if(SingleNum <= 9)
		{
			OLED_ShowNum(Line,Column + i,SingleNum,1);
		}
		else
		{
			OLED_ShowChar(Line,Column + i,SingleNum + 'A' - 10);
		}
	}
}

void OLED_ShowBinNum(uint8_t Line,uint8_t Column,uint32_t BinNum,uint8_t Length)
{
	for(uint8_t i = 0;i < Length;i ++)
	{
		OLED_ShowNum(Line,Column + i,BinNum / OLED_Pow(2,Length - i -1) % 2,1);
	}
}

void Convert64x16_to_8x128(const uint8_t src[64][16], uint8_t dst[8][128])
{
    uint8_t src_row; 
    uint8_t src_col;
    uint8_t dst_row;
    uint8_t dst_col;

    for (src_row = 0; src_row < 64; src_row++)
    {
        dst_row = src_row / 8;
        uint8_t dst_col_start = (src_row % 8) * 16;

        for (src_col = 0; src_col < 16; src_col++)
        {
            dst_col = dst_col_start + src_col;
            dst[dst_row][dst_col] = src[src_row][src_col];
        }
    }
}

void OLED_ShowBMP(const uint8_t bmp[][16]) 
{ 
    uint8_t BMPArray[8][128];
    Convert64x16_to_8x128(bmp,BMPArray);
    uint8_t i,j; 
    for (i = 0;i < 8;i ++) 
    { 
        OLED_SetCursor(i,0); 
        for (j = 0;j < 128;j ++) 
        { 
            OLED_WriteData(BMPArray[i][j]); 
        } 
    } 
}

void OLED_Init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}
