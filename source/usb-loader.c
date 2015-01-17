#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <unistd.h>
#include <string.h>
#include <sys/iosupport.h>

#include "disc.h"
#include "gui.h"
#include "menu.h"
#include "restart.h"
#include "subsystem.h"
#include "sys.h"
#include "video.h"
#include "cfg.h"
#include "wpad.h"
#include "fat.h"
#include "util.h"
#include "gettext.h"
#include "mload.h"
#include "usbstorage.h"
#include "console.h"
#include "RuntimeIOSPatch.h"

// libogc < 1.8.5 can hang if wiimote is initialized multiple times
// (in case ios is reload 2x) so we delay wpad to later
// libogc = 1.8.5 can crash if started with 2+ wiimotes
// #define WPAD_EARLY

int __nop_write(struct _reent *r,int fd,const char *ptr,size_t len)
{
	return len;
}

const devoptab_t dotab_nopout =
{
	"nopout",	// device name
	0,			// size of file structure
	NULL,		// device open
	NULL,		// device close
	__nop_write,	// device write
	NULL,		// device read
	NULL,		// device seek
	NULL,		// device fstat
	NULL,		// device stat
	NULL,		// device link
	NULL,		// device unlink
	NULL,		// device chdir
	NULL,		// device rename
	NULL,		// device mkdir
	0,			// dirStateSize
	NULL,		// device diropen_r
	NULL,		// device dirreset_r
	NULL,		// device dirnext_r
	NULL,		// device dirclose_r
	NULL		// device statvfs_r
};


void print_ios()
{
	printf("%*s", 60, "");
	/*if ( (strncasecmp(CFG.partition, "fat", 3) == 0) && CFG.ios_mload ) {
		printf("%d-fat", CFG.ios);
	} else {*/
		printf("%s", ios_str(CFG.game.ios_idx));
	//}
	__console_flush(0);
}

int main(int argc, char **argv)
{
	s32 ret;
	
	IOSPATCH_Apply();

	devoptab_list[STD_OUT] = &dotab_nopout;
	devoptab_list[STD_ERR] = &dotab_nopout;
	__console_disable = 1;
	dbg_printf("main(%d)\n", argc);

	get_time(&TIME.boot1);

	/* Initialize system */
	mem_init();
	Sys_Init();

	cfg_parsearg_early(argc, argv);
	InitDebug();
	// reset in 60 seconds in case an exception (CODE DUMP) occurs
	#if OGC_VER >= 180
	extern void __exception_setreload(int t);
	__exception_setreload(60);
	#endif

	/* Set video mode */
	Video_SetMode();

	__console_disable = 0;
	Gui_DrawIntro();

	if (CFG.debug) __console_scroll = 1;
	get_title_id();

	/* Check for stub IOS */
	ret = checkIOS(CFG.ios);

	if (ret == -1) {
		__console_disable = 0;
		printf_x(gt("ERROR:"));
		printf("\n");
		printf_(gt("Custom IOS %d is a stub!\nPlease reinstall it."), CFG.ios);
		printf("\n");
		goto out;
	} else if (ret == -2) {
		__console_disable = 0;
		printf_x(gt("ERROR:"));
		printf("\n");
		printf_(gt("Custom IOS %d could not be found!\nPlease install it."), CFG.ios);
		printf("\n");
		goto out;
	}

	/* Load Custom IOS */
	get_time(&TIME.ios11);
	print_ios();
	if (get_ios_idx_type(CFG.game.ios_idx) == IOS_TYPE_WANIN) {
		ret = IOS_ReloadIOS(CFG.ios);
		CURR_IOS_IDX = CFG.game.ios_idx;
		if (is_ios_type(IOS_TYPE_WANIN) && IOS_GetRevision() >= 18) {
			//load_dip_249();
			mk_mload_version();
			mload_close();
		}
		//usleep(200000); // this seems to cause hdd spin down/up
	} else {
		ret = ReloadIOS(0, 0);
	}
	printf("\n");
	dbg_printf("reload ios: %d = %d\n", CFG.ios, ret);
	get_time(&TIME.ios12);

	/* Initialize subsystems */
	//Subsystem_Init();
	
	/* Initialize Wiimote subsystem */
	#ifdef WPAD_EARLY
	Wpad_Init();
	#endif

	// Initial usb init, no retry at this point
	USBStorage_Init();

	dbg_printf("Mount SD\n");
	MountSDHC();
	
	dbg_printf("Mount USB\n");
	if (MountUSB() == 0)
		mount_find(USB_DRIVE);

	//save_dip();

	/* Load configuration */
	long long io1, io2;
	io1 = TIME_D(sd_init) + TIME_D(sd_mount) + TIME_D(usb_init) + TIME_D(usb_mount);
	//dbg_printf("CFG Load\n");
	get_time(&TIME.cfg1);
	CFG_Load(argc, argv);
	get_time(&TIME.cfg2);
	io2 = TIME_D(sd_init) + TIME_D(sd_mount) + TIME_D(usb_init) + TIME_D(usb_mount);
	TIME.cfg2 -= (io2 - io1);

	//cfg_ios_set_idx(CFG_IOS_222_MLOAD); // debug test
	if (CURR_IOS_IDX != CFG.game.ios_idx) {
		get_time(&TIME.ios21);
		#ifdef WPAD_EARLY
		Wpad_Disconnect();
		#endif
		print_ios();
		usleep(300000);
		ret = ReloadIOS(0, 0);
		usleep(300000);
		printf("\n");
		#ifdef WPAD_EARLY
		Wpad_Init();
		#endif
		get_time(&TIME.ios22);
	}

	/* Check if Custom IOS is loaded */
	if (ret < 0) {
		__console_disable = 0;
		printf_x(gt("ERROR:"));
		printf("\n");
		printf_(gt("Custom IOS %d could not be loaded! (ret = %d)"), CFG.ios, ret);
		printf("\n");
		goto out;
	}
	
	// set widescreen as set by CFG.widescreen
	Video_SetWide();

	util_init();

	// Gui Init
	Gui_Init();

	#ifndef WPAD_EARLY
	Wpad_Init();
	#endif

	if (CFG.debug) {
		Menu_PrintWait();
	} else {
		__console_disable = 1;
	}

	// Show background
	//Gui_DrawBackground();
	get_time(&TIME.conbg1);
	Gui_LoadBackground();
	get_time(&TIME.conbg2);

	/* Initialize console */
	Gui_InitConsole();

	/* Set up config parameters */
	//printf("CFG_Setup\n"); //Wpad_WaitButtons();
	CFG_Setup(argc, argv);

	// Notify if ios was reloaded by cfg
	if (CURR_IOS_IDX != DEFAULT_IOS_IDX) {
		printf("\n");
		printf_x(gt("Custom IOS %s Loaded OK"), ios_str(CURR_IOS_IDX));
		printf("\n\n");
		sleep(2);
	}
	
	/* Initialize DIP module */
	ret = Disc_Init();
	if (ret < 0) {
		Gui_Console_Enable();
		printf_x(gt("ERROR:"));
		printf("\n");
		printf_(gt("Could not initialize DIP module! (ret = %d)"), ret);
		printf("\n");
		goto out;
	}

	/* Menu loop */
	//dbg_printf("Menu_Loop\n"); //Wpad_WaitButtons();
	Menu_Loop();

out:
	sleep(8);
	/* Restart */
	exit(0);
	Restart_Wait();

	return 0;
}

