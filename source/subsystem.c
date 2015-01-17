#include <stdio.h>
#include <ogcsys.h>

#include "fat.h"
#include "wpad.h"
#include "music.h"
#include "subsystem.h"
#include "net.h"

void Subsystem_Init(void)
{
	/* Initialize Wiimote subsystem */
	Wpad_Init();

	/* Mount SDHC */
	MountSDHC();
}

void Services_Close()
{
	// stop music
	Music_Stop();

	// stop gui
	extern void Gui_Close();
	Gui_Close();
}

void Subsystem_Close(void)
{
	// close network
	Net_Close(0);

	/* Disconnect Wiimotes */
	Wpad_Disconnect();

	// Unmount all filesystems
	UnmountAll(NULL);

	// USB general (including bluetooth)
	USB_Deinitialize();
}

