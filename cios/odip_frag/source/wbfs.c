/*
 *  Copyright (C) 2010 Spaceman Spiff
 *  Copyright (C) 2010 Hermes
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

#include <ipc.h>

#include <types.h>
#include <utils.h>
#include <syscalls.h>

#define WBFS_BASE (('W'<<24)|('F'<<16)|('S'<<8))
#define USB_IOCTL_WBFS_OPEN_DISC		(WBFS_BASE+0x1)
#define USB_IOCTL_WBFS_READ_DISC		(WBFS_BASE+0x2)
#define USB_IOCTL_WBFS_READ_DIRECT_DISC		(WBFS_BASE+0x3)
#define USB_IOCTL_WBFS_STS_DISC			(WBFS_BASE+0x4)

s32 usb_is_dvd=0;

s32 usb_device_fd=-1;

s32 fat_mode=0;

s32 null_mode=0;

#define USB_DATA_SIZE 64
// Ver atributo packed
static struct _usb_data{
	struct _ioctl ioctlv[4];
	u32 values[8];
} usb_data ATTRIBUTE_ALIGN(32);

static struct _usb_data* usb_data_ptr;

char filename_data[256] ATTRIBUTE_ALIGN(32) ="filename_data";


#define MAX_DEVICES 2
static char *usb_device[] = {"/dev/usb123", 
			     "/dev/sdio/sdhc", 
			     "/dev/usb123"};

s32 usb_dvd_inserted(void)
{
	if (!usb_is_dvd)
		return 1;
	
	if (usb_device_fd < 0)
		return 1;
	
	return os_ioctlv(usb_device_fd, USB_IOCTL_WBFS_STS_DISC, 0,0, usb_data_ptr->ioctlv);
}

extern int ciso_size;
extern u32 *table_lba;

extern u8 mem_index[2048];

s32 usb_read_device(u8 *outbuf, u32 size, u32 lba)
{
	s32 res;
	int l;

	if(null_mode)
		{
		dip_memset(outbuf,0xff, size);
		return 0;
		}

	if (usb_device_fd < 0)
		return -1;
	
// FAT Mode from Hermes
	if(fat_mode)
	{
		if(fat_mode==1 || fat_mode==2)
		{
		u32 lba_glob=0;
	
		if(table_lba==NULL) table_lba= (void *) os_heap_alloc_aligned(0, 2048*4, 32); // from global heap
		
		/* Allocate memory */
		u8 * buff =  dip_alloc_aligned(0x800, 32);
		if (!buff)
			fat_mode=-1;
       
		if(fat_mode>=0)
			{

			lba_glob=(16)<<9;
			
				res=os_seek(usb_device_fd, 0, 0);
				if(res>=0) res=os_read(usb_device_fd, buff, 2048);
				if(res<0) {fat_mode=-1;}
				else
					if(!(buff[0]=='C' && buff[1]=='I' && buff[2]=='S' && buff[3]=='O')) fat_mode=-1;
				
				
				
			}
	 
		if(fat_mode>=0 && table_lba)
			{

			ciso_size=(((u32)buff[4])+(((u32)buff[5])<<8)+(((u32)buff[6])<<16)+(((u32)buff[7])<<24))/4;
		
			dip_memset(mem_index,0,2048);


			for(l=0;l<16384;l++)
				{
				if(((l+8) & 2047)==0 && (l+8)>=2048) 
					{
					if(fat_mode==2) res=os_seek(usb_device_fd, (((l+8)>>11))<<9, 0);
					else
						res=os_seek(usb_device_fd, (((l+8)>>11))<<11, 0); // read 16 cached sectors

					if(res>=0) res=os_read(usb_device_fd, buff, 2048);
					if(res<0) {fat_mode=-1;break;}
					}

				if((l & 7)==0) table_lba[l>>3]=lba_glob;
				
				if(buff[(8+l) & 2047])
					{
					mem_index[l>>3]|=1<<(l & 7);
					lba_glob+=ciso_size;
					}
				}

			if(fat_mode>=0) 
				{
				fat_mode|=4;
				
				}
			
				
			}
		
		/* Free memory */
		if (buff)
			dip_free(buff);
		}

	if(fat_mode>=4)
		{
		u32 temp=(lba)/ciso_size;
		

		u32 read_lba=table_lba[temp>>3];

		for(l=0;l<(temp & 7);l++) if((mem_index[temp>>3]>>l) & 1) read_lba+=ciso_size;

		read_lba+=(lba) & ((ciso_size)-1);
		
		if(fat_mode & 2) os_seek(usb_device_fd, read_lba, 0);
		else
			os_seek(usb_device_fd, read_lba<<2, 0); // read sectors

		res=os_read(usb_device_fd, outbuf, size);
		//if(r<0) return r;
		return 0;
		

		}
	else
		{
		
		/* Free memory */
		if (table_lba)
			os_heap_free(0, table_lba);

		table_lba=NULL;

		return -1;

		}

	}

	usb_data_ptr->values[0] = lba;
	usb_data_ptr->values[7] = size;

	usb_data_ptr->ioctlv[2].data = outbuf;
	usb_data_ptr->ioctlv[3].data = (u32 *) size;  // No es necesario, pero estaba en el codigo original

	os_sync_after_write(usb_data_ptr, USB_DATA_SIZE);
	os_sync_after_write(outbuf, size);

	if (usb_is_dvd) 
		res = os_ioctlv(usb_device_fd, USB_IOCTL_WBFS_READ_DIRECT_DISC, 2, 1, usb_data_ptr->ioctlv);
	else
		res = os_ioctlv(usb_device_fd, USB_IOCTL_WBFS_READ_DISC, 2, 1, usb_data_ptr->ioctlv);
		
	os_sync_before_read(outbuf, size);

	return res;
}


s32 usb_open_device(u32 device_nr, u8 *id, u32 partition)
{
	u32 res;
	u32 part = partition;	
 
	if (device_nr >= MAX_DEVICES)
		return -1;
	
	if(usb_device_fd==0x666999) usb_device_fd=-1;

	if (usb_data_ptr == 0) {
		usb_data_ptr = &usb_data;
		usb_device_fd = -1;
	} else 	if (usb_device_fd>=0)
		os_close(usb_device_fd);
	usb_device_fd=-1;

	usb_is_dvd = 0; fat_mode=0;


	if((id[0]=='_' && id[1]=='N' && id[2]=='U' && id[3]=='L'))
	{
		fat_mode=1;null_mode=1;usb_device_fd=0x666999;

	return 0;

	}

	if((id[0]=='_' && id[1]=='D' && id[2]=='E' && id[3]=='V'))
	{
		fat_mode=1;
		if(id[4]=='W') fat_mode=2; // change the seek operations of the device from bytes to words
       
		os_sync_before_read((void *) filename_data, 256);

		usb_device_fd = (int) os_open((void *) filename_data, 0);
		if(usb_device_fd<0) {fat_mode=0;return usb_device_fd;}

	return 0;

	}

	if (id[0] == '_' && id[1] == 'D' && id[2] == 'V' && id[3] == 'D') 
		usb_is_dvd = 1;
	
	usb_device_fd = os_open(usb_device[device_nr] , 1);
	if (usb_device_fd < 0) {
		if (device_nr == 0)
			usb_device_fd = os_open(usb_device[0], 1);
	}

	if (usb_device_fd < 0)
		return usb_device_fd;

	dip_memcpy((void *)(usb_data_ptr->values), id, 6);
	dip_memcpy((void *) &(usb_data_ptr->values[7]), (u8 *) &part, sizeof(part));

	usb_data_ptr->ioctlv[0].data = (void *) (usb_data_ptr->values);
	usb_data_ptr->ioctlv[0].len = 6;
	usb_data_ptr->ioctlv[1].data = (void *) &(usb_data_ptr->values[7]);
	usb_data_ptr->ioctlv[1].len = sizeof(u32);

	os_sync_after_write(usb_data_ptr, USB_DATA_SIZE);
	res = os_ioctlv(usb_device_fd, USB_IOCTL_WBFS_OPEN_DISC, 2, 0, usb_data_ptr->ioctlv);

	usb_data_ptr->ioctlv[0].data = (void *) (usb_data_ptr->values);
	usb_data_ptr->ioctlv[0].len = sizeof(u32);
	usb_data_ptr->ioctlv[1].data = (void *) &(usb_data_ptr->values[7]);
	usb_data_ptr->ioctlv[1].len = sizeof(u32);
	
	os_sync_after_write(usb_data_ptr, USB_DATA_SIZE);

	return res;
}

