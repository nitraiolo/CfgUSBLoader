/*
 fat_frag.c

 get list of sector fragments used by a file

 Copyright (c) 2008-2011 oggzee

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#include "fatfile.h"

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>

#include "cache.h"
#include "file_allocation_table.h"
#include "bit_ops.h"
#include "filetime.h"
#include "lock.h"

typedef int (*_frag_append_t)(void *ff, u32 offset, u32 sector, u32 count);

int _FAT_get_fragments (const char *path, _frag_append_t append_fragment, void *callback_data)
{
	struct _reent r;
	FILE_STRUCT file;
	PARTITION* partition;
	u32 cluster;
	u32 sector;
	u32 offset; // in sectors
	u32 size;   // in sectors
	int ret = -1;
	int fd;

	fd = _FAT_open_r (&r, &file, path, O_RDONLY, 0);
	if (fd == -1) return -1;
	if (fd != (int)&file) return -1;

	partition = file.partition;
	_FAT_lock(&partition->lock);

	size = file.filesize / partition->bytesPerSector;
	cluster = file.startCluster;
	offset = 0;

	do {
		if (!_FAT_fat_isValidCluster(partition, cluster)) {
			// invalid cluster
			goto out;
		}
		// add cluster to fileinfo
		sector = _FAT_fat_clusterToSector(partition, cluster);
		if (append_fragment(callback_data, offset, sector, partition->sectorsPerCluster)) {
			// too many fragments
			goto out;
		}
		offset += partition->sectorsPerCluster;
		cluster = _FAT_fat_nextCluster (partition, cluster);
	} while (offset < size);

	// set size
	append_fragment(callback_data, size, 0, 0);
	// success
	ret = 0;

	out:
	_FAT_unlock(&partition->lock);
	_FAT_close_r(&r, fd);
	return ret;
}

