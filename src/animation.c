#include "wx_runtime.h"
#include <math.h>

// Interpolate the offline sampled skeletal palette, then skin only resident actors.
// At most 256 matrices (12 KiB) are staged, independent of scene complexity.
unsigned wx_animation_interval(float distance_squared,unsigned clip,int focused){
    if(focused||(clip!=0&&clip!=5)||!isfinite(distance_squared)||distance_squared<=35*35)return 0;
    return distance_squared<=80*80?66:100;
}
void wx_animate_scheduled(WxScene* s,unsigned time_ms,const uint32_t* ids,const unsigned* intervals,unsigned count) {
    float palette[256*12];
    const float* palette_source=NULL;unsigned palette_time=0,palette_bones=0,palette_frames=0,palette_duration=0;
    s->animating=(WxAnimationMetrics){0};
    for(int n=0;n<WX_CACHE_SLOTS;n++) {
        WxResident* r=&s->slots[n];
        if(r->entry<0||!r->poses)continue;
        unsigned interval=0;
        if(ids){unsigned i=0;while(i<count&&ids[i]!=s->entries[r->entry].id)i++;if(i==count)continue;
            if(intervals){interval=intervals[i];
                for(unsigned j=i+1;j<count;j++)if(ids[j]==ids[i]&&intervals[j]<interval)interval=intervals[j];}}
        unsigned sample=interval?time_ms-time_ms%interval:time_ms;
        if(r->animated&&r->animated_time==sample){s->animating.skipped++;continue;}
        WxAnimation* a=&r->animation;
        if(palette_source!=r->poses||palette_time!=sample||palette_bones!=a->bones||palette_frames!=a->frames||palette_duration!=a->duration_ms){
        float t=(sample%a->duration_ms)*(float)a->frames/a->duration_ms;
        unsigned frame=(unsigned)t,next=(frame+1)%a->frames;
        float blend=t-frame;
        for(unsigned i=0;i<a->bones*12;i++) {
            float first=r->poses[frame*a->bones*12+i];
            palette[i]=first+(r->poses[next*a->bones*12+i]-first)*blend;
        }
        palette_source=r->poses;palette_time=sample;palette_bones=a->bones;palette_frames=a->frames;palette_duration=a->duration_ms;
        s->animating.palettes++;
        }
        for(unsigned i=0;i<s->entries[r->entry].vertex_count;i++) {
            const WxVertex* src=&r->bind_vertices[i];WxVertex* dst=&r->vertices[i];
            const WxSkinVertex* skin=&r->skin[i];unsigned sum=0;
            for(int k=0;k<4;k++)sum+=skin->weights[k];
            float p[3]={0},normal[3]={0};
            for(int k=0;k<4;k++)if(skin->weights[k]) {
                const float* m=&palette[skin->bones[k]*12];float weight=(float)skin->weights[k]/sum;
                for(int j=0;j<3;j++) {
                    p[j]+=weight*(m[j*4]*src->p[0]+m[j*4+1]*src->p[1]+m[j*4+2]*src->p[2]+m[j*4+3]);
                    normal[j]+=weight*(m[j*4]*src->n[0]+m[j*4+1]*src->n[1]+m[j*4+2]*src->n[2]);
                }
            }
            float length=sqrtf(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
            for(int j=0;j<3;j++){dst->p[j]=p[j];dst->n[j]=length>0.0001f?normal[j]/length:src->n[j];}
        }
        r->animated=1;r->animated_time=sample;s->animating.sampled++;s->animating.vertices+=s->entries[r->entry].vertex_count;
    }
}
void wx_animate_ids(WxScene* s,unsigned time_ms,const uint32_t* ids,unsigned count){wx_animate_scheduled(s,time_ms,ids,NULL,count);}
void wx_animate(WxScene* s,unsigned time_ms){wx_animate_ids(s,time_ms,0,0);}
