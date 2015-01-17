typedef struct _dolheader {
	u32 text_pos[7];
	u32 data_pos[11];
	u32 text_start[7];
	u32 data_start[11];
	u32 text_size[7];
	u32 data_size[11];
	u32 bss_start;
	u32 bss_size;
	u32 entry_point;
} dolheader;


u32 load_dol_start(void *dolstart);
bool load_dol_image(void **offset, u32 *pos, u32 *len);
u32 load_dol_image_args(void *dolstart, struct __argv *argv);


