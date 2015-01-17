#ifndef SAVEGAME_H_
#define SAVEGAME_H_

#include "disc.h"

void CreateTitleTMD(const char *path, struct discHdr *hdr);
void CreateSavePath(struct discHdr *hdr);
void Menu_dump_savegame(struct discHdr *header);

#endif
