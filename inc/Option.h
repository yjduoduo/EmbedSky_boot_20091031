/**************************************************************
 NAME: option.h
 DESC: To measuure the USB download speed, the WDT is used.
       To measure up to large time, The WDT interrupt is used.
 **************************************************************/
 
#ifndef __OPTION_H__
#define __OPTION_H__

#define FIN			(12000000)	

//use variable
#ifdef GLOBAL_CLK
	U32 FCLK;
	U32 HCLK;
	U32 PCLK;
	U32 UCLK;
#else
	extern unsigned int FCLK;
	extern unsigned int HCLK;
	extern unsigned int PCLK;
	extern unsigned int UCLK;
#endif

// BUSWIDTH : 16,32
#define BUSWIDTH    (32)

//64MB
// 0x30000000 ~ 0x33DFFFFF : 其他程序使用区域
// 0x33E00000 ~ 0x33F7FFFF : 显存使用区域
// 0x33F80000 ~ 0x33FF47FF : 本程序使用区域
// 0x33FF4800 ~ 0x33FF7FFF : FIQ ~ User Stack Area
// 0x33FF8000 ~ 0x33FFFEFF : Not Useed Area
// 0x33FFFF00 ~ 0x33FFFFFF : Exception & ISR Vector Table

#define _RAM_STARTADDRESS			0x30000000
#define _ISR_STARTADDRESS			0x33ffff00     
#define _MMUTT_STARTADDRESS			0x33ff8000
#define _STACK_BASEADDRESS			0x33ff8000
#define HEAPEND					0x33ff0000
#define _NONCACHE_STARTADDRESS			0x31000000

#define VA_BASE					0x8C000000  // defined in OEMAddressTable
#define VIRTUAL_TO_PHYSICAL(va)			((va) - VA_BASE + _RAM_STARTADDRESS)

#endif /*__OPTION_H__*/
