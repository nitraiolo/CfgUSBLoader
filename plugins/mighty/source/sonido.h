#ifndef _SONIDO_H_
#define _SONIDO_H_

typedef struct SoundInfo
{
	void *dsp_data;
	int size;
	int channels;
	int rate;
	int loop;
} SoundInfo;

void hex_dump2(void *p, int size);

static inline u32 _be32(const u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}
static inline u32 _le32(const void *d)
{
	const u8 *p = d;
	return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
}
static inline u32 _le16(const void *d)
{
	const u8 *p = d;
	return (p[1] << 8) | p[0];
}

void parse_banner_snd(void *data_hdr, SoundInfo *snd);

#endif