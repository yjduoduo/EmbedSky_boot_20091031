//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
--*/
#include <string.h>

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "nand.h"
#include "loader.h"

void __RdPage512(UCHAR *bufPt); 
void __RdPage2048(UCHAR *bufPt); 

extern void drawProcessBar(U32 , U32 );
#define	puts	Uart_Printf
#define	printf	Uart_Printf
#define	getch	Uart_Getch
#define	putch	Uart_SendByte


#define k9f1208		0xec76
#define k9f1g08		0xecf1
#define k9f2g08		0xecda
#define k9f4g08		0xecdc
#define k9f8g08		0xecd3

char LB_Ext_Addr = 1;
char isLBNand = 1;
char NandID;

TOC toc; // made global because it's too big for our tiny stack
DWORD JumpAddr;

//
//  Reset the chip
//
void NF_Reset()
{
	NF_CE_L();
	NF_CLEAR_RB();
	NF_CMD(CMD_RESET);  
	NF_CE_H();
}


BOOL 
Nand_ReadSector( SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff);

void NF_Init(void)
{
	rNFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<0);
	rNFCONT = (0<<13)|(0<<12)|(0<<10)|(0<<9)|(0<<8)|(1<<6)|(1<<5)|(1<<4)|(1<<1)|(1<<0);
	rNFSTAT = 0;
}

U32 WaitNFBusy(void)	// R/B 未接好?
{
	U8 stat;
	
	NF_CMD(CMD_STATUS);
	do
	{
		stat = RdNFDat();
		//Uart_Printf("%x\n", stat);
	}while(!(stat&0x40));
	NF_CMD(CMD_READ);
	return stat&1;
}

char NF_ReadID()
{
	char	pMID;
	char	pDID;
	int 	nCnt;
	int	nBuff;
	char	n4thcycle;
	int	i;

	LB_Ext_Addr = 0;
	n4thcycle = nBuff = 0;

	NF_nFCE_L();    
	NF_CLEAR_RB();
	NF_CMD(CMD_READID);	// read id command
	NF_ADDR(CMD_READ);
	for ( i = 0; i < 100; i++ );

	/* tREA is necessary to Get a MID. */
	for (nCnt = 0; nCnt < 5; nCnt++)
	{
		pMID = (BYTE) NF_RDDATA();
		if (0xEC == pMID)
			break;
	}

	pDID = (BYTE) NF_RDDATA();

	nBuff     = (BYTE) NF_RDDATA();
	n4thcycle = (BYTE) NF_RDDATA();
	NF_nFCE_H();

	if (pDID >= 0xA0)
	{
		LB_Ext_Addr = 1;
	}

	return (pDID);
}


int SB_CheckBadBlk(U32 addr)
{
	U8 dat;

	addr &= ~0x1f;

	NF_nFCE_L();   

	NF_CMD(CMD_READ);	//point to area c
	NF_ADDR(5);		//mark offset 4,5,6,7
	NF_ADDR(8);
	NF_ADDR(addr);
	NF_ADDR(addr>>8);
	NF_ADDR(addr>>16);
	NF_CMD(CMD_READ3);
	WaitNFBusy();
	dat = RdNFDat();

	NF_CMD(CMD_READ);	//point to area a

	NF_nFCE_H();

	return (dat!=0xff);
}

int LB_CheckBadBlk(U32 addr)
{
	U8 dat;

	addr &= ~0x1f;

	NF_nFCE_L();   

	NF_CMD(CMD_READ);	//point to area c
	NF_ADDR(5);		//mark offset 4,5,6,7
	NF_ADDR(8);
	NF_ADDR(addr);
	NF_ADDR(addr>>8);
	if(LB_Ext_Addr == 1)
		NF_ADDR(addr>>16);
	NF_CMD(CMD_READ3);
	WaitNFBusy();
	dat = RdNFDat();

	NF_CMD(CMD_READ);	//point to area a

	NF_nFCE_H();

	return (dat!=0xff);
}

static U32 SB_EraseBlock(U32 addr)
{
	U8 stat;

	addr &= ~0x1f;

	NF_nFCE_L();
	NF_CLEAR_RB();
	NF_CMD(CMD_ERASE1);
	NF_ADDR(addr);
	NF_ADDR(addr>>8);
	if(LB_Ext_Addr == 1)
		NF_ADDR(addr>>16);
	NF_CMD(CMD_ERASE2);
	stat = WaitNFBusy();
	NF_nFCE_H();

#ifdef	ER_BAD_BLK_TEST
	if(!((addr+0xe0)&0xff)) stat = 1;	//just for test bad block
#endif

	//printf("Erase block 0x%x %s\n", addr, stat?"fail":"ok");

	return stat;
}

static U32 LB_EraseBlock(U32 addr)
{
	U8 stat;

	addr &= ~0x1f;

	NF_nFCE_L();
	NF_CLEAR_RB();
	NF_CMD(CMD_ERASE1);
	NF_ADDR(addr);
	NF_ADDR(addr>>8);
	NF_ADDR(addr>>16);
	NF_CMD(CMD_ERASE2);
	stat = WaitNFBusy();
	NF_nFCE_H();

#ifdef	ER_BAD_BLK_TEST
	if(!((addr+0xe0)&0xff)) stat = 1;	//just for test bad block
#endif

	//printf("Erase block 0x%x %s\n", addr, stat?"fail":"ok");

	return stat;
}

static void SB_MarkBadBlk(U32 addr)
{
	addr &= ~0x1f;

	NF_nFCE_L();
	NF_CLEAR_RB();
	// this command disable for k9F4G 
	NF_CMD(CMD_WRITE1);

	NF_ADDR(4);		
	//
	NF_ADDR(8);		//mark offset 4,5,6,7
	//
	NF_ADDR(addr);
	NF_ADDR(addr>>8);
	NF_ADDR(addr>>16);
	NF_WRDATA(0);			//mark with 0
	NF_WRDATA(0);
	NF_WRDATA(0);			//mark with 0
	NF_WRDATA(0);
	NF_CMD(CMD_WRITE1);

	WaitNFBusy();		//needn't check return status

	NF_nFCE_H();
}

static void LB_MarkBadBlk(U32 addr)
{
	addr &= ~0x1f;

	NF_nFCE_L();
	NF_CLEAR_RB();
	// this command disable for k9F4G 
	NF_CMD(CMD_WRITE1);

	NF_ADDR(4);		
	//
	NF_ADDR(8);		//mark offset 4,5,6,7
	//
	NF_ADDR(addr);
	NF_ADDR(addr>>8);
	NF_ADDR(addr>>16);
	NF_WRDATA(0);			//mark with 0
	NF_WRDATA(0);
	NF_WRDATA(0);			//mark with 0
	NF_WRDATA(0);
	NF_CMD(CMD_WRITE1);

	WaitNFBusy();		//needn't check return status

	NF_nFCE_H();
}

void SB_ReadPage(U32 addr, unsigned char * to)
{
	U32 j;
	//  Initialize ECC register
	NF_RSTECC();
	NF_MECC_UnLock();

	//  Enable the chip
	NF_nFCE_L();
	NF_CLEAR_RB();

	// Issue Read command
	NF_CMD(CMD_READ);

	//  Set up address
	NF_ADDR(0x00);
	NF_ADDR((addr) & 0xff);
	NF_ADDR((addr >> 8) & 0xff);
	NF_ADDR((addr >> 16) & 0xff);

	for (j = 0; j < 10; j++);   // wait tWB(100ns)

	NF_DETECT_RB();		// wait tR(max 12us)

	__RdPage512(to);

	NF_MECC_Lock();

	NF_nFCE_H();

}

void LB_ReadPage(U32 addr, unsigned char * to)
{
	U32 j;
	//  Initialize ECC register
	NF_RSTECC();
	NF_MECC_UnLock();

	//  Enable the chip
	NF_nFCE_L();   
	NF_CLEAR_RB();

	// Issue Read command
	NF_CMD(CMD_READ);

	//  Set up address
	NF_ADDR(0x00);
	NF_ADDR(0x00);
	NF_ADDR((addr) & 0xff);
	NF_ADDR((addr >> 8) & 0xff);
	if(LB_Ext_Addr == 1)
		NF_ADDR((addr >> 16) & 0xff);

	NF_CMD(CMD_READ3);
	for (j = 0; j < 10; j++);   // wait tWB(100ns)

	NF_DETECT_RB();		// wait tR(max 12us)

	__RdPage2048(to);

	NF_MECC_Lock();

	NF_nFCE_H();

}

static U32 SB_WritePage(U32 addr, U8 * buf)
{
	U32 i, mecc;
	U8 stat, tmp[7];

	NF_nFCE_L();   
	NF_CLEAR_RB();
	NF_CMD(CMD_WRITE1);
	NF_ADDR(0);
	NF_ADDR(0);
	NF_ADDR(addr);
	NF_ADDR(addr>>8);
	NF_ADDR(addr>>16);

	for(i=0; i<2048; i++)
		NF_WRDATA(buf[i]);

	NF_CMD(CMD_WRITE2);

	NF_RSTECC();	//reset mecc and secc
	NF_MECC_UnLock();

	NF_MECC_Lock();

	mecc = NF_RDMECC();
	
	tmp[0] = mecc&0xff;
	tmp[1] = (mecc>>8)&0xff;
	tmp[2] = (mecc>>16)&0xff;
	tmp[3] = (mecc>>24)&0xff;
	tmp[5] = 0xff;	//mark good block

	NF_SECC_UnLock();
	NF_WRDATA(tmp[0]);
	NF_WRDATA(tmp[1]);
	NF_WRDATA(tmp[2]);
	NF_WRDATA(tmp[3]);
	NF_SECC_Lock();
	NF_WRDATA(tmp[4]);
	NF_WRDATA(tmp[5]);

	stat = WaitNFBusy();
	NF_nFCE_H();
//	printf("Write nand flash 2048 Byte\n");

#ifdef	WR_BAD_BLK_TEST
	if((addr&0xff)==0x17)
		stat = 1;	//just for test bad block
#endif

	if(stat)
		printf("Write nand flash 0x%x fail\n", addr);
	else
	{	
		U8 RdDat[2048];
		ReadPage(addr, RdDat);		
		for(i=0; i<2048; i++)	
			if(RdDat[i]!=buf[i])
			{
				printf("Check data at page 0x%x, offset 0x%x fail\n", addr, i);
				stat = 1;
				break;
			}
	}

	return stat;	
}

static U32 LB_WritePage(U32 addr, U8 * buf)
{
	U32 i, mecc;
	U8 stat, tmp[7];

	NF_nFCE_L();   
	NF_CLEAR_RB();
	NF_CMD(CMD_WRITE1);
	NF_ADDR(0);
	NF_ADDR(0);
	NF_ADDR(addr);
	NF_ADDR(addr>>8);
	NF_ADDR(addr>>16);

	for(i=0; i<2048; i++)
		NF_WRDATA(buf[i]);

	NF_CMD(CMD_WRITE2);

	NF_RSTECC();	//reset mecc and secc
	NF_MECC_UnLock();

	NF_MECC_Lock();

	mecc = NF_RDMECC();
	
	tmp[0] = mecc&0xff;
	tmp[1] = (mecc>>8)&0xff;
	tmp[2] = (mecc>>16)&0xff;
	tmp[3] = (mecc>>24)&0xff;
	tmp[5] = 0xff;	//mark good block

	NF_SECC_UnLock();
	NF_WRDATA(tmp[0]);
	NF_WRDATA(tmp[1]);
	NF_WRDATA(tmp[2]);
	NF_WRDATA(tmp[3]);
	NF_SECC_Lock();
	NF_WRDATA(tmp[4]);
	NF_WRDATA(tmp[5]);

	stat = WaitNFBusy();
	NF_nFCE_H();
//	printf("Write nand flash 2048 Byte\n");

#ifdef	WR_BAD_BLK_TEST
	if((addr&0xff)==0x17)
		stat = 1;	//just for test bad block
#endif

	if(stat)
		printf("Write nand flash 0x%x fail\n", addr);
	else
	{	
		U8 RdDat[2048];
		ReadPage(addr, RdDat);		
		for(i=0; i<2048; i++)	
			if(RdDat[i]!=buf[i])
			{
				printf("Check data at page 0x%x, offset 0x%x fail\n", addr, i);
				stat = 1;
				break;
			}
	}

	return stat;	
}

int CheckBadBlk(U32 addr)
{
	if(isLBNand == 0)
		return SB_CheckBadBlk(addr);
	else
		return LB_CheckBadBlk(addr);
}


U32 EraseBlock(U32 addr)
{
	if(isLBNand == 0)
		return SB_EraseBlock(addr);
	else
		return LB_EraseBlock(addr);
}

void MarkBadBlk(U32 addr)
{
	if(isLBNand == 0)
		SB_MarkBadBlk(addr);
	else
		LB_MarkBadBlk(addr);
}

void ReadPage(U32 addr, unsigned char * to)
{
	if(isLBNand == 0)
		SB_ReadPage(addr, to);
	else
		LB_ReadPage(addr, to);
}

U32 WritePage(U32 addr, unsigned char * buf)
{
	if(isLBNand == 0)
		return SB_WritePage(addr, buf);
	else
		return LB_WritePage(addr, buf);
}


//-------------------------------------------------------
// 下面是WinCE启动程序
//-------------------------------------------------------

//  FMD_ReadSector
//
//  Read the content of the sector.
//
//  startSectorAddr: Starting page address
//  pSectorBuff  : Buffer for the data portion
//  pSectorInfoBuff: Buffer for Sector Info structure
//  dwNumSectors : Number of sectors
//
BOOL 
FMD_ReadSector( SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff, PSectorInfo pSectorInfoBuff, DWORD dwNumSectors )
{
	DWORD   i, r = 0;
	BYTE   ecc0,ecc1,ecc2;
	BOOL   rc = 1;
	ECCRegVal eccRegVal;
	BYTE se[16];

//	Uart_Printf("startSectorAddr=0x%x\n", startSectorAddr);

	//  BUGBUGBUG: I need to come back to support dwNumSectors > 1
	//
	//  Sanity check
	if (!pSectorBuff && !pSectorInfoBuff || dwNumSectors > 1 || !pSectorBuff)
	{
		Uart_SendString("ERROR_INVALID_PARAMETER\n");
		return FALSE;
	}

//	Uart_SendString("R: ");
//	Uart_SendDWORD(startSectorAddr, TRUE);

_FMD_ReadSectorretry:
	//  Initialize ECC register
	NF_RSTECC();
	NF_MECC_UnLock();

	//  Enable the chip
	NF_nFCE_L();   
	NF_CLEAR_RB();

	// Issue Read command
	NF_CMD(CMD_READ);

	//  Set up address
	NF_ADDR(0x00);
	NF_ADDR(0x00);
	NF_ADDR((startSectorAddr) & 0xff);
	NF_ADDR((startSectorAddr >> 8) & 0xff);
	NF_ADDR((startSectorAddr >> 16) & 0xff);
	NF_CMD(CMD_READ3);

	for (i = 0; i < 10; i++);   // wait tWB(100ns)

	NF_DETECT_RB();		// wait tR(max 12us)

	// read the data
	__RdPage2048(pSectorBuff);
//	for(i=0; i<512; i++)
//		pSectorBuff[i] = RdNFDat();
	NF_MECC_Lock();

//	for ( i = 0; i < 512; i++ )
//	{
//		Uart_SendDWORD(*(pSectorBuff+i), TRUE);
//		Uart_SendByte(*(pSectorBuff+i));
//	}

	eccRegVal.dwECCVal = NF_ECC();
//	Uart_SendString("ECCv: ");
//	Uart_SendDWORD(eccRegVal.dwECCVal, TRUE);

//	NF_DETECT_RB();		// wait tR(max 12us)
	for(i=0;i<16;i++)
	{
		se[i] = RdNFDat();
//		Uart_SendDWORD(se[i], TRUE);
	}

	ecc0 = se[8];
	ecc1 = se[9];
	ecc2 = se[10];
	
//	Uart_SendString("9ECC0: ");
//	Uart_SendDWORD(ecc0, TRUE);
//	Uart_SendString("ECC1: ");
//	Uart_SendDWORD(ecc1, TRUE);
//	Uart_SendString("ECC2: ");
//	Uart_SendDWORD(ecc2, TRUE);
	NF_nFCE_H();

	if ( !rc && r++ < 3 )
	{
		Uart_SendString("FMD_ReadSector: ");
		Uart_SendDWORD(startSectorAddr, 1);

		NF_Reset();

		for (i = 0; i < 5; i++);   // delay

		rc = TRUE;

		goto _FMD_ReadSectorretry;
	}
	
	if ( startSectorAddr < 0x120 ) // NO ECC Check about EBOOT
	{
		rc = TRUE;
	}
	else
	{
		if(	ecc0 != eccRegVal.bECCBuf[0] || ecc0 != eccRegVal.bECCBuf[0] || ecc0 != eccRegVal.bECCBuf[0] )
		{
//			Uart_SendString("ECC mismatch for Sector: ");
//			Uart_SendDWORD(startSectorAddr, TRUE);
			rc = FALSE;
		}
	}

	return rc;
}

BOOL
TOC_Read()
{
	if ( !FMD_ReadSector(TOC_SECTOR, (LPBYTE)&toc, NULL, 1) )
	{
//		Uart_SendString("ERR_DISK_OP_FAIL1\n");
		return ERR_DISK_OP_FAIL1;
	}

	if ( !VALID_TOC(&toc) )
	{
//		Uart_SendString("ERR_INVALID_TOC: ");
//		Uart_SendDWORD(toc.dwSignature, TRUE);
		return ERR_INVALID_TOC;
	}
	return 0;
}

//  Nand_ReadSector
//
//  Read the content of the sector.
//
//  startSectorAddr: Starting page address
//  pSectorBuff  : Buffer for the data portion
//
BOOL 
Nand_ReadSector( SECTOR_ADDR startSectorAddr, LPBYTE pSectorBuff)
{
	DWORD   i, r = 0;
//	BYTE   ecc0,ecc1,ecc2;
	BOOL   rc = 1;
//	ECCRegVal eccRegVal;
//	BYTE se[16];

//	Uart_Printf("startSectorAddr=0x%x\n", startSectorAddr);

	//  BUGBUGBUG: I need to come back to support dwNumSectors > 1
	//
	//  Sanity check
	if (!pSectorBuff || !pSectorBuff)
	{
		Uart_SendString("ERROR_INVALID_PARAMETER\n");
		return FALSE;
	}

//	Uart_SendString("R: ");
//	Uart_SendDWORD(startSectorAddr, TRUE);

_Nand_ReadSectorretry:
	//  Initialize ECC register
	NF_RSTECC();
	NF_MECC_UnLock();

	//  Enable the chip
	NF_nFCE_L();   
	NF_CLEAR_RB();

	// Issue Read command
	NF_CMD(CMD_READ);

	//  Set up address
	NF_ADDR(0x00);
	NF_ADDR(0x00);
	NF_ADDR((startSectorAddr) & 0xff);
	NF_ADDR((startSectorAddr >> 8) & 0xff);
	NF_ADDR((startSectorAddr >> 16) & 0xff);
	NF_CMD(CMD_READ3);

	for (i = 0; i < 10; i++);   // wait tWB(100ns)

	NF_DETECT_RB();		// wait tR(max 12us)

	// read the data
	__RdPage2048(pSectorBuff);

	NF_MECC_Lock();

	NF_nFCE_H();

	if ( !rc && r++ < 3 )
	{
		Uart_SendString("Nand_ReadSector: ");
		Uart_SendDWORD(startSectorAddr, 1);

		NF_Reset();

		for (i = 0; i < 5; i++);   // delay

		rc = TRUE;

		goto _Nand_ReadSectorretry;
	}

	return rc;
}

// -----------------------------------------------------------------------------
//	boot_WinCE:
//	Reads nk.nb0 from NAND
//	Returns ERR_Xxx
// -----------------------------------------------------------------------------
DWORD
boot_WinCE(DWORD dwEntry, DWORD dwSig)
{
	DWORD dwSectorsNeeded;
	DWORD dwSector, dwLength;		 // Start Sector & Length
	DWORD dwRAM, i;
	int abcdefg = 0;				//HJ_add 滚动条
	int gfedcba = 0;				//HJ_add 滚动条

	if ( !(toc.id[dwEntry].dwImageType & IMAGE_TYPE_RAMIMAGE) )
	{
		Uart_SendString("ERR_INVALID_FILE_TYPE: ");
		Uart_SendDWORD(toc.id[dwEntry].dwImageType, TRUE);
		return ERR_INVALID_FILE_TYPE;
	}

	dwSectorsNeeded = toc.id[dwEntry].dwTtlSectors;		// xipkernel size = 0x9B4
	
//	Uart_SendString("Sector addr on NAND: ");
//	Uart_SendDWORD(toc.id[dwEntry].sgList[0].dwSector, TRUE);
//	Uart_SendString("TotalSector: ");
//	Uart_SendDWORD(dwSectorsNeeded, TRUE);

	dwRAM	= VIRTUAL_TO_PHYSICAL(toc.id[dwEntry].dwLoadAddress);

	JumpAddr = toc.id[dwEntry].dwJumpAddress ? VIRTUAL_TO_PHYSICAL(toc.id[dwEntry].dwJumpAddress) :
	VIRTUAL_TO_PHYSICAL(toc.id[dwEntry].dwLoadAddress);

	//
	// Load the disk image directly into RAM
	//
	Uart_SendString("Reading WinCE Image from NAND, Please Waiting ...\r\n");
	i = 0;

	while (dwSectorsNeeded && i < MAX_SG_SECTORS)
	{
		dwSector = toc.id[dwEntry].sgList[i].dwSector;
		dwLength = toc.id[dwEntry].sgList[i].dwLength;

		Uart_SendString("    dwSector: ");
		Uart_SendDWORD(dwSector, TRUE);
		Uart_SendString("    dwLength: ");
		Uart_SendDWORD(dwLength, TRUE);
		Uart_SendString("    dwRAM: ");
		Uart_SendDWORD(dwRAM, TRUE);

		gfedcba = dwLength;			//HJ_add 滚动条
		// read each sg segment
		while (dwLength)
		{
			abcdefg ++ ;					//HJ_add 滚动条
			drawProcessBar(gfedcba, abcdefg);		//HJ_add 滚动条
//			if ( !FMD_ReadSector(dwSector, (LPBYTE)dwRAM, NULL, 1) )
			if (CheckBadBlk(dwSector))			//如果遇到坏块，即跳过该块（块地址+0x20）
			{
//				Uart_Printf("bad block dwSector = 0x%x\n",dwSector);
				dwSector += 0x20;
			}
			if ( !Nand_ReadSector(dwSector, (LPBYTE)dwRAM) )
			{
				Uart_SendString("ERR_DISK_OP_FAIL2: ");
				Uart_SendDWORD(dwSector, TRUE);

				dwSector++;
				continue;

			}
			dwSector++;
			dwLength--;
			dwRAM += SECTOR_SIZE;
		}

		dwSectorsNeeded -= toc.id[dwEntry].sgList[i].dwLength;
		i++;
	}

	//  We only do this if the dwRAM is not zero (The default tocblock1 
	//  set the dwRAM to be 0)
	if (toc.chainInfo.dwLoadAddress == 0)
	{
		return ERR_SUCCESS;
	}

	dwRAM = VIRTUAL_TO_PHYSICAL(toc.chainInfo.dwLoadAddress);		// 0x303c0000
	dwSectorsNeeded = toc.chainInfo.dwLength;							// 0x20
	dwSector = toc.chainInfo.dwFlashAddress;								// 0x103c0

	Uart_SendString("Reading Chain from NAND\r\n");
	Uart_SendString("LoadAddr: ");
	Uart_SendDWORD(dwRAM, TRUE);
	Uart_SendString("NAND SectorAddr: ");
	Uart_SendDWORD(dwSector, TRUE);
	Uart_SendString("Length: ");
	Uart_SendDWORD(dwSectorsNeeded, TRUE);

	while(dwSectorsNeeded)
	{
//		if (!FMD_ReadSector(dwSector, (LPBYTE) dwRAM, NULL, 1) )
		if ( !Nand_ReadSector(dwSector, (LPBYTE)dwRAM) )
		{
			Uart_SendString("Failed reading Chain.bin:");
			Uart_SendDWORD(dwSector, TRUE);

			dwSector++;
			continue;
		}
		dwSector++;
		dwSectorsNeeded--;
		dwRAM += SECTOR_SIZE;
	}

	return ERR_SUCCESS;
}


//---------------------------------------------------------------------------
//**  下面是linux启动程序
//---------------------------------------------------------------------------
#define LINUX_KERNEL_OFFSET			0x8000
#define LINUX_PARAM_OFFSET			0x100
#define LINUX_PAGE_SIZE					0x00001000
#define LINUX_PAGE_SHIFT				12
#define LINUX_ZIMAGE_MAGIC				0x016f2818
#define DRAM_SIZE						0x04000000

/* * Usage: 
     *  - do not go blindly adding fields, add them at the end
     *  - when adding fields, don't rely on the address until
     *    a patch from me has been released
     *  - unused fields should be zero (for future expansion)
     *  - this structure is relatively short-lived - only
     *    guaranteed to contain useful data in setup_arch()
     */
#define COMMAND_LINE_SIZE 1024
/* This is the old deprecated way to pass parameters to the kernel */
struct param_struct
{
	union
	{
		struct
		{
			unsigned long page_size;				/*  0 */
			unsigned long nr_pages;				/*  4 */
			unsigned long ramdisk_size;			/*  8 */
			unsigned long flags;					/* 12 */
			#define FLAG_READONLY		1
			#define FLAG_RDLOAD		4
			#define FLAG_RDPROMPT		8
			unsigned long rootdev;					/* 16 */
			unsigned long video_num_cols;			/* 20 */
			unsigned long video_num_rows;		/* 24 */
			unsigned long video_x;				/* 28 */
			unsigned long video_y;				/* 32 */
			unsigned long memc_control_reg;		/* 36 */
			unsigned char sounddefault;			/* 40 */
			unsigned char adfsdrives;				/* 41 */
			unsigned char bytes_per_char_h;		/* 42 */
			unsigned char bytes_per_char_v;		/* 43 */
			unsigned long pages_in_bank[4];		/* 44 */
			unsigned long pages_in_vram;			/* 60 */
			unsigned long initrd_start;				/* 64 */
			unsigned long initrd_size;				/* 68 */
			unsigned long rd_start;				/* 72 */
			unsigned long system_rev;				/* 76 */
			unsigned long system_serial_low;		/* 80 */
			unsigned long system_serial_high;		/* 84 */
			unsigned long mem_fclk_21285;       	/* 88 */
		}s;
		char unused[256];
	}u1;
	union 
	{
		char paths[8][128];
		struct
		{
			unsigned long magic;
			char n[1024 - sizeof(unsigned long)];
		} s; 
	} u2; 
	char commandline[COMMAND_LINE_SIZE];
};

/*************************************************************/
static __inline void cpu_arm920_cache_clean_invalidate_all(void)
{
	__asm{
		mov	r1, #0		
		mov	r1, #7 << 5			  	/* 8 segments */
cache_clean_loop1:		
		orr	r3, r1, #63UL << 26	  	/* 64 entries */
cache_clean_loop2:	
		mcr	p15, 0, r3, c7, c14, 2	/* clean & invalidate D index */
		subs	r3, r3, #1 << 26
		bcs	cache_clean_loop2		/* entries 64 to 0 */
		subs	r1, r1, #1 << 5
		bcs	cache_clean_loop1		/* segments 7 to 0 */
		mcr	p15, 0, r1, c7, c5, 0	/* invalidate I cache */
		mcr	p15, 0, r1, c7, c10, 4	/* drain WB */
	}
}
void cache_clean_invalidate(void)
{
	cpu_arm920_cache_clean_invalidate_all();
}

static __inline void cpu_arm920_tlb_invalidate_all(void)
{
	__asm{
		mov	r0, #0
		mcr	p15, 0, r0, c7, c10, 4	/* drain WB */
		mcr	p15, 0, r0, c8, c7, 0	/* invalidate I & D TLBs */
	}
}

void tlb_invalidate(void)
{
	cpu_arm920_tlb_invalidate_all();
}

void disable_irq(void);

void call_linux(U32 a0, U32 a1, U32 a2)
{
	void (*goto_start)(U32, U32);
	
	rINTMSK=BIT_ALLMSK;
	
	cache_clean_invalidate();
	tlb_invalidate();	

	__asm{
		mov	ip, #0
		mcr	p15, 0, ip, c13, c0, 0	/* zero PID */
		mcr	p15, 0, ip, c7, c7, 0	/* invalidate I,D caches */
		mcr	p15, 0, ip, c7, c10, 4	/* drain write buffer */
		mcr	p15, 0, ip, c8, c7, 0	/* invalidate I,D TLBs */
		mrc	p15, 0, ip, c1, c0, 0	/* get control register */
		bic	ip, ip, #0x0001			/* disable MMU */
		mcr	p15, 0, ip, c1, c0, 0	/* write control register */
	}

	goto_start = (void (*)(U32, U32))a2;
	(*goto_start)(a0, a1);	
}

extern int sprintf(char * /*s*/, const char * /*format*/, ...);

/*
 * pram_base: base address of linux paramter
 */
static void setup_linux_param(ULONG param_base)
{
	struct param_struct *params = (struct param_struct *)param_base; 
	char parameters[512];

	Uart_Printf("Setup linux parameters at 0x%08lx\n", param_base);
	memset(params, 0, sizeof(struct param_struct));

	params->u1.s.page_size = LINUX_PAGE_SIZE;
	params->u1.s.nr_pages = (DRAM_SIZE >> LINUX_PAGE_SHIFT);

	/* set linux command line */
	memset(parameters, 0, sizeof(parameters));
	sprintf(parameters, "noinitrd root=/dev/mtdblock2 init=/linuxrc console=ttySAC0 mem=64MB");

	if (parameters== NULL)
	{
		Uart_Printf("Wrong magic: could not found linux command line\n");
	}
	else
	{
		memcpy(params->commandline, parameters, strlen(parameters) + 1);
		Uart_Printf("Set boot params = %s\n", params->commandline);
	}
}

void boot_Linux(void)
{
	U32 i;
	U32 start_addr;
	ULONG boot_mem_base = 0x30000000;	/* base address of bootable memory */
	unsigned char * to =  (unsigned char * )boot_mem_base + LINUX_KERNEL_OFFSET;
	ULONG mach_type;
	int size;

	/* copy kerne image */
	start_addr = 0x200000;
	size = 0x200000;
	Uart_Printf("Copy linux kernel from 0x%08lx to 0x%08lx, size = 0x%08lx ... ",start_addr, to, size);

	for(i = (start_addr >> 11);size >0;)
	{
		if (CheckBadBlk(i))			//如果遇到坏块，即跳过该块（块地址+0x20）
		{
			Uart_Printf("bad block dwSector = 0x%x\n",i);
			i += 0x40;
			size -= 2048*64;
		}
		ReadPage(i, to);
		i++;
		size -= 2048;
		to += 2048;
	}
/*
	if (*(ULONG *)(to + 9*4) != LINUX_ZIMAGE_MAGIC)
	{
		Uart_Printf("Warning: this binary is not compressed linux kernel image\n");
		Uart_Printf("zImage magic = 0x%08lx\n", *(ULONG *)(to + 9*4));
	}
	else
	{
//		Uart_Printf("zImage magic = 0x%08lx\n", *(ULONG *)(to + 9*4));
		;
	}
*/
	/* Setup linux parameters and linux command line */
	setup_linux_param(boot_mem_base + LINUX_PARAM_OFFSET);

	/* Get machine type */
	mach_type = 168;
//	Uart_Printf("MACH_TYPE = %d\n", mach_type);

	/* Go Go Go */
	Uart_Printf("NOW, Booting Linux......\n");	
	call_linux(0, mach_type, (boot_mem_base + LINUX_KERNEL_OFFSET));
}


//-----------------------------------------------------------
//下面是LCD部分
//-----------------------------------------------------------

/**************************************************************
The initial and control for 16Bpp TFT LCD
**************************************************************/

#include "LCD_TFT.h"

#define M5D(n)				((n) & 0x1fffff)	// To get lower 21bits

extern void Uart_Printf(char *f, ...) ;

// LCD Params

U16 (*frameBuffer16BitTft)[SCR_XSIZE_TFT];

/**************************************************************
LCD视频和控制信号输出或者停止，1开启视频输出
**************************************************************/
void Lcd_EnvidOnOff(int onoff)
{
	if(onoff==1)
		rLCDCON1|=1; // ENVID=ON
	else
		rLCDCON1 =rLCDCON1 & 0x3fffe; // ENVID Off
}

/**************************************************************
TFT LCD 电源控制引脚使能
**************************************************************/
void Lcd_PowerEnable(int invpwren,int pwren)
{
	//GPG4 is setted as LCD_PWREN
	rGPGUP=rGPGUP&(~(1<<4))|(1<<4); // Pull-up disable
	rGPGCON=rGPGCON&(~(3<<8))|(3<<8); //GPG4=LCD_PWREN
	rGPGDAT = rGPGDAT | (1<<4) ;
	//invpwren=pwren;
	//Enable LCD POWER ENABLE Function
	rLCDCON5=rLCDCON5&(~(1<<3))|(pwren<<3);   // PWREN
	rLCDCON5=rLCDCON5&(~(1<<5))|(invpwren<<5);   // INVPWREN
}

/**************************************************************
TFT LCD单个象素的显示数据输出
**************************************************************/
void PutPixel(U32 x,U32 y, U32 c )
{

	if ( (x < LCD_XSIZE_TFT) && (y < LCD_YSIZE_TFT) )
		frameBuffer16BitTft[(y)][(x)] = c;
}

/**************************************************************
TFT LCD全屏填充特定颜色单元或清屏
**************************************************************/
void Lcd_ClearScr( U32 c)
{
	unsigned int x,y ;

	for( y = 0 ; y < LCD_YSIZE_TFT ; y++ )
	{
		for( x = 0 ; x < LCD_XSIZE_TFT ; x++ )
			frameBuffer16BitTft[(y)][(x)] = c;
	}
}

/*******************************************************************
LCD滚动条的显示(在Nand Flash读取时调用)
current当前字节数，total总共的字节数，PBcolor滚动条的颜色
*******************************************************************/
U32 currWidth = 0;
void drawProcessBar(U32 total, U32 current )
{
	U32 const bar_height = 8;

	U32 bar_base = LCD_YSIZE_TFT - bar_height;
	int i = (int) LCD_XSIZE_TFT / 8;

	U32 j;
	int pbcolor ;

	if(total != -1)
	{
		int bar_width = (int) LCD_XSIZE_TFT * ((current * 1.0) / total);
		if (bar_width <= i)
			pbcolor = 0x7FF;
		//sky blue
		else if((bar_width > i) && (bar_width <= i * 2))
			pbcolor = 0x1F;
		//blue
		else if((bar_width > i * 2) && (bar_width <= i * 3))
			pbcolor = 0x0;
		//black
		else if((bar_width > i * 3) && (bar_width <= i * 4))
			pbcolor = 0xF81F;
		//purple
		else if((bar_width > i * 4) && (bar_width <= i * 5))
			pbcolor = 0xFFFF;
		//white
		else if((bar_width > i * 5) && (bar_width <= i * 6))
			pbcolor = 0xF800;
		//red
		else if((bar_width > i * 6) && (bar_width <= i * 7))
			pbcolor = 0xFFE0;
		//yellow
		else if((bar_width > i * 7) && (bar_width <= i * 8))
			pbcolor = 0x7E0;
		//green
		if(bar_width > currWidth)
		{
			for (j = 0; j < bar_height; j++)	
			{
				PutPixel(bar_width, j + bar_base, pbcolor);
			}
			currWidth = bar_width;
		}
		//printk("width= %d, height= %d, bar_base= %d\n",bar_width,j + bar_base,bar_base);
	}
}

/**************************************************************
在LCD屏幕上指定坐标点画一个指定大小的图片
**************************************************************/
void Paint_Bmp(int x0,int y0,int width,int height,unsigned char bmp[])
{
	int x,y;
	U32 c;
	int p = 0;
	
    for( y = y0 ; y < height ; y++ )
    {
    	for( x = x0 ; x < width ; x++ )
    	{
    		c = bmp[p+1] | (bmp[p]<<8) ;

			if ( ( (x0+x) < LCD_XSIZE_TFT) && ( (y0+y) < LCD_YSIZE_TFT) )
				frameBuffer16BitTft[y0+y][x0+x] = c ;
    		p = p + 2 ;
    	}
    }
}

/**************************************************************
LCD初始化
**************************************************************/
void LCD_TFT_Init(void)
{

	rGPCUP  = 0x00000000;
	rGPCCON = 0xaaaa02a9; 
	 
	rGPDUP  = 0x00000000;
	rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]

	rLCDCON1=(CLKVAL_TFT<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD<<24)|(LINEVAL_TFT<<14)|(VFPD<<6)|(VSPW);
	rLCDCON3=(HBPD<<19)|(HOZVAL_TFT<<8)|(HFPD);
	rLCDCON4=(MVAL<<8)|(HSPW);
	rLCDCON5 = (1<<11) | (0<<10) | (1<<9) | (1<<8) | (0<<7) | (0<<6) | (1<<3)  |(BSWP<<1) | (HWSWP);

	frameBuffer16BitTft = (U16 (*)[SCR_XSIZE_TFT])LCDFRAMEBUFFER;
	rLCDSADDR1 = (((U32)LCDFRAMEBUFFER>>22)<<21)|M5D((U32)LCDFRAMEBUFFER>>1);
	rLCDSADDR2 = M5D( ((U32)LCDFRAMEBUFFER+(SCR_XSIZE_TFT*LCD_YSIZE_TFT*2))>>1 );
	rLCDSADDR3 = (((SCR_XSIZE_TFT-LCD_XSIZE_TFT)/1)<<11)|(LCD_XSIZE_TFT/1);

	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
	rTCONSEL &= (~7) ;     // Disable LPC3480
	rTPAL=0; // Disable Temp Palette
	
}

/**************************************************************
LCD显示使能
**************************************************************/
void LCD_Display(void)
{
	//GPG4 is setted as LCD_PWREN
	rGPGUP=rGPGUP&(~(1<<4))|(1<<4); // Pull-up disable
	rGPGCON=rGPGCON&(~(3<<8))|(3<<8); //GPG4=LCD_PWREN
	rGPGDAT = rGPGDAT | (1<<4) ;
	//invpwren=pwren;
	//Enable LCD POWER ENABLE Function
	rLCDCON5=rLCDCON5&(~(1<<3))|(1<<3);   // PWREN
	rLCDCON5=rLCDCON5&(~(1<<5))|(0<<5);   // INVPWREN
	
	rLCDCON1|=1; // ENVID=ON
}

//*************************************************************

