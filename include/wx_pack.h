#ifndef WX_PACK_H
#define WX_PACK_H
#include <stdint.h>
#define WX_PACK_VERSION 10
#define WX_MAX_ENTRIES 32768
#define WX_RENDER_SLOTS 256
#define WX_COLLISION_SLOTS 64
#define WX_CACHE_SLOTS (WX_RENDER_SLOTS+WX_COLLISION_SLOTS)
#define WX_KIND_TERRAIN 1
#define WX_KIND_STATIC 2
#define WX_KIND_CHARACTER 3
#define WX_KIND_COLLISION 4
#define WX_TEX_SWIZZLED 1u
#define WX_ALPHA_TEST 2u
#define WX_ANIMATED 4u
#define WX_MIPMAPPED 8u
#define WX_PLACEMENT_ID 16u
#define WX_TEX_DXT1 32u
#define WX_TEX_DXT5 64u
#define WX_BLEND_SHIFT 7u
#define WX_BLEND_MASK (7u<<WX_BLEND_SHIFT)
#define WX_UNLIT (1u<<10)
#define WX_UNFOGGED (1u<<11)
#define WX_NO_DEPTH_TEST (1u<<12)
#define WX_NO_DEPTH_WRITE (1u<<13)
#define WX_MATERIAL_MOTION (1u<<14)
#define WX_CLAMP_U (1u<<15)
#define WX_CLAMP_V (1u<<16)
#define WX_MATERIAL_V7 (WX_MATERIAL_MOTION|WX_CLAMP_U|WX_CLAMP_V)
#define WX_VERTEX_COLOR (1u<<17)
#define WX_BAKED_LIGHT (1u<<18)
#define WX_MATERIAL_V8 (WX_VERTEX_COLOR|WX_BAKED_LIGHT)
#define WX_TEXTURE_SEQUENCE (1u<<19)
#define WX_LIQUID (1u<<20)
#define WX_MATERIAL_V10 (WX_TEXTURE_SEQUENCE|WX_LIQUID)
#define WX_MATERIAL_FLAGS (WX_BLEND_MASK|WX_UNLIT|WX_UNFOGGED|WX_NO_DEPTH_TEST|WX_NO_DEPTH_WRITE)
#define WX_TEXTURE_ENCODING (WX_TEX_SWIZZLED|WX_TEX_DXT1|WX_TEX_DXT5)
typedef struct WxSkinVertex { uint8_t bones[4], weights[4]; } WxSkinVertex;
typedef struct WxAnimation {
    uint32_t frames, bones, duration_ms, skin_offset, poses_offset;
} WxAnimation;
typedef struct WxVertex { float p[3], n[3], uv[2]; } WxVertex;
typedef struct WxPackHeader {
    char magic[4];
    uint32_t version, count, entry_size;
    float spawn[3];
    uint32_t file_size;
} WxPackHeader;
typedef struct WxEntry {
    uint32_t kind, id, vertex_count, index_count;
    uint32_t vertex_offset, index_offset, texture_offset, width, height;
    float center[3], radius;
    uint32_t flags, reserved[2];
} WxEntry;
/* v10 liquid flipbooks: high 16 bits = frame count, low 16 = mip count.
   Each complete frame/mip chain is padded to 128 bytes for NV2A offsets.
   Frames share one bounded, atomically published texture allocation. */
static inline unsigned wx_texture_levels(const WxEntry* e){unsigned n=e->reserved[1];if(e->flags&WX_TEXTURE_SEQUENCE)n&=65535u;return n?n:1;}
static inline unsigned wx_texture_frames(const WxEntry* e){return (e->flags&WX_TEXTURE_SEQUENCE)?e->reserved[1]>>16:1;}
static inline unsigned wx_texture_frame(const WxEntry* e,unsigned time_ms){
    unsigned n=wx_texture_frames(e);if(n<2||n>32)return 0;
    unsigned phase=(time_ms%1250u)*n,frame=phase/1250u;
    /* Original 30-frame/1250 ms nearest-even(framePhase - .5) policy. */
    if(!(phase%1250u)&&(frame&1))frame--;
    return frame;
}
/* Optional v8 RGBA8 lighting colours precede the optional v7 motion block,
   which still immediately precedes vertices. Alpha is reserved, always 255;
   WMO MOCV alpha is not surface opacity. */
static inline unsigned wx_vertex_color_bytes(const WxEntry* e){return (e->flags&WX_VERTEX_COLOR)?e->vertex_count*4u:0;}
#ifdef __cplusplus
static_assert(sizeof(WxVertex)==32 && sizeof(WxPackHeader)==32 && sizeof(WxEntry)==64);
#endif
#endif
