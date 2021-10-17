//
// GBA-FlightSim.c
//

#include <stdio.h>
#include <tonc.h>

#include "nums.h"
#include "GBA-FlightSim.h"
#include "magvel_hmap.h"
#include "Magvel_Map_sunset.h"
#include "oceanmap.h"
#include "sky_wrap.h"
#include "sky_wrap_lighter.h"
#include "sky_wrap_darker.h"
#include "sky_wrap_sunset.h"
#include "ue4_magvel_wmap.h"

// === CONSTANTS & MACROS =============================================



// === GLOBALS ========================================================


// === FUNCTIONS ======================================================

void init_main()
{

}

void input_main()
{

	// key_poll();

}

IWRAM_CODE void Render()
{
	memcpy32((void *) 0x6000000, (const void *) sky_wrapBitmap, (128*160>>1));
}

int main()
{
	init_main();

	// enable hblank register
	irq_init(NULL);
	irq_add(II_HBLANK, NULL);
	// and vblank int for vsync
	irq_add(II_VBLANK, NULL);

	REG_DISPCNT= DCNT_MODE5 | DCNT_BG2 | DCNT_OBJ;
	REG_BG2PA=0x00;	//rotate and stretch
	REG_BG2PB=0xFF0C; //a bit bigger than the screen (-0xF4?)
	REG_BG2PC=0x85; //
	REG_BG2PD=0x00;	//
	REG_BG2X=0x9e40;	//offset 'horizontal' can bump 0x180 each way
	REG_BG2Y = 0x180;     //can bump it 0x180 each way

	Render();

	// main loop
	while(1)
	{
		VBlankIntrWait();
		input_main();
	}
	return 0;
}

