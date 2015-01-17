#ifndef _WPAD_H_
#define _WPAD_H_

#include <wiiuse/wpad.h>
#include <ogc/pad.h>

/* Prototypes */
s32  Wpad_Init(void);
void Wpad_Disconnect(void);
u32  Wpad_GetButtons(void);
u32  Wpad_WaitButtons(void);
u32  Wpad_WaitButtonsRpt(void);
u32  Wpad_WaitButtonsCommon(void);
u32  Wpad_WaitButtonsTimeout(int ms);
u32  Wpad_Held_1(int n);
u32  Wpad_Held();
u32  Wpad_HeldButtons();
void Wpad_getIRx(int n, struct ir_t *ir);
void Wpad_getIR(struct ir_t *ir);
bool Wpad_set_pos(struct ir_t *ir, float x, float y);

#define WPAD_BUTTON_Z					(0x0001<<16)
#define WPAD_BUTTON_C					(0x0002<<16)
#define WPAD_BUTTON_X					(0x0008<<16)
#define WPAD_BUTTON_Y					(0x0020<<16)
#define WPAD_BUTTON_R					(0x0200<<16)
#define WPAD_BUTTON_L					(0x2000<<16)

#define NUM_BUTTON_UP    0
#define NUM_BUTTON_RIGHT 1
#define NUM_BUTTON_DOWN  2
#define NUM_BUTTON_LEFT  3
#define NUM_BUTTON_MINUS 4
#define NUM_BUTTON_PLUS  5
#define NUM_BUTTON_A     6
#define NUM_BUTTON_B     7
#define NUM_BUTTON_HOME  8
#define NUM_BUTTON_1     9
#define NUM_BUTTON_2    10
#define NUM_BUTTON_X    11
#define NUM_BUTTON_Y    12
#define NUM_BUTTON_Z    13
#define NUM_BUTTON_C    14
#define NUM_BUTTON_L    15
#define NUM_BUTTON_R    16

#define MAX_BUTTONS	17
#define WIIMOTE	0
#define CLASSIC	1
#define GCPAD	2
#define GUITAR	3
#define NUNCHUK	4
#define MASTER  5

const char *button_names[MAX_BUTTONS];
void makeButtonMap(void);
u32 buttonmap[6][MAX_BUTTONS];

#endif
