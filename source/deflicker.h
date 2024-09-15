/*
 * Deflicker filter patching by wiidev (blackb0x @ GBAtemp)
 */

enum
{
	DEFLICKER_DEFAULT,
	DEFLICKER_OFF,
	DEFLICKER_OFF_EXTENDED,
	DEFLICKER_ON_LOW,
	DEFLICKER_ON_MEDIUM,
	DEFLICKER_ON_HIGH
};

u8 vfilter_off[7]    = {0, 0, 21, 22, 21, 0, 0};
u8 vfilter_low[7]    = {4, 4, 16, 16, 16, 4, 4};
u8 vfilter_medium[7] = {4, 8, 12, 16, 12, 8, 4};
u8 vfilter_high[7]   = {8, 8, 10, 12, 10, 8, 8};

void patch_vfilters(u8 *addr, u32 len, u8 *vfilter);
void patch_vfilters_rogue(u8 *addr, u32 len, u8 *vfilter);
void deflicker_patch(u8 *addr, u32 len);
void dithering_patch(u8 *addr, u32 len);
void framebuffer_patch(void *addr, u32 len);