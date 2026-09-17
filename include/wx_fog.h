#ifndef WX_FOG_H
#define WX_FOG_H
#include <stdint.h>
/* World-space units and linear RGB. Shared by every world material/pass.
   Unknown indoor/underwater lighting must not inherit the outdoor fallback. */
enum {WX_FOG_OUTDOOR,WX_FOG_INDOOR,WX_FOG_UNDERWATER};
typedef struct WxFog {float color[3],start,end;uint32_t enabled,environment;} WxFog;
void wx_fog_fallback(WxFog* fog,unsigned environment);
int wx_fog_valid(const WxFog* fog);
float wx_fog_visibility(const WxFog* fog,float distance);
void wx_fog_blend(WxFog* current,const WxFog* target,float seconds);
uint32_t wx_fog_background(const WxFog* fog);
void wx_fog_bind(const WxFog* fog);
#endif
