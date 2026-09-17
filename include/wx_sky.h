#ifndef WX_SKY_H
#define WX_SKY_H
#include "wx_lighting.h"
#include "wx_ui.h"
/* Uses the existing UI mesh/white atlas sample; no extra allocation/surface.
   Caller starts with an empty UI queue, draws it, and clears before menus. */
unsigned wx_sky_build(WxUi* ui,const WxLightPalette* palette,const WxFog* fog,
                      const float right[4],const float up[4],const float forward[4]);
#endif
