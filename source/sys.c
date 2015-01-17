#include <stdio.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <time.h>

#include "sys.h"
#include "subsystem.h"
#include "cfg.h"
#include "fat.h"
#include "menu.h"
#include "gettext.h"
#include "sha1.h"
#include "sdhc.h"
#include "usbstorage.h"
#include "wpad.h"
#include "apploader.h"
#include "mem.h"
#include "nand.h"
#include "RuntimeIOSPatch.h"

typedef struct {
	u32 block_size;
	u32 free_blocks;
	u32 used_blocks;
	u32 unk3;
	u32 unk4;
	u32 free_inodes;
	u32 unk5;
} isfs_stats_t;

/* Constants */
#define CERTS_LEN	0x280

typedef struct _iosinfo_t {
	u32 magicword; //0x1ee7c105
	u32 magicversion; // 1
	u32 version; // Example: 5
	u32 baseios; // Example: 56
	char name[0x10]; // Example: d2x
	char versionstring[0x10]; // Example: beta2
} __attribute__((packed)) iosinfo_t;

bool get_iosinfo(int ios, signed_blob *TMD, iosinfo_t *iosinfo);
char* get_ios_info_from_tmd();

extern char dm_boot_drive[16];

/* Variables */
static const char certs_fs[] ATTRIBUTE_ALIGN(32) = "/sys/cert.sys";

u8 shutdown = 0;

void __Sys_ResetCallback(void)
{
	/* Reboot console */
	Sys_Reboot();
}

void __Sys_PowerCallback(void)
{
	/* Poweroff console */
	//Sys_Shutdown();
	shutdown = 1;
}


void Sys_Init(void)
{
	/* Initialize video subsytem */
	VIDEO_Init();

	/* Set RESET/POWER button callback */
	SYS_SetResetCallback(__Sys_ResetCallback);
	SYS_SetPowerCallback(__Sys_PowerCallback);

	/* Prepare random seed */
	srand(time(0));
}

void Sys_Reboot(void)
{
	/* Restart console */
	STM_RebootSystem();
}

void Sys_Shutdown(void)
{
	SDHC_Close();
	USBStorage_Deinit();
	Wpad_Disconnect();
	/* Poweroff console */
	if(CONF_GetShutdownMode() == CONF_SHUTDOWN_IDLE) {
		s32 ret;

		/* Set LED mode */
		ret = CONF_GetIdleLedMode();
		if(ret >= 0 && ret <= 2)
			STM_SetLedMode(ret);

		/* Shutdown to idle */
		STM_ShutdownToIdle();
	} else {
		/* Shutdown to standby */
		STM_ShutdownToStandby();
	}
}

void Sys_LoadMenu(void)
{
	/* Return to the Wii system menu */
	SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}


s32 Sys_GetCerts(signed_blob **certs, u32 *len)
{
	static signed_blob certificates[CERTS_LEN] ATTRIBUTE_ALIGN(32);

	s32 fd, ret;

	/* Open certificates file */
	fd = IOS_Open(certs_fs, 1);
	if (fd < 0)
		return fd;

	/* Read certificates */
	ret = IOS_Read(fd, certificates, sizeof(certificates));

	/* Close file */
	IOS_Close(fd);

	/* Set values */
	if (ret > 0) {
		*certs = certificates;
		*len   = sizeof(certificates);
	}

	return ret;
}

void prep_exit()
{
	dbg_printf("Services_Close\n");
	Services_Close();
	dbg_printf("Subsystem_Close\n");
	Subsystem_Close();
	dbg_printf("Video_Close\n");
	extern void Video_Close();
	Video_Close();
}

void Sys_Exit()
{
	prep_exit();
	exit(0);
}

void Sys_HBC()
{
	if (CFG.disable_options == 1) return;
	prep_exit();
	WII_Initialize();
	dbg_printf("HBC107\n");
	WII_LaunchTitle(TITLE_ID(0x00010001,0x4C554C5A)); // LULZ
	WII_LaunchTitle(TITLE_ID(0x00010001,0xAF1BF516)); // 1.07
	WII_LaunchTitle(TITLE_ID(0x00010001,0x4A4F4449)); // JODI
	WII_LaunchTitle(TITLE_ID(0x00010001,0x48415858)); // HAXX
	dbg_printf("exit\n");
	exit(0);
}

void Sys_Channel(u32 channel)
{
	prep_exit();
	WII_Initialize();
	WII_LaunchTitle(TITLE_ID(0x00010001,channel));
}


// mload from uloader by Hermes


#include "mload.h"
#include "mload_modules.h"

// uLoader 2.5:
//#define size_ehcmodule2 20340
//#define size_dip_plugin2 3304
//extern unsigned char ehcmodule2[size_ehcmodule2];
//extern unsigned char dip_plugin2[size_dip_plugin2];

// uLoader 2.8D:
//#define size_ehcmodule3 22264
//#define size_dip_plugin3 3352
//extern unsigned char ehcmodule3[size_ehcmodule3];
//extern unsigned char dip_plugin3[size_dip_plugin3];

// uLoader 3.0B:
//#define size_ehcmodule4 32384
//#define size_dip_plugin4 3512
// uLoader 3.0C:
//#define size_ehcmodule4 32432
// uLoader 3.1:
//#define size_ehcmodule4 32400
//#define size_dip_plugin4 3080 // uloader 3.1
//#define size_dip_plugin4 3224 // uloader 3.2
//#define size_dip_plugin4 5920 // uloader 5.1 odip
// EHCFAT module:
//#define size_ehcmodule_frag 70529 // cfg50-52
//#define size_ehcmodule_frag 70613 // cfg53
//extern unsigned char ehcmodule_frag[size_ehcmodule_frag];

#define size_odip_frag 9120 // odip + frag
extern unsigned char odip_frag[size_odip_frag];

//#define size_ehcmodule5 25287 // 5.0/5.1 hermes
#define size_ehcmodule5 27134 // 5.1 rodries
extern unsigned char ehcmodule5[size_ehcmodule5];
	
#define size_sdhc_module 5672
extern unsigned char sdhc_module[size_sdhc_module];
	
/*

cksum:

cios_mload_4.0

846383702 25614 ehcmodule.elf
3703302275 5920 odip_plugin.bin

uloader 4.9A 5.0

846383702 25614 ehcmodule.elf
2749576855 5920 odip_plugin.bin

cios_mload_4.1

2551980440 25287 ehcmodule.elf
3870739346 5920 odip_plugin.bin

uloader 5.0C 5.1B 5.1D 5.1E

2551980440 25287 ehcmodule.elf
3870739346 5920 odip_plugin.bin
 
*/

//int mload_ehc_fat = 0;
//int mload_need_fat = 0;

// current
void *ehcmodule;
int size_ehcmodule;

void *dip_plugin;
int size_dip_plugin;

// external2
/*
char ehc_path[200];
void *external_ehcmodule2 = NULL;
int size_external_ehcmodule2 = 0;

// external3
char ehc_path3[200];
void *external_ehcmodule3 = NULL;
int size_external_ehcmodule3 = 0;

// external4
char ehc_path4[200];
void *external_ehcmodule4 = NULL;
int size_external_ehcmodule4 = 0;

// external_fat
char ehc_path_fat[200];
void *external_ehcmodule_frag = NULL;
int size_external_ehcmodule_frag = 0;
*/

/*
static u32 ios_36[16] ATTRIBUTE_ALIGN(32)=
{
	0, // DI_EmulateCmd
	0,
	0x2022DDAC, // dvd_read_controlling_data
	0x20201010+1, // handle_di_cmd_reentry (thumb)
	0x20200b9c+1, // ios_shared_alloc_aligned (thumb)
	0x20200b70+1, // ios_shared_free (thumb)
	0x20205dc0+1, // ios_memcpy (thumb)
	0x20200048+1, // ios_fatal_di_error (thumb)
	0x20202b4c+1, // ios_doReadHashEncryptedState (thumb)
	0x20203934+1, // ios_printf (thumb)
};

static u32 ios_38[16] ATTRIBUTE_ALIGN(32)=
{
	0, // DI_EmulateCmd
	0,
	0x2022cdac, // dvd_read_controlling_data
	0x20200d38+1, // handle_di_cmd_reentry (thumb)
	0x202008c4+1, // ios_shared_alloc_aligned (thumb)
	0x20200898+1, // ios_shared_free (thumb)
	0x20205b80+1, // ios_memcpy (thumb)
	0x20200048+1, // ios_fatal_di_error (thumb)
	0x20202874+1, // ios_doReadHashEncryptedState (thumb)
	0x2020365c+1, // ios_printf (thumb)
};
*/

u32 patch_datas[8] ATTRIBUTE_ALIGN(32);

data_elf my_data_elf;
int my_thread_id=0;

/*
void load_ext_ehc_module(int verbose)
{
	if(!external_ehcmodule2)
	{
		snprintf(ehc_path, sizeof(ehc_path), "%s/ehcmodule.elf", USBLOADER_PATH);
		size_external_ehcmodule2 = Fat_ReadFile(ehc_path, &external_ehcmodule2);
		if (size_external_ehcmodule2 < 0) {
			size_external_ehcmodule2 = 0;
		}
	}
	if(!external_ehcmodule3)
	{
		snprintf(ehc_path3, sizeof(ehc_path3), "%s/ehcmodule3.elf", USBLOADER_PATH);
		size_external_ehcmodule3 = Fat_ReadFile(ehc_path3, &external_ehcmodule3);
		if (size_external_ehcmodule3 < 0) {
			size_external_ehcmodule3 = 0;
		}
	}
	if(!external_ehcmodule4)
	{
		snprintf(ehc_path4, sizeof(ehc_path4), "%s/ehcmodule4.elf", USBLOADER_PATH);
		size_external_ehcmodule4 = Fat_ReadFile(ehc_path4, &external_ehcmodule4);
		if (size_external_ehcmodule4 < 0) {
			size_external_ehcmodule4 = 0;
		}
	}
	if(!external_ehcmodule_frag)
	{
		snprintf(ehc_path_fat, sizeof(ehc_path_fat), "%s/ehcmodule_frag.elf", USBLOADER_PATH);
		size_external_ehcmodule_frag = Fat_ReadFile(ehc_path_fat, &external_ehcmodule_frag);
		if (size_external_ehcmodule_frag < 0) {
			size_external_ehcmodule_frag = 0;
		}
	}
}
*/

static char mload_ver_str[100];

static int mload_ver = 0;

void mk_mload_version()
{
	mload_ver_str[0] = 0;
	mload_ver = 0;
	if (CFG.ios_mload
			|| (is_ios_type(IOS_TYPE_WANIN) && IOS_GetRevision() >= 18) )
	{
		if (IOS_GetRevision() >= 4) {
			if (is_ios_type(IOS_TYPE_WANIN)) {
				char *info = get_ios_info_from_tmd();
				if (info) {
					sprintf(mload_ver_str, "Base: IOS%s ", info);
				} else {
					sprintf(mload_ver_str, "Base: IOS?? DI:%d ", wanin_mload_get_IOS_base());
				}
			} else {
				sprintf(mload_ver_str, "Base: IOS%d ", mload_get_IOS_base());
			}
		}
		if (IOS_GetRevision() > 4) {
			int v, s = 0;
			v = mload_ver = mload_get_version();
			if (v>0) {
				s = v & 0x0F;
				v = v >> 4;
			}
			sprintf(mload_ver_str + strlen(mload_ver_str), "mload v%d.%d ", v, s);
		} else {
			sprintf(mload_ver_str + strlen(mload_ver_str), "mload v%d ", IOS_GetRevision());
		}
	}
}

void print_mload_version_str(char *str)
{
	int new_wanin = is_ios_type(IOS_TYPE_WANIN) && IOS_GetRevision() >= 18;
	*str = 0;
	if (CFG.ios_mload || new_wanin) {
		strcpy(str, mload_ver_str);
	}
}

void print_mload_version()
{
	char str[100];
	print_mload_version_str(str);
	printf("%s", str);
}

int load_ehc_module_ex(int verbose)
{
	//extern int wbfs_part_fat;
	/*
	mload_ehc_fat = 0;
	if (mload_need_fat)
	{
		if (verbose) {
			if (strncasecmp(CFG.partition, "ntfs", 4) == 0) {
				printf("[NTFS]");
			} else {
				printf("[FAT]");
			}
		}
		if (IOS_GetRevision() < 4) {
			printf("\nERROR: IOS%u rev%u\n", IOS_GetVersion(), IOS_GetRevision());
			printf(gt("FAT mode only supported with ios 222 rev4"));
			printf("\n");
			sleep(5);
			return -1;
		}
		mload_ehc_fat = 1;
		size_ehcmodule = size_ehcmodule_frag;
		ehcmodule = ehcmodule_frag;
		size_dip_plugin = size_dip_plugin4;
		dip_plugin = dip_plugin4;
		if(external_ehcmodule_frag) {
			printf("\n");
			printf_("external: %s\n", ehc_path_fat);
			printf_("");
			ehcmodule = external_ehcmodule_frag;
			size_ehcmodule = size_external_ehcmodule_frag;
		}
	}
	else if (IOS_GetRevision() <= 2)
	{
		size_dip_plugin = size_dip_plugin2;
		dip_plugin = dip_plugin2;
		//size_ehcmodule = size_ehcmodule2;
		//ehcmodule = ehcmodule2;
		if(external_ehcmodule2) {
			//if (verbose) {
				printf("\n");
				printf_("external: %s\n", ehc_path);
				printf_("");
			//}
			ehcmodule = external_ehcmodule2;
			size_ehcmodule = size_external_ehcmodule2;
		} else {
			printf("\n");
			printf(gt("ERROR: ehcmodule2 no longer included!\n"
						"external file ehcmodule.elf must be used!"));
			printf("\n");
			sleep(5);
			return -1;
		}

	} else if (IOS_GetRevision() == 3) {
			size_dip_plugin = size_dip_plugin3;
			dip_plugin = dip_plugin3;
			//size_ehcmodule = size_ehcmodule3;
			//ehcmodule = ehcmodule3;
			if(external_ehcmodule3) {
				//if (verbose) {
					printf("\n");
					printf_("external: %s\n", ehc_path3);
					printf_("");
				//}
				ehcmodule = external_ehcmodule3;
				size_ehcmodule = size_external_ehcmodule3;
			} else {
				printf("\n");
				printf(gt("ERROR: ehcmodule3 no longer included!\n"
							"external file ehcmodule3.elf must be used!"));
				printf("\n");
				sleep(5);
				return -1;
			}
	} else if (IOS_GetRevision() == 4) {

		size_dip_plugin = size_dip_plugin4;
		dip_plugin = dip_plugin4;
		//size_ehcmodule = size_ehcmodule4;
		//ehcmodule = ehcmodule4;
		// use ehcmodule_frag also for wbfs
		size_ehcmodule = size_ehcmodule_frag;
		ehcmodule = ehcmodule_frag;
		if(external_ehcmodule4) {
			//if (verbose) {
				printf("\n");
				printf_("external: %s\n", ehc_path4);
				printf_("");
			//}
			ehcmodule = external_ehcmodule4;
			size_ehcmodule = size_external_ehcmodule4;
		}
	*/
	if (IOS_GetRevision() < 4) {
		printf("ERROR: IOS%d-mload v%d not supported!\n",
			IOS_GetVersion(), IOS_GetRevision());
		sleep(10);
		return -1;
	}

	size_dip_plugin = size_odip_frag;
	dip_plugin = odip_frag;
	size_ehcmodule = size_ehcmodule5;
	ehcmodule = ehcmodule5;

	//mload_ehc_fat = 1;

	// force usb port 0
	u8 *ehc_cfg = search_for_ehcmodule_cfg(ehcmodule, size_ehcmodule);
	if (ehc_cfg) {
		ehc_cfg += 12;
		ehc_cfg[0] = 0; // usb port 0
	}

	// IOS_GetRevision() >= 4

	dbg_printf("load ehc %d %d\n", size_dip_plugin, size_ehcmodule);
	int ret = load_ehc_module();
	dbg_printf("load ehc = %d\n", ret);

	if (ret == 0) {
		dbg_printf("SDHC module\n");
		mload_elf((void *) sdhc_module, &my_data_elf);
		my_thread_id= mload_run_thread(my_data_elf.start, my_data_elf.stack,
				my_data_elf.size_stack, my_data_elf.prio);
		dbg_printf("mload_run %d\n", my_thread_id);
		if(my_thread_id<0) ret = -5;
	}

	if (ret == 0) mk_mload_version();
	mload_close();
	return ret;

#if 0
	if (IOS_GetRevision() <= 2) {

		if (mload_module(ehcmodule, size_ehcmodule)<0) return -1;

	} else { // v3

		if(mload_init()<0) return -1;
		mload_elf((void *) ehcmodule, &my_data_elf);
		my_thread_id= mload_run_thread(my_data_elf.start, my_data_elf.stack, my_data_elf.size_stack, my_data_elf.prio);
		if(my_thread_id<0) return -1;
	}

	usleep(350*1000);

	// Test for IOS
	int is_ios=0;
	
	mload_seek(0x20207c84, SEEK_SET);
	mload_read(patch_datas, 4);
	if(patch_datas[0]==0x6e657665) 
		{
		is_ios=38;
		}
	else
		{
		is_ios=36;
		}

	if(is_ios==36)
		{
		// IOS 36
		memcpy(ios_36, dip_plugin, 4);		// copy the entry_point
		memcpy(dip_plugin, ios_36, 4*10);	// copy the adresses from the array
		
		mload_seek(0x1377E000, SEEK_SET);	// copy dip_plugin in the starlet
		mload_write(dip_plugin,size_dip_plugin);

		// enables DIP plugin
		mload_seek(0x20209040, SEEK_SET);
		mload_write(ios_36, 4);
		 
		}
	if(is_ios==38)
		{
		// IOS 38

		memcpy(ios_38, dip_plugin, 4);	    // copy the entry_point
		memcpy(dip_plugin, ios_38, 4*10);   // copy the adresses from the array
		
		mload_seek(0x1377E000, SEEK_SET);	// copy dip_plugin in the starlet
		mload_write(dip_plugin,size_dip_plugin);

		// enables DIP plugin
		mload_seek(0x20208030, SEEK_SET);
		mload_write(ios_38, 4);

		}
	
	mk_mload_version();

	mload_close();

	return 0;
#endif
}

// Reload custom ios

#include "usbstorage.h"
#include "sdhc.h"
#include "fat.h"
#include "wbfs.h"
#include "restart.h"
#include "wpad.h"
#include "wdvd.h"
extern s32 wbfsDev;


int ReloadIOS(int subsys, int verbose)
{
	
	IOSPATCH_Apply();
	
	int ret = -1;
	MountTable mnt;

	if (verbose) {
		printf_("IOS(%d) ", CFG.ios);
		if (CFG.ios_mload) printf("mload ");
		else if (CFG.ios_yal) printf("yal ");
	}

	if (CURR_IOS_IDX == CFG.game.ios_idx) {
		//&& is_ios_type(IOS_TYPE_WANIN)) return 0;
		printf("\n");
		return 0;
	}
	
	/*
	mload_need_fat = 0;
	if (strncasecmp(CFG.partition, "fat", 3) == 0) {
		mload_need_fat = 1;
	}
	if (strncasecmp(CFG.partition, "ntfs", 4) == 0) {
		mload_need_fat = 1;
	}
	if (wbfsDev == WBFS_DEVICE_SDHC) {
		if (CFG.ios_mload)
		{
			// wbfs on sd with 222/223 requires fat module
			mload_need_fat = 1;
		}
	}

	if (CURR_IOS_IDX == CFG.game.ios_idx
		&& mload_need_fat == mload_ehc_fat)
	{
		if (verbose) {
			if (mload_ehc_fat) {
				if (strncasecmp(CFG.partition, "ntfs", 4) == 0) {
					printf("[NTFS]");
				} else {
					printf("[FAT]");
				}
			}
			printf("\n\n");
		}
		return 0;
	}
	if (CFG.ios_mload) {
		load_ext_ehc_module(verbose);
	}
	*/

	*mload_ver_str = 0;

	if (verbose) printf(".");

	// Close storage
	UnmountAll(&mnt);
	USBStorage_Deinit();
	SDHC_Close();

	// Close subsystems
	if (subsys) {
		Subsystem_Close();
		WDVD_Close();
	}
	
	Disable_Emu();

	// deinit isfs, inited in get_iosinfo
	ISFS_Deinitialize();

	/* Load Custom IOS */
	dbg_printf("IOS_Reload(%d)\n", CFG.ios);
	usleep(100000);
	ret = IOS_ReloadIOS(CFG.ios);
	usleep(100000);
	if (ret < 0) {
		//if (verbose) {
			printf("\n");
			printf_x(gt("ERROR:"));
			printf("\n");
			printf_(gt("Custom IOS %d could not be loaded! (ret = %d)"), CFG.ios, ret);
			printf("\n");
		//}
		goto err;
	}
	if (verbose) {
		printf(".");
	}

	// mload ehc & dip
	if (CFG.ios_mload) {
		ret = load_ehc_module_ex(verbose);
		if (ret < 0) {
			//if (verbose) {
				printf("\n");
				printf_x(gt("ERROR: Loading EHC module! (%d)"), ret);
				printf("\n");
			//}
			goto err;
		}
	}

	if (is_ios_type(IOS_TYPE_WANIN) && IOS_GetRevision() >= 18) {
		//load_dip_249();
		mk_mload_version();
		mload_close();
	}

	if (verbose) {
		printf(".");
		if (CFG.ios_mload) {
			printf("\n");
			printf_("");
			print_mload_version();
		}
	}

	// re-Initialize Storage
	MountAll(&mnt);
	if (verbose) printf(".");

	// re-initialize subsystems
	if (subsys) {
		// wpad
		Wpad_Init();
		// storage
		if (wbfsDev == WBFS_DEVICE_USB) {
			USBStorage_Init();
		}
		if (wbfsDev == WBFS_DEVICE_SDHC) {
			SDHC_Init();
		}
		// init dip
		ret = Disc_Init();
		if (ret < 0) {
			//if (verbose) {
				printf("\n");
				printf_x(gt("ERROR:"));
				printf("\n");
				printf_(gt("Could not initialize DIP module! (ret = %d)"), ret);
				printf("\n");
			//}
			goto err;
		}
	}
	if (verbose) printf(".\n");
	
	CURR_IOS_IDX = CFG.game.ios_idx;

	return 0;

err:
	if (subsys) {
		Wpad_Init();
		Restart_Wait();
	}
	return ret;
}

void Block_IOS_Reload()
{
	if (CFG.game.block_ios_reload == 0) return;

	if (is_ios_d2x() >= 5) {
		// d2x ios reload block
		int es_fd = IOS_Open("/dev/es", 0);
		if (es_fd < 0) {
			printf("Couldn't open ES module\n");
			sleep(5);
			return;
		}
		static ioctlv vector[0x08] ATTRIBUTE_ALIGN(32);
		static int mode ATTRIBUTE_ALIGN(32);
		static int ios ATTRIBUTE_ALIGN(32);
		mode = 2;
		ios = IOS_GetVersion();
		vector[0].data = &mode;
		vector[0].len = 4;
		vector[1].data = &ios;
		vector[1].len = 4;
		int ret = IOS_Ioctlv(es_fd, 0xA0, 2, 0, vector);
		IOS_Close(es_fd);
		dbg_printf("d2x ios reload block %d: %d\n", ios, ret);
		if (ret < 0) {
			printf_("d2x IOS reload block FAILED!\n");
			return;
		}
		goto out;
	}
	
	// if set to auto(2) and ios != d2x then don't do anything
	if (CFG.game.block_ios_reload == 2) return;

	if (CFG.ios_mload) {
		// hermes ios reload block
		patch_datas[0]=*((u32 *) (dip_plugin+16*4));
		mload_set_ES_ioctlv_vector((void *) patch_datas[0]);
	} else {
		// unsupported
		printf_("IOS Reload Block only supported\nwith d2x and hermes cios\n");
		sleep(3);
		return;
	}
out:
	printf_(gt("IOS Reload: Blocked"));
	printf("\n");
	if (CFG.game.block_ios_reload == 2) return;
	sleep(1);
}

u32 old_title_id = 0;

void get_title_id()
{
	u64 tid;
	if (ES_GetTitleID(&tid) >= 0) {
		old_title_id = (u32)tid;
	} else {
		// tid = (0x00010001ULL << 32) | *(u32 *)0x80000000;
		old_title_id = *(u32*)0x80000000;
	}
	if (old_title_id <= 2)
	{
		// HBC reload stub -> assuming boot from HBC
		if (strncmp("STUBHAXX", (char *)0x80001804, 8) == 0)
		{
			old_title_id = 0xAF1BF516;
		}	
	}
	dbg_printf("channel id: %08X\n", old_title_id);
}

// by davebaol, WiiPower
void d2x_return_to_channel()
{
	if (CFG.return_to == 1) {
		CFG.return_to = old_title_id;
		if (CFG.return_to <= 2) {
			CFG.return_to = 0;
		}
	}
	if (CFG.return_to <= 2) return;
	if (is_ios_d2x() < 4) return;
	dbg_printf("d2x_return_to_channel %08x\n", CFG.return_to);

	static u64 sm_title_id  ATTRIBUTE_ALIGN(32);
	// title id to be launched in place of the system menu
	sm_title_id = (0x00010001ULL << 32) | CFG.return_to;

	int ret;
	signed_blob *buf = NULL;
	u32 filesize;

	// Check if the title exists NeoGamma wants the cIOS to return to
	ret = GetTMD(sm_title_id, &buf, &filesize);
	SAFE_FREE(buf);
	if (ret != 0) {
		disable_return_to_patch = true;
		printf("Invalid return_to_channel=%08x\n", CFG.return_to);
		printf("Try using return_to_channel=auto\n");
		printf("Which will use channel id: %08x\n", old_title_id);
		sleep(5);
		return;
	}

	static ioctlv vector[0x08] ATTRIBUTE_ALIGN(32);

	vector[0].data = &sm_title_id;
	vector[0].len = 8;

	int es_fd = IOS_Open("/dev/es", 0);
	if (es_fd < 0)
	{
		printf("Couldn't open ES module(2)\n");
		sleep(5);
		return;
	}

	ret = IOS_Ioctlv(es_fd, 0xA1, 1, 0, vector);

	IOS_Close(es_fd);

	dbg_printf("d2x return to channel (%08x %d)\n",
			CFG.return_to, ret);
	if (ret >= 0) {
		// success
		disable_return_to_patch = true;
	}
}




u8 BCA_Data[64] ATTRIBUTE_ALIGN(32);
int have_bca_data = 0;

void load_bca_data(u8 *discid)
{
	if (CFG.disable_bca) return;
	char filename[100];
	int size = 0;
	void *buf = NULL;
	// ID6
	snprintf(D_S(filename), "%s/%.6s.bca", USBLOADER_PATH, (char*)discid);
	size = Fat_ReadFile(filename, &buf);
	if (size <= 0) {
		// ID4
		snprintf(D_S(filename), "%s/%.4s.bca", USBLOADER_PATH, (char*)discid);
		size = Fat_ReadFile(filename, &buf);
	}
	if (size < 64) {
		SAFE_FREE(buf);
		return;
	}
	printf_("BCA: %s\n", filename);
	memcpy(BCA_Data, buf, 64);
	have_bca_data = 1;
	SAFE_FREE(buf);
}

void Menu_Confirm_Exit()
{
	printf_h("Press HOME to exit\n");
	printf_h("Press any other button to continue\n");
	if (CFG.home == CFG_HOME_SCRSHOT) {
		CFG.home = CFG_HOME_REBOOT;
	}
	Wpad_WaitButtonsCommon();
}

int insert_bca_data()
{
	if (!have_bca_data) return 0;
	if (!CFG.ios_mload || IOS_GetRevision() < 4)
	{
		printf(gt("ERROR: CIOS222/223 v4 required for BCA"));
		printf("\n");
		sleep(2);
		return -1;
	}
	if(mload_init()<0) {
		printf("%s: BCA\n", gt("ERROR"));
		Menu_Confirm_Exit();
		return -1;
	}
	// offset 15 (bca_data area)
	mload_seek(*((u32 *) (dip_plugin+15*4)), SEEK_SET);
	mload_write(BCA_Data, 64);
	mload_close();
	have_bca_data = 2;
	return 0;
}

int verify_bca_data()
{
	u8 tmp_data[64] ATTRIBUTE_ALIGN(32);
	int ret;
	if (have_bca_data != 2) return 0;
	printf("BCA:");
	memset(tmp_data, 0xff, 64);
	ret = WDVD_Read_Disc_BCA(tmp_data);
	if (ret) {
		printf("%s: %d\n", gt("ERROR"), ret);
		goto fail;
	}
	printf("\n");
	hex_dump3(BCA_Data, 64);
	if (memcmp(BCA_Data, tmp_data, 64) != 0) {
		printf("%s\n", gt("ERROR"));
		goto fail;
	}
	printf("%s\n", gt("OK"));
	return 0;
fail:
	Menu_Confirm_Exit();
	return -1;
}


// WANINKOKO DIP PLUGIN

#if 0
void save_dip()
{
	//int dip_buf[0x5000/sizeof(int)];
	void *dip_buf = memalign(32,0x5000);
	int dip_size = 4000;
	printf("saving dip\n");
	if(mload_init()<0) {
		printf("mload init\n");
		sleep(3);
		return;
	}
	u32 base;
	int size;
	mload_get_load_base(&base, &size);
	printf("base: %08x %x\n", base, size);
	printf("mseek\n");
	mload_seek(0x13800000, SEEK_SET);
	memset(dip_buf, 0, sizeof(dip_buf));
	printf("mread\n");
	mload_read(dip_buf, dip_size);
	printf("fopen\n");
	FILE *f = fopen("sd:/dip.bin", "wb");
	if (!f) {
		printf("fopen\n");
		sleep(3);
		return;
	}
	printf("fwrite\n");
	fwrite(dip_buf, dip_size, 1, f);
	fclose(f);
	printf("dip saved\n");
	mload_close();
	sleep(3);
	printf("unmount\n");
	Fat_UnmountSDHC();
	printf("exit\n");
	exit(0);
}

void try_hello()
{
	int ret;
	printf("mload init\n");
	if(mload_init()<0) {
		sleep(3);
		return;
	}
	u32 base;
	int size;
   	mload_get_load_base(&base, &size);
	printf("base: %08x %x\n", base, size);
	mload_close();
	printf("disc init:\n");
	ret = Disc_Init();
	printf("= %d\n", ret);
	u32 x = 6;
	s32 WDVD_hello(u32 *status);
	ret = WDVD_hello(&x);
	printf("hello: %d %x %d\n", ret, x, x);
	ret = WDVD_hello(&x);
	printf("hello: %d %x %d\n", ret, x, x);
	sleep(1);
	//printf("exit\n");
	//exit(0);
}
#endif

#ifndef size_dip249
// #define size_dip249 5264  // r18-r20
#define size_dip249 5680  // r21
#endif

extern u8 dip_plugin_249[size_dip249];


void load_dip_249()
{
	int ret;
	// waninkoko cios rev18+ support mload
	if (!is_ios_type(IOS_TYPE_WANIN)) return;
	if (IOS_GetRevision() < 18) return;
	// d2x v6 already includes dip+frag
	if (is_ios_d2x() >= 6) return;
	WDVD_Close();
	//printf("[FRAG]");
	if(mload_init()<0) {
		printf("FRAG: mload_init ERROR\n");
		sleep(5);
		return;
	}
	/*
	   u32 base;
	   int size;
	   mload_get_load_base(&base, &size);
	   printf("base: %08x %x\n", base, size);
	   */
	ret = mload_module(dip_plugin_249, size_dip249);
	if (ret < 0) {
		printf("FRAG: load ERROR %d\n", ret);
		sleep(5);
	}
	//printf("load mod: %d\n", ret);
	//mk_mload_version();
	mload_close();
	//printf("OK\n");
	WDVD_Init();
}


int get_ios_type()
{
	switch (IOS_GetVersion()) {
		case 245:
		case 246:
		case 247:
		case 248:
		case 249:
		case 250:
		case 251:
			return IOS_TYPE_WANIN;
		case 222:
		case 223:
			if (IOS_GetRevision() == 1)
				return IOS_TYPE_KWIIRK;
		case 224:
			return IOS_TYPE_HERMES;
	}
	return IOS_TYPE_UNK;
}

int is_ios_type(int type)
{
	return (get_ios_type() == type);
}

int is_ios_d2x()
{
    signed_blob *TMD = NULL;
    u32 TMD_size = 0;
    u64 title_id = TITLE_ID(1, IOS_GetVersion());
    int ret;
    
    ret = GetTMD(title_id, &TMD, &TMD_size);
    if (ret >= 0) {
        iosinfo_t iosinfo;
        ret = get_iosinfo(IOS_GetVersion(), TMD, &iosinfo);
		SAFE_FREE(TMD);
		if (ret) {
            if (strcmp(iosinfo.name, "d2x") == 0) {
                return iosinfo.version;
            }
        }
    }
    
    if (is_ios_type(IOS_TYPE_WANIN)) {
		int rev = IOS_GetRevision();
        if (rev > 21000 && rev < 25000) {
            return rev % 100;
        }
    }
    return 0;
}

s32 GetTMD(u64 TicketID, signed_blob **Output, u32 *Length)
{
    signed_blob* TMD = NULL;

    u32 TMD_Length;
    s32 ret;

    /* Retrieve TMD length */
    ret = ES_GetStoredTMDSize(TicketID, &TMD_Length);
    if (ret < 0)
        return ret;

    /* Allocate memory */
    TMD = (signed_blob*)memalign(32, (TMD_Length+31)&(~31));
    if (!TMD)
        return IPC_ENOMEM;

    /* Retrieve TMD */
    ret = ES_GetStoredTMD(TicketID, TMD, TMD_Length);
    if (ret < 0)
    {
        free(TMD);
        return ret;
    }

    /* Set values */
    *Output = TMD;
    *Length = TMD_Length;

    return 0;
}

s32 checkIOS(u32 IOS)
{
	signed_blob *TMD = NULL;
    tmd *t = NULL;
    u32 TMD_size = 0;
    u64 title_id = 0;
    s32 ret = 0;

    // Get tmd to determine the version of the IOS
    title_id = (((u64)(1) << 32) | (IOS));
    ret = GetTMD(title_id, &TMD, &TMD_size);
    
    if (ret == 0) {
        t = (tmd*)SIGNATURE_PAYLOAD(TMD);
        if (t->title_version == 65280) {
			// stub
			ret = -1;
		}
    } else {
		// not installed
		ret = -2;
	}
    free(TMD);
    return ret;
}

// ios base detection from NeoGamma R9 (by WiiPower)


struct ios_hash_info
{
	u32 slot;
	u32 hash[5];
	char *info;
};

static struct ios_hash_info ios_info[] =
{
	{ 249, {0x20e60607, 0x4e02c484, 0x2bbc5758, 0xee2b40fc, 0x35a68b0a}, "?? rev13a" },
	{ 249, {0x620c57c7, 0xd155b67f, 0xa451e2ba, 0xfb5534d7, 0xaa457878}, "?? rev13b" },
	// rev 17
	{ 249, {0x369047f1, 0x4e37a34f, 0x3cca2936, 0xa60d527b, 0xac141d4a}, "38 rev17" },
	{ 249, {0x58ba3f8b, 0x7f8dccd3, 0x8b6c9109, 0x9a736e80, 0xc6c5ea67}, "38 rev17b" },
	// rev 18
	{ 249, {0x3c968e54, 0x9e915458, 0x9ecc3bda, 0x16d0a0d4, 0x8cac7917}, "37 rev18" },
	{ 249, {0xe811bca8, 0xe1df1e93, 0x779c40e6, 0x2006e807, 0xd4403b97}, "38 rev18" },
	{ 249, {0x697676f0, 0x7a133b19, 0x881f512f, 0x2017b349, 0x6243c037}, "57 rev18" },
	{ 249, {0x34ec540b, 0xd1fb5a5e, 0x4ae7f069, 0xd0a39b9a, 0xb1a1445f}, "60 rev18" },
	{ 249, {0xd98a4dd9, 0xff426ddb, 0x1afebc55, 0x30f75489, 0x40b27ade}, "70 rev18" },
	// rev 19
	{ 249, {0x0a49cd80, 0x6f8f87ff, 0xac9a10aa, 0xefec9c1d, 0x676965b9}, "37 rev19" },
	{ 249, {0x09179764, 0xeecf7f2e, 0x7631e504, 0x13b4b7aa, 0xca5fc1ab}, "38 rev19" },
	{ 249, {0x6010d5cf, 0x396415b7, 0x3c3915e9, 0x83ded6e3, 0x8f418d54}, "57 rev19" },
	{ 249, {0x589d6c4f, 0x6bcbd80a, 0xe768f258, 0xc53a322c, 0xd143f8cd}, "60 rev19" },
	{ 249, {0x8969e0bf, 0x7f9b2391, 0x31ecfd88, 0x1c6f76eb, 0xf9418fe6}, "70 rev19" },
	// rev 20 	
	{ 249, {0x30aeadfe, 0x8b6ea668, 0x446578c7, 0x91f0832e, 0xb33c08ac}, "36 rev20" },
	{ 249, {0xba0461a2, 0xaa26eed0, 0x482c1a7a, 0x59a97d94, 0xa607773e}, "37 rev20" },
	{ 249, {0xb694a33e, 0xf5040583, 0x0d540460, 0x2a450f3c, 0x69a68148}, "38 rev20" },
	{ 249, {0xf6058710, 0xfe78a2d8, 0x44e6397f, 0x14a61501, 0x66c352cf}, "53 rev20" },
	{ 249, {0xfa07fb10, 0x52ffb607, 0xcf1fc572, 0xf94ce42e, 0xa2f5b523}, "55 rev20" },
	{ 249, {0xe30acf09, 0xbcc32544, 0x490aec18, 0xc276cee6, 0x5e5f6bab}, "56 rev20" },
	{ 249, {0x595ef1a3, 0x57d0cd99, 0x21b6bf6b, 0x432f6342, 0x605ae60d}, "57 rev20" },
	{ 249, {0x687a2698, 0x3efe5a08, 0xc01f6ae3, 0x3d8a1637, 0xadab6d48}, "60 rev20" },
	{ 249, {0xea6610e4, 0xa6beae66, 0x887be72d, 0x5da3415b, 0xa470523c}, "61 rev20" },
	{ 249, {0x64e1af0e, 0xf7167fd7, 0x0c696306, 0xa2035b2d, 0x6047c736}, "70 rev20" },
	{ 249, {0x0df93ca9, 0x833cf61f, 0xb3b79277, 0xf4c93cd2, 0xcd8eae17}, "80 rev20" },
	// rev 21
	{ 249, {0x074dfb39, 0x90a5da61, 0x67488616, 0x68ccb747, 0x3a5b59b3}, "36 rev21" },
	{ 249, {0x6956a016, 0x59542728, 0x8d2efade, 0xad8ed01e, 0xe7f9a780}, "37 rev21" },
	{ 249, {0xdc8b23e6, 0x9d95fefe, 0xac10668a, 0x6891a729, 0x2bdfbca0}, "38 rev21" },
	{ 249, {0xaa2cdd40, 0xd628bc2e, 0x96335184, 0x1b51404c, 0x6592b992}, "53 rev21" },
	{ 249, {0x4a3d6d15, 0x014f5216, 0x84d65ffe, 0x6daa0114, 0x973231cf}, "55 rev21" },
	{ 249, {0xca883eb0, 0x3fe8e45c, 0x97cc140c, 0x2e2d7533, 0x5b369ba5}, "56 rev21" },
	{ 249, {0x469831dc, 0x918acc3e, 0x81b58a9a, 0x4493dc2c, 0xaa5e57a0}, "57 rev21" },
	{ 249, {0xe5af138b, 0x029201c7, 0x0c1241e7, 0x9d6a5d43, 0x37a1456a}, "58 rev21" },
	{ 249, {0x0fdee208, 0xf1d031d3, 0x6fedb797, 0xede8d534, 0xd3b77838}, "60 rev21" },
	{ 249, {0xaf588570, 0x13955a32, 0x001296aa, 0x5f30e37f, 0x0be91316}, "61 rev21" },
	{ 249, {0x50deaba2, 0x9328755c, 0x7c2deac8, 0x385ecb49, 0x65ea3b2b}, "70 rev21" },
	{ 249, {0x811b6a0b, 0xe26b9419, 0x7ffd4930, 0xdccd6ed3, 0x6ea2cdd2}, "80 rev21" },
	// hermes
	{ 222, {0xafbfc2c1, 0x28c59142, 0x953b1c6c, 0x809a984f, 0x427c9270}, "38 v4" },
	{ 222, {0xb72b71cd, 0xf42b2730, 0x3b9a4f2c, 0x41128ef9, 0x26f6dbcc}, "38+37 v4" },
	{ 222, {0xd6f1472f, 0x68740b4c, 0xbdf0078d, 0xb8ebb00a, 0x8c9afe2b}, "38+60 v4" },
	// v5.0
	{ 222, {0xfa8ad097, 0x6c18542a, 0x5691bdec, 0xd0c47a6a, 0xbb857b08}, "38 v5.0" },
	{ 223, {0x07b9c8f2, 0xa0dbad4d, 0xa6ee0761, 0x7c371591, 0x4e4c63ec}, "37 v5.0" },
	{ 223, {0x0d0a12e0, 0x16065574, 0x844e39b4, 0x2d2dbdf1, 0x5b497869}, "57 v5.0" },
	{ 223, {0x0aa83bf0, 0x8fbd610f, 0x87bae3c1, 0x06f43826, 0x39524429}, "60 v5.0" },
	// v5.1
	{ 222, {0xf865dfa5, 0xa909e4fb, 0x345f48ea, 0x804f5a64, 0x3704dd5a}, "38 v5.1" },
	{ 223, {0x889511aa, 0xdde5c849, 0xe30e6d20, 0x9687c95a, 0xb935342b}, "37 v5.1" },
	{ 223, {0x0584100d, 0xc3364080, 0xc3b8ffd0, 0x8c351aee, 0xf4632159}, "57 v5.1" },
	{ 223, {0x1cb7981c, 0xd791a2bf, 0x736395d6, 0x0224e181, 0x38324674}, "60 v5.1" },
	// modmii special
	{ 249, {0x00dc1209, 0x944c39db, 0x59eec2ab, 0x0212b86c, 0x7076cd3b}, "56 r21+r19 modmii" },
	{ 249, {0x00298dc2, 0x58fdae1a, 0x233b0958, 0x17269047, 0x8188633d}, "57 r21+r19 modmii" },
	// d2x v1
	{ 249, {0x00ed2993, 0x0bae0cb2, 0xc7e430a2, 0x2e6eaf18, 0x156a9a70}, "37 r21-d2x-v1" },
	{ 249, {0x00d74607, 0x2d3fe23e, 0x47ecb019, 0x0f5d4380, 0x37ea6b50}, "38 r21-d2x-v1" },
	{ 249, {0x003d11ce, 0x4eb3b8bb, 0xe2c02fda, 0x5f879e74, 0x44a257de}, "56 r21-d2x-v1" },
	{ 249, {0x00ba4b4f, 0x27803366, 0x8d9121fa, 0x954eb5d5, 0x92242691}, "57 r21-d2x-v1" },
	// d2x v2
	{ 249, {0x00475dce, 0x81a744dd, 0xf24157e4, 0x870fa3d8, 0xfc39fa8a}, "37 r21-d2x-v2" },
	{ 249, {0x00711af6, 0x017c48d4, 0xea0267d3, 0x1666600b, 0x38a8fe16}, "38 r21-d2x-v2" },
	{ 249, {0x00815782, 0x8604fe34, 0x474653b5, 0xbdbc5651, 0xf43b427a}, "56 r21-d2x-v2" },
	{ 249, {0x00d8e857, 0x8c96eb52, 0x4d006568, 0x95cf5415, 0xdb7712e8}, "57 r21-d2x-v2" },
	// d2x v3
	{ 249, {0x0054e91c, 0xe022e307, 0x26d72e03, 0x53b6e157, 0x42adbe49}, "37 r21-d2x-v3" },
	{ 249, {0x000bd035, 0xe649cc22, 0x8bf647c5, 0xe0710e6a, 0xd79a5355}, "38 r21-d2x-v3" },
	{ 249, {0x00b8ca9c, 0x9b4053a3, 0x8de94a72, 0x1192fce5, 0x098e7404}, "56 r21-d2x-v3" },
	{ 249, {0x00e8e05f, 0x2aa4cd1e, 0x8c8f5529, 0x498f259b, 0xfa41258e}, "57 r21-d2x-v3" },
	{ 249, {0x0028dbf1, 0x3827be46, 0x28c82eb2, 0x122325c3, 0xc72dbd46}, "58 r21-d2x-v3" },
	// d2x v3 (r21003)
	{ 249, {0x006ec958, 0xbc59364d, 0x5b2f58a0, 0xf8feeac4, 0x89a0b697}, "37 r21-d2x-v3" },
	{ 249, {0x00fe6ad4, 0xbbf9a5e2, 0xeb2b0110, 0xc1fddbdf, 0xfb634350}, "38 r21-d2x-v3" },
	{ 249, {0x00ecc3a8, 0xec2d3b64, 0x506314e3, 0x740274ef, 0x6505cc75}, "56 r21-d2x-v3" },
	{ 249, {0x008e68fd, 0xa1221185, 0xc09a1a26, 0xfeb09ead, 0xf375c2f2}, "57 r21-d2x-v3" },
	{ 249, {0x006237ad, 0xda4cb0e1, 0xa97e4b41, 0xf1bb24a2, 0xd663b7f7}, "58 r21-d2x-v3" },

	// d2x v4 (r21004) beta 2
	{ 249, {0x00733fa4, 0x1d3e6245, 0xb0311e24, 0x675868b1, 0x353d882c}, "37 r21-d2x-v4-b2" },
	{ 249, {0x007b9fca, 0x0a6f40c5, 0xccd37b25, 0x1c49064b, 0x1041ddb3}, "38 r21-d2x-v4-b2" },
	{ 249, {0x000d243c, 0x07a183df, 0x0592ce22, 0x2bb81b46, 0x64cce4a7}, "56 r21-d2x-v4-b2" },
	{ 249, {0x00c7b39a, 0xed42a4a0, 0xcc125669, 0xf23c1f6e, 0x2244cb9b}, "57 r21-d2x-v4-b2" },
	{ 249, {0x00120cb0, 0x4cb9b4b1, 0xbe02e0e6, 0x30bfcb95, 0xfbfcaba5}, "58 r21-d2x-v4-b2" },

	// d2x v4 (r21004) release / hb installer
	{ 249, {0xba124a8e, 0x73f5b2cb, 0x5e2439be, 0x76629335, 0xa3f36418}, "37 r21-d2x-v4" },
	{ 249, {0x2e5b15a4, 0x6e638735, 0x6d3d7403, 0xa78cdcc0, 0xe62a106b}, "38 r21-d2x-v4" },
	{ 249, {0xf13c731c, 0x3d77c021, 0x48c3119f, 0x7679939f, 0xbde8857f}, "53 r21-d2x-v4" },
	{ 249, {0x292464d3, 0xf94267d3, 0x849fd15c, 0x03200bde, 0xe8e0d6e8}, "55 r21-d2x-v4" },
	{ 249, {0xfefb9f74, 0x9961dd76, 0xe5416e0f, 0x7df6a95b, 0x923d2561}, "56 r21-d2x-v4" },
	{ 249, {0xb7272b2f, 0x72bdab83, 0x31d0639f, 0xfabc719d, 0xad818d91}, "57 r21-d2x-v4" },
	{ 249, {0x91a7d59f, 0x4ae0671a, 0x9bdf2593, 0xf7535426, 0x85af4073}, "58 r21-d2x-v4" },

	// d2x v4 (r21004) release / modmii
	{ 249, {0x00de8cad, 0x17183381, 0x889a1299, 0x834a85eb, 0x45b59e05}, "37 r21-d2x-v4" },
	{ 249, {0x0007e951, 0x173de10f, 0x0324b33f, 0xaa1f93c7, 0x28461fbe}, "38 r21-d2x-v4" },
	{ 249, {0x00f92f4f, 0x8f989389, 0xcd9e2732, 0x7752e50b, 0xa47fde40}, "53 r21-d2x-v4" },
	{ 249, {0x0056c457, 0x99c9a90f, 0xf0d9d994, 0x0724362a, 0xbe8ac29f}, "55 r21-d2x-v4" },
	{ 249, {0x00a1872f, 0x412f94cf, 0xd90c818b, 0xde15681e, 0x63c41b52}, "56 r21-d2x-v4" },
	{ 249, {0x00b06c85, 0xab7a94c2, 0x674785fc, 0x8f133335, 0xc9b84d49}, "57 r21-d2x-v4" },
	{ 249, {0x000530f4, 0x0c472b29, 0xb8f22f5a, 0x752b0613, 0x109bace1}, "58 r21-d2x-v4" },

	/*
	// modmii 249
	{ 249, {0x005b6439, 0xf4a2e0b7, 0xfce05f75, 0xdb1a66ce, 0x7a0811c1}, "38 r17 modmii" },	
	{ 249, {0x007da65a, 0x1b57b279, 0x06a5443f, 0xc61fd6cb, 0x4ea9866a}, "37 r19 modmii" },
	{ 249, {0x00cb38fc, 0x3abc550f, 0xc128f0aa, 0xa1dc96b8, 0x3d3ed542}, "38 r19 modmii" },
	{ 249, {0x00cc7ee1, 0xc1af6682, 0x8a5a2b6f, 0xc417eb3d, 0x607377ec}, "57 r19 modmii" },
	{ 249, {0x00e96e3f, 0xe861fd7a, 0x092d0fcb, 0xa65af414, 0xd375d6bb}, "38 r20 modmii" }, 
	{ 249, {0x009e5dde, 0x2589d21d, 0x4db9dfaa, 0x765c4279, 0xc4a5ba75}, "56 r20 modmii" },
	{ 249, {0x003c1bd8, 0x7830d7dc, 0x79e74949, 0x9609bb13, 0x4b5c5072}, "57 r20 modmii" },
	{ 249, {0x000f5724, 0xe6002c66, 0x6718313c, 0x8c4ec895, 0x478480ce}, "37 r21 modmii" },
	{ 249, {0x00c61cad, 0x30328d5d, 0xe69eb487, 0x27f77d5e, 0xc3c47d0d}, "38 r21 modmii" },
	{ 249, {0x00994d20, 0xbe74e78b, 0x00d4f00c, 0xfc9da208, 0x262c5f90}, "56 r21 modmii" },
	{ 249, {0x009fb539, 0x5a56f778, 0x329fbfd7, 0xc3a8ff58, 0x6fdb010b}, "57 r21 modmii" },
	{ 249, {0x002a7dfe, 0xdc36d6d9, 0x9af35191, 0x54862ecb, 0xd8087cb3}, "58 r21 modmii" },
	// modmii 250
	{ 250, {0x00186ce5, 0xe6ced602, 0x552e621d, 0xaf882fb8, 0xa479e47b}, "37 r19 modmii" },
	{ 250, {0x00e2ea30, 0x56c4568d, 0x4f2c165d, 0xc00471bd, 0x6939c9f1}, "38 r19 modmii" },
	{ 250, {0x005849db, 0xa1fc4519, 0x530a963b, 0x31810e9e, 0xea1f207a}, "57 r19 modmii" },
	{ 250, {0x00641c3c, 0xa7346fa0, 0x0fa518a6, 0xeeac8097, 0x60eb2e87}, "38 r20 modmii" },
	{ 250, {0x009c97f0, 0xda7da42e, 0xd3320862, 0xb33d22db, 0xdc80b9e2}, "56 r20 modmii" },
	{ 250, {0x006d76ce, 0x0a294191, 0x62c9705d, 0x355ec87b, 0xff152dd5}, "57 r20 modmii" },
	{ 250, {0x006937f9, 0x8fdccb08, 0x4f9396b3, 0xc91b8761, 0x8ff1f1bb}, "37 r21 modmii" },
	{ 250, {0x004ba2f2, 0x9e4269c6, 0xe5d91fd0, 0x2e6410db, 0x772b1986}, "38 r21 modmii" },
	{ 250, {0x00bca8d8, 0x82105397, 0xb140ddb2, 0x8774d57a, 0x66418504}, "56 r21 modmii" },
	{ 250, {0x000ac379, 0x719a8850, 0x469445e7, 0xcf51108e, 0xb832d628}, "57 r21 modmii" },
	{ 250, {0x008567af, 0xe41cdb0b, 0xce85dc29, 0x10970d12, 0xe0b608f3}, "58 r21 modmii" },
	// modmii hermes
	{ 222, {0x00ec3a7b, 0xc9869c77, 0x013cf962, 0x4eef5726, 0xda9d1488}, "38 v4 modmii" },
	{ 223, {0x0073251f, 0x88b53db8, 0x390234af, 0x26910ff6, 0x041f3d3f}, "38+37 v4 modmii" },
	{ 222, {0x00d801b3, 0xe280e6e2, 0x1c99b236, 0x470ed5a9, 0xf3544f86}, "38 v5.0 modmii" },
	{ 223, {0x007d38f5, 0xb6b841b4, 0xf57579db, 0xa47526fe, 0xc3b3d12a}, "37 v5.0 modmii" },
	{ 223, {0x00f08071, 0x2672d68b, 0xc63bed5a, 0x06ae3b3c, 0xcff913d7}, "57 v5.0 modmii" },
	*/
};

const int ios_info_number = sizeof(ios_info) / sizeof(struct ios_hash_info);

s32 brute_tmd(tmd *p_tmd) 
{
	u16 fill;
	for(fill=0; fill<65535; fill++) 
	{
		p_tmd->fill3 = fill;
		sha1 hash;
		SHA1((u8 *)p_tmd, TMD_SIZE(p_tmd), hash);;
		if (hash[0]==0) 
		{
			return 0;
		}
	}
	return -1;
}

s32 brute_modmii(signed_blob *TMD, int size, unsigned char *hash) 
{
	u16 fill;
	tmd *p_tmd = (tmd*)SIGNATURE_PAYLOAD(TMD);
	for(fill=0; fill<65535; fill++) 
	{
		p_tmd->fill3 = fill;
		SHA1((u8 *)TMD, size, hash);;
		if (hash[0]==0) 
		{
			return 0;
		}
	}
	return -1;
}

s32 read_file_from_nand(char *filepath, u8 **buffer, u32 *filesize)
{
	s32 Fd;
	int ret;

	if (buffer == NULL)
	{
		//printf("NULL Pointer\n");
		return -1;
	}

	Fd = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (Fd < 0)
	{
		//printf("ISFS_Open %s failed %d\n", filepath, Fd);
		return Fd;
	}

	fstats *status;
	status = memalign(32, sizeof(fstats));
	if (status == NULL)
	{
		//printf("Out of memory for status\n");
		return -1;
	}
	
	ret = ISFS_GetFileStats(Fd, status);
	if (ret < 0)
	{
		//printf("ISFS_GetFileStats failed %d\n", ret);
		ISFS_Close(Fd);
		free(status);
		return -1;
	}
	
	*buffer = memalign(32, (status->file_length+31)&(~31));
	if (*buffer == NULL)
	{
		//printf("Out of memory for buffer\n");
		ISFS_Close(Fd);
		free(status);
		return -1;
	}
		
	ret = ISFS_Read(Fd, *buffer, status->file_length);
	if (ret < 0)
	{
		//printf("ISFS_Read failed %d\n", ret);
		ISFS_Close(Fd);
		free(status);
		free(*buffer);
		return ret;
	}
	
	ISFS_Close(Fd);

	*filesize = status->file_length;
	free(status);

	if (*filesize > 0)
	{
		DCFlushRange(*buffer, *filesize);
		ICInvalidateRange(*buffer, *filesize);
	}

	return 0;
}

bool get_iosinfo(int ios, signed_blob *TMD, iosinfo_t *iosinfo)
{
	char filepath[ISFS_MAXPATH] ATTRIBUTE_ALIGN(0x20);
	u8 *buffer = NULL;
	u32 filesize;
	int ret;
	bool retval = false;

	ISFS_Initialize();
	sprintf(filepath, "/title/%08x/%08x/content/%08x.app",
			0x00000001, ios, *(u8 *)((u32)TMD+0x1E7));
	ret = read_file_from_nand(filepath, &buffer, &filesize);
	// ISFS_Deinitialize();
	// If executed, it causes Castlevania to freeze as soon as the game "sees" a wii mote
	// It is executed now before an IOS Reload is executed
	if (ret >= 0 && buffer) {
		memcpy(iosinfo, buffer, sizeof(*iosinfo));
		if (iosinfo->magicword == 0x1ee7c105 && iosinfo->magicversion == 1) {
			retval = true;
		}
	}
	SAFE_FREE(buffer);
	return retval;
}

u8 get_base_ios_from_tmd(int ios_slot, u32 *version) {
	u8 retValue = 0;
	signed_blob *TMD = NULL;
	tmd *t = NULL;
	u32 TMD_size = 0;
	u64 title_id = TITLE_ID(1, ios_slot);
	u32 i;
	int retry_count = 0;
	sha1 hash;
	int ret;
	int is_modmii = 0;

	//static char default_info[32];
	//sprintf(default_info, "IOS%u (Rev %u)", IOS_GetVersion(), IOS_GetRevision());
	//info = (char *)default_info;

	ret = GetTMD(title_id, &TMD, &TMD_size);
	if (ret != 0) goto out;

	t = (tmd*)SIGNATURE_PAYLOAD(TMD);

	dbg_printf("\ntmd id: %llx %x-%x t: %x v: %d",
			t->title_id, TITLE_HIGH(t->title_id), TITLE_LOW(t->title_id),
			t->title_type, t->title_version);
	if (version) *version = t->title_version;

	// safety check
	if (t->title_id != title_id) goto out;

	iosinfo_t iosinfo;
	if (get_iosinfo(ios_slot, TMD, &iosinfo)) {
		//sprintf(info, "%s%uv%u%s (%u)", iosinfo->name, iosinfo->baseios, iosinfo->version, iosinfo->versionstring, CIOS_VERSION);				
		// Example: "d2x56v5beta2 (249)"
		// "56 r21-d2x-v4"
		retValue = iosinfo.baseios;			
		goto out;
	}

	// modmii check
	// brute match?
	signed_blob *TMD_copy = malloc(TMD_size);
	memcpy(TMD_copy, TMD, TMD_size);
	tmd *tt = (tmd*)SIGNATURE_PAYLOAD(TMD_copy);
	brute_tmd(tt);
	int match = memcmp(TMD, TMD_copy, TMD_size) == 0;
	if (!match) dbg_printf("\nbrute match: %d %u %u\n", match, t->fill3, tt->fill3);
	SAFE_FREE(TMD_copy);
	SHA1((u8 *)TMD, TMD_size, hash);
	if (!match && hash[0] == 0) {
		is_modmii = 1;
	}

retry:;
	if (retry_count) {
		if (retry_count > 100) goto out;
		brute_tmd(t);
	}
	retry_count++;

	SHA1((u8 *)TMD, TMD_size, hash);

	int mm;
	for (mm = 0; mm < 2; mm++) {
		for (i = 0; i < ios_info_number; i++)
		{
			if (ios_info[i].slot != TITLE_LOW(t->title_id)) continue;
			if (memcmp((void *)hash, &ios_info[i].hash, sizeof(sha1)) == 0)
			{
				retValue = atoi(ios_info[i].info);
				break;
			}
		}
		if (retValue == 0 && is_modmii) {
			// use alternative brute hash
			// to search for modmii hashes in table
			brute_modmii(TMD, TMD_size, hash);
		} else {
			break;
		}
	}

	if (retValue == 0) {
		if (is_modmii == 1) {
			is_modmii++;
			// if modmii, first do the brute_tmd
			goto retry;
		}
		// patch title id, so hash matches
		if (ios_slot >= 245 && ios_slot <= 251) {
			if (t->title_id != TITLE_ID(1, 249)) {
				t->title_id = TITLE_ID(1, 249);
				goto retry;
			}
			if (ios_slot == 250) {
				// ios 250 has a fixed version of 65535 patch to a lower one
				if (t->title_version > 13) {
					if (t->title_version > 21) {
						t->title_version = 21;
					} else {
						t->title_version--;
					}
					goto retry;
				}
			}
		}
		if (t->title_id == TITLE_ID(1, 224)) {
			t->title_id = TITLE_ID(1, 223);
			goto retry;
		}
		if (t->title_id == TITLE_ID(1, 223)) {
			t->title_id = TITLE_ID(1, 222);
			goto retry;
		}
	}

out:
	SAFE_FREE(TMD);
    return retValue;
}

char* get_iosx_info_from_tmd(int ios_slot, u32 *version)
{
	signed_blob *TMD = NULL;
	tmd *t = NULL;
	u32 TMD_size = 0;
	u64 title_id = TITLE_ID(1, ios_slot);
	u32 i;
	int retry_count = 0;
	sha1 hash;
	int ret;
	char *info = NULL;
	static char info_str[32];
	int is_modmii = 0;

	//static char default_info[32];
	//sprintf(default_info, "IOS%u (Rev %u)", IOS_GetVersion(), IOS_GetRevision());
	//info = (char *)default_info;

	ret = GetTMD(title_id, &TMD, &TMD_size);
	if (ret != 0) goto out;

	t = (tmd*)SIGNATURE_PAYLOAD(TMD);

	dbg_printf("\ntmd id: %llx %x-%x t: %x v: %d",
			t->title_id, TITLE_HIGH(t->title_id), TITLE_LOW(t->title_id),
			t->title_type, t->title_version);
	if (version) *version = t->title_version;

	// safety check
	if (t->title_id != title_id) goto out;

	iosinfo_t iosinfo;
	if (get_iosinfo(ios_slot, TMD, &iosinfo)) {
		//sprintf(info, "%s%uv%u%s (%u)", iosinfo->name, iosinfo->baseios, iosinfo->version, iosinfo->versionstring, CIOS_VERSION);				
		// Example: "d2x56v5beta2 (249)"
		// "56 r21-d2x-v4"
		info = info_str;
		sprintf(info, "%u %s v%u%s", iosinfo.baseios, iosinfo.name,
				iosinfo.version, iosinfo.versionstring);				
		goto out;
	}

	// modmii check
	// brute match?
	signed_blob *TMD_copy = malloc(TMD_size);
	memcpy(TMD_copy, TMD, TMD_size);
	tmd *tt = (tmd*)SIGNATURE_PAYLOAD(TMD_copy);
	brute_tmd(tt);
	int match = memcmp(TMD, TMD_copy, TMD_size) == 0;
	if (!match) dbg_printf("\nbrute match: %d %u %u\n", match, t->fill3, tt->fill3);
	SAFE_FREE(TMD_copy);
	SHA1((u8 *)TMD, TMD_size, hash);
	if (!match && hash[0] == 0) {
		is_modmii = 1;
	}

retry:;
	if (retry_count) {
		if (retry_count > 100) goto out;
		brute_tmd(t);
	}
	retry_count++;

	SHA1((u8 *)TMD, TMD_size, hash);

	int mm;
	for (mm = 0; mm < 2; mm++) {
		for (i = 0; i < ios_info_number; i++)
		{
			if (ios_info[i].slot != TITLE_LOW(t->title_id)) continue;
			if (memcmp((void *)hash, &ios_info[i].hash, sizeof(sha1)) == 0)
			{
				info = ios_info[i].info;
				break;
			}
		}
		if (info == NULL && is_modmii) {
			// use alternative brute hash
			// to search for modmii hashes in table
			brute_modmii(TMD, TMD_size, hash);
		} else {
			break;
		}
	}

	if (info == NULL) {
		if (is_modmii == 1) {
			is_modmii++;
			// if modmii, first do the brute_tmd
			goto retry;
		}
		// patch title id, so hash matches
		if (ios_slot >= 245 && ios_slot <= 251) {
			if (t->title_id != TITLE_ID(1, 249)) {
				t->title_id = TITLE_ID(1, 249);
				goto retry;
			}
			if (ios_slot == 250) {
				// ios 250 has a fixed version of 65535 patch to a lower one
				if (t->title_version > 13) {
					if (t->title_version > 21) {
						t->title_version = 21;
					} else {
						t->title_version--;
					}
					goto retry;
				}
			}
		}
		if (t->title_id == TITLE_ID(1, 224)) {
			t->title_id = TITLE_ID(1, 223);
			goto retry;
		}
		if (t->title_id == TITLE_ID(1, 223)) {
			t->title_id = TITLE_ID(1, 222);
			goto retry;
		}
	}

	out:
	SAFE_FREE(TMD);
	if (info && is_modmii) {
		if (!strstr(info, "modmii")) {
			strcpy(info_str, info);
			strcat(info_str, " modmii");
			info = info_str;
		}
	}
    return info;
}

void fill_base_array() {
	int i = 0;

	ISFS_Initialize();
	
	for (i = 0; i < 10; i++) {
		cIOS_base[i] = 0;
	}
	
	cIOS_base[0] = get_base_ios_from_tmd(245, NULL);// CFG_IOS_245
	cIOS_base[1] = get_base_ios_from_tmd(246, NULL);// CFG_IOS_246
	cIOS_base[2] = get_base_ios_from_tmd(247, NULL);// CFG_IOS_247
	cIOS_base[3] = get_base_ios_from_tmd(248, NULL);// CFG_IOS_248
	cIOS_base[4] = get_base_ios_from_tmd(249, NULL);// CFG_IOS_249
	cIOS_base[5] = get_base_ios_from_tmd(250, NULL);// CFG_IOS_250
	cIOS_base[6] = get_base_ios_from_tmd(251, NULL);// CFG_IOS_251
	cIOS_base[7] = get_base_ios_from_tmd(222, NULL);// CFG_IOS_222
	cIOS_base[8] = get_base_ios_from_tmd(223, NULL);// CFG_IOS_223
	cIOS_base[9] = get_base_ios_from_tmd(224, NULL);// CFG_IOS_224
}

char* get_ios_info_from_tmd()
{
	return get_iosx_info_from_tmd(IOS_GetVersion(), NULL);
}

void get_all_ios_info_str(char *str, int size)
{
	int i;
	char *info;
	int ret;
	for (i=222; i<=251; i++) {
		if (i > 224 && i < 245) continue;
		snprintf(str, size, "IOS%d ", i);
		str_seek_end(&str, &size);
		ret = checkIOS(i);
		if (ret == -2) {
			snprintf(str, size, ": not installed\n");
		} else if (ret == -1) {
			snprintf(str, size, ": stub\n");
		} else {
			u32 ver;
			info = get_iosx_info_from_tmd(i, &ver);
			snprintf(str, size, "Base: %s (r%u)\n", info ? info : "??", ver);
		}
		str_seek_end(&str, &size);
	}
	/*
	isfs_stats_t st ATTRIBUTE_ALIGN(32);
	memset(&st, 0, sizeof(st));
	ret = ISFS_GetStats(&st);
	snprintf(str, size, "%d NAND Free Blocks: %d %d %d\n",
			ret, st.free_blocks, st.block_size, st.used_blocks);
	*/
}

void print_all_ios_info_str(char *str, int size)
{
	static int got_ios = 0;
	static char ios_info_str[400];
	if (!got_ios) {
		get_all_ios_info_str(ios_info_str, sizeof(ios_info_str));
		got_ios = 1;
	}
	strcopy(str, ios_info_str, size);
}

void print_all_ios_info(FILE *f)
{
	char str[400];
	print_all_ios_info_str(str, sizeof(str));
	fprintf(f, "%s", str);
}

char* get_ios_tmd_hash_str(char *str)
{
	signed_blob *TMD = NULL;
	tmd *t = NULL;
	u32 TMD_size = 0;
	char *info = NULL;

	int ret = GetTMD((((u64)(1) << 32) | (IOS_GetVersion())), &TMD, &TMD_size);
	if (ret != 0) goto out;
	t = (tmd*)SIGNATURE_PAYLOAD(TMD);
	dbg_printf("\ntmd id: %llx %x-%x t: %x v: %d",
			t->title_id, TITLE_HIGH(t->title_id), TITLE_LOW(t->title_id),
			t->title_type, t->title_version);
	// safety check
	if (t->title_id != TITLE_ID(1, IOS_GetVersion())) goto out;
	sha1 hash;
	SHA1((u8 *)TMD, TMD_size, hash);
	int *x = (int*)&hash;
	sprintf(str, "{0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x},",
		x[0], x[1], x[2], x[3], x[4]);
	info = str;
	out:
	SAFE_FREE(TMD);
    return info;
}

bool shadow_mload()
{
	if (!is_ios_type(IOS_TYPE_HERMES)) return false;
	int v51 = (5 << 4) | 1;
	if (mload_ver >= v51) {
		// shadow /dev/mload supported in hermes cios v5.1
		printf_("[shadow ");
		//IOS_Open("/dev/usb123/OFF",0);// this disables ehc completely
		IOS_Open("/dev/mload/OFF",0);
		printf("mload]\n");
		return true;
	}
	return false;
}

u16 get_miosinfo()
{
	u16 mios = CFG_MIOS;
	
	struct tm time;

	// Timestamp of DML r52
	strptime("Mar 7 2012 19:36:06", "%b %d %Y %H:%M:%S", &time);
	const time_t dml_r52_time = mktime(&time);

	// Timestamp of DML 1.2
	strptime("Apr 24 2012 19:44:08", "%b %d %Y %H:%M:%S", &time);
	const time_t dml_1_2_time = mktime(&time);

	// Timestamp of DML 1.4b
	//strptime("May  7 2012 21:12:47", "%b %d %Y %H:%M:%S", &time);
	//const time_t dml_1_4b_time = mktime(&time);

	// Timestamp of DML 1.5
	//strptime("Jun 14 2012 00:05:09", "%b %d %Y %H:%M:%S", &time);
	//const time_t dml_1_5_time = mktime(&time);

	// Timestamp of DM 2.0
	//strptime("Jun 23 2012 19:43:21", "%b %d %Y %H:%M:%S", &time);
	//const time_t dm_2_0_time = mktime(&time);

	// Timestamp of DM 2.1
	strptime("Jul 17 2012 11:25:35", "%b %d %Y %H:%M:%S", &time);
	const time_t dm_2_1_time = mktime(&time);

	// Timestamp of DM 2.2 initial release
	strptime("Jul 18 2012 16:57:47", "%b %d %Y %H:%M:%S", &time);
	const time_t dm_2_2_time = mktime(&time);

	// Timestamp of DM 2.2 update2
	//strptime("Jul 20 2012 14:49:47", "%b %d %Y %H:%M:%S", &time);
	//const time_t dm_2_2_2_time = mktime(&time);

	// Timestamp of DML 2.2 initial release
	strptime("Aug  6 2012 15:19:17", "%b %d %Y %H:%M:%S", &time);
	const time_t dml_2_2_time = mktime(&time);

	// Timestamp of DML 2.2 update1
	//strptime("Aug 13 2012 00:12:46", "%b %d %Y %H:%M:%S", &time);
	//const time_t dml_2_2_1_time = mktime(&time);

	// Timestamp of DML 2.3 mirror link
	//strptime("Sep 24 2012 13:13:42", "%b %d %Y %H:%M:%S", &time);
	//const time_t dml_2_3m_time = mktime(&time);

	// Timestamp of DM 2.3
	//strptime("Sep 24 2012 15:51:54", "%b %d %Y %H:%M:%S", &time);
	//const time_t dm_2_3_time = mktime(&time);

	// Timestamp of DML 2.3 main link
	//strptime("Sep 25 2012 03:03:41", "%b %d %Y %H:%M:%S", &time);
	//const time_t dml_2_3_time = mktime(&time);

	// Timestamp of DM 2.4
	//strptime("Oct 21 2012 22:57:12", "%b %d %Y %H:%M:%S", &time);
	//const time_t dm_2_4_time = mktime(&time);

	// Timestamp of DML 2.4
	//strptime("Oct 21 2012 22:57:17", "%b %d %Y %H:%M:%S", &time);
	//const time_t dml_2_4_time = mktime(&time);

	// Timestamp of DM 2.5
	//strptime("Nov  9 2012 21:18:52", "%b %d %Y %H:%M:%S", &time);
	//const time_t dm_2_5_time = mktime(&time);

	// Timestamp of DML 2.5
	//strptime("Nov  9 2012 21:18:56", "%b %d %Y %H:%M:%S", &time);
	//const time_t dml_2_5_time = mktime(&time);
	
	// Timestamp of DM 2.6.0	// Dec  1 2012 01:52:53
 	// Timestamp of DML 2.6		// Dec  1 2012 16:22:29
 	// Timestamp of DM 2.6.1	// Dec  1 2012 16:42:34
 	// Timestamp of DM 2.7		// Feb 20 2013 14:54:33
 	// Timestamp of DML 2.7		// Feb 21 2013 03:13:49
	// Timestamp of DML 2.8		// Feb 24 2013 13:30:29
 	// Timestamp of DM 2.8		// Feb 24 2013 14:17:03
	// Timestamp of DML 2.9		// Apr  5 2013 18:20:33
	// Timestamp of DM 2.9		// Apr  5 2013 18:29:35    

	// Timestamp of DML 2.10	// May 24 2013 18:51:58
	strptime("May 24 2013 18:51:58", "%b %d %Y %H:%M:%S", &time);
	const time_t dml_2_10_time = mktime(&time);

	// Timestamp of DM 2.10		// May 24 2013 21:22:22    
	strptime("May 24 2013 21:22:22", "%b %d %Y %H:%M:%S", &time);
	const time_t dm_2_10_time = mktime(&time);

	u32 size = 0;
	u32 i = 0;
	s32 ret = 0;
	u8 *appfile = NULL;

	ISFS_Initialize();
	
	dbg_printf("Reading 0000000c.app from MIOS\n");
	ret = read_file_from_nand("/title/00000001/00000101/content/0000000c.app", &appfile, &size);
	if(ret >= 0 && appfile)
	{
		dbg_printf("Scanning 0000000c.app from MIOS\n");
		for(i = 0; i < size; ++i) 
		{
			if((*(vu32*)(appfile+i)) == 0x44494F53 && (*(vu32*)(appfile+i+5)) == 0x4D494F53) //DIOS MIOS
			{
				if(*(vu32*)(appfile+i+10) == 0x4C697465) //Lite
				{
					char *buffer = (char*)(appfile+i+31);
					
					strptime(buffer, "%b %d %Y %H:%M:%S", &time);
					time_t unixTime = mktime(&time);
					
					strcpy(dm_boot_drive, "sd:");
					mios = CFG_DML_R52;
					sprintf(DIOS_MIOS_INFO, "DIOS MIOS Lite\n%s", buffer);
					dbg_printf("\nMIOS is %s", DIOS_MIOS_INFO);
					
					if (difftime(unixTime, dml_2_10_time) >= 0) {
						dbg_printf("\nMIOS is DIOS MIOS Lite 2.10+\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS Lite 2.10+\n%s\n", buffer);
						mios = CFG_DM_2_2;
					} else if (difftime(unixTime, dml_2_2_time) >= 0) {
						dbg_printf("\nMIOS is DIOS MIOS Lite 2.2+\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS Lite 2.2+\n%s\n", buffer);
						mios = CFG_DM_2_2;
					} else if (difftime(unixTime, dml_1_2_time) >= 0) {
						dbg_printf("\nMIOS is DIOS MIOS Lite 1.2+\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS Lite 1.2+\n%s\n", buffer);
						mios = CFG_DML_1_2;
					} else if (difftime(unixTime, dml_r52_time) >= 0) {
						dbg_printf("\nMIOS is DIOS MIOS Lite r52+\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS Lite r52+\n%s\n", buffer);
						mios = CFG_DML_R52;
					} else {
						dbg_printf("\nMIOS is DIOS MIOS Lite r51-\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS Lite r51-\n%s\n", buffer);
						mios = CFG_DML_R51;
					}
					
					break;
				} else {
					char *buffer = (char*)(appfile+i+27);
					
					strcpy(dm_boot_drive, "usb:");
					mios = CFG_DM_2_0;
					sprintf(DIOS_MIOS_INFO, "DIOS MIOS\n%s", buffer);
					dbg_printf("\nMIOS is %s", DIOS_MIOS_INFO);
					
					strptime(buffer, "%b %d %Y %H:%M:%S", &time);
					time_t unixTime = mktime(&time);
					
					if (difftime(unixTime, dm_2_10_time) >= 0) {
						dbg_printf("\nMIOS is DIOS MIOS 2.10+\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS 2.10+\n%s\n", buffer);
						mios = CFG_DM_2_2;
					} else if (difftime(unixTime, dm_2_2_time) >= 0) {
						dbg_printf("\nMIOS is DIOS MIOS 2.2+\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS 2.2+\n%s\n", buffer);
						mios = CFG_DM_2_2;
					} else if (difftime(unixTime, dm_2_1_time) >= 0) {
						dbg_printf("\nMIOS is DIOS MIOS 2.1\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS 2.1\n%s\n", buffer);
						mios = CFG_DM_2_1;
					} else {
						dbg_printf("\nMIOS is DIOS MIOS 2.0+\n");
						sprintf(DIOS_MIOS_INFO, "DIOS MIOS 2.0+\n%s\n", buffer);
						mios = CFG_DM_2_0;
					}
					
					break;
				}
			}
		}
		SAFE_FREE(appfile);
	}
	if (mios == CFG_MIOS)
		strcpy(dm_boot_drive, "usb:");
	return mios;
}

void *allocate_memory(u32 size)
{
	void *temp;
	temp = memalign(32, (size+31)&(~31) );
	memset(temp, 0, (size+31)&(~31) );
	return temp;
}

s32 Identify_GenerateTik(signed_blob **outbuf, u32 *outlen)
{
	signed_blob *buffer   = NULL;

	sig_rsa2048 *signature = NULL;
	tik         *tik_data  = NULL;

	u32 len;

	/* Set ticket length */
	len = STD_SIGNED_TIK_SIZE;

	/* Allocate memory */
	buffer = (signed_blob *)memalign(32, len);
	if (!buffer)
		return -1;

	/* Clear buffer */
	memset(buffer, 0, len);

	/* Generate signature */
	signature       = (sig_rsa2048 *)buffer;
	signature->type = ES_SIG_RSA2048;

	/* Generate ticket */
	tik_data  = (tik *)SIGNATURE_PAYLOAD(buffer);

	strcpy(tik_data->issuer, "Root-CA00000001-XS00000003");
	memset(tik_data->cidx_mask, 0xFF, 32);

	/* Set values */
	*outbuf = buffer;
	*outlen = len;

	return 0;
}

s32 identify(u64 titleid, u8 *tmdBuffer, u32 tmdSize)
{
	char filepath[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32);
	signed_blob *tikBuffer = NULL;
	u32 tikSize;
	u8 *certBuffer = NULL;
	u32 certSize;

	s32 ret;

	Identify_GenerateTik(&tikBuffer,&tikSize);

	sprintf(filepath, "/sys/cert.sys");
	ret = read_file_from_nand(filepath, &certBuffer, &certSize);
	if (ret < 0)
	{
		dbg_printf("Reading certs failed\n");
		free(tikBuffer);
		return ret;
	}
	//Print("done\n");

	ret = ES_Identify((signed_blob*)certBuffer, certSize, (signed_blob*)tmdBuffer, tmdSize, tikBuffer, tikSize, NULL);
	if (ret < 0)
	{
		switch(ret)
		{
			case ES_EINVAL:
				dbg_printf("Error! ES_Identify (ret = %d;) Data invalid!\n", ret);
				break;
			case ES_EALIGN:
				dbg_printf("Error! ES_Identify (ret = %d;) Data not aligned!\n", ret);
				break;
			case ES_ENOTINIT:
				dbg_printf("Error! ES_Identify (ret = %d;) ES not initialized!\n", ret);
				break;
			case ES_ENOMEM:
				dbg_printf("Error! ES_Identify (ret = %d;) No memory!\n", ret);
				break;
			default:
				dbg_printf("Error! ES_Identify (ret = %d)\n", ret);
				break;
		}
		free(tikBuffer);
		free(certBuffer);
		return ret;
	}
	//Print("done\n");

	free(tikBuffer);
	free(certBuffer);
	return 0;
}