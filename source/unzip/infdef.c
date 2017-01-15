#include <stdio.h>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include "../debug.h"
#include "infdef.h"

#define CHUNK 16384

/* Decompress from file fs to file ft until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */

int zlib_inf(char *source, char *target)
	{
    int ret;
    unsigned have;
    z_stream strm;

    unsigned char *in;
    unsigned char *out;

	FILE *fs, *ft;
	
	memset (&strm, 0, sizeof (z_stream));
    ret = inflateInit (&strm);

    if (ret != Z_OK)
        return ret;
		
	fs = fopen (source, "rb");
	if (!fs)
		{
		return Z_FOPEN_ERROR ; // boh, maybe this isn't the right value to return
		}
		
	ft = fopen (target, "wb");
	if (!ft)
		{
		fclose (fs);
		return Z_FOPEN_ERROR; // boh, maybe this isn't the right value to return
		}
		
	// allocate chunk memory
	in = malloc (CHUNK);
	if (!in) 
		{
		return Z_ALLOC_ERROR;
		}

	out = malloc (CHUNK);
	if (!out) 
		{
		free (in);
		return Z_ALLOC_ERROR;
		}

    /* decompress until deflate stream ends or end of file */
    do 
		{
        strm.avail_in = fread(in, 1, CHUNK, fs);
		
        if (ferror(fs)) 
			{
			ret = Z_FILE_ERROR;
			goto end;
			}
		
        if (strm.avail_in == 0)
            break;

        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do 
			{
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            
            switch (ret) 
				{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_STREAM_ERROR:
				case Z_MEM_ERROR:
					goto end;
				}
            
			have = CHUNK - strm.avail_out;
			
            if (fwrite(out, 1, have, ft) != have || ferror(ft)) 
				{
				ret = Z_FILE_ERROR;
                goto end;
				}
			} 
		while (strm.avail_out == 0);

        /* done when inflate() says it's done */
		} 
	while (ret != Z_STREAM_END);
	
end:	
	fclose (fs);
	fclose (ft);
	
	free (in);
	free (out);
	
    inflateEnd(&strm);
	dbg_printf ("zinf#5");
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
	} 

/* Compress from file fs to file ft until EOF on fs.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */

int zlib_def(char *source, char *target, int level)
	{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char *in;
    unsigned char *out;

	FILE *fs, *ft;

    /* allocate deflate state */
	memset (&strm, 0, sizeof (z_stream));
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

	fs = fopen (source, "rb");
	if (!fs)
		{
		return Z_FOPEN_ERROR ; // boh, maybe this isn't the right value to return
		}
		
	ft = fopen (target, "wb");
	if (!ft)
		{
		fclose (fs);
		return Z_FOPEN_ERROR; // boh, maybe this isn't the right value to return
		}
		
	// allocate chunk memory
	in = malloc (CHUNK);
	if (!in) 
		{
		return Z_ALLOC_ERROR;
		}

	out = malloc (CHUNK);
	if (!out) 
		{
		free (in);
		return Z_ALLOC_ERROR;
		}

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, fs);
        if (ferror(fs)) 
			{
			ret = Z_ERRNO;
			goto end;
			}
			
        flush = feof(fs) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of fs has been read in */
        do 
			{
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, ft) != have || ferror(ft)) 
				{
                ret = Z_ERRNO;
				goto end;
				}
			} 
		while (strm.avail_out == 0);
        
		//assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    
    /* clean up and return */
 end:	
	fclose (fs);
	fclose (ft);
	
	free (in);
	free (out);
	
    deflateEnd(&strm);
	
    return Z_OK;
}
 