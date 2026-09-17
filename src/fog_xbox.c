#include "wx_fog.h"
#include <pbkit/pbkit.h>
void wx_fog_bind(const WxFog* f){
    int enabled=wx_fog_valid(f)&&f->enabled;uint32_t* p=pb_begin();
    p=pb_push1(p,NV097_SET_FOG_ENABLE,enabled);
    if(enabled){
        uint32_t rgb=wx_fog_background(f),abgr=(rgb&0xff00ff00)|((rgb&255)<<16)|((rgb>>16)&255);
        p=pb_push1(p,NV097_SET_FOG_COLOR,abgr);
        p=pb_push1(p,NV097_SET_FOG_MODE,NV097_SET_FOG_MODE_V_LINEAR);
        p=pb_push1(p,NV097_SET_FOG_GEN_MODE,NV097_SET_FOG_GEN_MODE_V_FOG_X);
        // NV2A linear fog = bias + coordinate * multiplier - 1.
        float inverse=1/(f->end-f->start);
        p=pb_push3f(p,NV097_SET_FOG_PARAMS,1+f->end*inverse,-inverse,0);
        // Final RGB: fog.a * lit texture + (1-fog.a) * fog.rgb.
        // Source 3 = fog, 4 = diffuse (world combiner's lit texture result).
        p=pb_push1(p,NV097_SET_COMBINER_SPECULAR_FOG_CW0,(3u<<24)|(1u<<28)|(4u<<16)|(3u<<8));
    }else p=pb_push1(p,NV097_SET_COMBINER_SPECULAR_FOG_CW0,4);
    // Preserve the texture/material alpha for cutouts and blended materials.
    p=pb_push1(p,NV097_SET_COMBINER_SPECULAR_FOG_CW1,(4u<<8)|(1u<<12));pb_end(p);
}
