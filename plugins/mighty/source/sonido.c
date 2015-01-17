
// Banner sounds by oggzee
// Fixes by Clipper

#include <stdio.h>
#include <string.h>
#include <gctypes.h>
#include <malloc.h>
#include <asndlib.h>

#include "sonido.h"


/*
void hex_dump2(void *p, int size)
{
	int i = 0, j, x = 12;
	char *c = p;
	//printf("\n");
	while (i<size) {
		//printf("%02x ", i);
		//for (j=0; j<x && i+j<size; j++) printf("%02x", (int)c[i+j]);
		//printf(" |");
		for (j=0; j<x && i+j<size; j++) {
			unsigned cc = (unsigned char)c[i+j];
			if (cc < 32 || cc > 128) cc = '.';
			//printf("%c", cc);
		}
		//printf("|\n");
		i += x;
	}	
}

void dbg_hex_dump(void *p, int size)
{
	hex_dump2(p, size);
}*/

typedef struct {
	//u8 zeroes[128]; // padding
	u8 zeroes[0x40]; // padding
	u8 imet[4]; // "IMET"
	u8 unk[8];  // 0x0000060000000003 fixed, unknown purpose
	u32 sizes[3]; // icon.bin, banner.bin, sound.bin
	u32 flag1; // unknown
	u8 names[7][84]; // JP, EN, DE, FR, ES, IT, NL
	u8 zeroes_2[0x348]; // padding
	u8 crypto[16]; // MD5 of 0x40 to 0x640 in header.
} IMET;

struct U8_archive_header
{
	u8  tag[4]; // 0x55AA382D "U.8-"
	u32 rootnode_offset; // offset to root_node, always 0x20.
	u32 header_size; // size of header from root_node to end of string table.
	u32 data_offset; // offset to data -- this is rootnode_offset + header_size, aligned to 0x40.
	u8 zeroes[16];
};

struct U8_node 
{
	u8 type;
	u8 name_offset[3]; //really a "u24"
	u32 data_offset;
	u32 size;
};

typedef struct {
	u32 imd5; // 'IMD5'
	u32 filesize; //size of rest of file
	u8 zeroes[8]; //padding
	u8 crypto[16]; //MD5 of rest of file
} IMD5;

// Header
typedef struct {
	char bns[4]; // 'BNS '
	u32 version; // 0xFEFF0100 endianness and format version check
	u32 filesize; // size of entire BNS
	u16 headersize; // size of BNS header (including chunkinfo)
	u16 chunkcount; // number of chunks
	struct {
		u32 offset; // offset from start of BNS of chunk header
		u32 size; // size of chunk including header
	} chunkinfo[0]; //[chunkcount]; // info for each chunk
} BNS;
 
// Chunk header
typedef struct {
	char type[4]; // 'INFO' or 'DATA'
	u32 size; // size including header
} BNS_chunk;
 
// INFO chunk
typedef struct {
	u8 unk; // 0, possibly format control
	u8 loop; // loop flag, 0 = no loop, 1 = loop
	u8 channels; // channel count
	u8 unk2; // 0, padding?
	u16 samplerate; // sample rate (Hz)
	u16 pad; // 0
	u32 loopstart; // loop start sample
	u32 samples; // total sample count
	u32 offset; // offset (in INFO) to channel info offset list
	u32 unk4; // 0, padding?
} INFO_chunk;
 
// channel info offset list
typedef struct {
//	u32 offset[channels]; // offset (in INFO) to channel info for each channel
	u32 offset[0]; // offset (in INFO) to channel info for each channel
} channel_info_offset_list;
 
// channel info (for DSP ADPCM)
typedef struct {
	u32 data_offset; // offset (in DATA) of audio data for channel
	u32 info_offset; // offset (in INFO) of DSP info (0x30 byte block)
	u32 unk; // 0
} channel_info;


void adpcm_decode(void *dsp_buf, void *adpcm_info, void *adpcm_data,
		int samples, int channels, int cur_chan)
{
	// read:
	short *CoEfficients = adpcm_info; //[16];
	u8 *cdata = adpcm_data;
	// write:
	//int dsp_size = size / 8 * 14;
	int rsize = samples * 8 / 14;
	short *dsp = dsp_buf;
	// state
	int rpos = 0, wpos = cur_chan;
	short adpcmhistory1 = 0;
	short adpcmhistory2 = 0;

	while (rpos < rsize) {
		int sample;
		short nibbles[14];
		int index1;
		int i,j;
		u8 header;
		short delta;
		u8 thisbyte;
		short hist = adpcmhistory1;
		short hist2 = adpcmhistory2;
		// If we have loop data then we write to our looping file.
		//if (mscdata.Position == looppos)
		//	bwpdata = new BinaryWriter(msploop);

		//header = brcdata.ReadByte();
		header = cdata[rpos++];
		i = header & 0xFF;
		delta = (short) (1 << (i & 0xff & 0xf));
		index1 = (i & 0xff) >> 4;
		
		for(i = 0; i < 14; i += 2) {
			//thisbyte = brcdata.ReadByte();
			thisbyte = cdata[rpos++];
			j = (thisbyte & 0xff) >> 4;
			nibbles[i] = (short) j;
			j = thisbyte & 0xff & 0xf;
			nibbles[i+1] = (short) j;
		}

		for(i = 0; i < 14; i++) {
			if(nibbles[i] >= 8)
				nibbles[i] = (short)(nibbles[i] - 16);       
		}
			
		for(i = 0; i<14 ; i++) {
			sample = (delta * nibbles[i])<<11;
			sample += CoEfficients[index1 * 2] * hist;
			sample += CoEfficients[index1 * 2 + 1] * hist2;
			sample = sample + 1024;
			sample = sample >> 11;
			if(sample > 32767)
				sample = 32767;

			if(sample < -32768)
				sample = -32768;
			//bwpdata.Write((short)sample);
			dsp[wpos] = (short)sample;
			wpos += channels;
			if (wpos >= samples * channels) {
				rpos = rsize;
				break;
			}
			hist2 = hist;
			hist = (short)sample;
		}
		adpcmhistory1 = hist;
		adpcmhistory2 = hist2;

	}
}

u8* decompress_lz77(u8 *data, size_t data_size, size_t* decompressed_size)
{
	u8 *data_end;
	u8 *decompressed_data;
	size_t unpacked_size;
	u8 *in_ptr;
	u8 *out_ptr;
	u8 *out_end;

	in_ptr = data;
	data_end = data + data_size;
	// Assume this for now and grow when needed
	unpacked_size = data_size;
	decompressed_data = malloc(unpacked_size);
	out_end = decompressed_data + unpacked_size;
	out_ptr = decompressed_data;

	while (in_ptr < data_end) {
		int bit;
		u8 bitmask = *in_ptr;

		in_ptr++;
		for (bit = 0x80; bit != 0; bit >>= 1) {
			if (bitmask & bit) {
				// Next section is compressed
				u8 rep_length;
				u16 rep_offset;

				rep_length = (*in_ptr >> 4) + 3;
				rep_offset = *in_ptr & 0x0f;
				in_ptr++;
				rep_offset = *in_ptr | (rep_offset << 8);
				in_ptr++;
				if (out_ptr-decompressed_data < rep_offset) {
					//printf("Inconsistency in LZ77 encoding %x\n", in_ptr-data);
				}

				for ( ; rep_length > 0; rep_length--) {
					*out_ptr = out_ptr[-rep_offset-1];
					out_ptr++;
					if (out_ptr >= out_end) {
						// Need to grow buffer
						decompressed_data = realloc(decompressed_data, unpacked_size*2);
						out_ptr = decompressed_data + unpacked_size;
						unpacked_size *= 2;
						out_end = decompressed_data + unpacked_size;
					}
				}
			} else {
				// Just copy byte
				*out_ptr = *in_ptr;
				out_ptr++;
				if (out_ptr >= out_end) {
					// Need to grow buffer
					decompressed_data = realloc(decompressed_data, unpacked_size*2);
					out_ptr = decompressed_data + unpacked_size;
					unpacked_size *= 2;
					out_end = decompressed_data + unpacked_size;
				}
				in_ptr++;
			}
		}
	}

	*decompressed_size = (out_ptr - decompressed_data);
	return decompressed_data;
}

void parse_bns(void *data_bns, SoundInfo *snd)
{
	BNS *bns = data_bns;
	BNS_chunk *b_chunk = NULL;
	INFO_chunk *i_chunk = NULL;
	void *d_chunk = NULL;
	channel_info_offset_list *ci_off;
	channel_info *ci;
	void *dsp_data = NULL;
	int dsp_size = 0;
	int i;

	//printf("BNS: %.4s 0x%x %x %d\n", bns->bns,
	//		bns->version, bns->filesize, bns->chunkcount);
	if (memcmp(bns, "BNS ", 4) != 0) return;

	for (i=0; i<bns->chunkcount; i++) {
		b_chunk = (void*)bns + bns->chunkinfo[i].offset;
		//printf("%d %4x %4x %.4s %x\n", i,
		//		bns->chunkinfo[i].offset,
		//		bns->chunkinfo[i].size,
		//		b_chunk->type,
		//		b_chunk->size);
		if (memcmp(b_chunk->type, "INFO", 4)==0) {
			if (!i_chunk) i_chunk = (void*)(b_chunk+1);
			//printf("   l:%d c:%d s:%d %x %x %x\n",
			//	i_chunk->loop,
			//	i_chunk->channels,
			//	i_chunk->samplerate,
			//	i_chunk->loopstart,
			//	i_chunk->samples,
			//	i_chunk->offset);
		}
		if (memcmp(&b_chunk->type, "DATA", 4)==0) {
			if (!d_chunk) d_chunk = (void*)(b_chunk+1);
		}
	}

	if (!i_chunk || !d_chunk) return;
	if (i_chunk->channels < 1 || i_chunk->channels > 2) return;

	dsp_size = i_chunk->samples * i_chunk->channels * sizeof(short);
	dsp_data = memalign(32, dsp_size + 14*2);


	ci_off = (void*)i_chunk + i_chunk->offset;
	for (i=0; i<i_chunk->channels; i++) {
		int off = ci_off->offset[i];
		ci = (void*)i_chunk + off;
		//printf("   [%d] %x data: %x info: %x\n", i, off,
		//		ci->data_offset, ci->info_offset); 
		void *adpcm_data = (void*)d_chunk + ci->data_offset;
		void *adpcm_info = (void*)i_chunk + ci->info_offset;
		adpcm_decode(dsp_data, adpcm_info, adpcm_data,
				i_chunk->samples, i_chunk->channels, i);
	}
	snd->dsp_data = dsp_data;
	snd->size = dsp_size;
	snd->channels = i_chunk->channels;
	snd->rate = i_chunk->samplerate;
	snd->loop = i_chunk->loop;

	//printf("LOOPSTART: %d   ",i_chunk->loopstart);
}


/*
The canonical WAVE format starts with the RIFF header:

Offset  Size  Name             Description

0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                               (0x52494646 big-endian form).
4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                               4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                               This is the size of the rest of the chunk 
                               following this number.  This is the size of the 
                               entire file in bytes minus 8 bytes for the
                               two fields not included in this count:
                               ChunkID and ChunkSize.
8         4   Format           Contains the letters "WAVE"
                               (0x57415645 big-endian form).

The "WAVE" format consists of two subchunks: "fmt " and "data":
The "fmt " subchunk describes the sound data's format:

12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some 
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
          2   ExtraParamSize   if PCM, then doesn't exist
          X   ExtraParams      space for extra parameters

The "data" subchunk contains the size of the data and the actual sound:

36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this 
                               number.
44        *   Data             The actual sound data.
*/

typedef struct RIFF
{
	char ChunkID[4];	// Contains the letters "RIFF" in ASCII form
	int ChunkSize;		// 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
	char Format[4];		// Contains the letters "WAVE"
	//The "WAVE" format consists of two subchunks: "fmt " and "data":

	//The "fmt " subchunk describes the sound data's format:
	char Subchunk1ID[4];// Contains the letters "fmt "
	int Subchunk1Size;	// 16 for PCM.  This is the size of the rest of the Subchunk
	short AudioFormat;	// PCM = 1 (i.e. Linear quantization)
	short NumChannels;	// Mono = 1, Stereo = 2, etc.
	int SampleRate;		// 8000, 44100, etc.
	int ByteRate;		// == SampleRate * NumChannels * BitsPerSample/8
	short BlockAlign;	// == NumChannels * BitsPerSample/8
	short BitsPerSample;// 8 bits = 8, 16 bits = 16, etc.
    // 2 ExtraParamSize // if PCM, then doesn't exist
    // X   ExtraParams  // space for extra parameters
} RIFF;

typedef struct RIFF_chnk
{
	char id[4];
	int size; 
} RIFF_chnk;

void parse_riff(void *data, SoundInfo *snd)
{
	RIFF *riff = data;
	int riff_size = _le32(&riff->ChunkSize);
	int sub1_size = _le32(&riff->Subchunk1Size);
	//dbg_hex_dump(data, 128);
	//printf("'%.4s %x %.4s %.4s %x\n",
	//		riff->ChunkID, _le32(&riff->ChunkSize), riff->Format,
	//		riff->Subchunk1ID, sub1_size);
	if (memcmp(riff->ChunkID, "RIFF", 4)) return;
	if (memcmp(riff->Format,  "WAVE", 4)) return;
	if (memcmp(riff->Subchunk1ID,  "fmt ", 4)) return;

	short AudioFormat   = _le16(&riff->AudioFormat);
	short NumChannels   = _le16(&riff->NumChannels);
	short BitsPerSample = _le16(&riff->BitsPerSample);
	int SampleRate      = _le32(&riff->SampleRate);
	if (AudioFormat != 1 || BitsPerSample != 16) return;
	if (NumChannels < 1 || NumChannels > 2) return;

	//printf("f:%d c:%d r:%d b:%d\n", AudioFormat, NumChannels,
	//		SampleRate, BitsPerSample);
	RIFF_chnk *chnk = ((void*)riff) + 12 + 8 + sub1_size;
	RIFF_chnk *riff_data = NULL;
	while (((void*)chnk > data) && ((void*)chnk < data + riff_size)) {
		int size = _le32(&chnk->size);
		//printf("[%.4s] %x\n", chnk->id, size);
		//dbg_pause();
		if (memcmp(chnk->id, "data", 4)==0) {
			riff_data = chnk;
			break;
		}
		if (size <= 0) break;
		chnk = ((void*)chnk) + sizeof(RIFF_chnk) + size;
	}
	if (riff_data == NULL) return;

	int Size = _le32(&riff_data->size);
	short *ss = (void*)(riff_data + 1);
	short *dsp_data;
	int i;
	dsp_data = memalign(32, Size);
	if (!dsp_data) return;
	// byte order
	for (i=0; i<Size/2; i++) {
		dsp_data[i] = _le16(ss);
		ss++;
	}
	snd->dsp_data = dsp_data;
	snd->channels = NumChannels;
	snd->rate = SampleRate;
	snd->size = Size;
	snd->loop = 0;
}

/*
Byte order: Big-endian

Offset   Length   Contents
  0      4 bytes  "FORM"
  4      4 bytes  <File size - 8>
  8      4 bytes  "AIFF"
[ // Chunks
         4 bytes  <Chunk magic>
         4 bytes  <Chunk data size(x)>
        (x)bytes  <Chunk data>
]*

COMM chunk: Must be defined
  0      4 bytes  "COMM"
  4      4 bytes  <COMM chunk size>  // (=18)
  8      2 bytes  <Number of channels(c)>
 10      4 bytes  <Number of frames(f)>
 14      2 bytes  <bits/samples(b)>  // 1..32
 16     10 bytes  <Sample rate (Extended 80-bit floating-point format)>

SSND chunk: Must be defined
  0      4 bytes  "SSND"
  4      4 bytes  <Chunk size(x)>
  8      4 bytes  <Offset(n)>
 12      4 bytes  <block size>       // (=0)
 16     (n)bytes  Comment
 16+(n) (s)bytes  <Sample data>      // (s) := (x) - (n) - 8
*/   

typedef struct CHNK
{
	char id[4];
	int  size;
} CHNK;

typedef struct AIFF
{
	char id[4]; // FORM
	int  size;
	char aiff[4]; // AIFF
} AIFF;

/*
typedef struct COMM
{
	char id[4];		// 4 bytes  "COMM"
	int size;		// 4 bytes  <COMM chunk size>  // (=18)
	u16 channels;	// 2 bytes  <Number of channels(c)>
	int frames;		// 4 bytes  <Number of frames(f)>
	u16 bits;		// 2 bytes  <bits/samples(b)>  // 1..32
	char rate[10];  //10 bytes  <Sample rate (Extended 80-bit floating-point format)>
} COMM;
*/
typedef struct COMM
{
	char id[4];		// 4 bytes  "COMM"
	int size;		// 4 bytes  <COMM chunk size>  // (=18)
	u16 channels;	// 2 bytes  <Number of channels(c)>
	u8  frames[4];	// 4 bytes  <Number of frames(f)>
	u16 bits;		// 2 bytes  <bits/samples(b)>  // 1..32
	u8  rate[10];
	//u16 pad1;
	//u16 rate;
	//u8  pad[6];  //10 bytes  <Sample rate (Extended 80-bit floating-point format)>
} COMM;

typedef struct SSND
{
	char id[4];	// 4 bytes  "SSND"
	int size;	// 4 bytes  <Chunk size(x)>
	int offset;	// 4 bytes  <Offset(n)>
	int bsize;	// 4 bytes  <block size>       // (=0)
	//(n)bytes  Comment
	//(s)bytes  <Sample data>      // (s) := (x) - (n) - 8
} SSND;

double ext_to_double(unsigned char a_extended[10])
{
	int exponent,i;
	char found;
	union {
		double dval;
		unsigned char cval[8];
	} value;
	unsigned char extended[10];
	memcpy(extended, a_extended, 10);
	//for (i=0;i<10;i++) printf("%02x ", extended[i]); puts("");
	/* I'll assume little endian for now.  Easy to fix to big endian anyway */
	/* also assuming positive for simplicity */
	found=0;
	for (i = 2; i < 10; i++) found |= extended[i];
	if (!found) return 0;
	exponent = (((extended[0]&0x7F) << 8) | extended[1]) - 15359;
	found=0;
	while(!found) {
		found = (extended[2] & 0x80);
		for (i = 2; i < 9; i++) extended[i] = (extended[i] << 1) | (extended[i+1] >> 7);
		extended[9] = extended[9] << 1;
		exponent--;
	}
	/* Little Endian here:
	   value.cval[7] = extended[0]&0x80 | (exponent >> 4);
	   value.cval[6] = (exponent << 4) | (extended[2] >> 4);
	   for (i=0;i<6; i++) {
	   value.cval[5-i] = (extended[i+2] << 4) | (extended[i+3] >> 4);
	   }
	 */
	value.cval[0] = (extended[0]&0x80) | (exponent >> 4);
	value.cval[1] = (exponent << 4) | (extended[2] >> 4);
	for (i=0;i<6; i++) {
		value.cval[i+2] = (extended[i+2] << 4) | (extended[i+3] >> 4);
	}
	return value.dval;
}

void parse_aiff(void *data, SoundInfo *snd)
{
	AIFF *aiff = data;
	CHNK *chnk = (CHNK*)(aiff+1);
	COMM *comm = NULL;
	SSND *ssnd = NULL;
	int rate = 0;
	//printf("%.4s %x %.4s\n", aiff->id, aiff->size, aiff->aiff);
	while ((void*)chnk < data + aiff->size) {
		//printf("[%.4s] %d %x\n", chnk->id, chnk->size, chnk->size);
		if (memcmp(chnk->id, "COMM", 4)==0 && !comm) {
			comm = (COMM*)chnk;
			rate = (int)ext_to_double(comm->rate);
			//printf("c:%d f:%x b:%d r:%d\n", comm->channels,
			//		_be32(comm->frames), comm->bits, rate);
		}
		if (memcmp(chnk->id, "SSND", 4)==0 && !ssnd) {
			ssnd = (SSND*)chnk;
			//printf("%d %d\n", ssnd->offset, ssnd->bsize);
		}
		// next chunk:
		chnk = (void*)(chnk+1) + chnk->size;
	}
	if (!comm || !ssnd) return;
	int dsp_size = ssnd->size - ssnd->offset - 8;
	void *dsp_data = memalign(32, dsp_size);
	if (!dsp_data) return;
	memcpy(dsp_data, (void*)(ssnd+1) + ssnd->offset, dsp_size);
	snd->dsp_data = dsp_data;
	snd->channels = comm->channels;
	snd->rate = rate;
	snd->size = dsp_size;
	snd->loop = 0;
}

void parse_banner_snd(void *banner, SoundInfo *snd){
	//void *data_hdr = banner + sizeof(IMET);
	void *data_hdr = banner + 0x640;
	struct U8_archive_header *u8_hdr;
	struct U8_node *node;
	int i, num, off;
	char *name_start, *name;
	u8 u8_tag[4] = {0x55, 0xAA, 0x38, 0x2D}; // "U.8-"
	
	//dbg_hex_dump(banner, 128);
	u8_hdr = data_hdr;
	//printf("imet: %.4s\n", ((IMET*)banner)->imet);
	//printf("U8: %.4s 0x%x\n", u8_hdr->tag, u8_hdr->rootnode_offset);
	//dbg_pause();
	if (memcmp(u8_hdr->tag, u8_tag, 4)) return;

	node = data_hdr + u8_hdr->rootnode_offset;
	num = node->size;
	name_start = (char*)&node[num];
	if (num>5) num = 5;
	for (i=1; i<num; i++) {
		off = *(int*)(&node[i]) & 0x00FFFFFF;
		name = name_start + off;
		//printf("%d %.20s [0x%x] @ %x\n", i, name, node[i].size, node[i].data_offset);
		//dbg_pause();
		if (strcmp(name, "sound.bin") == 0) {
			IMD5 *imd5 = data_hdr + node[i].data_offset;
			void *sound_data = (void*)imd5 + sizeof(IMD5);
			unsigned size = imd5->filesize;
			void *lz_data = NULL;
			retry:
			//printf("[%.4s]\n", (char*)sound_data);
			//dbg_pause();
			if (memcmp(sound_data, "BNS ", 4)==0) {
				parse_bns(sound_data, snd);
			} else if (memcmp(sound_data, "RIFF", 4)==0) {
				parse_riff(sound_data, snd);
			} else if (memcmp(sound_data, "FORM", 4)==0) {
				parse_aiff(sound_data, snd);
			} else if (memcmp(sound_data, "LZ77", 4)==0 && lz_data == NULL) {
				//unsigned lz_hdr = ((unsigned*)sound_data)[1];
				lz_data = decompress_lz77(sound_data+8, size-8, &size);
				//printf("de-LZ: %p %x %x %x %x %.4s\n", lz_data,
				//		node[i].size, size, lz_hdr, lz_hdr>>8, (char*)lz_data);
				sound_data = lz_data;
				goto retry;
			}
			//SAFE_FREE(lz_data);
			if(lz_data){
				free(lz_data);
				lz_data=NULL;}
			
		}
	}
	//printf("snd: %p %x\n", snd->dsp_data, snd->size);
}


