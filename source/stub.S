	.rodata

	.globl bgImg
	.balign 32
bgImg:
	.incbin "../data/background"

	.globl cover_top
	.balign 32
cover_top:
	.incbin "../data/cover_top"

	.globl cover_side
	.balign 32
cover_side:
	.incbin "../data/cover_side"

	.globl cover_front
	.balign 32
cover_front:
	.incbin "../data/cover_front"

	.globl coverImg
	.balign 32
coverImg:
	.incbin "../data/nocover"

	.globl coverImg_full
	.balign 32
coverImg_full:
	.incbin "../data/nocover_full"

	.globl pointer
	.balign 32
pointer:
	.incbin "../data/pointer"

	.globl hourglass
	.balign 32
hourglass:
	.incbin "../data/hourglass"

	.globl gui_font
	.balign 32
gui_font:
	.incbin "../data/gui_font"

	.globl bg_gui
	.balign 32
bg_gui:
	.incbin "../data/bg_gui"

	.globl star_icon
	.balign 32
star_icon:
	.incbin "../data/star"

#;	.globl ehcmodule2
#;	.balign 32
#;ehcmodule2:
#;	.incbin "../data/ehcmodule2_elf"

#;	.globl dip_plugin2
#;	.balign 32
#;dip_plugin2:
#;	.incbin "../data/dip_plugin2_bin"

#;	.globl ehcmodule3
#;	.balign 32
#;ehcmodule3:
#;	.incbin "../data/ehcmodule3_elf"

#;	.globl dip_plugin3
#;	.balign 32
#;dip_plugin3:
#;	.incbin "../data/dip_plugin3_bin"

#;	.globl ehcmodule4
#;	.balign 32
#;ehcmodule4:
#;	.incbin "../data/ehcmodule4_elf"

#;	.globl dip_plugin4
#;	.balign 32
#;dip_plugin4:
#;	.incbin "../data/dip_plugin4_bin"

#;	.globl ehcmodule_frag
#;	.balign 32
#;ehcmodule_frag:
#;	.incbin "../data/ehcmodule_frag"

	.globl odip_frag
	.balign 32
odip_frag:
	.incbin "../data/odip_frag_bin"

	.globl ehcmodule5
	.balign 32
ehcmodule5:
	.incbin "../data/ehcmodule5_elf"

	.globl sdhc_module
	.balign 32
sdhc_module:
	.incbin "../data/sdhc_module_elf"

	.globl introImg2
	.balign 32
introImg2:
	.incbin "../data/intro2"

	.globl introImg3
	.balign 32
introImg3:
	.incbin "../data/intro3"

	.globl introImg41
	.balign 32
introImg41:
	.incbin "../data/intro41"

##; commented out
#;	.globl opening_bnr
#;	.balign 32
#;opening_bnr:
#;	.incbin "../data/opening_bnr"

	.globl dip_plugin_249
	.balign 32
dip_plugin_249:
	.incbin "../data/dip_frag_249_r21"

#define STUB 0x3400

        .text
        .section .text
        .globl _unstub_start

_unstub_start:
        isync
        // set MSR[DR:IR] = 00, jump to STUB
        lis 3,STUB@h
        ori 3,3,STUB@l
        mtsrr0 3

        mfmsr 3
        li 4,0x30
        andc 3,3,4
        mtsrr1 3
        rfi
