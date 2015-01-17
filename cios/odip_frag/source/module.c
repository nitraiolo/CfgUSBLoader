/*
 *  Copyright (C) 2010 Spaceman Spiff
 *  Copyright (C) 2010 Hermes
 *  Copyright (C) 2010 Waninkoko
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
#include <utils.h>
#include <module.h>
#include <syscalls.h>

#include <wbfs.h>
#include <di.h>
#include "frag.h"

#include <debug.h>

dipstruct dip= {0};

#define WII_DVD_SIGNATURE 0x5D1C9EA3

#define READINFO_SIZE_DATA 32

#define BCADATA_SIZE 64
const u8 bca_bytes[BCADATA_SIZE] ATTRIBUTE_ALIGN(32) = { 
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


void dummy_function(u32 *outbuf, u32 outbuf_size)
{
}


// From Waninkoko dip_plugin

/* Return codes */
#define DIP_EIO 0xA000
 
/* Disc lengths */
#define DVD5_LENGTH 0x46090000
#define DVD9_LENGTH 0x7ED38000
 
/* Error codes */
#define ERROR_BLOCK_RANGE 0x52100
#define ERROR_WRONG_DISC 0x53100

/* Disc types */
enum {
DISC_UNKNOWN = 0,
DISC_DVD5,
DISC_DVD9,
};
 
int dvd_type=0;

s32 __DI_CheckOffset(u32 offset)
{
u32 offmax;
 
/* Check disc type */
	switch (dvd_type) {
	/* Single layer */
	case DISC_DVD5:
	offmax = DVD5_LENGTH;
	break;
	 
	/* Dual layer */
	case DISC_DVD9:
	offmax = DVD9_LENGTH;
	break;
	 
	default:
	return 0;
	}
	 
	/* Check offset */
	if (offset >= offmax) {
	/* Set error */
	dip.currentError = ERROR_BLOCK_RANGE;
	 
	/* I/O error */
	return DIP_EIO;
	}
 
return 0;
}



// CISO mem area
int ciso_lba=-1;
int ciso_size=0;
u32 *table_lba=NULL;

extern s32 fat_mode;

u8 mem_index[2048] __attribute__ ((aligned (32)));

int read_from_device_out(void *outbuf, u32 size, u32 lba)
{

	if (dip.frag_mode) 
		return Frag_Read((u8 *)outbuf, size, lba);

	if (dip.has_id) 
		return usb_read_device((u8 *)outbuf, size, lba);

	if (dip.dvdrom_mode)
		return DIP_ReadDVDRom((u8 *) outbuf, size, lba);
	
	return DIP_ReadDVD((u8 *) outbuf, size, lba);
}

int read_from_device(void *outbuf, u32 size, u32 lba)
{
int r=-1;
int l;

 r = __DI_CheckOffset(lba);
 if (r) return r;

 r=-1;

	lba += dip.base + dip.offset;

	if (dip.has_id) 
		{
			/* Free memory */

		if(fat_mode<=0)
			{
			if (table_lba)
				os_heap_free(0, table_lba);
			table_lba=NULL;

			ciso_lba=-1;
			
			}
			
			return  read_from_device_out(outbuf, size, lba);
		}
	
	if(ciso_lba>=0 && ciso_lba!=0x7fffffff)
		{
		u32 lba_glob=0;
	
		
		if(table_lba==NULL) table_lba= (void *) os_heap_alloc_aligned(0, 2048*4, 32); // from global heap
		//ciso_lba=265;
		/* Allocate memory */
		u8 * buff =  dip_alloc_aligned(0x800, 32);
		if (!buff)
			ciso_lba=-1;

		if(ciso_lba>=0)
			{

			while(1)
				{
				lba_glob=(ciso_lba+16)<<9;
			
				
				r=read_from_device_out(buff, 2048, ciso_lba<<9); // read 16 cached sectors
				if(r<0) {ciso_lba=-1;break;}

				if((buff[0]=='C' && buff[1]=='I' && buff[2]=='S' && buff[3]=='O')) break;
				else
					{
					if(ciso_lba!=0) {ciso_lba=0;continue;}
					ciso_lba=-1;
					}
				
				break;
				}
			}
	   // if(ciso_lba>=0 && table_lba && buff) ciso_lba=0x7fffffff;
	  
		if(ciso_lba>=0 && table_lba)
			{

			ciso_size=(((u32)buff[4])+(((u32)buff[5])<<8)+(((u32)buff[6])<<16)+(((u32)buff[7])<<24))/4;
		
			dip_memset(mem_index,0,2048);


			for(l=0;l<16384;l++)
				{
				if(((l+8) & 2047)==0 && (l+8)>=2048) 
					{
					r=read_from_device_out(buff, 2048, (ciso_lba+((l+8)>>11))<<9); // read 16 cached sectors
					if(r<0) {ciso_lba=-1;break;}
					}

				if((l & 7)==0) table_lba[l>>3]=lba_glob;
				
				if(buff[(8+l) & 2047])
					{
					mem_index[l>>3]|=1<<(l & 7);
					lba_glob+=ciso_size;
					}
				}

			if(ciso_lba>=0) ciso_lba=0x7fffffff;
			}
		
		/* Free memory */
		if (buff)
			dip_free(buff);
		}


	if(ciso_lba==0x7fffffff)
		{
		u32 temp=(lba)/ciso_size;
		

		u32 read_lba=table_lba[temp>>3];

		for(l=0;l<(temp & 7);l++) if((mem_index[temp>>3]>>l) & 1) read_lba+=ciso_size;

		read_lba+=(lba) & ((ciso_size)-1);

		
		r=read_from_device_out(outbuf,  size, read_lba); 
		if(r<0) return r;
		

		}
	else
		{
		/* Free memory */
		if (table_lba)
			os_heap_free(0, table_lba);

		table_lba=NULL;

		return read_from_device_out(outbuf, size, lba);

		}


return r;


}

// From Waninkoko dip_plugin

void __DI_CheckDisc(void)
{
void *buffer;
s32 ret;
 
	/* Allocate buffer */
	buffer = (void *) os_heap_alloc_aligned(0, SECTOR_SIZE, 32); 
	if (!buffer)
	return;
	 
	/* Read second layer */
	ret = read_from_device(buffer, SECTOR_SIZE, 0x47000000);
	 
	/* Set disc type */
	dvd_type = (!ret) ? DISC_DVD9 : DISC_DVD5;
	 
	/* Free buffer */
	os_heap_free(0, buffer);

}
int read_id_from_image(u32 *outbuf, u32 outbuf_size)
{
	u32 res;

	res= read_from_device(outbuf, READINFO_SIZE_DATA, 0);
	if(res<0) return res;

	if (outbuf[6] == WII_DVD_SIGNATURE) {
		
		extern u8 * addr_dvd_read_controlling_data;
		
		addr_dvd_read_controlling_data[0] = 1;

		if (!addr_dvd_read_controlling_data[1])
			dip_doReadHashEncryptedState();
	}
	return res;
}


/*
Ioctl 0x13 -> usada por la función Disc_USB_DVD_Wait(), checkea si hay disco montado desde la unidad DVD. Solo se usa en uLoader

Ioctl 0x14 -> equivale a Ioctl 0x88, pero exceptuando el uso de DVD, devuelve un estado fake para WBFS y DVD USB

Ioctl 0x15 -> equivale a Ioctl 0x7a y devuelve el registro tal cual, exceptuando el bit 0 (para indicar la presencia del disco siempre)
 */

static int _noinit=1;

extern void * mem_bss;
extern int mem_bss_len;

int DI_EmulateCmd(u32 *inbuf, u32 *outbuf, u32 outbuf_size)
{
	int res = 0;

	u8 cmd = ((u8 *) inbuf)[0];

#ifdef DEBUG
	s_printf("DIP::DI_EmulateCmd(%x, %x, %x)\n", cmd, outbuf, outbuf_size);
#endif

	if(_noinit)
		{
		
		dip_memset(mem_bss, 0, mem_bss_len);

		os_sync_after_write(mem_bss, mem_bss_len);

		_noinit=0;
		}


	if (cmd != IOCTL_DI_REQERROR)
		dip.currentError = 0;

	switch (cmd) {
			case IOCTL_DI_REQERROR:
				 {
					
				 if (dip.currentError || dip.has_id)
					 {
					 dip_memset((u8 *) outbuf, 0, outbuf_size);
					 outbuf[0] = dip.currentError;
					 os_sync_after_write(outbuf, outbuf_size);
					 res = 0;
					 }
				else goto call_original_di;
				}
				
				break;

			case IOCTL_DI_SEEK:
				res = 0;
				
				if (!dip.dvdrom_mode && !dip.has_id) { 
					res = handleDiCommand(inbuf, outbuf, outbuf_size);
					
				}
				break;
			case IOCTL_DI_WAITCOVERCLOSE:
				if (!dip.has_id)
					goto call_original_di;
				res = 0;
				break;

			case IOCTL_DI_STREAMING:
				if (!dip.dvdrom_mode &&
					!dip.has_id) 
					goto call_original_di;
				dip_memset((u8*) outbuf, 0, outbuf_size);
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;

			case IOCTL_DI_SETBASE:
				dip.base = inbuf[1];
				res = 0;
				break;

			case IOCTL_DI_GETBASE:
				outbuf[0] = dip.base;
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;

			case IOCTL_DI_SETFORCEREAD:
				dip.low_read_from_device = inbuf[1];
				res = 0;
				break;

			case IOCTL_DI_GETFORCEREAD:
				outbuf[0] = dip.low_read_from_device;
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;

			case IOCTL_DI_SETUSBMODE: {  
				dip.has_id = inbuf[1];
				dip.frag_mode = 0;
				// Copy id
				if (dip.has_id) {
					dip_memcpy(dip.id, (u8 *)&(inbuf[2]), 6);
				}
				dip.partition = inbuf[5];
				res = 0;
				break;
			}

			case IOCTL_DI_GETUSBMODE:
				outbuf[0] = dip.has_id;
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;

			// Set FRAG mode
			case IOCTL_DI_FRAG_SET:
				{
					u32   device   = inbuf[1];
					void *fraglist = (void*)inbuf[2];
					int   size	 = inbuf[3];

					// Close frag
					Frag_Close();

					// Disable mode
					dip.frag_mode = 0;
					dip.has_id = 0;
					*outbuf = 0;

					// Check device
					if (device && fraglist && size) {
						// Convert address
						fraglist = VirtToPhys(fraglist);

						// Open device
						res = Frag_Init(device, fraglist, size);
						*outbuf = res;

						// Enable mode
						if (res > 0) {
							dip.frag_mode = 1;
							dip.has_id = device;
						}
					}
					
					res = 0;
					break;
				}

			// Get IO mode
			case IOCTL_DI_MODE_GET:
				{
					/* return all mode bits */
					*outbuf = 0;
					if (dip.frag_mode) *outbuf = 0x10;
					else if (fat_mode) *outbuf = 0x08;
					else if (dip.has_id) *outbuf = 0x04;
					else if (dip.dvdrom_mode) *outbuf = 0x01;
					break;
				}

			// debug stuff
			case IOCTL_DI_HELLO:
				{
					dip_memcpy((u8*)outbuf, (u8*)"HELO", 4);
					outbuf[1] = dip.has_id;
					outbuf[2] = dip.frag_mode;
					break;
				}


			case IOCTL_DI_DISABLERESET:
				dip.disableReset = *((u8 *) (&inbuf[1]));
				res = 0;
				break;

			case IOCTL_DI_13:
			case IOCTL_DI_14: {
				volatile unsigned long *dvdio = (volatile unsigned long *) 0xD006000;	
				if (outbuf == NULL) {
					res = 0;
					break;
				}
				if (cmd == 0x13) {

					if (dip.has_id && usb_is_dvd){
					outbuf[0] = (usb_dvd_inserted() == 0)?0:2;
					} else {
						if (!dip.has_id || usb_device_fd<0) outbuf[0] = (dvdio[1] & 1)?0:2;
						else outbuf[0] = 2;
					}
					
			
				} else {
					
					// ioctl 0x14
					if (!dip.has_id || usb_device_fd<0) outbuf[0] = (dvdio[1] & 1)?0:2;
					else {outbuf[0] = 0x2;}

				}

				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;
			}
			case IOCTL_DI_15: {
				volatile unsigned long *dvdio = (volatile unsigned long *) 0xD006000;	
				outbuf[0] = dvdio[1] & (~0x00000001); 
				os_sync_after_write(outbuf, outbuf_size);
				res = 0;
				break;
			}

			case IOCTL_DI_CUSTOMCMD:
				res = DIP_CustomCommand((u8 *)((u32) inbuf[1] & 0x7FFFFFFF), (u8 *) outbuf);				
				break;

			case IOCTL_DI_OFFSET: {
				if (dip.dvdrom_mode || dip.has_id) {
					dip.offset =  ((inbuf[1] << 30 ) | inbuf[2]) & 0xFFFF8000; // ~(SECTOR_SIZE >> 2)
					res = 0;
				} else 
					goto call_original_di;
				break;
			}
			case IOCTL_DI_REPORTKEY:
				if (!dip.dvdrom_mode && !dip.has_id) 
					goto call_original_di;
				res = 0xA000;
				dip.currentError = 0x53100;
				break;

			case IOCTL_DI_DISC_BCA: {
				u32 cont = 0;
				os_sync_before_read((u8 *) bca_bytes, BCADATA_SIZE);
				while (bca_bytes[cont] == 0) {
					cont++;
					if (cont == 64)
						goto call_original_di;
				}
				dip_memcpy((u8 *) outbuf, bca_bytes, BCADATA_SIZE);
				os_sync_after_write(outbuf,BCADATA_SIZE);
				res = 0;
				break;
			}

			case IOCTL_DI_READ: {
				dip.reading = 1;
				if (!dip.low_read_from_device) 
					res = handleDiCommand(inbuf, outbuf, outbuf_size);
				else
					res = read_from_device(outbuf, inbuf[1], inbuf[2]);
				dip.reading = 0;
				
				break;
			}
			case IOCTL_DI_READID: {
				u32 cmdRes;

				cmdRes = 0;

				res=0;

				if (!dip.has_id) cmdRes = handleDiCommand(inbuf, outbuf, outbuf_size);


				if (cmdRes || (dip.base | dip.offset) || dip.has_id || ((u32 *) outbuf)[6] != WII_DVD_SIGNATURE) 
					{
					dip.dvdrom_mode = (cmdRes==0)?0:1;
					res = read_id_from_image(outbuf, outbuf_size);
					} 
				else
					dip.dvdrom_mode = 0;
				 if (!res)
					  __DI_CheckDisc();

				break;
			}
			case IOCTL_DI_RESET: {
				if (dip.disableReset) {
					res = 0;
					break;
				}
				dip.reading = 0;
				dip.low_read_from_device = 0;
				dip.dvdrom_mode = 0;
				dip.base = 0;
				dip.offset = 0;
				dip.currentError = 0;

				if (!dip.has_id) 
					{
					ciso_lba=dip.partition-1;
					goto call_original_di;
					}

				DIP_StopMotor();
				if (!dip.frag_mode) {
					res = usb_open_device(dip.has_id - 1, &(dip.id[0]), dip.partition);
				}
				break;
			}
			case IOCTL_DI_UNENCREAD:
			case IOCTL_DI_LOWREAD:
			case IOCTL_DI_READDVD: {
				u32 offset = inbuf[1];
				u32 len = inbuf[2];
				if (cmd == IOCTL_DI_READDVD) {
					offset = offset << 11;
					len = len << 9;
				}
				res = read_from_device(outbuf, offset, len);
			/*	if (res == 0 && dip.reading == 0)
						dummy_function(outbuf, outbuf_size);*/
				break;
			}

			default:
		call_original_di:
				res = handleDiCommand(inbuf, outbuf, outbuf_size);
				break;
	}
	dbg_printf("DIP::res=%d\n", res);
	return res;
}
