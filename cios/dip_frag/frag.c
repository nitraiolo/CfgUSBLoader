/*
 * DIP plugin for Custom IOS (FRAG mode)
 *
 * Copyright (C) 2010 Waninkoko, WiiGator, oggzee.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// frag list by oggzee

#include <string.h>

#include "dip_calls.h"
#include "syscalls.h"
#include "usbstorage.h"
#include "sdhc.h"
#include "frag.h"

#define DEV_NONE 0
#define DEV_USB  1
#define DEV_SDHC 2

#define MAX_IDX   640 // 640*16MB = 10GB
#define IDX_CHUNK 32768 // 16MB in sectors units: 16*1024*1024/512
#define IDX_SHIFT 15 // 1<<15 = 32768

static u16 frag_idx[MAX_IDX] = { 0 };
static u32 frag_init = 0;
static u32 frag_dev = 0;
static FragList fraglist_data = { 0 };
static FragList *frag_list = 0;
static u8 sector_buf[512] __attribute__ ((aligned (32)));

void optimize_frag_list(void);

// NOTE: all local variables are declared static
// because stack size is rather limited

// NOTE2: hmm, memcpy and char array writing seems
// to only work if length is word aligned... cache issue?

s32 Frag_Init(u32 device, void *fraglist, int size)
{
	static int ret;
	if (frag_init) Frag_Close();
	if (!device || device > 2) return -1;
	if (!size) return -1;
	if (size > sizeof(FragList)) return -2;

	if (device == DEV_USB) {
		ret = usbstorage_Init();
	} else {
		ret = sdhc_Init();
	}
   	if (ret) return ret;
	frag_dev = device;
	frag_list = &fraglist_data;
	os_sync_before_read(fraglist, size);
	memset(frag_list, 0, sizeof(FragList));
	memcpy(frag_list, fraglist, size);
	os_sync_after_write(fraglist, size);
	optimize_frag_list();
	frag_init = 1;
	return size;
}

void Frag_Close(void)
{
	frag_init = 0;
	frag_list = 0;
	frag_dev = 0;
}


void optimize_frag_list(void)
{
	int i;
	u32 off;
	u16 idx = 0;
	for (i=0; i<MAX_IDX; i++) {
		off = i << IDX_SHIFT;
		while ((off >= frag_list->frag[idx].offset + frag_list->frag[idx].count)
				&& (idx + 1 < frag_list->num))
		{
			idx++;
		}
		frag_idx[i] = idx;
	}
}

// in case a sparse block is requested,
// the returned poffset might not be equal to requested offset
// the difference should be filled with 0
int frag_get(FragList *ff, u32 offset, u32 count,
		u32 *poffset, u32 *psector, u32 *pcount)
{
	static int i;
	static u32 delta;
	static u32 idx_off;
	static u32 start_idx;
	
	// optimize seek inside frag list
	// jump to a precalculated index
	idx_off = offset >> IDX_SHIFT;
	if (idx_off > MAX_IDX) idx_off = MAX_IDX - 1;
	start_idx = frag_idx[idx_off];

	//printf("frag_get(%u %u)\n", offset, count);
	for (i=start_idx; i<ff->num; i++) {
		if (ff->frag[i].offset <= offset
			&& ff->frag[i].offset + ff->frag[i].count > offset)
		{
			delta = offset - ff->frag[i].offset;
			*poffset = offset;
			*psector = ff->frag[i].sector + delta;
			*pcount = ff->frag[i].count - delta;
			if (*pcount > count) *pcount = count;
			goto out;
		}
		if (ff->frag[i].offset > offset
			&& ff->frag[i].offset < offset + count)
		{
			delta = ff->frag[i].offset - offset;
			*poffset = ff->frag[i].offset;
			*psector = ff->frag[i].sector;
			*pcount = ff->frag[i].count;
			count -= delta;
			if (*pcount > count) *pcount = count;
			goto out;
		}
	}
	// not found
	if (offset + count > ff->size) {
		// error: out of range!
		return -2;
	}
	// if inside range, then it must be just sparse, zero filled
	// return empty block at the end of requested
	*poffset = offset + count;
	*psector = 0;
	*pcount = 0;
	out:
	//printf("=>(%u %u %u)\n", *poffset, *psector, *pcount);
	return 0;
}


// offset and len in sectors
int frag_read_sect(u32 offset, u8 *data, u32 len)
{
    static u32 off_ret;
    static u32 sector;
    static u32 count;
    static u32 delta;
    static int ret;

    while (len) {
        //int frag_get(FragList *ff, u32 offset, u32 count,
        //        u32 *poffset, u32 *psector, u32 *pcount)
        ret = frag_get(frag_list, offset, len,
                &off_ret, &sector, &count);
        if (ret) return ret; // err
        delta = off_ret - offset;
        if (delta) {
            // sparse block, fill with 0
            memset(data, 0, delta << 9);
            offset += delta;
            data   += delta << 9;
            len    -= delta;
        }
        if (count) {
            //int read_sector(void *ign, u32 lba, u32 count, void*buf)
            //ret = read_sector(0, sector, count, data);
            //if (ret) return ret;
			//bool usbstorage_ReadSectors(u32 sector, u32 numSectors, void *buffer)
            //ret = usbstorage_ReadSectors(sector, count, data);
            if (frag_dev == DEV_USB) {
                ret = __usbstorage_Read(sector, count, data);
            } else {
                ret = sdhc_Read(sector, count, data);
            }
            if (!ret) return -3;
            offset += count;
            data   += count << 9;
            len    -= count;
        }
        if (delta + count == 0) {
            // (should never happen)
            return -4;
        }
    }

    return 0;
}

// offset is pointing 32bit words to address the whole dvd, len is in bytes
int frag_read_partial(u32 offset, u8 *data, u32 len, u32 *read_len)
{
    static int ret;
    static u32 off_sec;
    static u32 mod;
    static u32 rlen;

    off_sec = offset >> 7; // word to sect
    mod = (offset & ((512-1) >> 2)) << 2; // offset from start of sector in bytes
    rlen = 512 - mod; // remaining len from mod to end of sector
    if (rlen > len) rlen = len;
    if (rlen == 512) rlen = 0; // don't read whole sectors
    if (rlen) {
        ret = frag_read_sect(off_sec, sector_buf, 1);
        if (ret) return ret;
        memcpy(data, sector_buf + mod, rlen);
    }
    *read_len = rlen;
    return 0;
}

// woffset is pointing 32bit words to address the whole dvd, len is in bytes
s32 Frag_Read(void *data, u32 len, u32 woffset)
{
    static int ret;
    static u32 rlen;
    static u32 off_sec;
    static u32 len_sec;

    // read leading partial non sector aligned data
    ret = frag_read_partial(woffset, data, len, &rlen);
    if (ret) return ret;
    woffset += rlen >> 2;
    data   += rlen;
    len    -= rlen; 
    if (len >= 512) {
        // read sector aligned data
        off_sec = woffset >> 7; // word to sect
        len_sec = len >> 9;    // byte to sect
        ret = frag_read_sect(off_sec, data, len_sec);
        if (ret) return ret;
        woffset += len_sec << 7;
        data   += len_sec << 9;
        len    -= len_sec << 9;
    }
    if (len) {
        // read trailing partial non sector aligned data
        ret = frag_read_partial(woffset, data, len, &rlen);
        if (ret) return ret;
        len -= rlen;
    }
    if (len) return -5; // should never happen
    // success
    return 0;
}

