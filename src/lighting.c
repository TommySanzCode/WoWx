#include "wx_lighting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern unsigned wx_free_memory(void);
_Static_assert(sizeof(WxLightHeader)==32&&sizeof(WxLightVolume)==40&&sizeof(WxLightProfile)==1104,"WXL1 v2 layout");
static float real(uint32_t bits){float f;memcpy(&f,&bits,4);return f;}
int wx_light_profile_valid(const WxLightProfile* p){
    if(!p||!p->id)return 0;
    for(unsigned b=0;b<3+WX_LIGHT_COLORS;b++){
        const WxLightBand* k=&p->bands[b];if(k->count>16)return 0;
        for(unsigned i=0;i<16;i++){
            if(i>=k->count){if(k->time[i]||k->value[i])return 0;continue;}
            if(k->time[i]>=2880||(i&&k->time[i]<=k->time[i-1]))return 0;
            if(!b||b>=3){if(k->value[i]>0xffffff)return 0;}
            else {float v=real(k->value[i]);if(!isfinite(v)||(b==1?(v<0||v>10000):(v< -4||v>=1)))return 0;}
        }
    }
    return 1;
}
static const WxLightProfile* profile(const WxLighting* c,uint32_t id){
    unsigned lo=0,hi=c->profile_count;while(lo<hi){unsigned m=lo+(hi-lo)/2;if(c->profiles[m].id<id)lo=m+1;else hi=m;}
    return lo<c->profile_count&&c->profiles[lo].id==id?&c->profiles[lo]:NULL;
}
void wx_light_close(WxLighting* c){if(c){free(c->volumes);free(c->profiles);memset(c,0,sizeof *c);}}
int wx_light_open(WxLighting* c,const char* path){
    if(!c||!path)return 0;FILE* f=fopen(path,"rb");WxLighting next={0};WxLightHeader h;
    if(!f)goto bad;
    if(fread(&h,1,sizeof h,f)!=sizeof h||h.magic!=0x314c5857||(h.version!=1&&h.version!=2)||!h.volumes||h.volumes>WX_LIGHT_MAX_VOLUMES||
       !h.profiles||h.profiles>WX_LIGHT_MAX_PROFILES||h.reserved[0]||h.reserved[1]||h.reserved[2])goto bad;
    next.bytes=h.volumes*sizeof(WxLightVolume)+h.profiles*sizeof(WxLightProfile);
    unsigned record_bytes=h.version==1?304:sizeof(WxLightProfile);
    if(next.bytes>WX_LIGHT_BUDGET||h.bytes!=sizeof h+h.volumes*sizeof(WxLightVolume)+h.profiles*record_bytes||wx_free_memory()<8*1024*1024+next.bytes)goto bad;
    if(fseek(f,0,SEEK_END)||ftell(f)!=(long)h.bytes||fseek(f,sizeof h,SEEK_SET))goto bad;
    next.volumes=malloc(h.volumes*sizeof(WxLightVolume));next.profiles=malloc(h.profiles*sizeof(WxLightProfile));
    if(!next.volumes||!next.profiles)goto bad;
    if(fread(next.volumes,sizeof(WxLightVolume),h.volumes,f)!=h.volumes)goto bad;
    for(unsigned i=0;i<h.profiles;i++){
        if(fread(&next.profiles[i],1,record_bytes,f)!=record_bytes)goto bad;
        if(record_bytes<sizeof(WxLightProfile))memset((char*)&next.profiles[i]+record_bytes,0,sizeof(WxLightProfile)-record_bytes);
    }
    next.volume_count=h.volumes;next.profile_count=h.profiles;
    for(unsigned i=0;i<h.profiles;i++)if(!wx_light_profile_valid(&next.profiles[i])||(i&&next.profiles[i-1].id>=next.profiles[i].id))goto bad;
    for(unsigned i=0;i<h.volumes;i++){
        const WxLightVolume* v=&next.volumes[i];
        if(!v->id||v->map>65535||!isfinite(v->inner)||!isfinite(v->outer)||v->inner<0||v->outer<v->inner||v->outer>40000)goto bad;
        for(unsigned axis=0;axis<3;axis++)if(!isfinite(v->position[axis])||fabsf(v->position[axis])>40000)goto bad;
        for(unsigned k=0;k<3;k++)if(v->profile[k]&&!profile(&next,v->profile[k]))goto bad;
        if(!v->profile[0]||(i&&next.volumes[i-1].id>=v->id))goto bad;
        if(!v->outer)for(unsigned j=0;j<i;j++)if(!next.volumes[j].outer&&next.volumes[j].map==v->map)goto bad;
    }
    fclose(f);wx_light_close(c);*c=next;return 1;
bad:
    if(f)fclose(f);wx_light_close(&next);c->failures++;return 0;
}
static void segment(const WxLightBand* b,float time,unsigned* a,unsigned* z,float* t){
    *a=*z=0;*t=0;if(b->count<2)return;
    for(unsigned i=0;i<b->count;i++){
        unsigned j=(i+1)%b->count;float start=b->time[i],end=b->time[j],at=time;
        if(!j){end+=2880;if(at<start)at+=2880;}
        if(at>=start&&at<end){*a=i;*z=j;*t=(at-start)/(end-start);return;}
    }
}
static float scalar(const WxLightBand* b,float time){unsigned a,z;float t;segment(b,time,&a,&z,&t);float x=real(b->value[a]);return x+(real(b->value[z])-x)*t;}
static void color(const WxLightBand* b,float time,float result[3]){
    unsigned a,z;float t;segment(b,time,&a,&z,&t);
    for(unsigned channel=0;channel<3;channel++){unsigned shift=16-channel*8;
        float x=(b->value[a]>>shift)&255,y=(b->value[z]>>shift)&255;result[channel]=(x+(y-x)*t)/255.f;}
}
static void sample_profile(const WxLighting* c,uint32_t id,float time,unsigned env,WxLightSample* out){
    memset(out,0,sizeof *out);wx_fog_fallback(&out->fog,env);wx_light_fallback(&out->palette);
    const WxLightProfile* p=profile(c,id);if(!p)return;
    for(unsigned k=0;k<WX_LIGHT_COLORS;k++)if(p->bands[k+3].count){color(&p->bands[k+3],time,out->palette.color[k]);out->palette.mask|=1u<<k;}
    if(!p->bands[0].count||!p->bands[1].count||!p->bands[2].count)return;
    WxFog* f=&out->fog;
    float end=scalar(&p->bands[1],time),fraction=scalar(&p->bands[2],time);if(end<=0)return;
    f->end=fminf(145,end);
    /* Compress the distant fade while retaining clear nearby geometry. Negative
       authored starts intentionally haze even near the camera. */
    f->start=fraction<0?f->end*fraction:fminf(end*fraction,f->end*(90.f/145.f));
    color(&p->bands[0],time,f->color);
    f->enabled=1;f->environment=env;out->authored=wx_fog_valid(f);
}
static uint32_t choice(const WxLightVolume* v,unsigned condition){return v->profile[condition]?v->profile[condition]:v->profile[0];}
static int before(const WxLightVolume* a,float aw,const WxLightVolume* b,float bw){
    return !b||aw>bw||(aw==bw&&(a->outer<b->outer||(a->outer==b->outer&&a->id<b->id)));
}
void wx_light_weather_sample(const WxLighting* c,uint32_t map,const float pos[3],float time,float weight,unsigned env,WxLightSample* out){
    if(!out)return;
    wx_light_sample(c,map,pos,time,env==WX_FOG_UNDERWATER?WX_LIGHT_WATER:WX_LIGHT_CLEAR,env,out);
    if(env!=WX_FOG_OUTDOOR||!isfinite(weight)||weight<=0)return;
    WxLightSample storm;wx_light_sample(c,map,pos,time,WX_LIGHT_RAIN,env,&storm);
    if(weight>=1){*out=storm;return;}
    for(unsigned k=0;k<3;k++)out->fog.color[k]+=(storm.fog.color[k]-out->fog.color[k])*weight;
    out->fog.start+=(storm.fog.start-out->fog.start)*weight;out->fog.end+=(storm.fog.end-out->fog.end)*weight;
    for(unsigned b=0;b<WX_LIGHT_COLORS;b++)for(unsigned k=0;k<3;k++)out->palette.color[b][k]+=(storm.palette.color[b][k]-out->palette.color[b][k])*weight;
    out->palette.mask&=storm.palette.mask;out->authored|=storm.authored;
    /* Profiles identify the dominant source; weather telemetry retains the
       blend weight rather than labelling a partial blend as fully rainy. */
    if(weight>=.5f)memcpy(out->profile,storm.profile,sizeof out->profile);
}
void wx_light_sample(const WxLighting* c,uint32_t map,const float pos[3],float time,unsigned condition,unsigned env,WxLightSample* out){
    if(!out)return;memset(out,0,sizeof *out);wx_fog_fallback(&out->fog,env);wx_light_fallback(&out->palette);
    if(!c||!pos||!isfinite(time)||condition>2||env>WX_FOG_UNDERWATER||env==WX_FOG_INDOOR)return;
    for(unsigned i=0;i<3;i++)if(!isfinite(pos[i])||fabsf(pos[i])>40000)return;
    time=fmodf(time,2880);if(time<0)time+=2880;
    const WxLightVolume* global=NULL;const WxLightVolume* top[2]={0};float weight[2]={0};
    for(unsigned i=0;i<c->volume_count;i++){
        const WxLightVolume* v=&c->volumes[i];if(v->map!=map)continue;
        if(v->outer==0){global=v;continue;}
        float d2=0;for(unsigned k=0;k<3;k++){float d=pos[k]-v->position[k];d2+=d*d;}
        if(d2>=v->outer*v->outer)continue;
        float w=1;if(d2>v->inner*v->inner){float t=(sqrtf(d2)-v->inner)/(v->outer-v->inner);w=1-t*t*(3-2*t);}
        if(before(v,w,top[0],weight[0])){top[1]=top[0];weight[1]=weight[0];top[0]=v;weight[0]=w;}
        else if(before(v,w,top[1],weight[1])){top[1]=v;weight[1]=w;}
    }
    WxLightSample base=*out;
    if(global){uint32_t id=choice(global,condition);sample_profile(c,id,time,env,&base);if(base.authored||base.palette.mask)out->profile[2]=id;out->authored=base.authored;}
    WxLightSample local[2];float sum=0;
    for(unsigned i=0;i<2;i++)if(top[i]){
        uint32_t id=choice(top[i],condition);
        sample_profile(c,id,time,env,&local[i]);
        if(local[i].authored||local[i].palette.mask){out->volume[i]=top[i]->id;out->profile[i]=id;out->weight[i]=weight[i];sum+=weight[i];out->authored|=local[i].authored;}
    }
    out->fog=base.fog;out->palette=base.palette;
    if(sum>0){
        float strength=fmaxf(out->weight[0],out->weight[1]);
        if(strength==1)out->palette.mask=(1u<<WX_LIGHT_COLORS)-1;
        for(unsigned i=0;i<2;i++)if(out->weight[i]>0){float amount=strength*out->weight[i]/sum;
            for(unsigned k=0;k<3;k++)out->fog.color[k]+=(local[i].fog.color[k]-base.fog.color[k])*amount;
            out->fog.start+=(local[i].fog.start-base.fog.start)*amount;out->fog.end+=(local[i].fog.end-base.fog.end)*amount;
            for(unsigned b=0;b<WX_LIGHT_COLORS;b++)for(unsigned k=0;k<3;k++)out->palette.color[b][k]+=(local[i].palette.color[b][k]-base.palette.color[b][k])*amount;
            out->palette.mask&=local[i].palette.mask;}
        out->fog.enabled=1;
    }
    /* Convex RGB blending can round a black channel a few ulps below zero. */
    for(unsigned k=0;k<3;k++)out->fog.color[k]=fminf(1,fmaxf(0,out->fog.color[k]));
    for(unsigned b=0;b<WX_LIGHT_COLORS;b++)for(unsigned k=0;k<3;k++)out->palette.color[b][k]=fminf(1,fmaxf(0,out->palette.color[b][k]));
    /* Pinned WoWee light travels toward the scene; shader normals use the
       opposite vector, toward the light. Compute in unscaled world space. */
    if(out->palette.mask){float angle=time*(6.28318530718f/2880),co=cosf(angle);
        float d[3]={-sinf(angle)*.6f,.6f-co*.4f,-co*.6f};float len=sqrtf(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
        for(unsigned k=0;k<3;k++)out->palette.direction[k]=d[k]/len;}
}
