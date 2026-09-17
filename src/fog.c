#include "wx_fog.h"
#include <math.h>
void wx_fog_fallback(WxFog* f,unsigned environment){
    if(environment==WX_FOG_UNDERWATER){if(f)*f=(WxFog){{.04f,.18f,.26f},5,60,1,environment};return;}
    if(f)*f=(WxFog){{113/255.f,150/255.f,181/255.f},90,145,environment==WX_FOG_OUTDOOR,environment};
}
int wx_fog_valid(const WxFog* f){
    if(!f||f->enabled>1||f->environment>WX_FOG_UNDERWATER||!isfinite(f->start)||!isfinite(f->end)||f->start< -640||f->end<=0||f->end<=f->start||f->end>160)return 0;
    for(unsigned i=0;i<3;i++)if(!isfinite(f->color[i])||f->color[i]<0||f->color[i]>1)return 0;
    return 1;
}
float wx_fog_visibility(const WxFog* f,float distance){
    if(!wx_fog_valid(f)||!f->enabled||!isfinite(distance))return 1;
    if(distance<=f->start)return 1;if(distance>=f->end)return 0;
    return (f->end-distance)/(f->end-f->start);
}
void wx_fog_blend(WxFog* f,const WxFog* target,float seconds){
    if(!f||!wx_fog_valid(target)||!isfinite(seconds)||seconds<=0)return;
    if(!wx_fog_valid(f)||f->environment!=target->environment||f->enabled!=target->enabled){*f=*target;return;}
    float amount=1-expf(-seconds*2);for(unsigned i=0;i<3;i++)f->color[i]+=(target->color[i]-f->color[i])*amount;
    f->start+=(target->start-f->start)*amount;f->end+=(target->end-f->end)*amount;
}
uint32_t wx_fog_background(const WxFog* f){
    if(!wx_fog_valid(f))return 0xff7196b5;
    return 0xff000000|((uint32_t)(f->color[0]*255+.5f)<<16)|((uint32_t)(f->color[1]*255+.5f)<<8)|(uint32_t)(f->color[2]*255+.5f);
}
