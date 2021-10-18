#ifndef __AAS_DATA__
#define __AAS_DATA__

#include "AAS.h"

#if AAS_VERSION != 0x113
#error AAS version does not match Conv2AAS version
#endif

AAS_BEGIN_DECLS

extern const AAS_s8* const AAS_DATA_SFX_START_falcon_bg_downsampled;

extern const AAS_s8* const AAS_DATA_SFX_END_falcon_bg_downsampled;

extern const AAS_s8* const AAS_DATA_SFX_START_flap;

extern const AAS_s8* const AAS_DATA_SFX_END_flap;

extern const AAS_s8* const AAS_DATA_SFX_START_windy;

extern const AAS_s8* const AAS_DATA_SFX_END_windy;

AAS_END_DECLS

#endif
