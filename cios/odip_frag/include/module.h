#ifndef _MODULE_H_
#define _MODULE_H_

/* Error codes */
#define ERR_EINVAL		 -1
#define ERR_ENOENT		 -6
#define ERR_ENOMEM		-22
#define ERR_EIO			  2
#define ERR_EINCMD		128

/* IOS calls */
#define IOS_OPEN		0x01
#define IOS_CLOSE		0x02
#define IOS_READ		0x03
#define IOS_WRITE		0x04
#define IOS_SEEK		0x05
#define IOS_IOCTL		0x06
#define IOS_IOCTLV		0x07

/* IOCTL calls */
#define IOCTL_DI_INQUIRY	0x12
#define IOCTL_DI_READID		0x70
#define IOCTL_DI_READ		0x71
#define IOCTL_DI_WAITCOVERCLOSE	0x79
#define IOCTL_DI_RESETNOTIFY	0x7E
#define IOCTL_DI_GETCOVER	0x88
#define IOCTL_DI_RESET		0x8A
#define IOCTL_DI_OPENPART       0x8B
#define IOCTL_DI_CLOSEPART      0x8C
#define IOCTL_DI_UNENCREAD	0x8D
#define IOCTL_DI_LOWREAD	0xA8
#define IOCTL_DI_SEEK		0xAB
#define IOCTL_DI_REPORTKEY      0xA4
#define IOCTL_DI_READDVD	0xD0
#define IOCTL_DI_STOPLASER	0xD2
#define IOCTL_DI_OFFSET		0xD9
#define IOCTL_DI_DISC_BCA       0xDA
#define IOCTL_DI_REQERROR	0xE0
#define IOCTL_DI_STOPMOTOR	0xE3
#define IOCTL_DI_STREAMING	0xE4

#define IOCTL_DI_SETBASE        0xF0
#define IOCTL_DI_GETBASE        0xF1
#define IOCTL_DI_SETFORCEREAD   0xF2
#define IOCTL_DI_GETFORCEREAD   0xF3
#define IOCTL_DI_SETUSBMODE     0xF4
#define IOCTL_DI_GETUSBMODE     0xF5
#define IOCTL_DI_DISABLERESET   0xF6
#define IOCTL_DI_CUSTOMCMD	0xFF

#define IOCTL_DI_FRAG_SET		0xF9
#define IOCTL_DI_MODE_GET		0xFA
#define IOCTL_DI_HELLO			0xFB

#define IOCTL_DI_13             0x13
#define IOCTL_DI_14             0x14
#define IOCTL_DI_15             0x15

/* Constants */
#define SECTOR_SIZE		0x800


/* DIP struct */
typedef struct {
	/* DIP config */
	u32 low_read_from_device;
	u32 dvdrom_mode;
	u32 base;
	u32 offset;
	u32 has_id;
	u32 partition;
	u8 id[8];
	u32 currentError; // offset 0x20
	u8 disableReset;
	u8 reading;
	u8 frag_mode;
} __attribute__((packed)) dipstruct;

extern dipstruct dip;

/* Call original ioctl command */
int handleDiCommand(u32 *inbuf, u32 *outbuf, u32 outbuf_size);

#endif
