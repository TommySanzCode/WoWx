#ifndef WX_MATERIAL_MOTION_H
#define WX_MATERIAL_MOTION_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#define WX_MOTION_KEYS 128u
#define WX_MOTION_MAGIC 0x31544d57u
enum {WX_MOTION_RGB,WX_MOTION_ALPHA,WX_MOTION_WEIGHT,WX_MOTION_UV,WX_MOTION_TRACKS};
/* Optional WXP v7 block immediately preceding its vertex payload. No reserved
   entry fields are reused; placement IDs, mip counts and skinning stay intact.
   Track flags: bit 0 linear, bit 1 global clock. Local tracks use clip time. */
typedef struct WxMotionTrack {uint32_t first,count,period_ms,flags;} WxMotionTrack;
typedef struct WxMotionKey {uint32_t time_ms;float value[4];} WxMotionKey;
typedef struct WxMaterialMotion {
    uint32_t magic,key_count;float color[4],weight;
    WxMotionTrack tracks[WX_MOTION_TRACKS];WxMotionKey keys[WX_MOTION_KEYS];
} WxMaterialMotion;
typedef struct WxMotionState {float color[4],uv[4];} WxMotionState;
#ifdef __cplusplus
static_assert(sizeof(WxMaterialMotion)==2652&&sizeof(WxMotionState)==32,"Material motion ABI changed");
#else
_Static_assert(sizeof(WxMaterialMotion)==2652&&sizeof(WxMotionState)==32,"Material motion ABI changed");
#endif
static inline float wx_motion_unit(float x){return x<0?0:x>1?1:x;}
static inline int wx_motion_valid(const WxMaterialMotion* m){
    if(!m||m->magic!=WX_MOTION_MAGIC||m->key_count>WX_MOTION_KEYS)return 0;
    for(unsigned k=0;k<4;k++)if(!isfinite(m->color[k])||m->color[k]<0||m->color[k]>1)return 0;
    if(!isfinite(m->weight)||m->weight<0||m->weight>1)return 0;
    for(unsigned i=0;i<WX_MOTION_TRACKS;i++){
        const WxMotionTrack* t=m->tracks+i;
        if(t->first>m->key_count||t->count>m->key_count-t->first||t->flags&~3u||
           (t->count&&(!t->period_ms||t->period_ms>86400000u)))return 0;
        for(unsigned k=0;k<t->count;k++){
            const WxMotionKey* key=m->keys+t->first+k;
            if(key->time_ms>t->period_ms||(k&&key->time_ms<=(key-1)->time_ms))return 0;
            for(unsigned j=0;j<4;j++)if(!isfinite(key->value[j])||fabsf(key->value[j])>65536.f)return 0;
        }
    }return 1;
}
static inline void wx_motion_sample(const WxMaterialMotion* m,unsigned which,unsigned local,unsigned global,float out[4]){
    const WxMotionTrack* t=m->tracks+which;if(!t->count)return;
    unsigned time=((t->flags&2)?global:local)%t->period_ms,lo=0,hi=t->count;
    const WxMotionKey* keys=m->keys+t->first;
    while(lo+1<hi){unsigned mid=(lo+hi)/2;if(keys[mid].time_ms<=time)lo=mid;else hi=mid;}
    memcpy(out,keys[lo].value,16);
    if(!(t->flags&1)||lo+1==t->count||time<=keys[lo].time_ms)return;
    float f=(float)(time-keys[lo].time_ms)/(keys[lo+1].time_ms-keys[lo].time_ms);
    for(unsigned j=0;j<4;j++)out[j]+=f*(keys[lo+1].value[j]-out[j]);
}
static inline void wx_motion_evaluate(const WxMaterialMotion* m,unsigned local,unsigned global,WxMotionState* out){
    memset(out,0,sizeof *out);for(unsigned i=0;i<4;i++)out->color[i]=1;
    if(!m)return;
    float rgb[4]={m->color[0],m->color[1],m->color[2],0},alpha[4]={m->color[3],0,0,0},weight[4]={m->weight,0,0,0};
    wx_motion_sample(m,WX_MOTION_RGB,local,global,rgb);wx_motion_sample(m,WX_MOTION_ALPHA,local,global,alpha);
    wx_motion_sample(m,WX_MOTION_WEIGHT,local,global,weight);wx_motion_sample(m,WX_MOTION_UV,local,global,out->uv);
    for(unsigned i=0;i<3;i++)out->color[i]=wx_motion_unit(rgb[i]);out->color[3]=wx_motion_unit(alpha[0])*wx_motion_unit(weight[0]);
    out->uv[2]=out->uv[3]=0;
}
#endif
