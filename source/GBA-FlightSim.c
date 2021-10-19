//
// GBA-FlightSim.c
//

#include <stdio.h>
#include <tonc.h>

#include "all_gfx.h"
#include "GBA-FlightSim.h"
#include "AAS.h"
#include "AAS_Mixer.h"
#include "../AAS_Data.h"

// === CONSTANTS & MACROS =============================================

const int skies[] = {(int)(&sky_wrapBitmap), (int)(&sky_wrapBitmap), (int)(&sky_wrap_lighterBitmap), (int)(&sky_wrap_darkerBitmap), (int)(&sky_wrap_sunsetBitmap)};

const u8 WorldMapNodes[11][16] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 1, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1, 1, 6, 6, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0},
	{0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0},
	{0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 4, 4, 0, 2, 2, 2, 7, 7, 7, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 7, 7, 7, 0},
	{0, 0, 0, 0, 0, 0, 0, 3, 3, 2, 2, 2, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

//LUTs
const s16 cam_dx_Angles[16] = DX_TABLE(MOVEMENT_STEP);
const s16 cam_dy_Angles[16] = DY_TABLE(MOVEMENT_STEP);
const s16 cam_pivot_dx_Angles[16] = DX_TABLE((MIN_Z_DISTANCE+SHADOW_DISTANCE)); // camera distance from focal point
const s16 cam_pivot_dy_Angles[16] = DY_TABLE((MIN_Z_DISTANCE+SHADOW_DISTANCE)); 

// === GLOBALS ========================================================

FlightSimData CurrentFlightSim; //using a struct so we can pass it into the asm renderer
int GameClock = 0;
int BG2X_buffer = 0x9e40; //use this as a buffer
int BG2Y_buffer = 0x180;     //can bump it 0x180 each way
int BG2PA_buffer = 0x00;	//rotate and stretch	
int BG2PB_buffer =0xFF0C; //a bit bigger than the screen (-0xF4?)
int BG2PC_buffer =0x85; //
int BG2PD_buffer =0x00;	//
int deltaVolume = 0;
int musicVolume = 64;

OBJ_ATTR obj_buffer[128];
OBJ_AFFINE *obj_aff_buffer = (OBJ_AFFINE*)obj_buffer;

int CurrentFPS = 0;
int CounterFPS = 0;


// === FUNCTIONS ======================================================

IWRAM_CODE void VBlankHandler()
{
	GameClock++;
	//bob up and down
	int animClock = GameClock;
	animClock &= 0x3F;
	REG_BG2X = BG2X_buffer;
	REG_BG2Y = BG2Y_buffer;
	REG_BG2PA = BG2PA_buffer;
	REG_BG2PB = BG2PB_buffer;
	REG_BG2PC = BG2PC_buffer;
	REG_BG2PD = BG2PD_buffer;
	if ((animClock < 0x10) | (animClock > 0x30))
	{
		BG2X_buffer -= 0x30;
	}
	else if (BG2X_buffer<0x9fd0)
	{
		BG2X_buffer += 0x30;
	}

	if (animClock==0)
	{
		CurrentFPS = CounterFPS;
		CounterFPS = 0;
	}

	if (animClock == 0x20) AAS_SFX_Play(2, 64, 16000, AAS_DATA_SFX_START_flap, AAS_DATA_SFX_END_flap, AAS_NULL);

	if (deltaVolume != 0)
	{
		musicVolume += deltaVolume;
		AAS_SFX_SetVolume(0, musicVolume);
		if (musicVolume==0)
		{
			AAS_SFX_Stop(0);
			deltaVolume = 0;
		}
		else if (musicVolume==64) deltaVolume = 0;
	}

	oam_copy(oam_mem, obj_buffer, 32); //draw 32 sprites max for now
	AAS_DoWork();
}

void AAS_SFX_FadeIn(int chan)
{
	AAS_SFX_Resume(chan);
	deltaVolume = 4;
}

void AAS_SFX_FadeOut(int chan)
{
	// AAS_SFX_Stop(chan);
	deltaVolume = -4;
}

void init_main()
{
	
	//set up stats
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
	CurrentFlightSim.playMusic = 1;

	CurrentFlightSim.vid_page = (u16*) 0x600a000; //draw to the back page first


	//load images into vram
	LZ77UnCompVram(&pkTiles, &tile_mem[5][0]); //first tile of the hi block 0x6014000
	LZ77UnCompVram(&locationsTiles, &tile_mem[5][64]); //yeah 
	LZ77UnCompVram(&cursorTiles, &tile_mem[5][96]);
	LZ77UnCompVram(&minimapTiles, &tile_mem[5][97]);
	LZ77UnCompVram(&fpsTiles, &tile_mem[5][161]); //fps numbers
	LZ77UnCompVram(&lensflareTiles, &tile_mem[5][193]);

	//set palettes
	memcpy32(&(pal_obj_mem[0x00]), &pkPal, pkPalLen/4);
	memcpy32(&(pal_obj_mem[0x10]), &minimapPal, minimapPalLen/4);
	memcpy32(&(pal_obj_mem[0x20]), &lensflarePal, lensflarePalLen/4);

	//set up oam buffer
	oam_init(obj_buffer, 128);
	
	//set bldcnt
	REG_BLDCNT = BLD_BUILD(BLD_BACKDROP, BLD_BG2 | BLD_BG3 | BLD_OBJ, (BLD_BLACK>>6)); //dammit tonc fix your bld define
	REG_BLDALPHA = BLDA_BUILD(0x04, 0x10);

	//set up dispcnt, mode5 on its side
	REG_DISPCNT= DCNT_MODE5 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
	BG2PA_buffer=0x00;	//rotate and stretch
	BG2PB_buffer=0xFF0C; //a bit bigger than the screen (-0xF4?)
	BG2PC_buffer=0x85; //
	BG2PD_buffer=0x00;	//
	REG_BG2X=0x9e40;	//offset 'horizontal' can bump 0x180 each way
	BG2Y_buffer = 0x180;     //can bump it 0x180 each way
	REG_BG2CNT = BG_PRIO(3);

	//set up audio
	AAS_SetConfig(AAS_CONFIG_MIX_24KHZ, AAS_CONFIG_CHANS_8, AAS_CONFIG_SPATIAL_MONO, AAS_CONFIG_DYNAMIC_ON);
	
	AAS_SFX_Play(0, musicVolume, 22050, AAS_DATA_SFX_START_falcon_bg_downsampled, AAS_DATA_SFX_END_falcon_bg_downsampled, AAS_DATA_SFX_START_falcon_bg_downsampled+170710);
	AAS_SFX_Play(1, 48, 16000, AAS_DATA_SFX_START_windy, AAS_DATA_SFX_END_windy, AAS_DATA_SFX_START_windy+41472);
};

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
			BG2Y_buffer =0x180;	//offset horizontal
			BG2X_buffer =0x9280;
			BG2PA_buffer=0x000E; 
			BG2PB_buffer=0xFF1C;
			BG2PC_buffer=0x0080;
			BG2PD_buffer=0x0008;
			break;
		case bump_right:
			BG2Y_buffer =0x0500;	//offset horizontal
			BG2X_buffer =0x9c40;
			BG2PA_buffer=0xFFF2; 
			BG2PB_buffer=0xFF1C;
			BG2PC_buffer=0x0080;
			BG2PD_buffer=0xFFF8;
			break;
		default: //no bump
			BG2PA_buffer=0x00;	//rotate and stretch	
			BG2PB_buffer=0xFF0C; //a bit bigger than the screen (-0xF4?)
			BG2PC_buffer=0x85; //
			BG2PD_buffer=0x00;	//
			BG2X_buffer =0x9e40;	//offset 'horizontal' can bump 0x180 each way
			BG2Y_buffer =0x180;     //can bump it 0x180 each way
	};
};

IWRAM_CODE void Draw()
{

	//update sprites

	//flier
	u8 animClock = GameClock&0x3f;
	int pkTID;
	if (animClock<0x10) pkTID = 0xa00;
	else if (animClock<0x20) pkTID = 0xa10;
	else if (animClock<0x30) pkTID = 0xa20;
	else pkTID = 0xa30;
	OBJ_ATTR *pkSprite= &obj_buffer[0];
	obj_set_attr(pkSprite, ATTR0_SQUARE, ATTR1_SIZE_32, ATTR2_PALBANK(0) | pkTID);
	obj_set_pos(pkSprite, 0x68, 0x58);

	//fps counter
	OBJ_ATTR *fpsSprite = &obj_buffer[1];
	if (CurrentFlightSim.ShowFPS)
	{
		obj_set_attr(fpsSprite, ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(0) | (0xaa1 + (CurrentFPS)));
		obj_set_pos(fpsSprite, 0, 0);
	}
	else obj_hide(fpsSprite);

	//minimap and cursor
	OBJ_ATTR *cursorSprite = &obj_buffer[2];
	OBJ_ATTR *minimapSprite = &obj_buffer[3];
	if (CurrentFlightSim.ShowMap)
	{
		obj_set_attr(minimapSprite, ATTR0_SQUARE, ATTR1_SIZE_64, ATTR2_PALBANK(1) | 0xa61); //draw behind cursor
		obj_set_pos(minimapSprite, 176, 0);
		if ((CurrentFlightSim.sFocusPtY > MAP_YOFS) && (CurrentFlightSim.sFocusPtY < (MAP_DIMENSIONS - MAP_YOFS)) && (CurrentFlightSim.sFocusPtX > 0) && (CurrentFlightSim.sFocusPtX < MAP_DIMENSIONS))
		{
			obj_set_attr(cursorSprite, ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_PALBANK(0) | 0xa60);
			obj_set_pos(cursorSprite, 176 + (CurrentFlightSim.sFocusPtX>>4), ((CurrentFlightSim.sFocusPtY - MAP_YOFS)>>4));
		}
		else obj_hide(cursorSprite);
	}
	else 
	{
		obj_hide(minimapSprite);
		obj_hide(cursorSprite);
	}

	//location marker
	OBJ_ATTR *locationSprite = &obj_buffer[4];
	if (CurrentFlightSim.location) {
		obj_set_attr(locationSprite, ATTR0_WIDE, ATTR1_SIZE_16, ATTR2_PALBANK(0) | (0xa3c + (CurrentFlightSim.location<<2)));
		obj_set_pos(locationSprite, 0x10, 0x10);
	}
	else obj_hide(locationSprite);

	//check for lens flare
	OBJ_ATTR *flareSprite = &obj_buffer[5];
	if (CurrentFlightSim.sunsetVal < 3)
	{
		//draw lens flare test
		int flarex = 64;
		int flarey = 80 - (CurrentFlightSim.sPlayerStepZ<<2) - ((BG2X_buffer - 0x9e40)>>10);
		switch(CurrentFlightSim.sPlayerYaw){
			default:
			obj_hide(flareSprite);
			break;
			case a_W:
			flarex += 32;
			case a_WSW:
			flarex += 32;
			case a_SW:
			flarex += 32;
			case a_SSW:
			// ObjInsertSafe(9, flarex, flarey, &gObj_aff32x32, 0x3aa1+31);
			obj_set_attr(flareSprite, ATTR0_SQUARE | ATTR0_BLEND, ATTR1_SIZE_32, ATTR2_PALBANK(2) | 0xac1);
			obj_set_pos(flareSprite, flarex, flarey);
		};
	}
	else obj_hide(flareSprite);

	// do the render in asm	
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

	if (key_held(0xFFFF) == (KEY_A | KEY_B | KEY_START | KEY_SELECT))
	{
		REG_IME = FALSE;
		REG_SNDCNT = 0;
		asm("ldr r3, =0x3007f00");
		asm("mov sp, r3");
		RegisterRamReset(0);
		SoftReset();
	}

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
	CurrentFlightSim.sPlayerPosX += cam_dx_Angles[CurrentFlightSim.sPlayerYaw]; 
	CurrentFlightSim.sPlayerPosY += cam_dy_Angles[CurrentFlightSim.sPlayerYaw];
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

	if (key_hit(KEY_SELECT)) CurrentFlightSim.ShowFPS ^= 1;
	if (key_hit(KEY_R)) CurrentFlightSim.ShowMap ^= 1;
	if (key_hit(KEY_START))
	{
		CurrentFlightSim.playMusic ^= 1;
		if (CurrentFlightSim.playMusic) AAS_SFX_FadeIn(0);
		else AAS_SFX_FadeOut(0);
	}

	//prevent leaving the area 
	if (CurrentFlightSim.sPlayerPosX > (MAP_DIMENSIONS-10)) CurrentFlightSim.sPlayerPosX = MAP_DIMENSIONS-10;
	else if (CurrentFlightSim.sPlayerPosX < 10) CurrentFlightSim.sPlayerPosX = 10;

	if (CurrentFlightSim.sPlayerPosY > (MAP_DIMENSIONS-10)) CurrentFlightSim.sPlayerPosY = MAP_DIMENSIONS-10;
	else if (CurrentFlightSim.sPlayerPosY < 10) CurrentFlightSim.sPlayerPosY = 10;


	//check if player is in a zone
	int posX = CurrentFlightSim.sFocusPtX;
	int posY = CurrentFlightSim.sFocusPtY;

	u8 loc = 0;

	if ((posY > MAP_YOFS) && (posY < (MAP_DIMENSIONS - MAP_YOFS)) && (posX > 0) && (posX < MAP_DIMENSIONS)) {
		posX >>= 6;
		posY = (posY-MAP_YOFS)>>6;
		loc = WorldMapNodes[posY][posX];
	};
	CurrentFlightSim.location = loc;

}


int main()
{

	irq_init(NULL);
	// timer for audio (does it need highest prio?)
	irq_add(II_TIMER1, AAS_FastTimer1InterruptHandler);
	// enable hblank register
	// irq_add(II_HBLANK, NULL);
	// and vblank int for vsync
	irq_add(II_VBLANK, VBlankHandler);

	init_main();
	// main loop
	while(1)
	{
		UpdateState();
		Draw();
		VBlankIntrWait();
		CurrentFlightSim.vid_page = vid_flip();
		CounterFPS++;
	}
	return 0;
}

