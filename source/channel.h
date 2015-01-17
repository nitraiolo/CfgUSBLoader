#include <gccore.h>
#include "util.h"
#include "disc.h"

char *get_channel_name(u64 titleid, FILE *fp);
int CHANNEL_Banner(struct discHdr *hdr, SoundInfo *snd);
u64 getChannelSize(struct discHdr *hdr);
u64 getChannelReqIos(struct discHdr *hdr);
s32 Channel_RemoveGame(struct discHdr *hdr);
