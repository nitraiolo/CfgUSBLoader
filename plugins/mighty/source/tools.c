/*******************************************************************************
 * tools.c
 *
 * Copyright (c) 2009 The Lemon Man
 * Copyright (c) 2009 Nicksasa
 * Copyright (c) 2009 WiiPower
 *
 * Distributed under the terms of the GNU General Public License (v2)
 * See http://www.gnu.org/licenses/gpl-2.0.txt for more info.
 *
 * Description:
 * -----------
 *
 ******************************************************************************/

#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include <stdlib.h>

#include "tools.h"

void *allocate_memory(u32 size){
	return memalign(32, (size+31)&(~31) );
}

/*
u32 Pad_WaitButtons(void){
	u32 buttons=0;

	for(;;){
		//Obtener botones pulsados de Wiimotes
		buttons=0;
		WPAD_ScanPads();
		//for (cnt = 0; cnt < MAX_WIIMOTES; cnt++)
		//	buttons |= WPAD_ButtonsDown(cnt);
		buttons=WPAD_ButtonsDown(0);
		if(buttons){
			return buttons;
		}
		VIDEO_WaitVSync();

		//Obtener botones pulsados de mandos GC
		buttons=0;
		PAD_ScanPads();
		buttons=PAD_ButtonsDown(0);
		if(buttons){
			if(buttons & PAD_BUTTON_UP)
				return WPAD_BUTTON_UP;
			else if(buttons & PAD_BUTTON_DOWN)
				return WPAD_BUTTON_DOWN;
			else if(buttons & PAD_BUTTON_LEFT)
				return WPAD_BUTTON_LEFT;
			else if(buttons & PAD_BUTTON_RIGHT)
				return WPAD_BUTTON_RIGHT;
			else if(buttons & PAD_BUTTON_A)
				return WPAD_BUTTON_A;
			else if(buttons & PAD_BUTTON_B)
				return WPAD_BUTTON_B;
			else if(buttons & PAD_BUTTON_Y)
				return WPAD_BUTTON_1;
			else if(buttons & PAD_BUTTON_X)
				return WPAD_BUTTON_2;
			else if(buttons & PAD_TRIGGER_R)
				return WPAD_BUTTON_PLUS;
			else if(buttons & PAD_TRIGGER_L)
				return WPAD_BUTTON_MINUS;
			else if(buttons & PAD_BUTTON_START)
				return WPAD_BUTTON_HOME;
		}
		VIDEO_WaitVSync();

	}
}*/

s32 __FileCmp(const void *a, const void *b){
	dirent_t *hdr1 = (dirent_t *)a;
	dirent_t *hdr2 = (dirent_t *)b;
	
	if (hdr1->type == hdr2->type){
		return strcmp(hdr1->name, hdr2->name);
	}else{
		return 0;
	}
}

s32 getdir(char *path, dirent_t **ent, u32 *cnt){
	s32 res;
	u32 num = 0;

	int i, j, k;
	
	res = ISFS_ReadDir(path, NULL, &num);
	if(res != ISFS_OK){
		printf("Error: could not get dir entry count! (result: %d)\n", res);
		return -1;
	}

	char ebuf[ISFS_MAXPATH + 1];

	char *nbuf = (char *)allocate_memory((ISFS_MAXPATH + 1) * num);
	if(nbuf == NULL){
		printf("ERROR: could not allocate buffer for name list!\n");
		return -2;
	}

	res = ISFS_ReadDir(path, nbuf, &num);
	DCFlushRange(nbuf,13*num); //quick fix for cache problems?
	if(res != ISFS_OK){
		printf("ERROR: could not get name list! (result: %d)\n", res);
		free(nbuf);
		return -3;
	}
	
	*cnt = num;
	
	*ent = allocate_memory(sizeof(dirent_t) * num);
	if(*ent==NULL){
		printf("Error: could not allocate buffer\n");
		free(nbuf);
		return -4;
	}

	for(i = 0, k = 0; i < num; i++){	    
		for(j = 0; nbuf[k] != 0; j++, k++)
			ebuf[j] = nbuf[k];
		ebuf[j] = 0;
		k++;

		strcpy((*ent)[i].name, ebuf);
	}
	
	qsort(*ent, *cnt, sizeof(dirent_t), __FileCmp);
	
	free(nbuf);
	return 0;
}

s32 read_file(char *filepath, u8 **buffer, u32 *filesize){
	s32 Fd;
	int ret;

	if(buffer == NULL){
		printf("NULL Pointer\n");
		return -1;
	}

	Fd = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (Fd < 0){
		//printf("\n   * ISFS_Open %s failed %d", filepath, Fd);
		//Pad_WaitButtons();
		return Fd;
	}

	fstats *status;
	status = allocate_memory(sizeof(fstats));
	if (status == NULL){
		printf("Out of memory for status\n");
		return -1;
	}
	
	ret = ISFS_GetFileStats(Fd, status);
	if (ret < 0){
		printf("ISFS_GetFileStats failed %d\n", ret);
		ISFS_Close(Fd);
		free(status);
		return -1;
	}
	
	*buffer = allocate_memory(status->file_length);
	if (*buffer == NULL){
		printf("Out of memory for buffer\n");
		ISFS_Close(Fd);
		free(status);
		return -1;
	}
		
	ret = ISFS_Read(Fd, *buffer, status->file_length);
	if (ret < 0){
		printf("ISFS_Read failed %d\n", ret);
		ISFS_Close(Fd);
		free(status);
		free(*buffer);
		return ret;
	}
	ISFS_Close(Fd);

	*filesize = status->file_length;
	free(status);

	return 0;
}

s32 Identify_GenerateTik(signed_blob **outbuf, u32 *outlen){
	signed_blob *buffer   = NULL;

	sig_rsa2048 *signature = NULL;
	tik *tik_data  = NULL;

	u32 len;

	/* Set ticket length */
	len = STD_SIGNED_TIK_SIZE;

	/* Allocate memory */
	buffer = (signed_blob *)memalign(32, len);
	if (!buffer)
		return -1;

	/* Clear buffer */
	memset(buffer, 0, len);

	/* Generate signature */
	signature=(sig_rsa2048 *)buffer;
	signature->type = ES_SIG_RSA2048;

	/* Generate ticket */
	tik_data  = (tik *)SIGNATURE_PAYLOAD(buffer);

	strcpy(tik_data->issuer, "Root-CA00000001-XS00000003");
	memset(tik_data->cidx_mask, 0xFF, 32);

	/* Set values */
	*outbuf = buffer;
	*outlen = len;

	return 0;
}


s32 identify(u64 titleid, u32 *ios){
	char filepath[ISFS_MAXPATH] ATTRIBUTE_ALIGN(0x20);
	u8 *tmdBuffer = NULL;
	u32 tmdSize;
	signed_blob *tikBuffer = NULL;
	u32 tikSize;
	u8 *certBuffer = NULL;
	u32 certSize;
	
	int ret;

	printf("Reading TMD...");
	fflush(stdout);
	
	sprintf(filepath, "/title/%08x/%08x/content/title.tmd", TITLE_UPPER(titleid), TITLE_LOWER(titleid));
	ret = read_file(filepath, &tmdBuffer, &tmdSize);
	if (ret < 0)
	{
		printf("Reading TMD failed\n");
		return ret;
	}
	printf("done\n");

	*ios = (u32)(tmdBuffer[0x18b]);

	printf("Generating fake ticket...");
	fflush(stdout);
	/*
	sprintf(filepath, "/ticket/%08x/%08x.tik", TITLE_UPPER(titleid), TITLE_LOWER(titleid));
	ret = read_file(filepath, &tikBuffer, &tikSize);
	if (ret < 0)
	{
		printf("Reading ticket failed\n");
		free(tmdBuffer);
		return ret;
	}*/
	Identify_GenerateTik(&tikBuffer,&tikSize);
	printf("done\n");

	printf("Reading certs...");
	fflush(stdout);

	sprintf(filepath, "/sys/cert.sys");
	ret = read_file(filepath, &certBuffer, &certSize);
	if (ret < 0)
	{
		printf("Reading certs failed\n");
		free(tmdBuffer);
		free(tikBuffer);
		return ret;
	}
	printf("done\n");
	
	printf("ES_Identify...");
	fflush(stdout);

	ret = ES_Identify((signed_blob*)certBuffer, certSize, (signed_blob*)tmdBuffer, tmdSize, tikBuffer, tikSize, NULL);
	if (ret < 0)
	{
		switch(ret)
		{
			case ES_EINVAL:
				printf("Error! ES_Identify (ret = %d;) Data invalid!\n", ret);
				break;
			case ES_EALIGN:
				printf("Error! ES_Identify (ret = %d;) Data not aligned!\n", ret);
				break;
			case ES_ENOTINIT:
				printf("Error! ES_Identify (ret = %d;) ES not initialized!\n", ret);
				break;
			case ES_ENOMEM:
				printf("Error! ES_Identify (ret = %d;) No memory!\n", ret);
				break;
			default:
				printf("Error! ES_Identify (ret = %d)\n", ret);
				break;
		}
		free(tmdBuffer);
		free(tikBuffer);
		free(certBuffer);
		return ret;
	}
	printf("done\n");
	
	free(tmdBuffer);
	free(tikBuffer);
	free(certBuffer);
	return 0;
}

