#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>

#include "cfg.h"

/* Constants */
#define IOCTL_DI_READID		0x70
#define IOCTL_DI_READ		0x71
#define IOCTL_DI_WAITCVRCLOSE	0x79
#define IOCTL_DI_GETCOVER	0x88
#define IOCTL_DI_RESET		0x8A
#define IOCTL_DI_OPENPART	0x8B
#define IOCTL_DI_CLOSEPART	0x8C
#define IOCTL_DI_UNENCREAD	0x8D
#define IOCTL_DI_SEEK		0xAB
#define IOCTL_DI_STOPLASER	0xD2
#define IOCTL_DI_OFFSET		0xD9
#define IOCTL_DI_DISC_BCA	0xDA
#define IOCTL_DI_STOPMOTOR	0xE3
#define IOCTL_DI_SETWBFSMODE	0xF4
#define IOCTL_DI_GETWBFSMODE	0xF5 // odip
#define IOCTL_DI_DISABLERESET   0xF6 // odip
#define IOCTL_DI_DVDLowAudioBufferConfig 0xE4

// YAL / CIOS 222 DI
#define YAL_DI_OPENPARTITION IOCTL_DI_OPENPART
#define YAL_DI_SETOFFSETBASE 0xf1
#define YAL_DI_SETWBFSMODE   0xfe

#define IOCTL_DI_SETFRAG	0xF9
#define IOCTL_DI_GETMODE	0xFA
#define IOCTL_DI_HELLO		0xFB

/* Variables */
static u32 inbuf[8]  ATTRIBUTE_ALIGN(32);
static u32 outbuf[8] ATTRIBUTE_ALIGN(32);

static const char di_fs[] ATTRIBUTE_ALIGN(32) = "/dev/di";
static s32 di_fd = -1;

s32 WDVD_setstreaming(void)
{
	u8 ioctl;
	ioctl = IOCTL_DI_DVDLowAudioBufferConfig;

	memset(inbuf, 0, 0x20);
	memset(outbuf, 0, 0x20);

	inbuf[0] = (ioctl << 24);

	if ( (*(u32*)0x80000008)>>24 )
	{
		inbuf[1] = 1;
		if( ((*(u32*)0x80000008)>>16) & 0xFF )
		{
			inbuf[2] = 10;
		} else 
		{
			inbuf[2] = 0;
		}
	}
	else
	{		
		inbuf[1] = 0;
		inbuf[2] = 0;
	}			
	DCFlushRange(inbuf, 0x20);
	
	int Ret = IOS_Ioctl(di_fd, ioctl, inbuf, 0x20, outbuf, 0x20);

	return ((Ret == 1) ? 0 : -Ret);
}

s32 WDVD_Init(void)
{
	/* Open "/dev/di" */
	if (di_fd < 0) {
		di_fd = IOS_Open(di_fs, 0);
		if (di_fd < 0)
			return di_fd;
	}

	return 0;
}

s32 WDVD_Close(void)
{
	/* Close "/dev/di" */
	if (di_fd >= 0) {
		IOS_Close(di_fd);
		di_fd = -1;
	}

	return 0;
}

s32 WDVD_GetHandle(void)
{
	/* Return di handle */
	return di_fd;
}

s32 WDVD_Reset(void)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Reset drive */
	inbuf[0] = IOCTL_DI_RESET << 24;
	inbuf[1] = 1;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_RESET, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_ReadDiskId(void *id)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Read disc ID */
	inbuf[0] = IOCTL_DI_READID << 24;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_READID, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	if (ret == 1) {
		memcpy(id, outbuf, sizeof(dvddiskid));
		return 0;
	}

	return -ret;
}

s32 WDVD_Seek(u64 offset)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Drive seek */
	inbuf[0] = IOCTL_DI_SEEK << 24;
	inbuf[1] = (u32)(offset >> 2);

	ret = IOS_Ioctl(di_fd, IOCTL_DI_SEEK, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;

}

s32 WDVD_Offset(u64 offset)
{
	//u32 *off = (u32 *)((void *)&offset);
	union { u64 off64; u32 off32[2]; } off;
	off.off64 = offset;
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Set offset */
	inbuf[0] = IOCTL_DI_OFFSET << 24;
	inbuf[1] = (off.off32[0]) ? 1: 0;
	inbuf[2] = (off.off32[1] >> 2);
	// hmm this might be better:
	//off.off64 = offset >> 2;
	//inbuf[1] = off.off32[0];
	//inbuf[2] = off.off32[1];

	ret = IOS_Ioctl(di_fd, IOCTL_DI_OFFSET, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_StopLaser(void)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Stop laser */
	inbuf[0] = IOCTL_DI_STOPLASER << 24;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_STOPLASER, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_StopMotor(void)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Stop motor */
	inbuf[0] = IOCTL_DI_STOPMOTOR << 24;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_STOPMOTOR, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_Eject(void)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Stop motor */
	inbuf[0] = IOCTL_DI_STOPMOTOR << 24;
	/* Eject DVD */
	inbuf[1] = 1;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_STOPMOTOR, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_OpenPartition(u64 offset, u8 *tmd)
{
	u8 *vector = NULL;

	u32 *buffer = NULL;
	s32 ret;

	/* Allocate memory */
	buffer = (u32 *)memalign(32, 0x5000);
	if (!buffer)
		return -1;

	/* Set vector pointer */
	vector = (u8 *)buffer;

	memset(buffer, 0, 0x5000);

	/* Open partition */
	buffer[0] = (u32)(buffer + 0x10);
	buffer[1] = 0x20;
	buffer[3] = 0x2a4;
	buffer[6] = (u32)tmd;
	buffer[7] = 0x49e4;
	buffer[8] = (u32)(buffer + 0x360);
	buffer[9] = 0x20;

	buffer[(0x40 >> 2)]     = IOCTL_DI_OPENPART << 24;
	buffer[(0x40 >> 2) + 1] = offset >> 2;

	ret = IOS_Ioctlv(di_fd, IOCTL_DI_OPENPART, 3, 2, (ioctlv *)vector);

	/* Free memory */
	free(buffer);

	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_ClosePartition(void)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Close partition */
	inbuf[0] = IOCTL_DI_CLOSEPART << 24;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_CLOSEPART, inbuf, sizeof(inbuf), NULL, 0);
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_UnencryptedRead(void *buf, u32 len, u64 offset)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Unencrypted read */
	inbuf[0] = IOCTL_DI_UNENCREAD << 24;
	inbuf[1] = len;
	inbuf[2] = (u32)(offset >> 2);

	//printf("UNCR-: %p", buf);
	//hex_dump2(buf, 8);
	ret = IOS_Ioctl(di_fd, IOCTL_DI_UNENCREAD, inbuf, sizeof(inbuf), buf, len);
	//printf("=%d : %08x", ret, *((int*)buf));
	//hex_dump2(buf, 8);
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_Read(void *buf, u32 len, u64 offset)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Disc read */
	inbuf[0] = IOCTL_DI_READ << 24;
	inbuf[1] = len;
	inbuf[2] = (u32)(offset >> 2);

	ret = IOS_Ioctl(di_fd, IOCTL_DI_READ, inbuf, sizeof(inbuf), buf, len);
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_WaitForDisc(void)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Wait for disc */
	inbuf[0] = IOCTL_DI_WAITCVRCLOSE << 24;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_WAITCVRCLOSE, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_GetCoverStatus(u32 *status)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Get cover status */
	inbuf[0] = IOCTL_DI_GETCOVER << 24;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_GETCOVER, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	if (ret == 1) {
		/* Copy cover status */
		memcpy(status, outbuf, sizeof(u32));

		return 0;
	}

	return -ret;
}

s32 WDVD_SetWBFSMode(u32 mode, u8 *discid)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Set USB mode */
	inbuf[0] = IOCTL_DI_SETWBFSMODE << 24;
	inbuf[1] = mode;

	/* Copy disc ID */
	if (discid)
		memcpy(&inbuf[2], discid, 6);

	ret = IOS_Ioctl(di_fd, IOCTL_DI_SETWBFSMODE, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_Read_Disc_BCA(void *buf)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Disc read */
	inbuf[0] = IOCTL_DI_DISC_BCA << 24;
	//inbuf[1] = 64;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_DISC_BCA, inbuf, sizeof(inbuf), buf, 64);
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

// frag

s32 WDVD_SetFragList(int device, void *fraglist, int size)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	/* Set FRAG mode */
	inbuf[0] = IOCTL_DI_SETFRAG << 24;
	inbuf[1] = device;
	inbuf[2] = (u32)fraglist;
	inbuf[3] = size;

	dbg_printf("SetFragList %d %p %d\n", device, fraglist, size);

	DCFlushRange(fraglist, size);
	ret = IOS_Ioctl(di_fd, IOCTL_DI_SETFRAG, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	dbg_printf(" = %d %d %d\n", ret, outbuf[0], outbuf[1]);

	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

s32 WDVD_hello(u32 *status)
{
	s32 ret;

	memset(inbuf, 0, sizeof(inbuf));

	inbuf[0] = IOCTL_DI_HELLO << 24;

	ret = IOS_Ioctl(di_fd, IOCTL_DI_HELLO, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if (ret < 0)
		return ret;

	if (ret == 1) {
		if (status) memcpy(status, outbuf, sizeof(u32));
		hex_dump2(outbuf, 12);
		return 0;
	}

	return -ret;
}


// YAL / CIOS 222 DI



#define ALIGNED(x) __attribute__((aligned(x)))
static unsigned int Command[8] ALIGNED(32);
static unsigned int Output[8] ALIGNED(32);
#define Device_Handle di_fd

/*******************************************************************************
 * Set_OffsetBase: Sets the base of read commands
 * -----------------------------------------------------------------------------
 * Return Values:
 *	returns result of Ioctl command
 *
 ******************************************************************************/

int YAL_Set_OffsetBase(u64 Base)
{
	memset(Command, 0, 0x20);
	Command[0] = YAL_DI_SETOFFSETBASE << 24;
	Command[1] = Base >> 2;


	int Ret = IOS_Ioctl(Device_Handle, YAL_DI_SETOFFSETBASE, Command, 0x20, Output, 0x20);

	return ((Ret == 1) ? 0 : -Ret);
}


/*******************************************************************************
 * Open_Partition: Opens a partition for reading
 * -----------------------------------------------------------------------------
 * Return Values:
 *	returns result of Ioctl command
 *
 ******************************************************************************/

int YAL_Open_Partition(unsigned int Offset, void* Ticket,
		void* Certificate, unsigned int Cert_Len, void* Out)
{
	static ioctlv	Vectors[5]		__attribute__((aligned(0x20)));

	Command[0] = YAL_DI_OPENPARTITION << 24;
	Command[1] = Offset;

	Vectors[0].data		= Command;
	Vectors[0].len		= 0x20;
	Vectors[1].data		= (Ticket == NULL) ? 0 : Ticket;
	Vectors[1].len		= (Ticket == NULL) ? 0 : 0x2a4;
	Vectors[2].data		= (Certificate == NULL) ? 0 : Certificate;
	Vectors[2].len		= (Certificate == NULL) ? 0 : Cert_Len;
	Vectors[3].data		= Out;
	Vectors[3].len		= 0x49e4;
	Vectors[4].data		= Output;
	Vectors[4].len		= 0x20;

	int Ret = IOS_Ioctlv(Device_Handle, YAL_DI_OPENPARTITION, 3, 2, Vectors);

	return ((Ret == 1) ? 0 : -Ret);
}

/*******************************************************************************
 * Set_USB_LBA: allow to select different discs into HD
 * -----------------------------------------------------------------------------
 * Return Values:
 *	returns result of Ioctl command
 *
 ******************************************************************************/
int YAL_Enable_WBFS(void*discid)
{
	memset(Command, 0, 0x20);
	Command[0] = YAL_DI_SETWBFSMODE << 24;
	Command[1] = discid ? 1 : 0;
	if (discid) memcpy(&Command[2],discid,6);

	int Ret = IOS_Ioctl(Device_Handle, YAL_DI_SETWBFSMODE, Command, 0x20, Output, 0x20);

	return ((Ret == 1) ? 0 : -Ret);
}


// MLOAD

// mode == device
s32 MLOAD_SetWBFSMode2(u32 mode, u8 *id, s32 partition)
{
	s32 ret;

	//printf("mload wbfs(%.6s,%d)\n", id, partition); sleep(1);

	memset(inbuf, 0, sizeof(inbuf));

	/* Set USB mode */
	inbuf[0] = IOCTL_DI_SETWBFSMODE << 24;
	inbuf[1] = (id) ? mode : 0;
	

	/* Copy ID */
	if (id)
		{
		memcpy(&inbuf[2], id, 6);
		inbuf[5] = partition;
		}

	//ret = IOS_Ioctl(di_fd, IOCTL_DI_SETUSBMODE, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	ret = IOS_Ioctl(di_fd, IOCTL_DI_SETWBFSMODE, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));
	if(ret!=1)
		{ // Try old cIOS 222
		/* Set USB mode */
		inbuf[0] = YAL_DI_SETWBFSMODE << 24;
		ret = IOS_Ioctl(di_fd, YAL_DI_SETWBFSMODE, inbuf, sizeof(inbuf), outbuf, sizeof(outbuf));	
		}

	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -ret;
}

