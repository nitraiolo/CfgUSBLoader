
// by oggzee
// random music added by usptactical

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <fcntl.h>
#include <string.h>

#include <asndlib.h>
#include <mp3player.h>
#include <gcmodplay.h>

#include "dir.h"
#include "cfg.h"
#include "music.h"
#include "wpad.h"
#include "video.h"
#include "gettext.h"
#include "gc_wav.h"

#define FORMAT_MP3 1
#define FORMAT_MOD 2
static int music_format = 0;
static void *music_buf = NULL;
static FILE *music_f = NULL;
static int music_size = 0;
static MODPlay mod;
static char music_fname[200] = "";
static bool was_playing = false;

static bool first_time = true;
static int fileCount = 0;
static int lastPlayed = -1;
static int *musicArray = NULL;
static int music_paused = 0;
static int mp3_read_busy = 0;
static int mp3_read_pos = 0;

void _Music_Stop();

FILE* music_open()
{
	music_f = fopen(music_fname, "rb");
	//dbg_printf("\nopen(%s) %p\n", music_fname, music_f);
	return music_f;
}

//pauses the music thread
void Music_Pause(){
	int i;
	if (CFG.music != 0) {
		music_paused = 1;
		SND_Pause(1);
		for (i=0; i<100 && mp3_read_busy; i++) usleep(10000);
		if (music_f) {
			mp3_read_pos = ftell(music_f);
			fclose(music_f);
			//printf("\nclose %d\n", mp3_read_pos);
			//sleep(2);
		}
	}
}

//unpauses the music thread
void Music_UnPause(){
	if (!music_paused) return;
	if (CFG.music != 0) {
		music_open();
		if (music_f) {
			int ret;
			ret = fseek(music_f, mp3_read_pos, SEEK_SET);
			if (ret < 0) printf("\nerror seek %d %d\n", mp3_read_pos, ret);
			//sleep(5);
		}
		music_paused = 0;
		SND_Pause(0);
	}
	music_paused = 0;
}

//inits an array with values from 1 to cnt
void initializeArray(int *a, int cnt) {
	int i;
	for (i = 0; i < cnt; i++) {
		a[i] = i + 1;
	}
}

//randomizes an array with distinct values from 1 to cnt
void randomizeArray(int *a, int cnt) {
	int j, r, tmp;
	for (j = (cnt - 1); j >= 0; j--) {
		r = rand() % cnt;
		if (r != j) {
			tmp = a[j];
			a[j] = a[r];
			a[r] = tmp;
		}
	}
}

//Determines if ext exists in name
bool match_ext(char *name, char *ext)
{
  if (strlen(name) < strlen(ext)) return false;
	//dbg_printf("Music: match_ext: name=%s  ext=%s\n", name, ext);
    return stricmp(name+strlen(name)-strlen(ext), ext) == 0;
}

//Creates the random music array (fist time through) and retrieves the
// filepath of the next music file to play.
void get_random_file(char *filetype, char *path)
{
	DIR_ITER *dir = NULL;
	char filename[1024];
	int cnt = 0;
	int next = 0;
	struct stat filestat;
	
	if (fileCount==0 && !first_time) goto out;
	
	dbg_printf(gt("Music: Looking for %s files in: %s"), filetype, path);
		dbg_printf("\n");

	// Open directory
	//snprintf(dirname, sizeof(dirname), "%s", path);
	dir = diropen(path);
	if (!dir) return;

	if (first_time) {
		while (!dirnext(dir, filename, &filestat)) {
			// Ignore invalid entries
			if (match_ext(filename, filetype) && !(filestat.st_mode & S_IFDIR))
				fileCount++;
		}
		dbg_printf(gt("Music: Number of %s files found: %i"), filetype, fileCount);
		dbg_printf("\n");

		first_time = false;
		//if no files found then no need to continue
		if (fileCount==0) goto out;
		
		//allocate the random music array
		musicArray = realloc(musicArray, fileCount * sizeof(int));
		if (!musicArray) {
			fileCount = 0;
			goto out;
		}
		initializeArray(musicArray, fileCount);
		randomizeArray(musicArray, fileCount);

		//check array contents
		int i;
		dbg_printf(gt("Music: musicArray contents: "));
		for (i=0; i<fileCount; i++)
			dbg_printf("%i ", musicArray[i]);
		dbg_printf("\n");

		//reset the directory
		dirreset(dir);
	}

	if (fileCount > 0) {
		//get the next file index
		lastPlayed++;
		if (lastPlayed > fileCount-1) lastPlayed = 0;
		next = musicArray[lastPlayed];
		dbg_printf(gt("Music: Next file index to play: %i"), next);
		dbg_printf("\n");
		
		//iterate through and find our file
		while (!dirnext(dir, filename, &filestat)) {
			if (match_ext(filename, filetype) && !(filestat.st_mode & S_IFDIR)) {
				cnt++;
				if (cnt==next) {
					//save path
					snprintf(music_fname, sizeof(music_fname), "%s/%s", path, filename);
					goto out;
				}
			}
		}
	}

	out:;
	//close the directory
	dirclose(dir);	 
}

//Gets the next music file to play.  If a music file was set in the 
// config, that is used.  Otherwise, fetches a random music file.
s32 get_music_file() 
{
	struct stat st;
	char dirpath[200];
	bool useDir = true;
	DIR_ITER *dir = NULL;

	//check for config set music file or path
	if (*CFG.music_file) {
	
		//if ((strstr(CFG.music_file, ".mp3") != NULL)
		//	|| (strstr(CFG.music_file, ".mod") != NULL)) {
		if (match_ext(CFG.music_file, ".mp3")
			|| match_ext(CFG.music_file, ".mod")) {
			//load a specific file
			strcpy(music_fname, CFG.music_file);
			dbg_printf(gt("Music file: %s"), music_fname);
			dbg_printf("\n");
			if (strlen(music_fname) < 5) return 0;
			if (stat(music_fname, &st) != 0) {
				dbg_printf(gt("File not found! %s"), music_fname);
				dbg_printf("\n");
				return 0;
			}
			useDir = false;
		} else {
			
			//load a specific directory
			dir = diropen(CFG.music_file);
			if (!dir) {
				//not a valid dir so try default
				strcpy(dirpath, USBLOADER_PATH);
			} else {
				dirclose(dir);
				strcpy(dirpath, CFG.music_file);
			}
		}
	} else {
		strcpy(dirpath, USBLOADER_PATH);
	}
	
	//if a directory is being used, then get a random file to play
	if (useDir) {
		// try music.mp3 or music.mod
		get_random_file(".mp3", dirpath);
		//snprintf(music_fname, sizeof(music_fname), "%s/%s", USBLOADER_PATH, "music.mp3");
		if (stat(music_fname, &st) != 0) {
			first_time = true;
			get_random_file(".mod", dirpath);
			//snprintf(music_fname, sizeof(music_fname), "%s/%s", USBLOADER_PATH, "music.mod");
			if (stat(music_fname, &st) != 0) {
				dbg_printf(gt("music.mp3 or music.mod not found!"));
				dbg_printf("\n");
				return 0;
			}
		}
		dbg_printf(gt("Music file from dir: %s"), music_fname);
		dbg_printf("\n");
	}

	music_size = st.st_size;
	dbg_printf(gt("Music file size: %d"), music_size);
	dbg_printf("\n");
	if (music_size <= 0) return 0;

	if (match_ext(music_fname, ".mp3")) {
		music_format = FORMAT_MP3;
	} else if (match_ext(music_fname, ".mod")) {
		music_format = FORMAT_MOD;
	} else {
		music_format = 0;
		return 0;
	}

	music_open();
	if (!music_f) {
		if (*CFG.music_file || CFG.debug) {
			printf(gt("Error opening: %s"), music_fname);
			printf("\n");
		   	sleep(2);
		}
		return 0;
	}
	return 1;  //success
}

s32 mp3_reader(void *fp, void *dat, s32 size)
{
	s32 ret, rcnt = 0;
	retry:
	while (music_paused) usleep(10000);
	if (!music_f) return -1;
	mp3_read_busy = 1;
	ret = fread(dat, 1, size, music_f);
	mp3_read_busy = 0;
	//if (ret != size) printf("mp3 read: %d\n", ret);
	if (ret == 0 && rcnt == 0) {
		// eof - reopen
		fclose(music_f);
		ret = get_music_file();
		if (ret==0) return ret;
		rcnt++;
		goto retry;
	}
	return ret;
}

void _Music_Start()
{
	int ret;

	// always init SND, so banner sounds play if music disabled
	ASND_Init();
	ASND_Pause(0);

	if (CFG.music == 0) {
		dbg_printf(gt("Music: Disabled"));
		dbg_printf("\n");
		return;
	} else {
		dbg_printf(gt("Music: Enabled"));
		dbg_printf("\n");
	}
	
	was_playing = false;

	//get a music file
	ret = get_music_file();
	if (ret==0) return;

	if (music_format == FORMAT_MP3) {

		MP3Player_Init();
		MP3Player_Volume(0x80); // of 255
		//ret = MP3Player_PlayBuffer(music_buf, music_size, NULL);
		ret = MP3Player_PlayFile(music_f, mp3_reader, NULL);
		dbg_printf("mp3 play: %s (%d)\n", ret? gt("ERROR"):gt("OK"), ret);
		if (ret) goto err_play;
		usleep(10000); // wait 10ms and verify if playing
		if (!MP3Player_IsPlaying()) {
			err_play:
			printf(gt("Error playing %s"), music_fname);
			printf("\n");
			Music_Stop();
			sleep(1);
		}

	} else {

		music_buf = malloc(music_size);
		if (!music_buf) {
			printf(gt("music file too big (%d) %s"), music_size, music_fname);
			printf("\n");
			sleep(1);
			music_format = 0;
			return;
		}
		//printf("Loading...\n");
		ret = fread(music_buf, music_size, 1, music_f);
		//printf("size: %d %d\n", music_size, ret); sleep(2);
		fclose(music_f);
		music_f = NULL;
		if (ret != 1) {
			printf(gt("error reading: %s (%d)"), music_fname, music_size); printf("\n"); sleep(2);
			free(music_buf);
			music_buf = NULL;
			music_size = 0;
			music_format = 0;
			return;
		}
		MODPlay_Init(&mod);
		ret = MODPlay_SetMOD(&mod, music_buf);
		dbg_printf("mod play: %s (%d)\n", ret?gt("ERROR"):gt("OK"), ret);
		if (ret < 0 ) {
			Music_Stop();
		} else  {
			MODPlay_SetVolume(&mod, 32,32); // fix the volume to 32 (max 64)
			MODPlay_Start(&mod); // Play the MOD
		}
	}
}

void Music_Start()
{
	_Music_Start();
}

#if 0
void Music_Loop()
{
	if (music_format != FORMAT_MP3) return;
	if (!music_f || MP3Player_IsPlaying()) return;
	//dbg_printf("Looping mp3\n");
	CFG.debug = 0;
	_Music_Stop();
	_Music_Start();
}

void Music_CloseFile()
{
	if (music_format != FORMAT_MP3) return;
	if (CFG.music_cache) return;
	if (!music_f) return;
	was_playing = true;
	_Music_Stop();
}

void Music_OpenFile()
{
	if (was_playing) {
		CFG.debug = 0;
		_Music_Start();
	}
}
#endif

void Music_Mute(bool mute)
{
	// when starting game
	if (mute) {
		SND_ChangeVolumeVoice(0, 0, 0);
	} else {
		SND_ChangeVolumeVoice(0, 0x80, 0x80);
	}
}

void Music_PauseVoice(bool pause)
{
	// don't unpause mp3 when changing games
	// pause = 1, unpause = 0
	SND_PauseVoice(0, pause ? 1 : 0);
}

void _Music_Stop()
{
	Music_Mute(true);
	Music_PauseVoice(false);
	SND_Pause(0);
	if (music_format == FORMAT_MP3) {
		//the thread freezes when paused and you can't kill it!
		Music_UnPause();
		MP3Player_Stop();
	} else if (music_format == FORMAT_MOD) {
		if (music_buf) {
			MODPlay_Stop(&mod);
			MODPlay_Unload(&mod);   
		}
	}
	if (music_f) fclose (music_f);
	music_f = NULL;
	if (music_buf) free (music_buf);
	music_buf = NULL;
	if (musicArray) free (musicArray);
	musicArray = NULL;
	music_size = 0;
	was_playing = false;
	music_format = 0;
}

void Music_Stop()
{
	dbg_printf("Music_Stop\n");
	_Music_Stop();
	ASND_End();
}