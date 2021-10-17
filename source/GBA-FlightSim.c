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

const int skies[] = {(int)(&sky_wrapBitmap), (int)(&sky_wrapBitmap), (int)(&sky_wrap_lighterBitmap), (int)(&sky_wrap_darkerBitmap), (int)(&sky_wrap_sunsetBitmap)};

//LUTs
const s16 cam_dx_Angles[16] = DX_TABLE(MOVEMENT_STEP);
const s16 cam_dy_Angles[16] = DY_TABLE(MOVEMENT_STEP);
const s16 cam_pivot_dx_Angles[16] = DX_TABLE((MIN_Z_DISTANCE+SHADOW_DISTANCE)); // camera distance from focal point
const s16 cam_pivot_dy_Angles[16] = DY_TABLE((MIN_Z_DISTANCE+SHADOW_DISTANCE)); 

// === GLOBALS ========================================================

FlightSimData CurrentFlightSim;

// === FUNCTIONS ======================================================

void init_main()
{
	CurrentFlightSim.sPlayerPosX = MAP_DIMENSIONS/2;
	CurrentFlightSim.sPlayerPosY = MAP_DIMENSIONS/2;
	CurrentFlightSim.sPlayerPosZ = CAMERA_MIN_HEIGHT+(2 * CAMERA_Z_STEP);
	CurrentFlightSim.sPlayerStepZ = 2;
	CurrentFlightSim.sPlayerYaw = a_SE;
	CurrentFlightSim.ShowMap = FALSE;
	CurrentFlightSim.ShowFPS = FALSE;
	// CurrentFlightSim.location = Frelia;
	CurrentFlightSim.sunsetVal = 0;
	CurrentFlightSim.sunTransition = 0;
	CurrentFlightSim.takeOffTransition = 1;
	CurrentFlightSim.landingTransition = 0;
	CurrentFlightSim.oceanClock = 1;

	CurrentFlightSim.vid_page = (u16*) 0x600a000; //draw to the back page first
}

u8 getPtHeight(int ptx, int pty){
	if((ptx >= MAP_DIMENSIONS)||(pty >= MAP_DIMENSIONS)||(ptx<0)||(pty<0)) return 0;
	u8* heightMap = (u8*)(magvel_hmapBitmap);
	return heightMap[(pty<<MAP_DIMENSIONS_LOG2)+ptx];
};

void BumpScreen(int direction){
	switch (direction){
		case bump_up:
			// REG_BG2X=0x9e40+0x180;	//offset horizontal
			break;
		case bump_down:
			// REG_BG2X=0x9e40-0x180;	//offset horizontal
			break;
		case bump_left:
			REG_BG2Y =0x180;	//offset horizontal
			REG_BG2X =0x9280;
			REG_BG2PA=0x000E; 
			REG_BG2PB=0xFF1C;
			REG_BG2PC=0x0080;
			REG_BG2PD=0x0008;
			break;
		case bump_right:
			REG_BG2Y =0x0500;	//offset horizontal
			REG_BG2X =0x9c40;
			REG_BG2PA=0xFFF2; 
			REG_BG2PB=0xFF1C;
			REG_BG2PC=0x0080;
			REG_BG2PD=0xFFF8;
			break;
		default: //no bump
			REG_BG2PA=0x00;	//rotate and stretch	
			REG_BG2PB=0xFF0C; //a bit bigger than the screen (-0xF4?)
			REG_BG2PC=0x85; //
			REG_BG2PD=0x00;	//
			REG_BG2X =0x9e40;	//offset 'horizontal' can bump 0x180 each way
			REG_BG2Y =0x180;     //can bump it 0x180 each way
	};
};

IWRAM_CODE void Draw()
{

	//update sprites somehow
	
	Render_arm(&CurrentFlightSim);
}

void UpdateState()
{
	if (CurrentFlightSim.oceanClock & 1)
	{
		if (CurrentFlightSim.oceanClock < (0x41-4))	CurrentFlightSim.oceanClock+=4;
		else CurrentFlightSim.oceanClock -= 1;
	}
	else
	{
		if (CurrentFlightSim.oceanClock > 4) CurrentFlightSim.oceanClock-=4;
		else CurrentFlightSim.oceanClock += 1;
	}

	key_poll();

	int newx, newy;

	if (key_held(KEY_LEFT))
	{
		newx = CurrentFlightSim.sPlayerPosX + cam_pivot_dx_Angles[CurrentFlightSim.sPlayerYaw]; // step forward to focal point
		newy = CurrentFlightSim.sPlayerPosY + cam_pivot_dy_Angles[CurrentFlightSim.sPlayerYaw]; // step forward to focal point
		CurrentFlightSim.sPlayerYaw = (CurrentFlightSim.sPlayerYaw - 1)&0xF; //16 angles so skip the conditional
		newx -= (cam_pivot_dx_Angles[CurrentFlightSim.sPlayerYaw]>>2)*3; // step back partway from focal point
		newy -= (cam_pivot_dy_Angles[CurrentFlightSim.sPlayerYaw]>>2)*3; // step back partway from focal point
		CurrentFlightSim.sPlayerPosX = newx;
		CurrentFlightSim.sPlayerPosY = newy;
		BumpScreen(bump_left);
	}
	else if (key_held(KEY_RIGHT))
	{
		newx = CurrentFlightSim.sPlayerPosX + cam_pivot_dx_Angles[CurrentFlightSim.sPlayerYaw]; // step forward to focal point
		newy = CurrentFlightSim.sPlayerPosY + cam_pivot_dy_Angles[CurrentFlightSim.sPlayerYaw]; // step forward to focal point
		CurrentFlightSim.sPlayerYaw = (CurrentFlightSim.sPlayerYaw + 1)&0xF; //16 angles so skip the conditional
		newx -= (cam_pivot_dx_Angles[CurrentFlightSim.sPlayerYaw]>>2)*3; // step back partway from focal point
		newy -= (cam_pivot_dy_Angles[CurrentFlightSim.sPlayerYaw]>>2)*3; // step back partway from focal point
		CurrentFlightSim.sPlayerPosX = newx;
		CurrentFlightSim.sPlayerPosY = newy;
		BumpScreen(bump_right);
	}
	else if (key_was_down(KEY_LEFT|KEY_RIGHT)) BumpScreen(4); //reset


	//always moving forward
	// CurrentFlightSim.sPlayerPosX += cam_dx_Angles[CurrentFlightSim.sPlayerYaw]; 
	// CurrentFlightSim.sPlayerPosY += cam_dy_Angles[CurrentFlightSim.sPlayerYaw];
	CurrentFlightSim.sFocusPtX = CurrentFlightSim.sPlayerPosX + cam_pivot_dx_Angles[CurrentFlightSim.sPlayerYaw]; // set focal point
	CurrentFlightSim.sFocusPtY = CurrentFlightSim.sPlayerPosY + cam_pivot_dy_Angles[CurrentFlightSim.sPlayerYaw]; // set focal point

	if (key_hit(KEY_L) && (CurrentFlightSim.sunTransition==0))
	{
		if (CurrentFlightSim.sunsetVal) CurrentFlightSim.sunTransition = -1;
		else CurrentFlightSim.sunTransition = 1;
		CurrentFlightSim.sunsetVal += CurrentFlightSim.sunTransition;
	};

	if (CurrentFlightSim.sunTransition!=0)
	{
		if ((CurrentFlightSim.sunsetVal > 0) & (CurrentFlightSim.sunsetVal < 8))
		{
			CurrentFlightSim.sunsetVal += CurrentFlightSim.sunTransition;
		}
		else
		{
			CurrentFlightSim.sunTransition = 0;
		}
	};


	if (key_held(KEY_UP))
	{ //turbo
		CurrentFlightSim.sPlayerPosX += cam_dx_Angles[CurrentFlightSim.sPlayerYaw];
		CurrentFlightSim.sPlayerPosY += cam_dy_Angles[CurrentFlightSim.sPlayerYaw];
	};
	if (key_held(KEY_DOWN))
	{ //hover
		CurrentFlightSim.sPlayerPosX -= cam_dx_Angles[CurrentFlightSim.sPlayerYaw];
		CurrentFlightSim.sPlayerPosY -= cam_dy_Angles[CurrentFlightSim.sPlayerYaw];
	};


	int player_terrain_ht = getPtHeight(CurrentFlightSim.sFocusPtX, CurrentFlightSim.sFocusPtY);
	int camera_terrain_ht = getPtHeight(CurrentFlightSim.sPlayerPosX, CurrentFlightSim.sPlayerPosY);
	int camera_ht = CurrentFlightSim.sPlayerPosZ - (CAMERA_Z_STEP) - 10;
	if ((player_terrain_ht > (camera_ht)) || (camera_terrain_ht > camera_ht)){
		if (CurrentFlightSim.sPlayerPosZ < CAMERA_MAX_HEIGHT)
		{
			CurrentFlightSim.sPlayerPosZ += CAMERA_Z_STEP;
			CurrentFlightSim.sPlayerStepZ += 1;
		}
	}
	else if (key_held(KEY_B))
	{ //prevent clipping through ground
		if ((CurrentFlightSim.sPlayerPosZ>CAMERA_MIN_HEIGHT) & (camera_ht > (player_terrain_ht+CAMERA_Z_STEP)) & (camera_ht > (camera_terrain_ht+CAMERA_Z_STEP))){
			CurrentFlightSim.sPlayerPosZ -= CAMERA_Z_STEP;
			CurrentFlightSim.sPlayerStepZ -= 1;
			BumpScreen(bump_down);
		};
	};
	if (key_held(KEY_A)){
		if (CurrentFlightSim.sPlayerPosZ<CAMERA_MAX_HEIGHT){
			CurrentFlightSim.sPlayerPosZ += CAMERA_Z_STEP;
			CurrentFlightSim.sPlayerStepZ += 1;
			BumpScreen(bump_up);
		};
	};
}


int main()
{
	init_main();

	// enable hblank register
	irq_init(NULL);
	irq_add(II_HBLANK, NULL);
	// and vblank int for vsync
	irq_add(II_VBLANK, NULL);

	//set up display, mode5 on its side
	REG_DISPCNT= DCNT_MODE5 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
	REG_BG2PA=0x00;	//rotate and stretch
	REG_BG2PB=0xFF0C; //a bit bigger than the screen (-0xF4?)
	REG_BG2PC=0x85; //
	REG_BG2PD=0x00;	//
	REG_BG2X=0x9e40;	//offset 'horizontal' can bump 0x180 each way
	REG_BG2Y = 0x180;     //can bump it 0x180 each way

	// main loop
	while(1)
	{
		UpdateState();
		Draw();
		VBlankIntrWait();
		CurrentFlightSim.vid_page = vid_flip();
	}
	return 0;
}

