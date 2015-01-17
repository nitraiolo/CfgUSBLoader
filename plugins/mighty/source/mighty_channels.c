#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <string.h> //for parsing parameters
#include <unistd.h>

#include "nand.h"
#include "menu.h"
#include "video.h"
#include "tools.h"

int main(int argc, char **argv){
	int mode=-1, ios=249;

	title gameToLaunch;
	
	// Initialize system
	Video_Init();
	
	Con_Init(CONSOLE_X, CONSOLE_Y, CONSOLE_WIDTH, CONSOLE_HEIGHT);
	
	printf("Mighty Channels Plugin v4\n");
	
	// Parse parameters
	if(argc>0){
		int i;
		for(i=0; i<argc; i++){
			printf("%s\n", argv[i]);
			if(strncmp("--ios=", argv[i], 6)==0){
				ios=atoi(strchr(argv[i], '=')+1);
			}else if(strncmp("--auto=SD", argv[i], 9)==0){
				mode=EMU_SD;
			}else if(strncmp("--auto=USB", argv[i], 10)==0){
				mode=EMU_USB;
			}else if(strncmp("--partition=", argv[i], 12)==0){
				Set_Partition(atoi(strchr(argv[i], '=')+1));
			}else if(strncmp("--path=", argv[i], 7)==0){
				Set_Path(strchr(argv[i], '=')+1);
				//if (strstr(argv[i], "usb:")) mode=EMU_USB;
				//if (strstr(argv[i], "sd:")) mode=EMU_SD;
			}else if(strncmp("--fullmode=", argv[i], 11)==0){
				Set_FullMode(atoi(strchr(argv[i], '=')+1));
			}else if(strncmp("--gameIdLower=", argv[i], 14)==0){
				gameToLaunch.idInt = TITLE_ID(0x00010001, strtol(strchr(argv[i], '=')+1, NULL, 16));
			}else if(strncmp("--videoMode=", argv[i], 12)==0){
				gameToLaunch.videoMode = atoi(strchr(argv[i], '=')+1);
			}else if(strncmp("--videoPatch=", argv[i], 13)==0){
				gameToLaunch.videoPatch = atoi(strchr(argv[i], '=')+1);
			}else if(strncmp("--language=", argv[i], 11)==0){
				gameToLaunch.language = atoi(strchr(argv[i], '=')+1);
			}else if(strncmp("--hooktype=", argv[i], 11)==0){
				gameToLaunch.hooktype = atoi(strchr(argv[i], '=')+1);
			}else if(strncmp("--ocarina=", argv[i], 10)==0){
				gameToLaunch.ocarina = atoi(strchr(argv[i], '=')+1);
			}else if(strncmp("--debugger=", argv[i], 11)==0){
				gameToLaunch.debugger = atoi(strchr(argv[i], '=')+1);
			}else if(strncmp("--bootMethod=", argv[i], 13)==0){
				gameToLaunch.bootMethod = atoi(strchr(argv[i], '=')+1);
			}
		}
	}
	// Load Custom IOS
	//printf("Loading IOS %d...\n", ios);
	//IOS_ReloadIOS(ios);
	
	//sleep(5);
	printf("Starting game...\n");
	__Start_Game(gameToLaunch, ios, mode);
	
	//sleep(5);

	exit(0);
}
