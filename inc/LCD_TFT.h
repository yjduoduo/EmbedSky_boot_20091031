/**************************************************************
The initial and control for TFT LCD
**************************************************************/

#define LCDW35			1		// ������(320X240)
#define LCDS35			2		// ������(320X240)
#define LCDT35			3		// ��֥��(240X320)
#define LCD43			4		// ����4.3����
#define LCD57			5		// 5.7����
#define LCD70			6		// 7.0����
#define VGA			7		//VGA
#define LCD104			8		//10.4����

#define LCD_Type		LCDW35		//�趨��������

#if(LCD_Type == LCDW35)				// ������

#define MVAL		(13)
#define MVAL_USED 	(0)			//0=each frame   1=rate by MVAL
#define INVVDEN		(1)			//0=normal       1=inverted
#define BSWP		(0)			//Byte swap control
#define HWSWP		(1)			//Half word swap control
#define PNRMODE		(3)			// ����ΪTFT��
#define BPPMODE		(12)			// ����Ϊ16bppģʽ

//TFT_SIZE
#define LCD_XSIZE_TFT 	(320)	
#define LCD_YSIZE_TFT 	(240)

//Timing parameter for 3.5' LCD
#define VBPD 		(12)			//��ֱͬ���źŵĺ��
#define VFPD 		(4)			//��ֱͬ���źŵ�ǰ��
#define VSPW 		(5)			//��ֱͬ���źŵ�����

#define HBPD 		(22)			//ˮƽͬ���źŵĺ��
#define HFPD 		(33)			//ˮƽͬ���źŵ�ǰ��
#define HSPW 		(44)			//ˮƽͬ���źŵ�����

#define CLKVAL_TFT 	(6) 	

#elif(LCD_Type == LCDS35)			// ������

#define MVAL		(13)
#define MVAL_USED 	(0)			//0=each frame   1=rate by MVAL
#define INVVDEN		(1)			//0=normal       1=inverted
#define BSWP		(0)			//Byte swap control
#define HWSWP		(1)			//Half word swap control
#define PNRMODE		(3)			// ����ΪTFT��
#define BPPMODE		(12)			// ����Ϊ16bppģʽ

//TFT_SIZE
#define LCD_XSIZE_TFT 	(320)	
#define LCD_YSIZE_TFT 	(240)

//Timing parameter for 3.5' LCD
#define VBPD 		(12)			//��ֱͬ���źŵĺ��
#define VFPD 		(4)			//��ֱͬ���źŵ�ǰ��
#define VSPW 		(5)			//��ֱͬ���źŵ�����

#define HBPD 		(8)			//ˮƽͬ���źŵĺ��
#define HFPD 		(16)			//ˮƽͬ���źŵ�ǰ��
#define HSPW 		(4)			//ˮƽͬ���źŵ�����

#elif(LCD_Type == LCDT35)			// ��֥��

#define MVAL		(13)
#define MVAL_USED 	(0)			//0=each frame   1=rate by MVAL
#define INVVDEN		(1)			//0=normal       1=inverted
#define BSWP		(0)			//Byte swap control
#define HWSWP		(1)			//Half word swap control
#define PNRMODE		(3)			// ����ΪTFT��
#define BPPMODE		(12)			// ����Ϊ16bppģʽ

//TFT_SIZE
#define LCD_XSIZE_TFT 	(240)	
#define LCD_YSIZE_TFT 	(320)

//Timing parameter for 3.5' LCD
#define VBPD 		(1)			//��ֱͬ���źŵĺ��
#define VFPD 		(2)			//��ֱͬ���źŵ�ǰ��
#define VSPW 		(6)			//��ֱͬ���źŵ�����

#define HBPD 		(5)			//ˮƽͬ���źŵĺ��
#define HFPD 		(10)			//ˮƽͬ���źŵ�ǰ��
#define HSPW 		(15)			//ˮƽͬ���źŵ�����

#define CLKVAL_TFT 	(4) 	

#elif(LCD_Type == LCD43)			// 4.3����

#define MVAL		(13)
#define MVAL_USED 	(0)			//0=each frame   1=rate by MVAL
#define INVVDEN		(1)			//0=normal       1=inverted
#define BSWP		(0)			//Byte swap control
#define HWSWP		(1)			//Half word swap control
#define PNRMODE		(3)			// ����ΪTFT��
#define BPPMODE		(12)			// ����Ϊ16bppģʽ

//TFT_SIZE
#define LCD_XSIZE_TFT 	(480)	
#define LCD_YSIZE_TFT 	(272)

//Timing parameter for 4.3' LCD
#define VBPD 		(2)			//��ֱͬ���źŵĺ��
#define VFPD 		(2)			//��ֱͬ���źŵ�ǰ��
#define VSPW 		(4)			//��ֱͬ���źŵ�����

#define HBPD 		(2)			//ˮƽͬ���źŵĺ��
#define HFPD 		(40)			//ˮƽͬ���źŵ�ǰ��
#define HSPW 		(41)			//ˮƽͬ���źŵ�����

#define CLKVAL_TFT 	(4) 	

#elif(LCD_Type == LCD57)			// 5.7����

#define MVAL		(13)
#define MVAL_USED 	(0)			//0=each frame   1=rate by MVAL
#define INVVDEN		(1)			//0=normal       1=inverted
#define BSWP		(0)			//Byte swap control
#define HWSWP		(1)			//Half word swap control
#define PNRMODE		(3)			// ����ΪTFT��
#define BPPMODE		(12)			// ����Ϊ16bppģʽ

//TFT_SIZE
#define LCD_XSIZE_TFT 	(320)	
#define LCD_YSIZE_TFT 	(240)

//Timing parameter for 5.7' LCD
#define VBPD 		(3)			//��ֱͬ���źŵĺ��
#define VFPD 		(5)			//��ֱͬ���źŵ�ǰ��
#define VSPW 		(15)			//��ֱͬ���źŵ�����

#define HBPD 		(5)			//ˮƽͬ���źŵĺ��
#define HFPD 		(15)			//ˮƽͬ���źŵ�ǰ��
#define HSPW 		(8)			//ˮƽͬ���źŵ�����

#define CLKVAL_TFT 	(7) 	

#elif(LCD_Type == VGA)				// VGA

#define MVAL		(13)
#define MVAL_USED 	(0)			//0=each frame   1=rate by MVAL
#define INVVDEN		(1)			//0=normal       1=inverted
#define BSWP		(0)			//Byte swap control
#define HWSWP		(1)			//Half word swap control
#define PNRMODE		(3)			// ����ΪTFT��
#define BPPMODE		(12)			// ����Ϊ16bppģʽ

//TFT_SIZE
#define LCD_XSIZE_TFT 	(640)	
#define LCD_YSIZE_TFT 	(480)

//Timing parameter for VGA
#define VBPD 		(29)			//��ֱͬ���źŵĺ��
#define VFPD 		(5)			//��ֱͬ���źŵ�ǰ��
#define VSPW 		(4)			//��ֱͬ���źŵ�����

#define HBPD 		(71)			//ˮƽͬ���źŵĺ��
#define HFPD 		(40)			//ˮƽͬ���źŵ�ǰ��
#define HSPW 		(31)			//ˮƽͬ���źŵ�����

#define CLKVAL_TFT 	(1) 	

#elif(LCD_Type == LCD70)			// 7.0����

#define MVAL		(13)
#define MVAL_USED 	(0)			//0=each frame   1=rate by MVAL
#define INVVDEN		(1)			//0=normal       1=inverted
#define BSWP		(0)			//Byte swap control
#define HWSWP		(1)			//Half word swap control
#define PNRMODE		(3)			// ����ΪTFT��
#define BPPMODE		(12)			// ����Ϊ16bppģʽ

//TFT_SIZE
#define LCD_XSIZE_TFT 	(800)	
#define LCD_YSIZE_TFT 	(480)

//Timing parameter for 7.0' LCD
#define VBPD 		(3)			//��ֱͬ���źŵĺ��
#define VFPD 		(5)			//��ֱͬ���źŵ�ǰ��
#define VSPW 		(15)			//��ֱͬ���źŵ�����

#define HBPD 		(5)			//ˮƽͬ���źŵĺ��
#define HFPD 		(15)			//ˮƽͬ���źŵ�ǰ��
#define HSPW 		(8)			//ˮƽͬ���źŵ�����

#define CLKVAL_TFT 	(7) 	

#elif(LCD_Type == LCD104)			// 10.4����

#define MVAL		(13)
#define MVAL_USED 	(0)			//0=each frame   1=rate by MVAL
#define INVVDEN		(1)			//0=normal       1=inverted
#define BSWP		(0)			//Byte swap control
#define HWSWP		(1)			//Half word swap control
#define PNRMODE		(3)			// ����ΪTFT��
#define BPPMODE		(12)			// ����Ϊ16bppģʽ

//TFT_SIZE
#define LCD_XSIZE_TFT 	(800)	
#define LCD_YSIZE_TFT 	(600)

//Timing parameter for 10.4' LCD
#define VBPD 		(5)			//��ֱͬ���źŵĺ��
#define VFPD 		(6)			//��ֱͬ���źŵ�ǰ��
#define VSPW 		(1)			//��ֱͬ���źŵ�����

#define HBPD 		(28)			//ˮƽͬ���źŵĺ��
#define HFPD 		(14)			//ˮƽͬ���źŵ�ǰ��
#define HSPW 		(180)			//ˮƽͬ���źŵ�����

#define CLKVAL_TFT 	(3) 	

#endif

#define SCR_XSIZE_TFT 	(LCD_XSIZE_TFT)
#define SCR_YSIZE_TFT 	(LCD_YSIZE_TFT)

#define HOZVAL_TFT	(LCD_XSIZE_TFT-1)
#define LINEVAL_TFT	(LCD_YSIZE_TFT-1)

#define LCDFRAMEBUFFER		0x33E00000			//LCD�Դ����ʼ��ַ�� 


void LCD_TFT_Init(void);

void LCD_Display(void);



