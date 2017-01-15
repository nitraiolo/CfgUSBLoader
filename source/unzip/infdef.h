#include <stdio.h>

#define Z_FOPEN_ERROR (Z_VERSION_ERROR - 1)
#define Z_ALLOC_ERROR (Z_VERSION_ERROR - 2)
#define Z_FILE_ERROR (Z_VERSION_ERROR - 3)

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */

int zlib_inf(char *source, char *target);

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */

int zlib_def(char *source, char *target, int level);
