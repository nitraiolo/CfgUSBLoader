#ifndef __WBFS_H__
#define __WBFS_H__

#include "types.h"

s32 usb_dvd_inserted(void);
s32 usb_read_device(u8 *outbuf, u32 size, u32 lba);
s32 usb_open_device(u32 device_nr, u8 *id, u32 partition);

extern s32 usb_is_dvd;

extern s32 usb_device_fd;

#endif
