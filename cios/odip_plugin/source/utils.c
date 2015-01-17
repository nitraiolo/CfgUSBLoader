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

#include <utils.h>

void dip_memset(u8 *buf, u32 ch, u32 size)
{
	u32 unaligned;
	u8 c = (u8) ch;

	int cnt = 0;

	if (size == 0)
		return;

	unaligned = (4 - ((u32) buf & 0x3)) & 0x3;
	
	if (unaligned > size) 
		unaligned = size;
	
	if (unaligned) 
		while (unaligned--) {
			buf[cnt] = c;
			cnt++;
		};

	// Process aligned bytes
	u32 c32 = (ch << 24) | (ch << 16) | (ch << 8) | ch;

	u32 *buf32 = (u32 *) (buf + cnt);
	while (cnt + 4 <= size) {
		*(buf32++) = c32;
		cnt += 4;
	}
	
	// Process unaligned tail bytes
	while (cnt++ < size) 
		buf[cnt] = c;
}
