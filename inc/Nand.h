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
#ifndef __NAND_H__
#define __NAND_H__

#define NUM_BLOCKS			0x1000	//  64 MB Smartmedia card.
#define SECTOR_SIZE			512
#define SPARE_SIZE			16
#define PAGES_PER_BLOCK			32

//  For flash chip that is bigger than 32 MB, we need to have 4 step address
//  
#define NFCONF_INIT			0xF830  // 512-byte 4 Step Address
#define NEED_EXT_ADDR			1
//#define NFCONF_INIT			0xA830  // 256-byte 4 Step Address
//#define NEED_EXT_ADDR			0

//#define NFCONF_INIT			0xF840

//  NAND Flash Command. This appears to be generic across all NAND flash chips
#define CMD_READ			0x00	//  Read
#define CMD_READ1			0x01	//  Read1
#define CMD_READ2			0x50	//  Read2
#define CMD_READ3			0x30	//  Read3
#define CMD_READID			0x90	//  ReadID
#define CMD_WRITE1			0x80	//  Write phase 1
#define CMD_WRITE2			0x10	//  Write phase 2
#define CMD_ERASE1			0x60	//  Erase phase 1
#define CMD_ERASE2			0xd0	//  Erase phase 2
#define CMD_STATUS			0x70	//  Status read
#define CMD_RESET			0xff	//  Reset

//  Status bit pattern
#define STATUS_READY			0x40	//  Ready
#define STATUS_ERROR			0x01	//  Error

//  Status bit pattern
#define STATUS_READY			0x40
#define STATUS_ERROR			0x01

#define NF_CMD(cmd)			{rNFCMD  = (cmd); }
#define NF_ADDR(addr)			{rNFADDR = (addr); }	
#define NF_nFCE_L()			{rNFCONT &= ~(1<<1); }
#define NF_nFCE_H()			{rNFCONT |= (1<<1); }
#define NF_RSTECC()			{rNFCONT |= (1<<4); }
#define NF_RDMECC()			(rNFMECC0 )
#define NF_RDSECC()			(rNFSECC )
#define NF_RDDATA()			(rNFDATA)
#define NF_RDDATA8()			(rNFDATA8)
#define NF_WRDATA(data)			{rNFDATA = (data); }
#define NF_WAITRB()			{while(!(rNFSTAT&(1<<0)));} 
#define NF_CLEAR_RB()			{rNFSTAT |= (1<<2); }
#define NF_DETECT_RB()			{while(!(rNFSTAT&(1<<2)));}
#define NF_MECC_UnLock()		{rNFCONT &= ~(1<<5); }
#define NF_MECC_Lock()			{rNFCONT |= (1<<5); }
#define NF_SECC_UnLock()		{rNFCONT &= ~(1<<6); }
#define NF_SECC_Lock()			{rNFCONT |= (1<<6); }

#define	RdNFDat8()			(rNFDATA8)	//byte access
#define	RdNFDat()			RdNFDat8()	//for 8 bit nand flash, use byte access
#define	WrNFDat8(dat)			(rNFDATA8 = (dat))	//byte access
#define	WrNFDat(dat)			WrNFDat8()	//for 8 bit nand flash, use byte access

#define pNFCONF				rNFCONF 
#define pNFCMD				rNFCMD  
#define pNFADDR				rNFADDR 
#define pNFDATA				rNFDATA 
#define pNFSTAT				rNFSTAT 
#define pNFECC				rNFECC0  

#define NF_CE_L()			NF_nFCE_L()
#define NF_CE_H()			NF_nFCE_H()
#define NF_DATA_R()			rNFDATA
#define NF_ECC()			rNFECC0

typedef union _ECCRegVal
{
	DWORD	dwECCVal;
	BYTE	bECCBuf[4];
} ECCRegVal;

// HCLK=100Mhz
#define TACLS				1	// 1-clk(0ns) 
#define TWRPH0				4	// 3-clk(25ns)
#define TWRPH1				0	// 1-clk(10ns)  //TACLS+TWRPH0+TWRPH1>=50ns



typedef DWORD  SECTOR_ADDR;
typedef PDWORD PSECTOR_ADDR;

typedef struct _SectorInfo
{
	DWORD dwReserved1;			// Reserved - used by FAL
	BYTE  bOEMReserved;				// For use by OEM
	BYTE  bBadBlock;					// Indicates if block is BAD
	WORD  wReserved2;				// Reserved - used by FAL
}SectorInfo, *PSectorInfo;

    
#define SECTOR_TO_BLOCK(sector) ((sector) >> 6 )
#define BLOCK_TO_SECTOR(block)  ((block)  << 6 )

//
// ERROR_Xxx
//
#define ERR_SUCCESS						0
#define ERR_DISK_OP_FAIL1				1
#define ERR_DISK_OP_FAIL2				2
#define ERR_INVALID_BOOT_SECTOR		3
#define ERR_INVALID_LOAD_ADDR			4
#define ERR_GEN_FAILURE				5
#define ERR_INVALID_PARAMETER			6
#define ERR_JUMP_FAILED				7
#define ERR_INVALID_TOC					8
#define ERR_INVALID_FILE_TYPE			9

BOOL 
FMD_ReadSector(
    SECTOR_ADDR startSectorAddr, 
    LPBYTE pSectorBuff,
    PSectorInfo pSectorInfoBuff, 
    DWORD dwNumSectors
    );

#ifdef READ_SECTOR_INFO
void 
NAND_ReadSectorInfo(
    SECTOR_ADDR sectorAddr, 
    PSectorInfo pInfo
    );
#endif


void NF_Reset(void);
void NF_Init(void);
char NF_ReadID(void);
int CheckBadBlk(U32 addr);
U32 EraseBlock(U32 addr);
void MarkBadBlk(U32 addr);
void ReadPage(U32 addr, unsigned char * to);
U32 WritePage(U32 addr, unsigned char * buf);

//#define NF_READID   1
#define READ_SECTOR_INFO				//to check out the bad block

#endif /*__NAND_H__*/
