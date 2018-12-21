/*------------------------------------------------------------------------
* Xradio
* Reproduction and Communication of this document is strictly prohibited
* unless specifically authorized in writing by Xradio
*-------------------------------------------------------------------------
* Public header file between host application, oem_api.dll and firmware
*-----------------------------------------------------------------------*/

/*

OEM_API_VER - 0x0001
	a) Initial

*/

#ifndef _OEM_API_H_
#define _OEM_API_H_

#define OEM_API_VER                  0x0101

#pragma once

#define LINKAGE	__declspec(dllexport)

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************
*                         efuse use                         *
************************************************************/

	//Function:	Get last status message	 
	//Param:	no
	//Return:	pointer of message string
	LINKAGE char* OEM_GetMsg();
	LINKAGE int OEM_SetHashKey(char* buf, int len);

	//Function:	Test encode data	 
	//Param:	encodeKey:	TRUE - encode key buf only
	//						FALSE - read encode result of last time
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_EncodeTest(char* buf, int len, BOOL encodeKey);

	//Function:	Init UART and key buffer
	//Param:	comNum: UART COM port
	//			keyBuf: buffer of encode key
	//			len: length of encode key
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_InitOem(int comNum, char* keyBuf, int len);
	LINKAGE int OEM_ExitOem();

	//Function:	Read/Write hosc
	//Param:	hosc: 0 - 26M   1 - 40M   2 - 24M   3 - 52M   -1 - invalid
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_WriteHoscType(int hosc);
	LINKAGE int OEM_ReadHoscType(int* hosc);

	//Function:	Read/Write secure boot
	//Param:	hash: hash key buffer poiter
	//			Read len: equal or more than 32
	//			Write len: 32 only
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_WriteScrBoot(char* hash, int len);
	LINKAGE int OEM_ReadScrBoot(char* hash, int len);

	//Function:	Read/Write DCXO trim
	//Param:	value: DCXO value
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_WriteDcxoTrim(char value);
	LINKAGE int OEM_ReadDcxoTrim(char* value);

	//Function:	Read/Write POUT CAL
	//Param:	rfCal: POUT CAL buffer
	//			Read len: equal or more than 3
	//			Write len: 3 only
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_WritePoutCal(char* rfCal, int len);
	LINKAGE int OEM_ReadPoutCal(char* rfCal, int len);

	//Function:	Read/Write MAC address
	//Param:	mac: MAC address buffer
	//			Read len: equal or more than 6
	//			Write len: 6 only
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_WriteMacAddr(char* mac, int len);
	LINKAGE int OEM_ReadMacAddr(char* mac, int len);

	//Function:	Read/Write Chip ID
	//Param:	chipId: Chip ID buffer
	//			Read len: equal or more than 16
	//			Write len: 16 only
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_WriteChipId(char* chipId, int len);
	LINKAGE int OEM_ReadChipId(char* chipId, int len);

	//Function:	Read/Write user area data
	//Param:	data: data buffer
	//			Read startAddr: 0 to 600
	//			Write startAddr: 0 to 600
	//			Read len: 1 to 601
	//			Write len: 1 to 601
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_WriteUserArea(char* data, int startAddr, int len);
	LINKAGE int OEM_ReadUserArea(char* data, int startAddr, int len);

	//Function:	Read/Write DCXO trim0
	//Param:	value: DCXO trim0 value
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_WriteDcxoTrim0(char value);
	LINKAGE int OEM_ReadDcxoTrim0(char* value);

/************************************************************
*                         flash use                         *
************************************************************/
	//Function:	Init UART and flash controler
	//Param:	comNum: UART COM port
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_InitFlash(int comNum);
	LINKAGE int OEM_ExitFlash();

	//Function:	Read/Write/erase flash
	//Param:	buf: data buffer
	//			address: address to be read/write/erase
	//			len: read/write data length
	//			erase_size: block size to be erased
	//			1 - chip	2 - 64kb	3 - 32kb	4 - 4kb		else - invalid
	//Return:	0 - Success		1 - Failed
	LINKAGE int OEM_ReadFlash(char* buf, int address, int len);
	LINKAGE int OEM_WriteFlash(char* buf, int address, int len);
	LINKAGE int OEM_EraseFlash(int address, int erase_size);


#ifdef __cplusplus
}
#endif

#endif // #ifndef _OEM_API_H_