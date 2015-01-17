/*
 *  Copyright (C) 2010 Spaceman Spiff
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <types.h>

#include <ipc.h>

#include <syscalls.h>

extern int in_ES_ioctlv(void *);
extern int out_ES_ioctlv(void *);

//void ios_sync_before_read(void* ptr, int size);
//void ios_sync_after_write(void* ptr, int size);

static u32 inside_ES_ioctl;

int ES_ioctlv(struct _ipcreq *dat )
{
	int r;
	u32 ios,version;

	inside_ES_ioctl = 1;

	os_sync_before_read(dat, sizeof(struct _ipcreq));

	if(dat->ioctlv.ioctl==8) { // reboot
        	os_sync_before_read( (void *) dat->ioctlv.argv[0].data, dat->ioctlv.argv[0].len);
		ios=*(((volatile u32 *)dat->ioctlv.argv[0].data)+1) ;
		version=1;
		os_sync_before_read((void *) 0x3140,8);
		*((volatile u32 *) 0x3140)= ((ios)<<16) | (version & 65535); // write fake IOS version/revision
		*((volatile u32 *) 0x3188)= ((ios)<<16) | (version & 65535); // write fake IOS version/revision
		os_sync_after_write((void *) 0x3140,4);
		os_sync_after_write((void *) 0x3188,4);
		inside_ES_ioctl = 0;
	        return 0;
	}
	r=out_ES_ioctlv(dat);
	inside_ES_ioctl = 0;
	return r;
}

