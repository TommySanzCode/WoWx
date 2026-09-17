#ifndef WX_BACKDROP_H
#define WX_BACKDROP_H
#include "wx_pack.h"
#include <stdio.h>
#define WX_BACKDROP_LIMIT (8u*1024u*1024u)
#define WX_BACKDROP_BONES 256u
#define WX_BACKDROP_BATCHES 128u
#define WX_BACKDROP_NONE UINT32_MAX
#define WX_BACKDROP_EMITTERS 64u
#define WX_BACKDROP_PARTICLES 2048u
#define WX_BACKDROP_LIGHTS 8u
#define WX_BACKDROP_IO_SLICE 65536u
/* Vanilla file bits, not CParticleEmitter2's differently numbered runtime bits.
   See docs/PARTICLE-FLAGS.md for the build-5875 branch evidence. */
#define WX_PARTICLE_NO_NORMALS 0x1u
#define WX_PARTICLE_UNSHADED 0x8u
#define WX_PARTICLE_LOCAL 0x10u
#define WX_PARTICLE_BONE_SCALE 0x20u
#define WX_PARTICLE_SPHERE_UP 0x100u
#define WX_PARTICLE_XY_QUAD 0x1000u
#define WX_PARTICLE_SUPPORTED (WX_PARTICLE_NO_NORMALS|WX_PARTICLE_UNSHADED|WX_PARTICLE_LOCAL|WX_PARTICLE_BONE_SCALE|WX_PARTICLE_SPHERE_UP|WX_PARTICLE_XY_QUAD)
static inline int wx_particle_flags_supported(uint32_t flags){
    /* Only the original unlit-scene combination of bit 8 has been traced.
       Keep the normal-bearing lighting path rejected until implemented. */
    return !(flags&~WX_PARTICLE_SUPPORTED)&&(!(flags&WX_PARTICLE_UNSHADED)||(flags&WX_PARTICLE_NO_NORMALS));
}
/* Private prepared M2 scene. All offsets are absolute, aligned little-endian. */
typedef struct WxBackdropHeader {
    char magic[4];uint32_t version,file_size,duration_ms;
    uint32_t vertices,vertex_offset,indices,index_offset,bones,bone_offset;
    uint32_t batches,batch_offset,textures,texture_offset,tracks,track_offset,keys,key_offset;
    uint32_t skin_offset,omitted_particles,omitted_ribbons,reserved;
    float camera[3],target[3],fov,near_clip,far_clip;
} WxBackdropHeader;
typedef struct WxBackdropTrack {uint32_t first,count,period_ms,interpolation,kind;} WxBackdropTrack;
typedef struct WxBackdropKey {uint32_t time_ms;float value[4];} WxBackdropKey;
typedef struct WxBackdropBone {int32_t parent;uint32_t flags;float pivot[3];uint32_t translation,rotation,scale;} WxBackdropBone;
typedef struct WxBackdropBatch {
    uint32_t first,count,texture,flags,blend,alpha,weight;float color[4];
} WxBackdropBatch;
typedef struct WxBackdropTexture {uint32_t offset,size,dimension,levels,flags,gpu_offset;} WxBackdropTexture;
/* Version 2 keeps the v1 header; reserved is the absolute extension offset. */
typedef struct WxBackdropEffects {
    uint32_t emitters,emitter_offset,lights,light_offset,color_offset,capacity;
} WxBackdropEffects;
/* Version 3 appends a character stand mark immediately after Effects. */
typedef struct WxBackdropAnchor {uint32_t present,bone;float position[3];} WxBackdropAnchor;
enum {WX_FX_SPEED,WX_FX_VARIATION,WX_FX_VERTICAL,WX_FX_HORIZONTAL,WX_FX_GRAVITY,
      WX_FX_LIFE,WX_FX_RATE,WX_FX_LENGTH,WX_FX_WIDTH,WX_FX_ZSOURCE,WX_FX_ENABLED,WX_FX_TRACKS};
typedef struct WxBackdropEmitter {
    uint32_t flags,bone,texture,blend,type,rows,columns,tile_rotation;
    float position[3];uint32_t tracks[WX_FX_TRACKS];
    float midpoint,color[3][4],scale[3];uint32_t cells[3];
} WxBackdropEmitter;
typedef struct WxBackdropLight {
    uint32_t type,bone;float position[3];
    uint32_t ambient,ambient_intensity,diffuse,diffuse_intensity,start,end,enabled;
} WxBackdropLight;
typedef struct WxEffectVertex {float p[3],color[4],uv[2];} WxEffectVertex;
typedef struct WxParticle {
    float position[3],velocity[3],age,life;uint32_t emitter;
} WxParticle;
typedef struct WxBackdropSimulation {
    WxParticle particles[WX_BACKDROP_PARTICLES];
    float accumulators[WX_BACKDROP_EMITTERS];
    uint32_t first[WX_BACKDROP_EMITTERS],count[WX_BACKDROP_EMITTERS];
    unsigned active,peak,born,dropped,clock_skips,clock_started,last_time,rng,quads;
} WxBackdropSimulation;
typedef struct WxBackdropView {float camera[3],right[3],up[3],forward[3],center[2],focal,near_clip,far_clip;} WxBackdropView;
typedef struct WxBackdropLightSample {float position[3],ambient[3],diffuse[3],start,end;unsigned type,enabled;} WxBackdropLightSample;
/* Followed in one allocation by one dirty/material byte per source vertex. */
typedef struct WxBackdropLighting {WxBackdropLightSample samples[WX_BACKDROP_LIGHTS];} WxBackdropLighting;
typedef struct WxBackdrop {
    WxBackdropHeader h;void* data;WxVertex* vertices;uint8_t* pixels;
    unsigned bytes,ready,failures,loads,updates,draws,triangles,time_ms,geometry_ready;
    float matrices[WX_BACKDROP_BONES][12],colors[WX_BACKDROP_BATCHES][4];
    WxBackdropEffects effects;WxBackdropSimulation* simulation;
    WxEffectVertex* effect_vertices;float (*lighting)[4];WxBackdropLighting* light_state;WxBackdropView view;
    FILE* pending;unsigned load_phase,load_read,load_texture,load_offset;
} WxBackdrop;
#ifdef __cplusplus
extern "C" {
#endif
unsigned wx_backdrop_error(const void* data,unsigned size);
int wx_backdrop_validate(const void* data,unsigned size);
int wx_backdrop_open(WxBackdrop* s,const char* path);
/* One bounded read/upload slice per pump. Close cancels any pending load. */
int wx_backdrop_begin(WxBackdrop* s,const char* path);
int wx_backdrop_pump(WxBackdrop* s,unsigned time_ms);
void wx_backdrop_close(WxBackdrop* s);
void wx_backdrop_sample(const WxBackdrop* s,unsigned track,unsigned time_ms,const float fallback[4],float result[4]);
void wx_backdrop_update(WxBackdrop* s,unsigned time_ms);
void wx_backdrop_draw(WxBackdrop* s);
/* Shared scene camera for mesh/effects and later character-preview placement. */
int wx_backdrop_view(WxBackdrop* s,float x,float y,float width,float height);
int wx_backdrop_anchor(const WxBackdrop* s,float position[3]);
unsigned wx_backdrop_effect_bytes(const WxBackdropHeader* h,const WxBackdropEffects* e);
int wx_backdrop_effect_validate(const void* data,unsigned size);
void wx_backdrop_effect_update(WxBackdrop* s,unsigned time_ms);
#ifdef __cplusplus
}
#endif
#endif
