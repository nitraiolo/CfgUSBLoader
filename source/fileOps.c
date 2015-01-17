/*////////////////////////////////////////////////////////////////////////////////////////

fsop contains coomprensive set of function for file and folder handling

en exposed s_fsop fsop structure can be used by callback to update operation status

////////////////////////////////////////////////////////////////////////////////////////*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h> //for mkdir 
#include <sys/statvfs.h>

#define GB_SIZE         1073741824.0

#include "fileOps.h"
#include "video.h"
#include "gettext.h"
#include "util.h"
#include "wpad.h"

#define SET(a, b) a = b; DCFlushRange(&a, sizeof(a));
#define STACKSIZE 8192

static u8 *buff = NULL;
static FILE *fs = NULL, *ft = NULL;
static u32 block = 65536 * 8;
static u32 blockIdx = 0;
static u32 blockInfo[2] = {0,0};
static u32 blockReady = 0;
static u32 stopThread;
static u64 folderSize = 0;
static u64 bytesCopiedPrevFiles = 0;

void __Copy_Spinner(s64 x, u64 max)
{
	static time_t start;
	static u32 expected;
	static u32 prev_d;

	f32 percent, size;
	u32 d, h, m, s;

	/* First time */
	if (!x) {
		start    = time(0);
		expected = 300;
	}

	/* Elapsed time */
	d = time(0) - start;

	if (x != max) {
		/* only update the display once a second results in 25% speed improvement */
		if (prev_d == d) return;
		prev_d = d;
		
		/* Expected time */
		if (d && x)
			expected = (expected * 3 + d * max / x) / 4;

		/* Remaining time */
		d = (expected > d) ? (expected - d) : 1;
	}

	/* Calculate time values */
	h =  d / 3600;
	m = (d / 60) % 60;
	s =  d % 60;

	/* Calculate percentage/size */
	percent = (x * 100.0) / max;
	size = (max / GB_SIZE);

	Con_ClearLine();

	/* Show progress */
	if (x != max) {
		printf_(gt("%.2f%% of %.2fGB (%c) ETA: %d:%02d:%02d"),
				percent, size, "/-\\|"[(x / 10) % 4], h, m, s);
		printf("\r");
		fflush(stdout);
	} else {
		printf_(gt("%.2fGB copied in %d:%02d:%02d"), size, h, m, s);
		printf("  \n");
//	printf(gt("Press any button..."));
//	printf("\n");
//	Wpad_WaitButtons();
	}

	__console_flush(1);
}

// return false if the file doesn't exist
bool fsop_GetFileSizeBytes (char *path, size_t *filesize)	// for me stats st_size report always 0 :(
{
	FILE *f;
	size_t size = 0;
	
	f = fopen(path, "rb");
	if (!f)
	{
		if (filesize) *filesize = size;
		return false;
	}

	//Get file size
	fseek( f, 0, SEEK_END);
	size = ftell(f);
	if (filesize) *filesize = size;
	fclose (f);
	
	
	return true;
}

/*
Recursive fsop_GetFolderBytes
*/
u64 fsop_GetFolderBytes (char *source)
{
	DIR *pdir;
	struct dirent *pent;
	char newSource[300];
	u64 bytes = 0;
	
	pdir=opendir(source);
	
	while ((pent=readdir(pdir)) != NULL) 
	{
		// Skip it
		if (strcmp (pent->d_name, ".") == 0 || strcmp (pent->d_name, "..") == 0)
			continue;
			
		sprintf (newSource, "%s/%s", source, pent->d_name);
		
		// If it is a folder... recurse...
		if (fsop_DirExist (newSource))
		{
			bytes += fsop_GetFolderBytes (newSource);
		}
		else	// It is a file !
		{
			size_t s;
			fsop_GetFileSizeBytes (newSource, &s);
			bytes += s;
		}
	}
	
	closedir(pdir);
	
	//Debug ("fsop_GetFolderBytes (%s) = %llu", source, bytes);
	
	return bytes;
	}

u32 fsop_GetFolderKb (char *source)
{
	u32 ret = (u32) round ((double)fsop_GetFolderBytes (source) / 1000.0);

	return ret;
}

u32 fsop_GetFreeSpaceKb (char *path) // Return free kb on the device passed
{
	struct statvfs s;
	
	statvfs (path, &s);
	
	u32 ret = (u32)round( ((double)s.f_bfree / 1000.0) * s.f_bsize);
	
	return ret ;
}

bool fsop_FileExist (char *fn)
{
	FILE * f;
	f = fopen(fn, "rb");
	if (!f) return false;
	fclose(f);
	return true;
}
	
bool fsop_DirExist (char *path)
{
	DIR *dir;
	
	dir=opendir(path);
	if (dir)
	{
		closedir(dir);
		return true;
	}
	
	return false;
}

static void *thread_CopyFileReader (void *arg)
{
	u32 rb;
	stopThread = 0;
	DCFlushRange(&stopThread, sizeof(stopThread));
	do
	{
		SET (rb, fread(&buff[blockIdx*block], 1, block, fs ));
		SET (blockInfo[blockIdx], rb);
		SET (blockReady, 1);

		while (blockReady && !stopThread) usleep(1);
	}
	while (stopThread == 0);

	stopThread = -1;
	DCFlushRange(&stopThread, sizeof(stopThread));

	return 0;
}

bool doCopyFile (char *source, char *target)
{
	int err = 0;

	u32 size;
	u32 bytes, rb,wb;
	
	if (strstr (source, "usb:") && strstr (target, "usb:"))
	{
		block = 1024*1024;
	}
	else block = 65536 * 8;
	
	fs = fopen(source, "rb");
	if (!fs)
	{
		return false;
	}

	ft = fopen(target, "wt");
	if (!ft)
	{
		fclose (fs);
		return false;
	}
	
	//Get file size
	fseek ( fs, 0, SEEK_END);
	size = ftell(fs);

	if (size == 0)
	{
		fclose (fs);
		fclose (ft);
		return true;
	}
		
	// Return to beginning....
	fseek( fs, 0, SEEK_SET);

	u8 * threadStack = NULL;
	lwp_t hthread = LWP_THREAD_NULL;

	buff = memalign (32, block*2);
	if (buff == NULL) 
	{
		fclose (fs);
		fclose (ft);
		return false;
	}

	blockIdx = 0;
	blockReady = 0;
	blockInfo[0] = 0;
	blockInfo[1] = 0;

	threadStack = memalign(32,STACKSIZE);
	LWP_CreateThread (&hthread, thread_CopyFileReader, NULL, threadStack, STACKSIZE, 30);

	while (stopThread != 0)
		usleep (5);

	bytes = 0;
	u32 bi;
	do
	{
		while (!blockReady) usleep (1); // Let's wait for incoming block from the thread
		
		bi = blockIdx;

		// let's th thread to read the next buff
		SET (blockIdx, 1 - blockIdx);
		SET (blockReady, 0);

		rb = blockInfo[bi];
		// write current block
		wb = fwrite(&buff[bi*block], 1, rb, ft);

		if (rb != wb) err = 1;
		if (rb == 0) err = 1;
		bytes += rb;

		__Copy_Spinner(bytes + bytesCopiedPrevFiles, folderSize);
	}
	while (bytes < size && err == 0);

	stopThread = 1;
	DCFlushRange(&stopThread, sizeof(stopThread));

	while (stopThread != -1)
		usleep (5);

	LWP_JoinThread(hthread, NULL);
	free(threadStack);

	stopThread = 1;
	DCFlushRange(&stopThread, sizeof(stopThread));

	fclose (fs);
	fclose (ft);
	
	free(buff);
	
	bytesCopiedPrevFiles += bytes; 		//keep total bytes when copying dir

	if (err) 
	{
		unlink (target);
		return false;
	}
	
	return true;
}

bool fsop_CopyFile (char *source, char *target)
{
	size_t s;
	fsop_GetFileSizeBytes (source, &s);
	folderSize = s;
	__Copy_Spinner(0, folderSize);				// init spinner
	bytesCopiedPrevFiles = 0;

	return doCopyFile (source, target);
}

/*
Semplified folder make
*/
bool fsop_MakeFolder (char *path)
{
	if (mkdir(path, S_IREAD | S_IWRITE) == 0) return true;
	
	return false;
}

/*
Recursive copyfolder
*/
static bool doCopyFolder (char *source, char *target)
{
	DIR *pdir;
	struct dirent *pent;
	char newSource[300], newTarget[300];
	bool ret = true;
	
	// If target folder doesn't exist, create it !
	if (!fsop_DirExist (target))
	{
		fsop_MakeFolder (target);
	}

	pdir=opendir(source);
	
	while ((pent=readdir(pdir)) != NULL && ret == true) 
	{
		// Skip it
		if (strcmp (pent->d_name, ".") == 0 || strcmp (pent->d_name, "..") == 0)
			continue;
			
		sprintf (newSource, "%s/%s", source, pent->d_name);
		sprintf (newTarget, "%s/%s", target, pent->d_name);
		
		// If it is a folder... recurse...
		if (fsop_DirExist (newSource))
		{
			ret = doCopyFolder (newSource, newTarget);
		}
		else	// It is a file !
		{
			ret = doCopyFile (newSource, newTarget);
		}
	}
	
	closedir(pdir);

	return ret;
}
	
bool fsop_CopyFolder (char *source, char *target)
{
	folderSize = fsop_GetFolderBytes(source);
	__Copy_Spinner(0, folderSize);				// init spinner
	bytesCopiedPrevFiles = 0;

	return doCopyFolder (source, target);
}

// Pass  <mount>://<folder1>/<folder2>.../<folderN> or <mount>:/<folder1>/<folder2>.../<folderN>
bool fsop_CreateFolderTree (char *path)
{
	int i;
	int start, len;
	char buff[300];
	char b[8];
	char *p;
	
	start = 0;
	
	strcpy (b, "://");
	p = strstr (path, b);
	if (p == NULL)
	{
		strcpy (b, ":/");
		p = strstr (path, b);
		if (p == NULL)
			return false; // path must contain
	}
	
	start = (p - path) + strlen(b);
	
	len = strlen(path);

	for (i = start; i <= len; i++)
	{
		if (path[i] == '/' || i == len)
		{
			strcpy (buff, path);
			buff[i] = 0;
			
			if (!fsop_DirExist(buff))
				fsop_MakeFolder(buff);
		}
	}
	
	// Check if the tree has been created
	return fsop_DirExist (path);
}

void fsop_deleteFolder(char *source)
{
	DIR *pdir;
	struct dirent *pent;
	char newSource[300];

	pdir = opendir(source);

	while ((pent=readdir(pdir)) != NULL) 
	{
		// Skip it
		if (strcmp (pent->d_name, ".") == 0 || strcmp (pent->d_name, "..") == 0)
			continue;

		sprintf (newSource, "%s/%s", source, pent->d_name);

		// If it is a folder... recurse...
		if (fsop_DirExist(newSource))
		{
			fsop_deleteFolder(newSource);
		}
		else	// It is a file !
		{
			dbg_printf("Deleting file: %s\n",newSource);
			remove(newSource);
		}
	}
	closedir(pdir);
	dbg_printf("Deleting directory: %s\n",source);
	unlink(source);
}
