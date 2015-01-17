/*   
	Custom IOS module for Wii.
    Copyright (C) 2008 neimod.
    Copyright (C) 2010 Spaceman Spiff.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

	.section ".init"
	.global _start

	.align	4
	.code 32


/*******************************************************************************
 *
 * crt0.s - IOS module startup code
 *
 *******************************************************************************
 *
 *
 * v1.0 - 26 July 2008				- initial release by neimod
 * v1.1 - 5 September 2008			- prepared for public release
 *
 */

.EQU	dip_plugin_id, 0x12340001

.EQU	ios36_dvd_read_controlling_data, 0x2022DDAC
.EQU	ios36_handle_di_cmd_reentry, 0x20201010
.EQU	ios36_shared_alloc_aligned, 0x20200b9c
.EQU	ios36_shared_free, 0x20200b70
.EQU	ios36_memcpy, 0x20205dc0
.EQU	ios36_fatal_di_error, 0x20200048
.EQU	ios36_doReadHashEncryptedState, 0x20202b4c
.EQU	ios36_printf, 0x20203934

.EQU	ios38_dvd_read_controlling_data, 0x2022cdac
.EQU	ios38_handle_di_cmd_reentry, 0x20200d38
.EQU	ios38_shared_alloc_aligned, 0x202008c4
.EQU	ios38_shared_free, 0x20200898
.EQU	ios38_memcpy, 0x20205b80
.EQU	ios38_fatal_di_error, 0x20200048
.EQU	ios38_doReadHashEncryptedState, 0x20202874
.EQU	ios38_printf, 0x2020365c

	.global addr_dvd_read_controlling_data
	.global addr_di_cmd_reentry
	.global addr_ios_shared_alloc_aligned
	.global	addr_ios_shared_free
	.global addr_ios_memcpy
	.global	addr_ios_fatal_di_error
	.global	addr_ios_doReadHashEncryptedState
	.global	addr_ios_printf

/* Begin DIP-Plugin compatible Table */
	.long	DI_EmulateCmd
	.long	dip_plugin_id
addr_dvd_read_controlling_data:
	.long	ios36_dvd_read_controlling_data
addr_di_cmd_reentry:
	.long	ios36_handle_di_cmd_reentry + 1
addr_ios_shared_alloc_aligned:
	.long	ios36_shared_alloc_aligned + 1
addr_ios_shared_free:
	.long	ios36_shared_free + 1
addr_ios_memcpy:
	.long	ios36_memcpy + 1
addr_ios_fatal_di_error:
	.long	ios36_fatal_di_error + 1
addr_ios_doReadHashEncryptedState:
	.long	ios36_doReadHashEncryptedState + 1
addr_ios_printf:
	.long	ios36_printf + 1

	.long	0        @ reserved @
	.long	0        @ reserved @
	.long	0        @ reserved @
	.long	0        @ reserved @
	.long	filename_data
	.long	bca_bytes
	.long	in_ES_ioctlv

	.global mem_bss
mem_bss:
	.long _ini_bss

	.global mem_bss_len
mem_bss_len:
	.long _len_bss

	.align 4
_start:
	.end
