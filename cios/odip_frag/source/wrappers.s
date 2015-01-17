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

	.align 4

	.global dip_alloc_aligned
	.code 32
dip_alloc_aligned:
	stmfd	sp!, {r7, lr}
	ldr	r7, =addr_ios_shared_alloc_aligned
	ldr	r7, [r7]
	bl	_callOriginal
	ldmfd	sp!, {r7, lr}
	bx	lr

	.global dip_free
	.code 32
dip_free:
	stmfd	sp!, {r7, lr}
	ldr	r7, =addr_ios_shared_free
	ldr	r7, [r7]
	bl	_callOriginal
	ldmfd	sp!, {r7, lr}
	bx	lr

	.global dip_memcpy
	.code 32
dip_memcpy:
	stmfd	sp!, {r7, lr}
	ldr	r7, =addr_ios_memcpy
	ldr	r7, [r7]
	bl	_callOriginal
	ldmfd	sp!, {r7, lr}
	bx	lr

	.global dip_fatal_di_error
	.code 32
dip_fatal_di_error:
	stmfd	sp!, {r7, lr}
	ldr	r7, =addr_ios_fatal_di_error
	ldr	r7, [r7]
	bl	_callOriginal
	ldmfd	sp!, {r7, lr}
	bx	lr

	.global dip_doReadHashEncryptedState
	.code 32
dip_doReadHashEncryptedState:
	stmfd	sp!, {r7, lr}
	ldr	r7, =addr_ios_doReadHashEncryptedState
	ldr	r7, [r7]
	bl	_callOriginal
	ldmfd	sp!, {r7, lr}
	bx	lr

	.global dip_printf
	.code 32
dip_printf:
	stmfd	sp!, {r7, lr}
	ldr	r7, =addr_ios_printf
	ldr	r7, [r7]
	bl	_callOriginal
	ldmfd	sp!, {r7, lr}
	bx	lr

	.global _callOriginal
	.code 32
_callOriginal:
	bx	r7

	.global handleDiCommand
	.code 16
	.thumb_func
handleDiCommand:
	push	{r4-r7,lr}
	mov 	r7, r11
	mov	r6, r10
	mov	r5, r9
	mov	r4, r8
	push	{r4-r7}
	ldr	r3, =addr_di_cmd_reentry
	ldr	r3, [r3]
	bx	r3

/* Address conversion */
	.code 32
	.global VirtToPhys
VirtToPhys:
	and     r0, #0x7fffffff
	bx      lr

	.global PhysToVirt
PhysToVirt:
	orr     r0, #0x80000000
	bx      lr
	
