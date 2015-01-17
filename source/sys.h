#ifndef _SYS_H_
#define _SYS_H_

#define TITLE_ID(x,y)       (((u64)(x) << 32) | (y))
#define TITLE_HIGH(x)       ((u32)((x) >> 32))
#define TITLE_LOW(x)		((u32)(x))

/* Prototypes */
void Sys_Init(void);
void Sys_Reboot(void);
void Sys_Shutdown(void);
void Sys_LoadMenu(void);
void Sys_Exit(void);
void Sys_HBC();
void Sys_Channel(u32 channel);
void prep_exit();

s32  Sys_GetCerts(signed_blob **, u32 *);
int ReloadIOS(int subsys, int verbose);
void Block_IOS_Reload();
void get_title_id();
void d2x_return_to_channel();
void load_bca_data(u8 *discid);
int insert_bca_data();
int verify_bca_data();
void print_mload_version_str(char *str);
void print_mload_version();
void mk_mload_version();
void load_dip_249();

#define IOS_TYPE_UNK    0
#define IOS_TYPE_WANIN  1
#define IOS_TYPE_HERMES 2
#define IOS_TYPE_KWIIRK 3

int get_ios_type();
int is_ios_type(int type);
int is_ios_d2x();

s32 GetTMD(u64 TicketID, signed_blob **Output,  u32 *Length);
s32 checkIOS(u32 IOS);
bool shadow_mload();
void print_all_ios_info_str(char *str, int size);
void print_all_ios_info(FILE *f);
char* get_ios_tmd_hash_str(char *str);
char* get_ios_info_from_tmd();
void fill_base_array();
s32 read_file_from_nand(char *filepath, u8 **buffer, u32 *filesize);

u16 get_miosinfo();

void *allocate_memory(u32 size);

#endif
