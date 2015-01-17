/*===========================================
        GRRLIB 4.0.0
        Code     : NoNameNo
        Additional Code : Crayon
        GX hints : RedShade

Download and Help Forum : http://grrlib.santo.fr
===========================================*/

// Modified by oggzee and usptactical

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "pngu/pngu.h"
#include <setjmp.h>
#include "jpeglib.h"
#include "my_GRRLIB.h"
#include <fat.h> 
#include "console.h"
#include "mem.h"

#define DEFAULT_FIFO_SIZE (256 * 1024)
void *gp_fifo = NULL;

/**
 * Convert a raw bmp (RGB, no alpha) to 4x4RGBA.
 * @author DrTwox
 * @param src
 * @param dst
 * @param width
 * @param height
*/
static void RawTo4x4RGBA(const unsigned char *src, void *dst, const unsigned int width, const unsigned int height) {
    unsigned int block;
    unsigned int i;
    unsigned int c;
    unsigned int ar;
    unsigned int gb;
    unsigned char *p = (unsigned char*)dst;

    for (block = 0; block < height; block += 4) {
        for (i = 0; i < width; i += 4) {
            /* Alpha and Red */
            for (c = 0; c < 4; ++c) {
                for (ar = 0; ar < 4; ++ar) {
                    /* Alpha pixels */
                    *p++ = 255;
                    /* Red pixels */
                    *p++ = src[((i + ar) + ((block + c) * width)) * 3];
                }
            }

            /* Green and Blue */
            for (c = 0; c < 4; ++c) {
                for (gb = 0; gb < 4; ++gb) {
                    /* Green pixels */
                    *p++ = src[(((i + gb) + ((block + c) * width)) * 3) + 1];
                    /* Blue pixels */
                    *p++ = src[(((i + gb) + ((block + c) * width)) * 3) + 2];
                }
            }
        } /* i */
    } /* block */
}

//*************************************************
//* jpeglib code cleanup and error handling added by usptactical

//error manager struct for jpeglib
struct my_error_mgr {
	struct jpeg_error_mgr pub;
	//return to the caller on error
	jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr * my_error_ptr;

/**
 * Overrides the standard jpeglib error handler
 * @param cinfo Pointer to my_error_mgr struct
 * @return void
 */
METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
	//cinfo->err really points to a my_error_mgr struct
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	//display the internal jpeglib error message
	//(*cinfo->err->output_message) (cinfo);

	jpeg_abort((j_common_ptr) cinfo);
	
	//Return control to the setjmp point
	longjmp(myerr->setjmp_buffer, 1);
}

/**
 * Output message handler for jpeglib errors
 * @param cinfo Pointer to my_error_mgr struct
 * @return void
 */
METHODDEF(void) output_message(j_common_ptr cinfo)
{
	// display an error message.
	printf("\nError with JPEG!\n");
}


/**
 * Load a jpg from a buffer.
 * Take Care to have a JPG finnishing by 0xFF 0xD9 !!!!
 * @param my_jpg the JPEG buffer to load.
 * @return allocated image buffer
 */
void* Load_JPG_RGB(const unsigned char my_jpg[], int *w, int *h)
{
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPROW row_pointer[1];
	unsigned char *tempBuffer = 0;
	unsigned int i;
	int n;

	//init the texture object
	*w = 0;
	*h = 0;

	//get the jpg size
	n = 0;
    if((my_jpg[0]==0xff) && (my_jpg[1]==0xd8) && (my_jpg[2]==0xff)) {
        while(1) {
            if((my_jpg[n]==0xff) && (my_jpg[n+1]==0xd9))
                break;
            n++;
        }
        n+=2;
    }
	
	//Set up the error handler first in case the initialization step fails
	cinfo.err = jpeg_std_error(&jerr.pub);
	cinfo.err->error_exit = my_error_exit;
	cinfo.err->output_message = output_message;
	//Establish the setjmp return context for my_error_exit to use
	if (setjmp(jerr.setjmp_buffer)) {
		//If we get here, the JPEG code has signaled an error.
		//We need to clean up the JPEG object, and return.
		//TODO: mem is not being cleaned up properly - I assume jpeglib's mem alloc is the culprit
		jpeg_destroy_decompress(&cinfo);
		SAFE_FREE(row_pointer[0]);
		SAFE_FREE(tempBuffer);
		*w = -666;
		return NULL;
	}
	
	//initialize the decompression object
    jpeg_create_decompress(&cinfo);
    cinfo.progress = NULL;

//	jpeg_memory_src(&cinfo, my_jpg, n);
	jpeg_mem_src(&cinfo, (unsigned char *)my_jpg, n);

    jpeg_read_header(&cinfo, TRUE);
    if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
        cinfo.out_color_space = JCS_RGB; //JCS_CMYK; //JCS_RGB;
	//these speed up decompression...
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_block_smoothing = FALSE;
	//initialize internal state, allocate working memory, and prepare for returning data
	jpeg_start_decompress(&cinfo);
	
	tempBuffer = malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);
    row_pointer[0] = malloc(cinfo.output_width * cinfo.output_components);

    size_t location = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, row_pointer, 1);
        for (i = 0; i < cinfo.image_width * cinfo.output_components; i++) {
            /* Put the decoded scanline into the tempBuffer */
            tempBuffer[ location++ ] = row_pointer[0][i];
        }
    }

    //Complete the decompression cycle. This causes working memory 
	// associated with the JPEG object to be released.
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    SAFE_FREE(row_pointer[0]);

	//set texture dimensions
    *w = cinfo.output_width;
    *h = cinfo.output_height;
    return tempBuffer;
}

/**
 * Load a texture from a buffer.
 * Take Care to have a JPG finnishing by 0xFF 0xD9 !!!!
 * @author DrTwox
 * @param my_jpg the JPEG buffer to load.
 * @return A GRRLIB_texImg structure filled with PNG informations.
 */
GRRLIB_texImg my_GRRLIB_LoadTextureJPG(const unsigned char my_jpg[])
{
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPROW row_pointer[1];
	unsigned char *tempBuffer = 0;
	GRRLIB_texImg my_texture;
	unsigned int i;
	int n;

	//init the texture object
	my_texture.w = 0;
	my_texture.h = 0;
	my_texture.data = NULL;

	//get the jpg size
	n = 0;
    if((my_jpg[0]==0xff) && (my_jpg[1]==0xd8) && (my_jpg[2]==0xff)) {
        while(1) {
            if((my_jpg[n]==0xff) && (my_jpg[n+1]==0xd9))
                break;
            n++;
        }
        n+=2;
    }
	
	//Set up the error handler first in case the initialization step fails
	cinfo.err = jpeg_std_error(&jerr.pub);
	cinfo.err->error_exit = my_error_exit;
	cinfo.err->output_message = output_message;
	//Establish the setjmp return context for my_error_exit to use
	if (setjmp(jerr.setjmp_buffer)) {
		//If we get here, the JPEG code has signaled an error.
		//We need to clean up the JPEG object, and return.
		//TODO: mem is not being cleaned up properly - I assume jpeglib's mem alloc is the culprit
		jpeg_destroy_decompress(&cinfo);
		SAFE_FREE(row_pointer[0]);
		SAFE_FREE(tempBuffer);
		SAFE_FREE(my_texture.data);
		my_texture.w = -666;
		return my_texture;
	}
	
	//initialize the decompression object
    jpeg_create_decompress(&cinfo);
    cinfo.progress = NULL;

//	jpeg_memory_src(&cinfo, my_jpg, n);
	jpeg_mem_src(&cinfo, (unsigned char *)my_jpg, n);

    jpeg_read_header(&cinfo, TRUE);
    if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
        cinfo.out_color_space = JCS_RGB; //JCS_CMYK; //JCS_RGB;
	//these speed up decompression...
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_block_smoothing = FALSE;
	//initialize internal state, allocate working memory, and prepare for returning data
	jpeg_start_decompress(&cinfo);
	
	
//TODO...this seems to work ok
    //tempBuffer = mem_alloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);
    //row_pointer[0] = mem_alloc(cinfo.output_width * cinfo.output_components);
    
	tempBuffer = malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);
    row_pointer[0] = malloc(cinfo.output_width * cinfo.output_components);

    size_t location = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, row_pointer, 1);
        for (i = 0; i < cinfo.image_width * cinfo.output_components; i++) {
            /* Put the decoded scanline into the tempBuffer */
            tempBuffer[ location++ ] = row_pointer[0][i];
        }
    }

    //Complete the decompression cycle. This causes working memory 
	// associated with the JPEG object to be released.
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    SAFE_FREE(row_pointer[0]);

    /* Create a buffer to hold the final texture */
    //my_texture.data = memalign(32, cinfo.output_width * cinfo.output_height * 4);
    my_texture.data = mem_alloc(cinfo.output_width * cinfo.output_height * 4);
    RawTo4x4RGBA(tempBuffer, my_texture.data, cinfo.output_width, cinfo.output_height);
    SAFE_FREE(tempBuffer);
	//set texture dimensions
    my_texture.w = cinfo.output_width;
    my_texture.h = cinfo.output_height;
    GRRLIB_FlushTex(&my_texture);
    return my_texture;
}

int _GRRLIB_Init(void *fb0, void *fb1)
{
    f32 yscale;
    u32 xfbHeight;
    Mtx44 perspective;

	if (fb0) {
	    xfb[0] = fb0;
	} else {
	    xfb[0] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	}

	if (fb1) {
	    xfb[1] = fb1;
	} else {
	    xfb[1] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	}

    if(xfb[0] == NULL || xfb[1] == NULL)
        return -1;

	if (fb0 == NULL)
		VIDEO_ClearFrameBuffer(rmode, xfb[0], COLOR_BLACK);
	if (fb1 == NULL)
		VIDEO_ClearFrameBuffer(rmode, xfb[1], COLOR_BLACK);

	fb = 0;

	if (fb0 == NULL) {
		VIDEO_SetNextFramebuffer(xfb[fb]);
		VIDEO_SetBlack(FALSE);
		VIDEO_Flush();
		VIDEO_WaitVSync();
		if(rmode->viTVMode&VI_NON_INTERLACE)
			VIDEO_WaitVSync();
	}

    gp_fifo = (u8 *) memalign(32, DEFAULT_FIFO_SIZE);
    if(gp_fifo == NULL)
        return -1;
    memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
    GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

    // clears the bg to color and clears the z buffer
    GXColor background = { 0, 0, 0, 0xff };
    GX_SetCopyClear (background, GX_MAX_Z24);

    // other gx setup
    yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

    if (rmode->aa)
        GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
    else
        GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

    GX_SetDispCopyGamma(GX_GM_1_0);


    // setup the vertex descriptor
    // tells the flipper to expect direct data
    GX_ClearVtxDesc();
    GX_InvVtxCache ();
    GX_InvalidateTexAll();

    GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);


    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetZMode (GX_FALSE, GX_LEQUAL, GX_TRUE);

    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

    guMtxIdentity(GXmodelView2D);
    guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

    guOrtho(perspective,0, 480, 0, 640, 0, 300.0F);
    GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_TRUE);

    GX_SetCullMode(GX_CULL_NONE);

    GRRLIB_Settings.antialias = true;
	
	return 0;
}

#if 0

void GRRLIB_Init()
{
	_GRRLIB_Init_Video();
	_GRRLIB_Init(NULL, NULL);
}

#endif

int GRRLIB_Init_VMode(GXRModeObj *a_rmode, void *fb0, void *fb1)
{
	if (a_rmode == NULL) return -1;
	rmode = a_rmode;
	return _GRRLIB_Init(fb0, fb1);
}

#if 0

/**
 * Call this function after drawing.
 */
void GRRLIB_Render() {
    GX_DrawDone ();

    fb ^= 1;        // flip framebuffer
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(xfb[fb], GX_TRUE);
    VIDEO_SetNextFramebuffer(xfb[fb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

#endif

void _GRRLIB_Render() {
    GX_DrawDone ();
    fb ^= 1;        // flip framebuffer
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(xfb[fb], GX_TRUE);
}

void _GRRLIB_VSync() {
    VIDEO_SetNextFramebuffer(xfb[fb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
}

void** _GRRLIB_GetXFB(int *cur_fb)
{
	*cur_fb = fb;
	return xfb;
}

void _GRRLIB_SetFB(int cur_fb)
{
	fb = cur_fb;
}

#if 0

/**
 * Call this before exiting your application.
 */
void GRRLIB_Exit() {
    GX_Flush();
    GX_AbortFrame();

    if(xfb[0] != NULL) {
        free(MEM_K1_TO_K0(xfb[0]));
        xfb[0] = NULL;
    }
    if(xfb[1] != NULL) {
        free(MEM_K1_TO_K0(xfb[1]));
        xfb[1] = NULL;
    }
    if(gp_fifo != NULL) {
        free(gp_fifo);
        gp_fifo = NULL;
    }
}

#endif

void _GRRLIB_Exit() {
    GX_Flush();
    GX_AbortFrame();

    if(gp_fifo != NULL) {
        free(gp_fifo);
        gp_fifo = NULL;
    }
}


void GRRLIB_DrawTile_begin0(GRRLIB_texImg *tex)
{
	GXTexObj texObj;

	GX_InitTexObj(&texObj, tex->data, tex->tilew*tex->nbtilew, tex->tileh*tex->nbtileh, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	const int antialias = 1;
	if (antialias == 0) {
		GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
		GX_SetCopyFilter(GX_FALSE, rmode->sample_pattern, GX_FALSE, rmode->vfilter);
	} else {
		GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
	}
	GX_LoadTexObj(&texObj, GX_TEXMAP0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);
}

void GRRLIB_DrawTile_begin(GRRLIB_texImg *tex, f32 x, f32 y, f32 scale)
{
	GRRLIB_DrawTile_begin0(tex);

	Mtx m, m1, m2, mv;
	guMtxIdentity (m1);
	guMtxScaleApply(m1, m1, scale, scale, 1.0f);
	guVector axis = (guVector) {0, 0, 1 };
	guMtxRotAxisDeg (m2, &axis, 0); // 0 degrees rotate
	guMtxConcat(m2, m1, m);
	guMtxTransApply(m, m, x, y, 0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (m, GX_PNMTX0);
}


inline void GRRLIB_DrawTile_draw(f32 xpos, f32 ypos, GRRLIB_texImg tex, float degrees, float scaleX, f32 scaleY, u32 color, int frame)
{
    f32 width, height;
    Mtx m, m1, m2, mv;

    // Frame Correction by spiffen
    f32 FRAME_CORR = 0.001f;
    f32 s1 = (((frame%tex.nbtilew))/(f32)tex.nbtilew)+(FRAME_CORR/tex.w);
    f32 s2 = (((frame%tex.nbtilew)+1)/(f32)tex.nbtilew)-(FRAME_CORR/tex.w);
    f32 t1 = (((int)(frame/tex.nbtilew))/(f32)tex.nbtileh)+(FRAME_CORR/tex.h);
    f32 t2 = (((int)(frame/tex.nbtilew)+1)/(f32)tex.nbtileh)-(FRAME_CORR/tex.h);

    width = tex.tilew * 0.5f;
    height = tex.tileh * 0.5f;
    guMtxIdentity (m1);
    guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0f);
    guVector axis = (guVector) {0, 0, 1 };
    guMtxRotAxisDeg (m2, &axis, degrees);
    guMtxConcat(m2, m1, m);
    guMtxTransApply(m, m, xpos+width, ypos+height, 0);
    guMtxConcat (GXmodelView2D, m, mv);
    GX_LoadPosMtxImm (mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position3f32(-width, -height, 0);
    GX_Color1u32(color);
    GX_TexCoord2f32(s1, t1);

    GX_Position3f32(width, -height,  0);
    GX_Color1u32(color);
    GX_TexCoord2f32(s2, t1);

    GX_Position3f32(width, height,  0);
    GX_Color1u32(color);
    GX_TexCoord2f32(s2, t2);

    GX_Position3f32(-width, height,  0);
    GX_Color1u32(color);
    GX_TexCoord2f32(s1, t2);
    GX_End();
    GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
}

inline void GRRLIB_DrawTile_draw1(f32 xpos, f32 ypos, GRRLIB_texImg *tex, u32 color, int frame)
{
    //f32 width, height;

    // Frame Correction by spiffen
    const f32 FRAME_CORR = 0.001f;
    f32 s1, s2, t1, t2;
    s1 = (frame % tex->nbtilew) * tex->ofnormaltexx;
    s2 = s1 + tex->ofnormaltexx;
    t1 = (int)(frame/tex->nbtilew) * tex->ofnormaltexy;
    t2 = t1 + tex->ofnormaltexy;
	s1 += FRAME_CORR;
	s2 -= FRAME_CORR;
	t1 += FRAME_CORR;
	t2 -= 0.0015f;


    //width = tex->tilew * 0.5f;
    //height = tex->tileh * 0.5f;
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position3f32(xpos, ypos, 0);
    GX_Color1u32(color);
    GX_TexCoord2f32(s1, t1);

    GX_Position3f32(xpos+tex->tilew, ypos,  0);
    GX_Color1u32(color);
    GX_TexCoord2f32(s2, t1);

    GX_Position3f32(xpos+tex->tilew, ypos+tex->tileh,  0);
    GX_Color1u32(color);
    GX_TexCoord2f32(s2, t2);

    GX_Position3f32(xpos, ypos+tex->tileh,  0);
    GX_Color1u32(color);
    GX_TexCoord2f32(s1, t2);
    GX_End();
}

inline void GRRLIB_DrawTile_end0(GRRLIB_texImg *tex)
{
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}

inline void GRRLIB_DrawTile_end(GRRLIB_texImg *tex)
{
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
	GRRLIB_DrawTile_end0(tex);
}

/**
 * Draw a slice.
 * @param xpos specifies the x-coordinate of the upper-left corner.
 * @param ypos specifies the y-coordinate of the upper-left corner.
 * @param tex texture to draw.
 * @param degrees angle of rotation.
 * @param scaleX
 * @param scaleY
 * @param color1 top
 * @param color2 bottom
 * @param x,y,w,h slice coords inside texture
 */
void GRRLIB_DrawSlice2(f32 xpos, f32 ypos, GRRLIB_texImg tex, float degrees,
		float scaleX, f32 scaleY, u32 color1, u32 color2,
		float x, float y, float w, float h)
{
    GXTexObj texObj;
    f32 width, height;
    Mtx m, m1, m2, mv;

    // Frame Correction by spiffen
	/*
    f32 FRAME_CORR = 0.001f;
    f32 s1 = (((frame%tex.nbtilew))/(f32)tex.nbtilew)+(FRAME_CORR/tex.w);
    f32 s2 = (((frame%tex.nbtilew)+1)/(f32)tex.nbtilew)-(FRAME_CORR/tex.w);
    f32 t1 = (((int)(frame/tex.nbtilew))/(f32)tex.nbtileh)+(FRAME_CORR/tex.h);
    f32 t2 = (((int)(frame/tex.nbtilew)+1)/(f32)tex.nbtileh)-(FRAME_CORR/tex.h);
	*/
    f32 FRAME_CORR = 0.001f;
    f32 s1 = (x     / (f32)tex.w) + (FRAME_CORR/tex.w);
    f32 s2 = ((x+w) / (f32)tex.w) - (FRAME_CORR/tex.w);
    f32 t1 = (y     / (f32)tex.h) + (FRAME_CORR/tex.h);
    f32 t2 = ((y+h) / (f32)tex.h) - (FRAME_CORR/tex.h);

    //GX_InitTexObj(&texObj, tex.data, tex.tilew*tex.nbtilew, tex.tileh*tex.nbtileh, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObj(&texObj, tex.data, tex.w, tex.h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
    GX_LoadTexObj(&texObj, GX_TEXMAP0);

    GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

    //width = tex.tilew * 0.5f;
    //height = tex.tileh * 0.5f;
    width = w * 0.5f;
    height = h * 0.5f;
    guMtxIdentity (m1);
    guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0f);
    guVector axis = (guVector) {0, 0, 1 };
    guMtxRotAxisDeg (m2, &axis, degrees);
    guMtxConcat(m2, m1, m);
    guMtxTransApply(m, m, xpos+width, ypos+height, 0);
    guMtxConcat (GXmodelView2D, m, mv);
    GX_LoadPosMtxImm (mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position3f32(-width, -height, 0);
    GX_Color1u32(color1);
    GX_TexCoord2f32(s1, t1);

    GX_Position3f32(width, -height,  0);
    GX_Color1u32(color1);
    GX_TexCoord2f32(s2, t1);

    GX_Position3f32(width, height,  0);
    GX_Color1u32(color2);
    GX_TexCoord2f32(s2, t2);

    GX_Position3f32(-width, height,  0);
    GX_Color1u32(color2);
    GX_TexCoord2f32(s1, t2);
    GX_End();
    GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}

void GRRLIB_DrawSlice(f32 xpos, f32 ypos, GRRLIB_texImg tex, float degrees,
		float scaleX, f32 scaleY, u32 color,
		float x, float y, float w, float h)
{
	GRRLIB_DrawSlice2(xpos, ypos, tex, degrees,
		scaleX, scaleY, color, color,
		x, y, w, h);
}

#define UF_CACHE_SIZE 512
int uf_cache_c[UF_CACHE_SIZE];
struct GRRLIB_texImg uf_cache_tx[UF_CACHE_SIZE];
void *uf_cache_data = NULL;
int uf_cache_next = 0;

void unifont_cache_init()
{
	uf_cache_data = mem2_alloc(16*16*4*UF_CACHE_SIZE);
	// seems texture data has to be in mem2
	// if it's in mem1 it doesn't display properly
	// strange...
}

void GRRLIB_InitTexture(GRRLIB_texImg *tx, int w, int h, void *data)
{
	memset(tx, 0, sizeof(GRRLIB_texImg));
    tx->w = w;
    tx->h = h;
    tx->data = data;
	memset(data, 0, w*h*4);
}

int unifont_to_tx(struct GRRLIB_texImg *tx, int c)
{
	int x, y, xx, i;
	u8 *glyph;
	int off, len;
	u32 color;
	if (!unifont) return 0;
	if (!unifont->index[c]) return 0;
	if (tx->data == NULL) return 0;
	off = unifont->index[c] >> 8;
	len = unifont->index[c] & 0x0F;
	if (len > 2) len = 2;
	GRRLIB_InitTileSet(tx, 8*len, 16, 0);
	glyph = unifont_glyph + off * 16;
	xx = 0;
	for (i=0; i<len; i++) {
		for (y=0; y<16; y++) {
			for (x=0; x<8; x++) {
				if (*glyph & (0x80>>x)) {
					color = 0xFFFFFFFF;
				} else {
					color = 0x00000000;
				}
				GRRLIB_SetPixelTotexImg(xx+x, y, tx, color);
			}
			glyph++;
		}
		xx += 8;
	}
	GRRLIB_FlushTex(tx);
	return len;
}


struct GRRLIB_texImg* get_unifont_cache(int c)
{
	int i;
	struct GRRLIB_texImg *tx;
	if (!uf_cache_data) return NULL;
	for (i=0; i<UF_CACHE_SIZE; i++) {
		if (uf_cache_c[i] == c) break;
	}
	if (i == UF_CACHE_SIZE) {
		i = uf_cache_next;
		uf_cache_c[i] = c;
		uf_cache_next++;
		uf_cache_next %= UF_CACHE_SIZE;
		tx = &uf_cache_tx[i];
		GX_DrawDone(); // in case we're overwriting a texture in GX queue
		GRRLIB_InitTexture(tx, 16, 16, uf_cache_data + 16*16*4*i);
		unifont_to_tx(tx, c);
		return tx;
	}
	tx = &uf_cache_tx[i];
	return tx;
}

float draw_unifont(f32 xpos, f32 ypos, float w, float h, u32 color, int c)
{
	struct GRRLIB_texImg *tx;
	int len;
	if (!unifont) return 0;
	if (!unifont->index[c]) return 0;
	if (!uf_cache_data) return 0;
	tx = get_unifont_cache(c);
	len = unifont->index[c] & 0x0F;
	if (len > 2) len = 2;
	float s = h / 16.0;
	//GRRLIB_DrawTile(xpos+2, ypos, tx, 0, s, s, color, 0);
	GRRLIB_DrawTile_begin0(tx);
	GRRLIB_DrawTile_draw1(xpos+2, ypos, tx, color, 0);
	GRRLIB_DrawTile_end0(tx);
	// heh, why xpos+2?
	// the 512 font chars are right aligned while unifont is left aligned
	// and if the two are together they will touch
	//GX_DrawDone();
	// dbg:
	//extern GRRLIB_texImg tx_font;
	//GRRLIB_Printf(50, xpos, tx_font, color, 1, "%d", c);
	return s * (float)len * 8.0;
}

void __GRRLIB_Print1w(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex,
		u32 color, const wchar_t *wtext)
{
	unsigned nc = tex->nbtilew * tex->nbtileh;
	wchar_t c;
	int i;
	unsigned cc;
	f32 w = tex->tilew;

	for (i = 0; wtext[i]; i++) {
		cc = c = wtext[i];
		// break on newline
		if (cc == '\r' || cc == '\n') break;
		if (cc >= nc) {
			cc = map_ufont(c);
			if (cc == 0  && (unsigned)c <= 0xFFFF) {
				if (unifont && unifont->index[(unsigned)c]) {
					// unifont contains almost all unicode chars
					GRRLIB_DrawTile_end0(tex);
					xpos += draw_unifont(xpos, ypos, tex->tilew, tex->tileh, color, c);
					GRRLIB_DrawTile_begin0(tex);
					continue;
				}
			}
			c = cc;
		}
		c -= tex->tilestart;
		GRRLIB_DrawTile_draw1(xpos, ypos, tex, color, c);
		xpos += w;
	}
}

void GRRLIB_Print4(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex, u32 color, u32 outline, u32 shadow, f32 scale, const char *text, int maxlen)
{
	if (maxlen < 0 ) maxlen = strlen(text);
	wchar_t wtext[maxlen+1];
	int wlen;
	wlen = mbstowcs(wtext, text, maxlen);
	wtext[wlen] = 0;

	GRRLIB_DrawTile_begin(tex, xpos, ypos, scale);
	xpos = ypos = 0; // position is set above
	if (shadow) {
		float x, y;
		if (outline) { x = y = 1; } else { x = y = 0; }
		__GRRLIB_Print1w(x+1, y+0, tex, shadow, wtext);
		__GRRLIB_Print1w(x+0, y+1, tex, shadow, wtext);
		__GRRLIB_Print1w(x+1, y+1, tex, shadow, wtext);
		__GRRLIB_Print1w(x+2, y+2, tex, shadow, wtext);
	}
	if (outline) {
		/*
		// x spread
		__GRRLIB_Print1w(xpos-1, ypos-1, tex, outline, wtext);
		__GRRLIB_Print1w(xpos+1, ypos-1, tex, outline, wtext);
		__GRRLIB_Print1w(xpos-1, ypos+1, tex, outline, wtext);
		__GRRLIB_Print1w(xpos+1, ypos+1, tex, outline, wtext);
		*/
		// + spread
		__GRRLIB_Print1w(xpos-1, ypos-0, tex, outline, wtext);
		__GRRLIB_Print1w(xpos+1, ypos-0, tex, outline, wtext);
		__GRRLIB_Print1w(xpos-0, ypos-1, tex, outline, wtext);
		__GRRLIB_Print1w(xpos-0, ypos+1, tex, outline, wtext);
	}
	__GRRLIB_Print1w(xpos, ypos, tex, color, wtext);
	GRRLIB_DrawTile_end(tex);
}

void GRRLIB_Print3(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex, u32 color, u32 outline, u32 shadow, f32 scale, const char *text)
{
	GRRLIB_Print4(xpos, ypos, tex, color, outline, shadow, scale, text, -1);
}

void GRRLIB_Print2(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex, u32 color, u32 outline, u32 shadow, const char *text)
{
	GRRLIB_Print3(xpos, ypos, tex, color, outline, shadow, 1.0, text);
}

void GRRLIB_Print(f32 xpos, f32 ypos, struct GRRLIB_texImg *tex, u32 color, const char *text)
{
	GRRLIB_Print2(xpos, ypos, tex, color, 0, 0, text);
}

//==============================================================
// Stencil code...
const int _stencilWidth = 128;
const int _stencilHeight = 128;

void GRRLIB_DrawImg_format(f32 xpos, f32 ypos, GRRLIB_texImg tex, u8 texFormat, float degrees, float scaleX, f32 scaleY, u32 color )
{
    GXTexObj texObj;
    u16 width, height;
    Mtx m, m1, m2, mv;

    GX_InitTexObj(&texObj, tex.data, tex.w, tex.h, texFormat, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_LoadTexObj(&texObj, GX_TEXMAP0);

    GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

    width = tex.w * 0.5;
    height = tex.h * 0.5;
    guMtxIdentity (m1);
    guMtxScaleApply(m1, m1, scaleX, scaleY, 1.0);
    guVector axis = (guVector) {0, 0, 1 };
    guMtxRotAxisDeg (m2, &axis, degrees);
    guMtxConcat(m2, m1, m);

    guMtxTransApply(m, m, xpos+width, ypos+height, 0);
    guMtxConcat (GXmodelView2D, m, mv);
    GX_LoadPosMtxImm (mv, GX_PNMTX0);

    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
    GX_Position3f32(-width, -height, 0);
    GX_Color1u32(color);
    GX_TexCoord2f32(0, 0);

    GX_Position3f32(width, -height, 0);
    GX_Color1u32(color);
    GX_TexCoord2f32(1, 0);

    GX_Position3f32(width, height, 0);
    GX_Color1u32(color);
    GX_TexCoord2f32(1, 1);

    GX_Position3f32(-width, height, 0);
    GX_Color1u32(color);
    GX_TexCoord2f32(0, 1);
    GX_End();
    GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}

static inline u32 coordsI8(u32 x, u32 y, u32 w)
{ 
	return (((y >> 2) * (w >> 3) + (x >> 3)) << 5) + ((y & 3) << 3) + (x & 7);
}

int GRRLIB_stencilVal(int x, int y, GRRLIB_texImg tx)
{
	u8 *truc = (u8*)tx.data;
    u32 offset;
	int col;

	if ((u32)x >= rmode->fbWidth || x < 0 || (u32)y >= rmode->efbHeight || y < 0)
		return 255;
	x = x * _stencilWidth / 640;
	y = y * _stencilHeight / 480;
	offset = coordsI8(x, y, (u32)_stencilWidth);
	if (offset >= (u32)(_stencilWidth * _stencilHeight))
		return 255;
	col = (int)*(truc+offset);
	return (col>255?255:col);
}

void GRRLIB_prepareStencil(void)
{
	GX_SetPixelFmt(GX_PF_Y8, GX_ZC_LINEAR);
	GX_SetViewport(0.f, 0.f, (float)_stencilWidth, (float)_stencilHeight, 0.f, 1.f);
	GX_SetScissor(0, 0, _stencilWidth, _stencilHeight);
	GX_InvVtxCache();
	GX_InvalidateTexAll();
}

void GRRLIB_renderStencil_buf(GRRLIB_texImg *tx)
{
	if (tx == NULL) return;
	if (tx->data == NULL) return;
    if (tx->w != _stencilWidth) return;
    if (tx->h != _stencilHeight) return;
	GX_DrawDone();
	GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_SetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);
	GX_SetTexCopySrc(0, 0, _stencilWidth, _stencilHeight);
	GX_SetTexCopyDst(_stencilWidth, _stencilHeight, GX_CTF_R8, GX_FALSE);
	GX_CopyTex(tx->data, GX_TRUE);
	GX_PixModeSync();
	DCFlushRange(tx->data, _stencilWidth * _stencilHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
}
//==============================================================


/**
 * Make a snapshot of the screen to a pre-allocated texture.  Used for AA.
 * @return A pointer to a texture representing the screen or NULL if an error occurs.
 */
void GRRLIB_AAScreen2Texture_buf(GRRLIB_texImg *tx, u8 gx_clear)
{
	if (tx == NULL) return;
	if (tx->data == NULL) return;
	if (tx->w != rmode->fbWidth) return;
	if (tx->h != rmode->efbHeight) return;

	GRRLIB_FlushTex(tx);
	GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_TRUE);
	//GX_DrawDone();
	GX_SetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);
	GX_SetTexCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetTexCopyDst(rmode->fbWidth, rmode->efbHeight, GX_TF_RGBA8, GX_FALSE);
	GX_CopyTex(tx->data, gx_clear);
	GX_PixModeSync();
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);	
	GRRLIB_FlushTex(tx);
}

/**
 * Make a snapshot of the screen in a texture.
 * @return A pointer to a texture representing the screen or NULL if an error occurs.
 */
GRRLIB_texImg GRRLIB_AAScreen2Texture() {
	GRRLIB_texImg my_texture;

	memset(&my_texture, 0, sizeof(my_texture));
	my_texture.w = 0;
	my_texture.h = 0;
	//my_texture.data = memalign (32, rmode->fbWidth * rmode->efbHeight * 4);
	my_texture.data = mem_alloc(rmode->fbWidth * rmode->efbHeight * 4);
	if (my_texture.data == NULL) goto out;
	my_texture.w = rmode->fbWidth;
	my_texture.h = rmode->efbHeight;
	out:
	return my_texture;
}

/**
 * Resets all the GX settings to the default
 */
void GRRLIB_ResetVideo() {
    f32 yscale;
    u32 xfbHeight;
    Mtx44 perspective;

    // other gx setup
	yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth, xfbHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (rmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetDispCopyGamma(GX_GM_1_0);

    // setup the vertex descriptor
    // tells the flipper to expect direct data
    GX_ClearVtxDesc();
    GX_InvVtxCache ();
    GX_InvalidateTexAll();

    GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);


    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_SetZMode (GX_FALSE, GX_LEQUAL, GX_TRUE);

    GX_SetNumChans(1);
    GX_SetNumTexGens(1);
    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

    guMtxIdentity(GXmodelView2D);
    guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

    guOrtho(perspective,0, 480, 0, 640, 0, 300.0F);
    GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

    GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_TRUE);

    GX_SetCullMode(GX_CULL_NONE);
}

//AA jitter values
const float _jitter2[2][2] = {
	{ 0.246490f, 0.249999f },
	{ -0.246490f, -0.249999f }
};
const float _jitter3[3][2] = {
	{ -0.373411f, -0.250550f },
	{ 0.256263f, 0.368119f },
	{ 0.117148f, -0.117570f }
};

const float _jitter4wide[4][2] = {
	{ -0.208147f, 0.353730f },
	{ 0.203849f, -0.353780f },
	{ -0.292626f, -0.149945f },
	{ 0.296924f, 0.149994f }
};

const float _jitter4[4][2] = {
	{ -0.108147f, 0.253730f },
	{ 0.103849f, -0.253780f },
	{ -0.192626f, -0.049945f },
	{ 0.196924f, 0.049994f }
};

bool grrlib_wide_aa = true;

void GRRLIB_prepareAAPass(int aa_cnt, int aaStep)
{
	float x = 0.0f;
	float y = 0.0f;
	u32 w = rmode->fbWidth;
	u32 h = rmode->efbHeight;
	switch (aa_cnt) {
		case 2:
			x += _jitter2[aaStep][0];
			y += _jitter2[aaStep][1];
			break;
		case 3:
			x += _jitter3[aaStep][0];
			y += _jitter3[aaStep][1];
			break;
		case 4:
			if (grrlib_wide_aa) {
				x += _jitter4wide[aaStep][0];
				y += _jitter4wide[aaStep][1];
			} else {
				x += _jitter4[aaStep][0];
				y += _jitter4[aaStep][1];
			}
			break;
	}
	//GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	GX_SetViewport(0+x, 0+y, rmode->fbWidth, rmode->efbHeight, 0, 1);
	GX_SetScissor(0, 0, w, h);
	GX_InvVtxCache();
	GX_InvalidateTexAll();
}

int aa_method = 1;

void GRRLIB_drawAAScene(int aa_cnt, GRRLIB_texImg *texAABuffer)
{
	if (aa_method) return;
	GXTexObj texObj[4];
	Mtx modelViewMtx;
	u8 texFmt = GX_TF_RGBA8;
	u32 tw = rmode->fbWidth;
	u32 th = rmode->efbHeight;
	float w = 640.f;
	float h = 480.f;
	float x = 0.f;
	float y = 0.f;
	int i = 0;

	GX_SetNumChans(0);
	for (i = 0; i < aa_cnt; ++i) {
		GX_InitTexObj(&texObj[i], texAABuffer[i].data, tw , th, texFmt, GX_CLAMP, GX_CLAMP, GX_FALSE);
		GX_LoadTexObj(&texObj[i], GX_TEXMAP0 + i);
	}
	
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
		
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xFF / 1, 0xFF / 5, 0xFF, 0xFF});    // Renders better gradients than 0xFF / aa
	GX_SetTevKColor(GX_KCOLOR1, (GXColor){0xFF / 2, 0xFF / 6, 0xFF, 0xFF});
	GX_SetTevKColor(GX_KCOLOR2, (GXColor){0xFF / 3, 0xFF / 7, 0xFF, 0xFF});
	GX_SetTevKColor(GX_KCOLOR3, (GXColor){0xFF / 4, 0xFF / 8, 0xFF, 0xFF});
	for (i = 0; i < aa_cnt; ++i) {
		GX_SetTevKColorSel(GX_TEVSTAGE0 + i, GX_TEV_KCSEL_K0_R + i);
		GX_SetTevKAlphaSel(GX_TEVSTAGE0 + i, GX_TEV_KASEL_K0_R + i);
		GX_SetTevOrder(GX_TEVSTAGE0 + i, GX_TEXCOORD0, GX_TEXMAP0 + i, GX_COLORNULL);
		GX_SetTevColorIn(GX_TEVSTAGE0 + i, i == 0 ? GX_CC_ZERO : GX_CC_CPREV, GX_CC_TEXC, GX_CC_KONST, GX_CC_ZERO);
		GX_SetTevAlphaIn(GX_TEVSTAGE0 + i, i == 0 ? GX_CA_ZERO : GX_CA_APREV, GX_CA_TEXA, GX_CA_KONST, GX_CA_ZERO);
		GX_SetTevColorOp(GX_TEVSTAGE0 + i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GX_SetTevAlphaOp(GX_TEVSTAGE0 + i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	}
  
/*	
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xFF / aa_cnt, 0, 0, 0}); // Causes a color accuracy loss, in previous and better code i was using the final blender with alpha = 1/1, 1/2, 1/3 etc.
	for (i = 0; i < aa_cnt; ++i) {
		GX_SetTevKColorSel(GX_TEVSTAGE0 + i, GX_TEV_KCSEL_K0_R);
		GX_SetTevKAlphaSel(GX_TEVSTAGE0 + i, GX_TEV_KASEL_K0_R);
		GX_SetTevOrder(GX_TEVSTAGE0 + i, GX_TEXCOORD0, GX_TEXMAP0 + i, GX_COLORNULL);
		GX_SetTevColorIn(GX_TEVSTAGE0 + i, GX_CC_ZERO, GX_CC_TEXC, GX_CC_KONST, i == 0 ? GX_CC_ZERO : GX_CC_CPREV);
		GX_SetTevAlphaIn(GX_TEVSTAGE0 + i, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST, i == 0 ? GX_CA_ZERO : GX_CA_APREV);
		GX_SetTevColorOp(GX_TEVSTAGE0 + i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		GX_SetTevAlphaOp(GX_TEVSTAGE0 + i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	}
*/

	GX_SetNumTevStages(aa_cnt);
	//
	GX_SetAlphaUpdate(GX_TRUE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_FALSE);
	GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	//
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	guMtxIdentity(modelViewMtx);
	GX_LoadPosMtxImm(modelViewMtx, GX_PNMTX0);
	
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
		GX_Position3f32(x, y, 0.f);
		GX_TexCoord2f32(0.f, 0.f);
		GX_Position3f32(x + w, y, 0.f);
		GX_TexCoord2f32(1.f, 0.f);
		GX_Position3f32(x + w, y + h, 0.f);
		GX_TexCoord2f32(1.f, 1.f);
		GX_Position3f32(x, y + h, 0.f);
		GX_TexCoord2f32(0.f, 1.f);
	GX_End();

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetNumTevStages(1);
}

/*
 // aa using viewport -- doesn't work
		int w = rmode->fbWidth;
		int h = rmode->efbHeight;
		int x, y, w2, h2;
		w2 = w * 2;
		h2 = h * 2;
		switch (aaStep) {
			default:
			case 0: x = 0; y = 0; break;
			case 1: x = w; y = 0; break;
			case 2: x = 0; y = h; break;
			case 3: x = w; y = h; break;
		}
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
		GX_SetViewport(-x, -y, w2, h2, 0, 1);
		GX_SetScissor(0, 0, w, h);
		GX_SetScissorBoxOffset(0, 0);
		GX_InvVtxCache();
		GX_InvalidateTexAll();
*/

void GRRLIB_DrawImgRect (float x, float y, float w, float h, GRRLIB_texImg *tex, const u32 color)
{
	guVector p[4];
	p[0].x = x;
	p[0].y = y;
	p[1].x = x + w;
	p[1].y = y;
	p[2].x = x + w;
	p[2].y = y + h;
	p[3].x = x;
	p[3].y = y + h;
	GRRLIB_DrawImgQuad(p, tex, color);
}

void GRRLIB_DrawImgNoAA(const f32 xpos, const f32 ypos, const GRRLIB_texImg *tex,
		const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color)
{
	int aa = GRRLIB_Settings.antialias;
	GRRLIB_Settings.antialias = false;
	GRRLIB_DrawImg(xpos, ypos, tex, degrees, scaleX, scaleY, color);
	GRRLIB_Settings.antialias = aa;
}

void tx_store(struct GRRLIB_texImg *dest, struct GRRLIB_texImg *src)
{
	if (src) {
		memcpy(dest, src, sizeof(struct GRRLIB_texImg));
		SAFE_FREE(src);
	} else {
		memset(dest, 0, sizeof(struct GRRLIB_texImg));
	}
}

/**
 * Draw a part of a texture.
 * @param pos Vector array of the 4 points.
 * @param partx Specifies the x-coordinate of the upper-left corner in the texture.
 * @param party Specifies the y-coordinate of the upper-left corner in the texture.
 * @param partw Specifies the width in the texture.
 * @param parth Specifies the height in the texture.
 * @param tex The texture containing the tile to draw.
 * @param color Color in RGBA format.
 */
void  GRRLIB_DrawPartQuad (const guVector pos[4], const GRRLIB_texImg *tex,
		const f32 partx, const f32 party, const f32 partw, const f32 parth,
		const u32 color)
{
	GXTexObj  texObj;
	Mtx       m1, mv;
	f32       s1, s2, t1, t2;

	if (tex == NULL || tex->data == NULL)  return;

	// The 0.001f/x is the frame correction formula by spiffen
	s1 = (partx /tex->w) +(0.001f /tex->w);
	s2 = ((partx + partw)/tex->w) -(0.001f /tex->w);
	t1 = (party /tex->h) +(0.001f /tex->h);
	t2 = ((party + parth)/tex->h) -(0.001f /tex->h);

	GX_InitTexObj(&texObj, tex->data,
			tex->w, tex->h,
			GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

	if (GRRLIB_Settings.antialias == false) {
		GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR,
				0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
		GX_SetCopyFilter(GX_FALSE, rmode->sample_pattern, GX_FALSE, rmode->vfilter);
	}
	else {
		GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
	}

	GX_LoadTexObj(&texObj,      GX_TEXMAP0);
	GX_SetTevOp  (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);

	guMtxIdentity  (m1);
	guMtxConcat(GXmodelView2D, m1, mv);

	GX_LoadPosMtxImm(mv, GX_PNMTX0);
	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	GX_Position3f32(pos[0].x, pos[0].y, 0);
	GX_Color1u32   (color);
	GX_TexCoord2f32(s1, t1);

	GX_Position3f32(pos[1].x, pos[1].y, 0);
	GX_Color1u32   (color);
	GX_TexCoord2f32(s2, t1);

	GX_Position3f32(pos[2].x, pos[2].y, 0);
	GX_Color1u32   (color);
	GX_TexCoord2f32(s2, t2);

	GX_Position3f32(pos[3].x, pos[3].y, 0);
	GX_Color1u32   (color);
	GX_TexCoord2f32(s1, t2);
	GX_End();
	GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp  (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
}


/**
 * Draw a tile range in a quad.
 * @param pos Vector array of the 4 points.
 * @param tex The texture to draw.
 * @param color Color in RGBA format.
 * @param frame Specifies the frame to draw.
 * @param nx num frames on x
 * @param ny num frames on y
 */
void  GRRLIB_DrawTileRectQuad (const guVector pos[4], GRRLIB_texImg *tex, const u32 color, const int frame, int nx, int ny)
{
    GXTexObj  texObj;
    Mtx       m, m1, m2, mv;
    f32       s1, s2, t1, t2;

    if (tex == NULL || tex->data == NULL)  return;

    // The 0.001f/x is the frame correction formula by spiffen
	// x
    s1 = ((     (frame %tex->nbtilew)   ) /(f32)tex->nbtilew) +(0.001f /tex->w);
    s2 = ((     (frame %tex->nbtilew) +nx) /(f32)tex->nbtilew) -(0.001f /tex->w);
	// y
    t1 = (((int)(frame /tex->nbtilew)   ) /(f32)tex->nbtileh) +(0.001f /tex->h);
    t2 = (((int)(frame /tex->nbtilew) +ny) /(f32)tex->nbtileh) -(0.001f /tex->h);

    GX_InitTexObj(&texObj, tex->data,
                  tex->tilew * tex->nbtilew, tex->tileh * tex->nbtileh,
                  GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);

    if (GRRLIB_Settings.antialias == false) {
        GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR,
                         0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
        GX_SetCopyFilter(GX_FALSE, rmode->sample_pattern, GX_FALSE, rmode->vfilter);
    }
    else {
        GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    }

    GX_LoadTexObj(&texObj,      GX_TEXMAP0);
    GX_SetTevOp  (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);

    guMtxIdentity  (m1);
    guMtxScaleApply(m1, m1, 1, 1, 1.0f);
    guVector axis = (guVector) {0, 0, 1 };
    guMtxRotAxisDeg(m2, &axis, 0);
    guMtxConcat    (m2, m1, m);
    guMtxConcat    (GXmodelView2D, m, mv);

    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position3f32(pos[0].x, pos[0].y, 0);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s1, t1);

        GX_Position3f32(pos[1].x, pos[1].y, 0);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s2, t1);

        GX_Position3f32(pos[2].x, pos[2].y, 0);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s2, t2);

        GX_Position3f32(pos[3].x, pos[3].y, 0);
        GX_Color1u32   (color);
        GX_TexCoord2f32(s1, t2);
    GX_End();
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

    GX_SetTevOp  (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
}

u32 fixGX_GetTexBufferSize(u16 wd, u16 ht, u32 fmt, u8 mipmap, u8 maxlod)
{
	if (mipmap) return GX_GetTexBufferSize(wd, ht, fmt, mipmap, maxlod + 1);
	return GX_GetTexBufferSize(wd, ht, fmt, mipmap, maxlod);
}

int GRRLIB_DataSize(uint w, uint h, int fmt, int lod)
{
	if (!fmt) fmt = GX_TF_RGBA8;
	return fixGX_GetTexBufferSize(w, h, fmt, lod > 0, lod);
}

int GRRLIB_TextureSize(GRRLIB_texImg *tx)
{
	return GRRLIB_DataSize(tx->w, tx->h, tx->tex_format, tx->tex_lod);
}

bool GRRLIB_AllocTextureDataX(GRRLIB_texImg *tx, int w, int h, int fmt, int lod)
{
	if (!tx) return false;
	if (!fmt) fmt = GX_TF_RGBA8;
	int size = GRRLIB_DataSize(w, h, fmt, lod);
	memset(tx, 0, sizeof(GRRLIB_texImg));
	tx->data = mem_alloc(size);
	if (!tx->data) return false;
	memset(tx->data, 0, size);
	tx->w = w;
	tx->h = h;
	tx->tex_format = fmt;
	tx->tex_lod = lod;
	GRRLIB_SetHandle(tx, 0, 0);
	GRRLIB_FlushTex(tx);
    return true;
}

bool GRRLIB_AllocTextureData(GRRLIB_texImg *tx, const uint w, const uint h)
{
	return GRRLIB_AllocTextureDataX(tx, w, h, 0, 0);
}

bool GRRLIB_ReallocTextureData(GRRLIB_texImg *tx, const uint w, const uint h)
{
	GRRLIB_FreeTextureData(tx);
	return GRRLIB_AllocTextureDataX(tx, w, h, 0, 0);
}

void GRRLIB_FreeData(GRRLIB_texImg *tex)
{
    if (tex) SAFE_FREE(tex->data);
}

void GRRLIB_FreeTextureData(GRRLIB_texImg *tex)
{
    if (tex) {
		SAFE_FREE(tex->data);
		memset(tex, 0, sizeof(GRRLIB_texImg));
	}
}

bool GRRLIB_CloneTexture(GRRLIB_texImg *dest, GRRLIB_texImg *src)
{
	dest->data = NULL;
	int size = GRRLIB_TextureSize(src);
	bool ret = GRRLIB_AllocTextureDataX(dest,
			src->w, src->h, src->tex_format, src->tex_lod);
	if (!ret) return false;
	void *data = dest->data;
	*dest = *src;
	dest->data = data;
	memcpy(dest->data, src->data, size);
	return true;
}

// if src is in mem1/2 then use src
// else copy src to mem1/2 and free src
bool GRRLIB_TextureMEM2(GRRLIB_texImg *dest, GRRLIB_texImg *src)
{
	if (!src->data) return false;
	GRRLIB_FreeTextureData(dest);
	if (mem_inside(3, src->data)) {
		*dest = *src;
		src->data = NULL;
	} else {
		GRRLIB_CloneTexture(dest, src);
		GRRLIB_FreeTextureData(src);
	}
	return dest->data != NULL;
}

bool GRRLIB_TextureToMEM2(GRRLIB_texImg *tx)
{
	if (!tx || !tx->data) return false;
	if (mem_inside(3, tx->data)) return true;
	GRRLIB_texImg tmp;
	GRRLIB_CloneTexture(&tmp, tx);
	GRRLIB_FreeData(tx);
	tx->data = tmp.data;
	GRRLIB_FlushTex(tx);
	return tx->data != NULL;
}

// override GRRLIB_texSetup.h

/**
 * Create an empty texture.
 * @param w Width of the new texture to create.
 * @param h Height of the new texture to create.
 * @return A GRRLIB_texImg structure newly created.
 */

GRRLIB_texImg* GRRLIB_CreateEmptyTexture(const uint w, const uint h)
{
    GRRLIB_texImg *my_texture = (struct GRRLIB_texImg *)calloc(1, sizeof(GRRLIB_texImg));
    if (my_texture) {
		if (!GRRLIB_AllocTextureData(my_texture, w, h)) {
			SAFE_FREE(my_texture);
			return NULL;
		}
    }
    return my_texture;
}

/**
 * Write the contents of a texture in the data cache down to main memory.
 * For performance the CPU holds a data cache where modifications are stored before they get written down to main memory.
 * @param tex The texture to flush.
 */
void  GRRLIB_FlushTex (GRRLIB_texImg *tex)
{
	int size = GRRLIB_TextureSize(tex);
	if (tex->data) DCFlushRange(tex->data, size);
}

/**
 * Free memory allocated for texture.
 * @param tex A GRRLIB_texImg structure.
 */
void GRRLIB_FreeTexture(GRRLIB_texImg *tex)
{
    if (tex != NULL) {
        SAFE_FREE(tex->data);
        SAFE_FREE(tex);
    }
}

/**
 * Clear a texture to transparent black.
 * @param tex Texture to clear.
 */
void  GRRLIB_ClearTex(GRRLIB_texImg* tex) {
	int size = GRRLIB_TextureSize(tex);
    bzero(tex->data, size);
    GRRLIB_FlushTex(tex);
}

