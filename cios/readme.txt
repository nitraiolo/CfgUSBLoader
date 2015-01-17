
dip_orig : waninkoko rev19
dip_frag : + frag (devkit arm 17)

odip_plugin : spaceman spiff & hermes cios_mload_4.1
odip_frag : + frag (devkit arm 15)

sdhc_orig : waninkoko rev20
sdhc : address change for hermes (devkit arm 15)


addr:


hermes cios 5.1 / uloader 5.1E:

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


cfg 59+

	frag list - overlaps fatffs:
			0x13730000 + 3A98C = 0x1376A98C
			( 20001*(3*4) = 240012 = 0x3A98C )

	sdhc: (for hermes cios, overlaps fatffs)
	exe(rwx)        : ORIGIN = 0x1376B000, LENGTH = 0x2000
	ram(rwx)        : ORIGIN = 0x1376D000, LENGTH = 0xB000 // END 13778000
	used:
	Writing segment 0x1376B000 to 0x00000108 (5184 bytes) - 1440 
	Writing segment 0x1376D000 to 0x00001548 (224 bytes) - a20c 

	odip_frag: (for hermes cios)
	exe(rwx)		: ORIGIN = 0x1377C000, LENGTH = 0x2200
	ram(rw)			: ORIGIN = 0x1377E200, LENGTH = 0x1E00 /* END 13780000 */
				  		/* end must not exceed mload ios_exe end */


cfg 5x

	dip_frag: (for waninkoko cios)
	exe(rwx)        : ORIGIN = 0x13700000, LENGTH = 0x2000
	ram(rw)         : ORIGIN = 0x13702000, LENGTH = 0x40000


waninkoko rev19:

	sdhc:
    exe(rwx)        : ORIGIN = 0x137e0000, LENGTH = 0x8000
    ram(rw)         : ORIGIN = 0x137e8000, LENGTH = 0x10000 // END 0x137f8000


