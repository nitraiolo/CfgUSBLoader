#ifndef _PLAYLOG_H_
#define _PLAYLOG_H_


typedef struct{
	u32 checksum;
	union{
		u32 data[31];
		struct{
			char name[84];	//u16 name[42];
			u64 ticks_boot;
			u64 ticks_last;
			char title_id[6];
			char unknown[18];
		} ATTRIBUTE_PACKED;
	};
} myrec_struct;

#define PLAYRECPATH "/title/00000001/00000002/data/play_rec.dat"

#define SECONDS_TO_2000  946684800LL
#define TICKS_PER_SECOND 60750000LL

u64 getWiiTime(void);
int set_playrec(u8 ID[6], u8 title[84]);

#endif
