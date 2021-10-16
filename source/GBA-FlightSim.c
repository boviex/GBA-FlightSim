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

int main()
{
	init_main();

	// enable hblank register
	irq_init(NULL);
	irq_add(II_HBLANK, NULL);
	// and vblank int for vsync
	irq_add(II_VBLANK, NULL);

	REG_DISPCNT= DCNT_MODE5 | DCNT_BG2;

	// main loop
	while(1)
	{
		VBlankIntrWait();
		input_main();
	}
	return 0;
}

