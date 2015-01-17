/*
 * Titles ID functions and defaults
 */

#define TITLE_ID(x,y)	(((u64)(x) << 32) | (y))
#define TITLE_UPPER(x)	((u32)((x) >> 32))
#define TITLE_LOWER(x)	((u32)(x))

#define TITLE_1(x)		((u8)((x) >> 8))
#define TITLE_2(x)		((u8)((x) >> 16))
#define TITLE_3(x)		((u8)((x) >> 24))
#define TITLE_4(x)		((u8)((x) >> 32))
#define TITLE_5(x)		((u8)((x) >> 40))
#define TITLE_6(x)		((u8)((x) >> 48))
#define TITLE_7(x)		((u8)((x) >> 56))

#define DOWNLOADED_CHANNELS	0x00010001
#define SYSTEM_CHANNELS		0x00010002
#define GAME_CHANNELS		0x00010004

//Region Free News Channel
#define RF_NEWS_CHANNEL		0x48414741
//Region Free Forecast Channel
#define RF_FORECAST_CHANNEL	0x48414641
//DVDX
#define DVDX_CHANNEL		0x44564458
//HAXX (HBC old)
#define HBC_OLD_CHANNEL		0x48415858
//JODI (HBC new)
#define HBC_NEW_CHANNEL		0x4A4F4449
//.... (HBC 1.0.7+)
#define HBC_107_CHANNEL		0xAF1BF516
//LULZ (HBC 1.1.2)
#define HBC_112_CHANNEL		0x4C554C5A