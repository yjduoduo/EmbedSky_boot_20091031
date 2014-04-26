/****************************************************************
 NAME: u2440mon.c
 DESC: u2440mon entry point,menu,download
 ****************************************************************/
#define	GLOBAL_CLK		1

#include <stdlib.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "nand.h"
#include "loader.h"
#include "LCD_TFT.h"

//
// Globals
//
extern DWORD JumpAddr;

extern void __RdPage512(UCHAR *); 
extern void __RdPage2048(UCHAR *); 
extern char NF_ReadID(void);
extern int CheckBadBlk(U32 );
extern DWORD boot_WinCE(DWORD , DWORD );
extern void boot_Linux(void);
extern char LB_Ext_Addr;
extern char isLBNand;
extern char NandID;

void Isr_Init(void);
void HaltUndef(void);
void HaltSwi(void);
void HaltPabort(void);
void HaltDabort(void);
void ClearMemory(void);


volatile unsigned char *downPt;
volatile U16 checkSum;
volatile unsigned int err=0;
volatile U32 totalDmaCount;

int download_run=0;
U32 tempDownloadAddress;
int menuUsed=0;

int consoleNum;

static U32 cpu_freq;
static U32 UPLL;
static void cal_cpu_bus_clk(void)
{
	U32 val;
	U8 m, p, s;
	
	val = rMPLLCON;
	m = (val>>12)&0xff;
	p = (val>>4)&0x3f;
	s = val&3;

	//(m+8)*FIN*2 不要超出32位数!
	FCLK = ((m+8)*(FIN/100)*2)/((p+2)*(1<<s))*100;
	
	val = rCLKDIVN;
	m = (val>>1)&3;
	p = val&1;	
	val = rCAMDIVN;
	s = val>>8;
	
	switch (m)
	{
		case 0:
			HCLK = FCLK;
			break;
		case 1:
			HCLK = FCLK>>1;
			break;
		case 2:
			if(s&2)
				HCLK = FCLK>>3;
			else
				HCLK = FCLK>>2;
			break;
		case 3:
			if(s&1)
				HCLK = FCLK/6;
			else
				HCLK = FCLK/3;
			break;
	}
	
	if(p)
		PCLK = HCLK>>1;
	else
		PCLK = HCLK;
	
	if(s&0x10)
		cpu_freq = HCLK;
	else
		cpu_freq = FCLK;
		
	val = rUPLLCON;
	m = (val>>12)&0xff;
	p = (val>>4)&0x3f;
	s = val&3;
	UPLL = ((m+8)*FIN)/((p+2)*(1<<s));
	UCLK = (rCLKDIVN&8)?(UPLL>>1):UPLL;
}


void Main(void)
{
	int i;
	U32 mpll_val = 0 ;
	//U32 divn_upll = 0 ;
	DWORD 	err; //, t0 = 0;
    
	U32 start_addr = 0x100000;
	unsigned char * to = (unsigned char * )LCDFRAMEBUFFER ;
	int size = 0x100000 ;

	Port_Init();
	
	Isr_Init();
	
	mpll_val = (92<<12)|(1<<4)|(1);		//400MHz
	
	//init FCLK=400M, so change MPLL first
	ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3);
	ChangeClockDivider();
	cal_cpu_bus_clk();
	
	LED_Display(0x140);
	LCD_TFT_Init() ;		// LCD initial
	Beep(2000, 100);

	MMU_EnableICache();
	NF_Init();
	consoleNum = 0;	// Uart 1 select for debug.
	Uart_Init( 0,115200 );
	Uart_Select( consoleNum );
	NandID = NF_ReadID();
	switch(NandID)
	{
		case 0x76:
			isLBNand = 0;
			break;
		case 0xf1:
			isLBNand = 1;
			LB_Ext_Addr = 0;
			break;
		case 0xda:
		case 0xdc:
		case 0xd3:
			isLBNand = 1;
			LB_Ext_Addr = 1;
			break;
		default:
			isLBNand = 1;
			LB_Ext_Addr = 1;
			break;
	}
	
	Uart_Printf("Nand Flash ID is 0xEC%x,is %s", NandID, isLBNand?"Large Block":"Short Block");
	 
//	Uart_Printf("startAddr = 0x%x, to = 0x%x, size = 0x%x\n",start_addr, to, size);
	for(i = (start_addr >> 11);size >0;)
	{
/*		if (CheckBadBlk(i))			//如果遇到坏块，即跳过该块（块地址+0x20）
		{
			Uart_Printf("bad block dwSector = 0x%x\n",i);
			i += 0x40;
			size -= 2048*64;
		}
*/
		ReadPage(i, to);
		i++;
		size -= 2048;
		to += 2048;
	}

	LCD_Display();

	Uart_Printf("\n\n");
	Uart_Printf("<***************************************>\n");
	Uart_Printf("               EmbedSky Boot\n");
	Uart_Printf("             www.embedsky.net\n");
	Uart_Printf("<***************************************>\n");

	rCLKCON = 0xfffff0;

	if (ERR_SUCCESS == TOC_Read())
	{
		boot_WinCE(1,0);
		Uart_Printf("JumpAddr : 0x%lx\n",JumpAddr);
		Uart_SendString("Launch WinCE\n");

		Launch(JumpAddr);
		err = ERR_JUMP_FAILED;
		Uart_SendString("\nBoot ERROR:");
	}
	else
		boot_Linux();
	Uart_SendString("\nBoot ERROR:");
	Uart_SendDWORD(err, TRUE);
	while (1);
}

void Isr_Init(void)
{
	pISR_UNDEF=(unsigned)HaltUndef;
	pISR_SWI  =(unsigned)HaltSwi;
	pISR_PABORT=(unsigned)HaltPabort;
	pISR_DABORT=(unsigned)HaltDabort;
	rINTMOD=0x0;	  // All=IRQ mode
	rINTMSK=BIT_ALLMSK;	  // All interrupt is masked.
}

void HaltUndef(void)
{
	Uart_Printf("Undefined instruction exception!!!\n");
	while(1);
}

void HaltSwi(void)
{
	Uart_Printf("SWI exception!!!\n");
	while(1);
}

void HaltPabort(void)
{
	Uart_Printf("Pabort exception!!!\n");
	while(1);
}

void HaltDabort(void)
{
	Uart_Printf("Dabort exception!!!\n");
	while(1);
}


void ClearMemory(void)
{
	int memError=0;
	U32 *pt;
	
	Uart_Printf("Clear Memory (%xh-%xh):WR",_RAM_STARTADDRESS,HEAPEND);

	pt=(U32 *)_RAM_STARTADDRESS;
	while((U32)pt < HEAPEND)
	{
		*pt=(U32)0x0;
		pt++;
	}
	
	if(memError==0)Uart_Printf("\b\bO.K.\n");
}

void Clk0_Enable(int clock_sel)	
{	// 0:MPLLin, 1:UPLL, 2:FCLK, 3:HCLK, 4:PCLK, 5:DCLK0
	rMISCCR = rMISCCR&~(7<<4) | (clock_sel<<4);
	rGPHCON = rGPHCON&~(3<<18) | (2<<18);
}
void Clk1_Enable(int clock_sel)
{	// 0:MPLLout, 1:UPLL, 2:RTC, 3:HCLK, 4:PCLK, 5:DCLK1	
	rMISCCR = rMISCCR&~(7<<8) | (clock_sel<<8);
	rGPHCON = rGPHCON&~(3<<20) | (2<<20);
}
void Clk0_Disable(void)
{
	rGPHCON = rGPHCON&~(3<<18);	// GPH9 Input
}
void Clk1_Disable(void)
{
	rGPHCON = rGPHCON&~(3<<20);	// GPH10 Input
}


