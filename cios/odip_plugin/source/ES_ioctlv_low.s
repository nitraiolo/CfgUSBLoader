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

   .align 2
   .code 16
   .global in_ES_ioctlv
   .thumb_func
in_ES_ioctlv:
   
   push {r2-r6}
   push {lr}
   bl ES_ioctlv+1
   pop {r1}
   pop {r2-r6}
   bx r1

   .global out_ES_ioctlv
   .thumb_func
out_ES_ioctlv:
   push   {r4-r6,lr}
   sub   sp, sp, #0x20
   ldr r5, [r0,#8]
   add r1, r0, #0
   ldr r3, = 0x201000D5
   bx r3
