#ifndef _FILEOPS
#define _FILEOPS

bool fsop_GetFileSizeBytes (char *path, size_t *filesize);	// for me stats st_size report always 0 :(
bool fsop_FileExist (char *fn);
bool fsop_DirExist (char *path);
bool fsop_CopyFile (char *source, char *target);
bool fsop_MakeFolder (char *path);
bool fsop_CopyFolder (char *source, char *target);
bool fsop_CreateFolderTree (char *path);
void fsop_deleteFolder(char *source);

u32 fsop_GetFolderKb (char *source);
u64 fsop_GetFolderBytes (char *source);
u32 fsop_GetFreeSpaceKb (char *path);

#endif