/*
	DIARIO.C
	This code allows to modify play_rec.dat in order to store the
	game time in Wii's log correctly.

	by Marc
	Thanks to tueidj for giving me some hints on how to do it :)
	Most of the code was taken from here:
	http://forum.wiibrew.org/read.php?27,22130
*/
// Changes by Clipper

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <ctype.h>
#include <wctype.h>
#include <wchar.h>
#include <locale.h>
#include <asndlib.h>

#include "disc.h"
#include "fat.h"
#include "cache.h"
#include "gui.h"
#include "menu.h"
#include "restart.h"
#include "sys.h"
#include "util.h"
#include "utils.h"
#include "video.h"
#include "wbfs.h"
#include "libwbfs/libwbfs.h"
#include "wpad.h"
#include "patchcode.h"
#include "cfg.h"
#include "http.h"
#include "dns.h"
#include "wdvd.h"
#include "music.h"
#include "subsystem.h"
#include "net.h"
#include "fst.h"
#include "wiip.h"
#include "frag.h"
#include "playlog.h"

u64 getWiiTime(void) {
	time_t uTime;
	uTime = time(NULL);
	return TICKS_PER_SECOND * (uTime - SECONDS_TO_2000);
}

int set_playrec(u8 ID[6], u8 title[84]){
	s32 ret,playrec_fd;
	u32 sum = 0;
	u8 i;
	u64 stime;
	myrec_struct playrec_buf;
	//u32 flag;
	//s32 err;
/*	printf("start\n");
Wpad_WaitButtons();
	 //Open play_rec.dat
	playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_RW);
	if(playrec_fd < 0) {
		printf("Could not open playrec (%d)\n",playrec_fd);
		return playrec_fd;
	}
	printf("open\n");
Wpad_WaitButtons();
	//Read play_rec.dat
	ret = IOS_Read(playrec_fd, &playrec_buf, sizeof(playrec_buf));
	if(ret != sizeof(playrec_buf)){
		printf("* Could not read playrec play_rec.dat (%d)\n",ret);
		return -1;
	}
	IOS_Close(playrec_fd);
	printf("read %lld %lld\n", playrec_buf.ticks_boot, playrec_buf.ticks_last);
Wpad_WaitButtons();
*/
	//Update channel name and ID


	memcpy(playrec_buf.name, title, 84);
	memcpy(playrec_buf.title_id, ID, 6);

	stime = getWiiTime();

	playrec_buf.ticks_boot = stime;
	playrec_buf.ticks_last = stime;

	memset(playrec_buf.unknown, 0, 18);

	//printf("Wanting to play %s (%6s) at time %lld", playrec_buf.name, playrec_buf.title_id, playrec_buf.ticks_boot);
	//printf("info\n");

	//Calculate and update checksum
	for(i=0; i<0x1f; i++)
		sum += playrec_buf.data[i];
	playrec_buf.checksum=sum;

	//Save new play_rec.dat
	/*ret = ISFS_Initialize();
	if(ret){
		ISFS_Deinitialize();
		printf("* ERROR: ISFS problem (%d)\n",ret);
		return -1;
	}*/
	//printf("isfs\n");
//Wpad_WaitButtons();
	//err = 1;
	
		playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_WRITE);
		//if (playrec_fd >= 0) {
		//	u32 owner;
		//	u16 group;
		//	u8 attributes, ownerperm, groupperm, otherperm;
		//	err = 0;
		//	err = ISFS_GetAttr(PLAYRECPATH, &owner, &group, &attributes, &ownerperm, &groupperm, &otherperm);
		//	printf("ret %d owner %d group %d attributes %X perm:%X%X%X", err, owner, (s32)group, (s32)attributes, (s32)ownerperm, (s32)groupperm, (s32)otherperm);
		//} else {
		//	printf("* ERROR: play_rec.dat (%d)\n", playrec_fd);
		//	err = 1;
		//	Wpad_WaitButtons();
		//	playrec_fd = ISFS_CreateFile(PLAYRECPATH, 0x80, 0xD0, 0x29, 0xC8);
		//	printf("* Created play_rec.dat (%d)\n", playrec_fd);
		//	Wpad_WaitButtons();
		//	playrec_fd = IOS_Open(PLAYRECPATH, IPC_OPEN_WRITE);
		//	if (playrec_fd >= 0) {
		//		err = 0;
		//	}else{
		//		printf("* Created play_rec.dat (%d)\n", playrec_fd);
		//	}
		//}
	
	
	if(playrec_fd<0){
		//ISFS_Deinitialize();
		printf("* ERROR: could not open play_rec.dat (%d)\n",playrec_fd);
		return playrec_fd;
	}
	//printf("open\n");
//Wpad_WaitButtons();

	ret = IOS_Write(playrec_fd, &playrec_buf, sizeof(playrec_buf));
	if(ret!=sizeof(playrec_buf)){
		IOS_Close(playrec_fd);
		//ISFS_Deinitialize();
		printf("* ERROR: could not write play_rec.dat (%d)\n",ret);
		return -1;
	}
	//printf("write\n");
//Wpad_WaitButtons();

	IOS_Close(playrec_fd);
	//ISFS_Deinitialize();

	return 0;
}

