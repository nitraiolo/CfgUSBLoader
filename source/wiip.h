#include <gccore.h>

void wipreset();
void wipregisteroffset(u32 dst, u32 len);
void wipparsebuffer(u8 *buffer, u32 length);
void load_wip_patches(u8 *discid);
void do_wip_patches();

