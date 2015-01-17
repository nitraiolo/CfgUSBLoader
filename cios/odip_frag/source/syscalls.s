/*
 *  Copyright (C) 2010 Spaceman Spiff
 *
 *  syscalls.s (c) 2009, Hermes 
 *  info from http://wiibrew.org/wiki/IOS/Syscalls
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

.macro syscall vec_sys
	.long 0xE6000010 +(\vec_sys<<5)
	bx lr
.endm

	.align 4
	.arm

	.code 32
	.global os_sync_after_write
os_sync_after_write:
	syscall 0x40

	.code 32
	.global os_sync_before_read
os_sync_before_read:
	syscall 0x3F

	.code 32
	.global os_heap_alloc_aligned
os_heap_alloc_aligned:
	syscall 0x19

	.code 32
	.global os_heap_free
os_heap_free:
	syscall 0x1a

	.code 32
	.global os_open
os_open:
	syscall 0x1c

	.code 32
	.global os_close
os_close:
	syscall 0x1d

	.code 32
	.global os_read
os_read:
	syscall 0x1e

	.code 32
	.global os_write
os_write:
	syscall 0x1f

	
	.code 32
	.global os_seek
os_seek:
	syscall 0x20


	.code 32
	.global os_ioctlv
os_ioctlv:
	syscall 0x22

	.code 32
	.global swi_mload_func
swi_mload_func:
	svc 0xCC
	bx lr


	.code 16
	.global os_puts
os_puts:
	mov	r2, lr
	add	r1, r0, #0
	mov	r0, #4
	svc	0xab
	bx	r2

/* 

   // De rodries:  http://www.elotrolado.net/hilo_utilidad-uloader-v2-1c-ocarina-y-forzado-de-video-idioma_1217626_s1050
   .code 32
   .global os_puts
os_puts:
   mov R2,lr
   adds r1,r0,#0
   movs R0,#4
   svc 0xAB
   bx r2
*/
