#ifndef _DI_H_
#define _DI_H_

/* Prototypes */
int DIP_StopMotor(void);
int DIP_CustomCommand(u8 *cmd, u8 *ans);
int DIP_ReadDVDVideo(void *dst, u32 len, u32 lba);
int DIP_ReadDVD(void *dst, u32 len, u32 sector);
int DIP_ReadDVDRom(u8 *outbuf, u32 len, u32 offset);

extern u8 * addr_dvd_read_controlling_data;
/*
int DI_Inquiry(void *outbuf);
int DI_Reset(void);
int DI_ResetNotify(void);
int DI_ReadDiskId(void *outbuf, unsigned long len);
int DI_Read(void *outbuf, unsigned long len, unsigned long offset);
int DI_ReadDvd(void *outbuf, unsigned long len, unsigned long lba);
int DI_StopLaser(void);
int DI_Seek(unsigned long offset);
int DI_Offset(unsigned long offset);
int DI_AudioStreaming(unsigned long len);
int DI_GetStatus(unsigned long *outbuf);
*/

#endif

