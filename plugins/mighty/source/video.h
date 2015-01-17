#ifndef _VIDEO_H_
#define _VIDEO_H_

#define CONSOLE_X		50
#define CONSOLE_Y		50
#define CONSOLE_WIDTH	384
#define CONSOLE_HEIGHT	224

/* Prototypes */
void Con_Init(u32, u32, u32, u32);
void Con_Clear(void);
void Con_ClearLine(void);

void Video_Init(void);

//void MRC_Capture(void);

#endif
