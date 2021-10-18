//
// m7_isrs.c
// Separate file for HBL interrupts because apparently it screws up 
//   on hardware now.

#include <tonc.h>
#include "params.h"

#ifndef __FLIGHTSIM__
#define __FLIGHTSIM__

#define __PAGEFLIP__ //remove to show draw
#define __ALWAYS_MOVE__
#define __FPSCOUNT__ //remove to hide fps counter

// === CONSTANTS & MACROS =============================================

// #define heightMap (u8*)(magvel_hmapBitmap)

#define MIN_Z_DISTANCE 24
#define MAX_Z_DISTANCE 512
#define SHADOW_DISTANCE MIN_Z_DISTANCE+16
#define FOG_DISTANCE (MAX_Z_DISTANCE>>1)
#define NUM_ALTITUDES 16
#define MAP_DIMENSIONS 1024
#define MAP_DIMENSIONS_LOG2 10
#define MAP_YOFS 170
#define INC_ZSTEP ((zdist>>6)+(zdist>>7)+((zdist>>8)<<2)+2)
#define MOVEMENT_STEP 4

#define SKY_COLOUR 0x7f0f
#define SEA_COLOUR 0x1840
#define SEA_COLOUR_SUNSET 0x0820
#define CEL_SHADE_THRESHOLD 6

#define DX_TABLE(step) {            \
  step *0,                           \
  step *0.38,                        \
  step *0.71,                        \
  step *0.92,                        \
  step *1,                           \
  step *0.92,                        \
  step *0.71,                        \
  step *0.38,                        \
  step *0,                           \
  step *-0.38,                       \
  step *-0.71,                       \
  step *-0.92,                       \
  step *-1,                          \
  step *-0.92,                       \
  step *-0.71,                       \
  step *-0.38                        \
}

#define DY_TABLE(step) {            \
  step *-1,                         \
  step *-0.92,                      \
  step *-0.71,                      \
  step *-0.38,                      \
  step *0,                          \
  step *0.38,                       \
  step *0.71,                       \
  step *0.92,                       \
  step *1,                          \
  step *0.92,                       \
  step *0.71,                       \
  step *0.38,                       \
  step *0,                          \
  step *-0.38,                      \
  step *-0.71,                      \
  step *-0.92                       \
}

// === GLOBALS ========================================================

// extern VECTOR cam_pos;
// extern u16 cam_phi;
// extern FIXED g_cosf, g_sinf;
// extern int g_state;


// === PROTOTYPES =====================================================

typedef struct FlightSimData FlightSimData;

struct FlightSimData
{
	int sPlayerPosX;	//0
	int sPlayerPosY;	//4
	int sPlayerPosZ;	//8
	int sPlayerStepZ;	//c
	int sPlayerYaw;		//10
	u16* vid_page;		//14
	s8 sunTransition;	//18
	u8 ShowMap:1;		//19
	u8 ShowFPS: 1;		//19
	u8 takeOffTransition: 1;	//19
	u8 landingTransition: 1;	//19
	u8 playMusic: 1;	//19
	u8 unused:3;		//19
	u8 oceanClock;		//1a
	u8 unused2;			//1b
	int sFocusPtX;		//1c
	int sFocusPtY;		//20
	int location;		//24
	int sunsetVal;		//28
};

//16 possible angles for yaw
enum Angles{
  a_N,
  a_NNE,
  a_NE,
  a_ENE,
  a_E,
  a_ESE,
  a_SE,
  a_SSE,
  a_S,
  a_SSW,
  a_SW,
  a_WSW,
  a_W,
  a_WNW,
  a_NW,
  a_NNW
};

enum BumpDirs{
  bump_up,
  bump_down,
  bump_left,
  bump_right,
  bump_fwd,
  bump_back
};

void Render_arm(FlightSimData* CurrFS);


#endif	// __FLIGHTSIM__
