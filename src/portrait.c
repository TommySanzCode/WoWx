#include "wx_portrait.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
int wx_portrait_valid(const WxPortraitEntry* e){
    if(!e||!e->key||e->source<1||e->source>3)return 0;
    if(e->key&WX_PORTRAIT_PLAYER){unsigned id=e->key&~WX_PORTRAIT_PLAYER;if(id<2||id>17)return 0;}
    else if(e->key>0xffffff)return 0;
    float d=0;
    for(unsigned i=0;i<3;i++){
        if(!isfinite(e->camera[i])||!isfinite(e->target[i])||fabsf(e->camera[i])>10000||fabsf(e->target[i])>10000)return 0;
        float delta=e->target[i]-e->camera[i];d+=delta*delta;
    }
    return d>.0001f&&isfinite(e->fov)&&e->fov>.01f&&e->fov<3.1f&&
        isfinite(e->near_clip)&&isfinite(e->far_clip)&&e->near_clip>=.001f&&
        e->far_clip>e->near_clip&&e->far_clip<=100000;
}
int wx_portrait_view(const WxPortraitEntry* e,WxPortraitView* v){
    if(!v)return 0;
    memset(v,0,sizeof *v);if(!wx_portrait_valid(e))return 0;
    float length=0;for(unsigned i=0;i<3;i++){v->camera[i]=e->camera[i];v->forward[i]=e->target[i]-e->camera[i];length+=v->forward[i]*v->forward[i];}
    length=sqrtf(length);for(unsigned i=0;i<3;i++)v->forward[i]/=length;v->camera[3]=1;
    /* Same +Z-up handedness as the world camera; vertical cameras get a stable axis. */
    float horizontal=hypotf(v->forward[0],v->forward[1]);
    if(horizontal<.0001f)v->right[0]=1;
    else {v->right[0]=v->forward[1]/horizontal;v->right[1]=-v->forward[0]/horizontal;}
    v->up[0]=v->right[1]*v->forward[2]-v->right[2]*v->forward[1];
    v->up[1]=v->right[2]*v->forward[0]-v->right[0]*v->forward[2];
    v->up[2]=v->right[0]*v->forward[1]-v->right[1]*v->forward[0];
    v->focal=32.f/tanf(e->fov*.5f);v->near_clip=e->near_clip;v->far_clip=e->far_clip;return 1;
}
int wx_portrait_open(WxPortraitCatalog* c,const char* path){
    if(!c)return 0;
    c->count=c->ready=0;
    FILE* f=fopen(path,"rb");if(!f){c->failures++;return 0;}
    uint32_t h[4];int ok=fread(h,1,16,f)==16&&h[0]==0x54505857&&h[1]==1&&h[2]&&h[2]<=WX_PORTRAIT_LIMIT&&h[3]==sizeof(WxPortraitEntry);
    if(ok)ok=fread(c->entries,sizeof(WxPortraitEntry),h[2],f)==h[2]&&fgetc(f)==EOF&&!ferror(f);
    if(fclose(f))ok=0;
    if(ok)for(unsigned i=0;i<h[2];i++)if(!wx_portrait_valid(&c->entries[i])||(i&&c->entries[i-1].key>=c->entries[i].key)){ok=0;break;}
    if(!ok){c->failures++;return 0;}c->count=h[2];c->ready=1;return 1;
}
const WxPortraitEntry* wx_portrait_find(const WxPortraitCatalog* c,uint32_t key){
    if(!c||!c->ready||c->count>WX_PORTRAIT_LIMIT)return NULL;
    unsigned lo=0,hi=c->count;while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(c->entries[mid].key<key)lo=mid+1;else hi=mid;}
    return lo<c->count&&c->entries[lo].key==key?&c->entries[lo]:NULL;
}
unsigned wx_portrait_spans(WxPortraitSpan spans[64],unsigned x,unsigned y){
    if(!spans||x>576||y>416)return 0;
    unsigned count=0;
    for(unsigned row=0;row<64;row++){
        int dy=2*(int)row-63;unsigned left=0;
        while(left<32){int dx=2*(int)left-63;if(dx*dx+dy*dy<=64*64)break;left++;}
        unsigned width=64-2*left;
        if(count&&spans[count-1].x==x+left&&spans[count-1].width==width)spans[count-1].height++;
        else spans[count++]=(WxPortraitSpan){x+left,y+row,width,1};
    }return count;
}
