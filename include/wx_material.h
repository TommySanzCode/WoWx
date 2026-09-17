#ifndef WX_MATERIAL_H
#define WX_MATERIAL_H
#include "wx_pack.h"
/* Original Vanilla blend IDs. Alpha key retains its v4/v5 flag encoding. */
enum {WX_FACTOR_ZERO,WX_FACTOR_ONE,WX_FACTOR_SRC_ALPHA,WX_FACTOR_INV_SRC_ALPHA,WX_FACTOR_DST_COLOR,WX_FACTOR_SRC_COLOR};
typedef struct WxMaterial {unsigned blend,src,dst,depth_test,depth_write,unlit,unfogged;float fog_neutral;} WxMaterial;
static inline unsigned wx_material_blend(unsigned flags){
    unsigned mode=(flags&WX_BLEND_MASK)>>WX_BLEND_SHIFT;return mode?mode:((flags&WX_ALPHA_TEST)?1:0);
}
static inline int wx_material_valid(unsigned flags,unsigned version,unsigned kind){
    unsigned mode=(flags&WX_BLEND_MASK)>>WX_BLEND_SHIFT;
    return !(flags&WX_MATERIAL_FLAGS) || (version>=6&&kind!=WX_KIND_COLLISION&&mode!=1&&mode!=7&&!(mode&&(flags&WX_ALPHA_TEST)));
}
static inline WxMaterial wx_material(unsigned flags){
    WxMaterial m={0};m.blend=wx_material_blend(flags);
    m.src=WX_FACTOR_SRC_ALPHA;m.dst=WX_FACTOR_INV_SRC_ALPHA;
    if(m.blend==3){m.src=WX_FACTOR_ONE;m.dst=WX_FACTOR_ONE;}
    if(m.blend==4)m.dst=WX_FACTOR_ONE;
    if(m.blend==5){m.src=WX_FACTOR_DST_COLOR;m.dst=WX_FACTOR_ZERO;}
    if(m.blend==6){m.src=WX_FACTOR_DST_COLOR;m.dst=WX_FACTOR_SRC_COLOR;}
    m.depth_test=!(flags&WX_NO_DEPTH_TEST);m.depth_write=m.blend<2&&!(flags&WX_NO_DEPTH_WRITE);
    m.unlit=!!(flags&WX_UNLIT);m.unfogged=!!(flags&WX_UNFOGGED);
    /* Add zero / multiply by one / twice multiply by one half at full fog. */
    m.fog_neutral=m.blend==5?1.f:m.blend==6?.5f:0.f;return m;
}
typedef struct WxDrawOrder {float depth;uint16_t slot,transparent;} WxDrawOrder;
/* Stable bounded insertion: opaque/cutout first, then back-to-front blended.
   Centers approximate intersecting transparent geometry; no per-triangle sort. */
static inline unsigned wx_material_order(WxDrawOrder* list,unsigned count,unsigned slot,float depth,unsigned flags){
    if(count>=WX_CACHE_SLOTS)return count;
    WxDrawOrder item={depth,(uint16_t)slot,(uint16_t)(wx_material_blend(flags)>=2)};unsigned at=count;
    if(item.transparent)while(at&&list[at-1].transparent&&list[at-1].depth<depth){list[at]=list[at-1];at--;}
    else while(at&&list[at-1].transparent){list[at]=list[at-1];at--;}
    list[at]=item;return count+1;
}
#endif
