OUTPUT_FORMAT("elf32-bigarm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
 
 
/*
	Change exe start and ram start as needed for the custom IOS module.
	Current settings are for the EHC module from IOS 31.  actually this is free space in this IOS
 */


MEMORY {
	exe(rwx)	: ORIGIN = 0x1377C000, LENGTH = 0x2200
	ram(rw)		: ORIGIN = 0x1377E200, LENGTH = 0x1E00 /* END 13780000 */
				  /* end must not exceed mload ios_exe end */
}

__exe_start_virt__			= 0x1377C000;
__exe_start_phys__			= 0x1377C000;
__ram_start_virt__			= 0x1377E200;
__ram_start_phys__			= 0x1377E200;
__ios_info_table_start		= 0x0;

/*
orig:
MEMORY {
		exe(rwx)	: ORIGIN = 0x1377E000, LENGTH = 0x1600
		ram(rw)		: ORIGIN = 0x1377F600, LENGTH = 0xA00
}
__exe_start_virt__			= 0x1377E000;
__exe_start_phys__			= 0x1377E000;
__ram_start_virt__			= 0x1377F600;
__ram_start_phys__			= 0x1377F600;
__ios_info_table_start		= 0x0;
*/


/*

uloader 5.1E:

	ehcmodule:
	exe(rwx)        : ORIGIN = 0x13700000, LENGTH = 0x6000
	ram(rw)         : ORIGIN = 0x13706000, LENGTH = 0x2A000 // END 0x1372E000

	fatffs:
	exe(rwx)        : ORIGIN = 0x13730000, LENGTH = 0xa800
	ram(rwx)        : ORIGIN = 0x1373a800, LENGTH = 0x40000 // END 1377A800

	odip:
	exe(rwx)        : ORIGIN = 0x1377E000, LENGTH = 0x1600
	ram(rw)         : ORIGIN = 0x1377F600, LENGTH = 0xA00

	mload:
	exe(rwx)        : ORIGIN = 0x138c0000, LENGTH = 0x4000
	ram(rw)         : ORIGIN = 0x138c8000, LENGTH = 0x8000 // END 138D0000
	ios_exe(rw)     : ORIGIN = 0x13700000, LENGTH = 0x80000 // END 13780000

cfg 5x dip_frag:
	exe(rwx)        : ORIGIN = 0x13700000, LENGTH = 0x2000
	ram(rw)         : ORIGIN = 0x13702000, LENGTH = 0x40000

*/


 
SECTIONS
{
    .debug_aranges 0 : { *(.debug_aranges) } .debug_pubnames 0 : { *(.debug_pubnames) } .debug_info 0 : { *(.debug_info) } .debug_abbrev 0 : { *(.debug_abbrev) } .debug_line 0 : { *(.debug_line) } .debug_frame 0 : { *(.debug_frame) } .debug_str 0 : { *(.debug_str) } .debug_loc 0 : { *(.debug_loc) } .debug_macinfo 0 : { *(.debug_macinfo) } .note.arm.ident 0 : { KEEP (*(.note.arm.ident)) }
    .init __exe_start_virt__ : AT (__exe_start_phys__) { . = .; KEEP (*(.init)) } > exe
    .text ALIGN (0x20) : { 
    	*(.text*) 
    	*(.gnu.warning) 
    	*(.rodata*) 
    	*(.gnu.linkonce.t.*) 
    	*(.init) 
    	*(.glue_7) 
    	*(.glue_7t) } > exe
    .data __ram_start_virt__ : AT (__ram_start_phys__) { KEEP( *(.ios_data) ) *(.data*) *(.data1) *(.gnu.linkonce.d.*)  . = ALIGN (4);  __CTOR_LIST__ = ABSOLUTE (.); KEEP (*(SORT (.ctors*))) __CTOR_END__ = ABSOLUTE (.); __DTOR_LIST__ = ABSOLUTE (.); KEEP (*(SORT (.dtors*))) __DTOR_END__ = ABSOLUTE (.); 
    	*(.dynamic) 
	*(.sdata*) 
	 *(.dynbss) 
	*(.gnu.linkonce.s.*) . = ALIGN (4); *(.2ram.*) } > ram 
    .fini : { . = .; *(.fini) } > ram
     .rodata ALIGN (0x4) : { 
    	. = .; 
    	*(.gnu.linkonce.r.*) } > ram
    .rodata1 ALIGN (0x4) : { . = .; *(.rodata1) } > ram
    .fixup ALIGN (0x4) : { . = .; *(.fixup) } > ram
    .gcc_except_table ALIGN (0x4) : { . = .; *(.gcc_except_table) } > ram
    .got ALIGN (0x4) : { *(.got.plt) *(.got) } > ram

    _ini_bss = . ;
    .bss ALIGN (0x20) : { *(.scommon) 
    			 *(.dynsbss)
			*(.sbss*) 
			 *(.gnu.linkonce.sb.*) 
			 *(.bss*) 
			 *(.gnu.linkonce.b.*) 
			 *(COMMON) KEEP( *(.ios_bss) ) } > ram
     _len_bss = . - _ini_bss ;
    
  	. = ALIGN(4);
}
