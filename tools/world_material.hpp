#pragma once
#include "wx_material.h"
#include <stdexcept>
// Material layout is decoded by the pinned WoWee loaders. Keep the authored
// blend ID even when a texture has no alpha: additive black must disappear.
static unsigned worldMaterial(unsigned blend,unsigned flags,bool m2){
    if(blend>6)throw std::runtime_error("Unsupported Vanilla world blend mode: "+std::to_string(blend));
    unsigned out=blend==1?WX_ALPHA_TEST:blend<<WX_BLEND_SHIFT;
    if(flags&1)out|=WX_UNLIT;
    if(flags&2)out|=WX_UNFOGGED;
    if(m2&&flags&8)out|=WX_NO_DEPTH_TEST;
    if(m2&&flags&16)out|=WX_NO_DEPTH_WRITE;
    return out;
}
