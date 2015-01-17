#ifndef _WDVD_H_
#define _WDVD_H_

/* Prototypes */
s32 WDVD_Init(void);
s32 WDVD_Close(void);
s32 WDVD_GetHandle(void);
s32 WDVD_Reset(void);
s32 WDVD_ReadDiskId(void *);
s32 WDVD_Seek(u64);
s32 WDVD_Offset(u64);
s32 WDVD_StopLaser(void);
s32 WDVD_StopMotor(void);
s32 WDVD_OpenPartition(u64, u8 *tmd);
s32 WDVD_ClosePartition(void);
s32 WDVD_UnencryptedRead(void *, u32, u64);
s32 WDVD_Read(void *, u32, u64);
s32 WDVD_WaitForDisc(void);
s32 WDVD_GetCoverStatus(u32 *);
s32 WDVD_SetWBFSMode(u32, u8 *);
s32 WDVD_Eject(void);
s32 WDVD_Read_Disc_BCA(void *buf);
s32 WDVD_SetFragList(int device, void *fraglist, int size);
s32 WDVD_hello(u32 *status);
s32 WDVD_setstreaming(void);

// yal
int YAL_Set_OffsetBase(unsigned int Base);
int YAL_Open_Partition(unsigned int Offset, void* Ticket,
		void* Certificate, unsigned int Cert_Len, void* Out);
int YAL_Enable_WBFS(void*discid);

// mload
s32 MLOAD_SetWBFSMode2(u32 mode, u8 *id, s32 partition);

#endif

