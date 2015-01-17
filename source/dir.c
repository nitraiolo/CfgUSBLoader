#if DEVKITPPCVER >= 23

#include <reent.h>
#include <sys/iosupport.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/dir.h>

 
DIR_ITER * diropen (const char *path) {
	struct _reent *r = _REENT;
	DIR_ITER *handle = NULL;
	DIR_ITER *dir = NULL;
	int dev;
 
	dev = FindDevice(path);
 
	if(dev!=-1 && devoptab_list[dev]->diropen_r) {
 
		handle = (DIR_ITER *)malloc( sizeof(DIR_ITER) + devoptab_list[dev]->dirStateSize );
 
		if ( NULL != handle ) {
			handle->device = dev;
			handle->dirStruct = ((void *)handle) + sizeof(DIR_ITER);
 
			dir = devoptab_list[dev]->diropen_r(r, handle, path);
 
			if ( dir == NULL ) {
				free (handle);
				handle = NULL;
			}
		} else {
			r->_errno = ENOSR;
			handle = NULL;
		}
	} else {
		r->_errno = ENOSYS;
	}
 
	return handle;
}

int dirreset (DIR_ITER *dirState) {
	struct _reent *r = _REENT;
	int ret = -1;
	int dev = 0;
 
	if (dirState != NULL) {
		dev = dirState->device;
 
		if(devoptab_list[dev]->dirreset_r) {
			ret = devoptab_list[dev]->dirreset_r(r, dirState);
		} else {
			r->_errno = ENOSYS;
		}
	}
	return ret;
}
 
int dirnext (DIR_ITER *dirState, char *filename, struct stat *filestat) {
	struct _reent *r = _REENT;
	int ret = -1;
	int dev = 0;
 
	if (dirState != NULL) {
		dev = dirState->device;
 
		if(devoptab_list[dev]->dirnext_r) {
			ret = devoptab_list[dev]->dirnext_r(r, dirState, filename, filestat);
		} else {
			r->_errno = ENOSYS;
		}
	}
	return ret;
}
 
int dirclose (DIR_ITER *dirState) {
	struct _reent *r = _REENT;
	int ret = -1;
	int dev = 0;
 
	if (dirState != NULL) {
		dev = dirState->device;
 
		if (devoptab_list[dev]->dirclose_r) {
			ret = devoptab_list[dev]->dirclose_r (r, dirState);
		} else {
			r->_errno = ENOSYS;
		}
 
		free (dirState);
	}
	return ret;
}

#endif

