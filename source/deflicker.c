/*
 * Deflicker filter patching by wiidev (blackb0x @ GBAtemp)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <sys/unistd.h>

#ifdef DEBUG_PATCH
#define debug_printf(fmt, args...) \
	printf(fmt, ##args)
#else
#define debug_printf(fmt, args...)
#endif

static u8 PATTERN[12][2] = {
    {6, 6}, {6, 6}, {6, 6},
    {6, 6}, {6, 6}, {6, 6},
    {6, 6}, {6, 6}, {6, 6},
    {6, 6}, {6, 6}, {6, 6}
};

static u8 PATTERN_AA[12][2] = {
    {3, 2}, {9, 6}, {3, 10},
    {3, 2}, {9, 6}, {3, 10},
    {9, 2}, {3, 6}, {9, 10},
    {9, 2}, {3, 6}, {9, 10}
};

// Patch known and unknown vfilters within GXRModeObj structures
void patch_vfilters(u8 *addr, u32 len, u8 *vfilter)
{
    u8 *addr_start = addr;
    while (len >= sizeof(GXRModeObj))
    {
        GXRModeObj *vidmode = (GXRModeObj *)addr_start;
        if ((memcmp(vidmode->sample_pattern, PATTERN, 24) == 0 || memcmp(vidmode->sample_pattern, PATTERN_AA, 24) == 0) &&
            (vidmode->fbWidth == 640 || vidmode->fbWidth == 608 || vidmode->fbWidth == 512) &&
            (vidmode->field_rendering == 0 || vidmode->field_rendering == 1) &&
            (vidmode->aa == 0 || vidmode->aa == 1))
        {
            debug_printf("Replaced vfilter %02x%02x%02x%02x%02x%02x%02x @ %p (GXRModeObj)\n",
                    vidmode->vfilter[0], vidmode->vfilter[1], vidmode->vfilter[2], vidmode->vfilter[3],
                    vidmode->vfilter[4], vidmode->vfilter[5], vidmode->vfilter[6], addr_start);
            memcpy(vidmode->vfilter, vfilter, 7);
            addr_start += (sizeof(GXRModeObj) - 4);
            len -= (sizeof(GXRModeObj) - 4);
        }
        addr_start += 4;
        len -= 4;
    }
}

// Patch rogue vfilters found in some games
void patch_vfilters_rogue(u8 *addr, u32 len, u8 *vfilter)
{
    u8 known_vfilters[7][7] = {
        {8, 8, 10, 12, 10, 8, 8}, // Ntsc480ProgSoft
        {4, 8, 12, 16, 12, 8, 4}, // Ntsc480ProgAa
        {7, 7, 12, 12, 12, 7, 7}, // Mario Kart Wii
        {5, 5, 15, 14, 15, 5, 5}, // Mario Kart Wii
        {4, 4, 15, 18, 15, 4, 4}, // Sonic Colors
        {4, 4, 16, 16, 16, 4, 4}, // DKC Returns
        {2, 2, 17, 22, 17, 2, 2}
    };
    u8 *addr_start = addr;
    u8 *addr_end = addr + len - 8;
    while (addr_start <= addr_end)
    {
        u8 known_vfilter[7];
		int i;
        for (i = 0; i < 7; i++)
        {
			int x;
            for (x = 0; x < 7; x++)
                known_vfilter[x] = known_vfilters[i][x];
            if (!addr_start[7] && memcmp(addr_start, known_vfilter, 7) == 0)
            {
                debug_printf("Replaced vfilter %02x%02x%02x%02x%02x%02x%02x @ %p\n", addr_start[0], addr_start[1],
                        addr_start[2], addr_start[3], addr_start[4], addr_start[5], addr_start[6], addr_start);
                memcpy(addr_start, vfilter, 7);
                addr_start += 7;
                break;
            }
        }
        addr_start += 1;
    }
}

// Patch GXSetCopyFilter to disable the deflicker filter
void deflicker_patch(u8 *addr, u32 len)
{
    u32 SearchPattern[18] = {
        0x3D20CC01, 0x39400061, 0x99498000,
        0x2C050000, 0x38800053, 0x39600000,
        0x90098000, 0x38000054, 0x39800000,
        0x508BC00E, 0x99498000, 0x500CC00E,
        0x90698000, 0x99498000, 0x90E98000,
        0x99498000, 0x91098000, 0x41820040};
    u8 *addr_start = addr;
    u8 *addr_end = addr + len - sizeof(SearchPattern);
    while (addr_start <= addr_end)
    {
        if (memcmp(addr_start, SearchPattern, sizeof(SearchPattern)) == 0)
        {
            *((u32 *)addr_start + 17) = 0x48000040; // Change beq to b
            debug_printf("Patched GXSetCopyFilter @ %p\n", addr_start);
            return;
        }
        addr_start += 4;
    }
}

// Patch Dithering
void dithering_patch(u8 *addr, u32 len)
{
    u32 SearchPattern[10] = {
		0x3C80CC01, 0x38A00061, 0x38000000,
		0x80C70220, 0x5066177A, 0x98A48000,
		0x90C48000, 0x90C70220, 0xB0070002,
		0x4E800020};
    u8 *addr_start = addr;
    u8 *addr_end = addr + len - sizeof(SearchPattern);
    while (addr_start <= addr_end)
    {
        if (memcmp(addr_start, SearchPattern, sizeof(SearchPattern)) == 0)
        {
            *((u32 *)addr_start - 1) = 0x48000028;
            debug_printf("Patched Dithering @ %p\n", addr_start - 1);
            return;
        }
        addr_start += 4;
    }
}

// Patch Framebuffer Width 
void framebuffer_patch(void *addr, u32 len)
{
    u8 SearchPattern[32] = {
        0x40, 0x82, 0x00, 0x08, 0x48, 0x00, 0x00, 0x1C,
        0x28, 0x09, 0x00, 0x03, 0x40, 0x82, 0x00, 0x08,
        0x48, 0x00, 0x00, 0x10, 0x2C, 0x03, 0x00, 0x00,
        0x40, 0x82, 0x00, 0x08, 0x54, 0xA5, 0x0C, 0x3C};
	u8 *addr_start = (u8 *)addr;
    u8 *addr_end = addr_start + len - sizeof(SearchPattern);
    while (addr_start <= addr_end)
    {
        if (memcmp(addr_start, SearchPattern, sizeof(SearchPattern)) == 0)
        {
            if (addr_start[-0x70] == 0xA0 && addr_start[-0x6E] == 0x00 && addr_start[-0x6D] == 0x0A)
            {
                if (addr_start[-0x44] == 0xA0 && addr_start[-0x42] == 0x00 && addr_start[-0x41] == 0x0E)
                {
                    u8 reg_a = (addr_start[-0x6F] >> 5);
                    u8 reg_b = (addr_start[-0x43] >> 5);

                    // Patch to the framebuffer resolution
                    addr_start[-0x41] = 0x04;

                    // Center the image
                    void *offset = addr_start - 0x70;

                    u32 old_heap_ptr = *(u32 *)0x80003110;
                    *(u32 *)0x80003110 = old_heap_ptr - 0x40;
                    u32 heap_space = old_heap_ptr - 0x40;

                    u32 org_address = (addr_start[-0x70] << 24) | (addr_start[-0x6F] << 16);
                    *(u32 *)(heap_space + 0x00) = org_address | 4;
                    *(u32 *)(heap_space + 0x04) = 0x200002D0 | (reg_b << 21) | (reg_a << 16);
                    *(u32 *)(heap_space + 0x08) = 0x38000002 | (reg_a << 21);
                    *(u32 *)(heap_space + 0x0C) = 0x7C000396 | (reg_a << 21) | (reg_b << 16) | (reg_a << 11);

                    *(u32 *)offset = 0x48000000 + ((heap_space - (u32)offset) & 0x3ffffff);
                    *(u32 *)(heap_space + 0x10) = 0x48000000 + ((((u32)offset + 0x04) - (heap_space + 0x10)) & 0x3ffffff);
                    debug_printf("Patched resolution. Branched from 0x%x to 0x%x\n", (u32) offset, (u32) heap_space);
                    return;
                }
            }
        }
        addr_start += 4;
    }
}