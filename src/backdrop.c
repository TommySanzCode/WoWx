#include "wx_backdrop.h"
#include "wx_runtime.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(WxBackdropHeader)==124&&sizeof(WxBackdropBone)==32&&sizeof(WxBackdropBatch)==44&&sizeof(WxBackdropKey)==20,"Backdrop format ABI");
#define PTR(s,type,offset) ((const type*)((const uint8_t*)(s)+(offset)))
static int finite_values(const float* v,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(v[i])||fabsf(v[i])>1000000)return 0;return 1;}
static int span(unsigned off,unsigned count,unsigned stride,unsigned size){return off>=sizeof(WxBackdropHeader)&&!(off&3)&&off<=size&&count<=(size-off)/stride;}
static int track_kind(const WxBackdropHeader* h,const void* d,unsigned id,unsigned kind){return id==WX_BACKDROP_NONE||(id<h->tracks&&PTR(d,WxBackdropTrack,h->track_offset)[id].kind==kind);}
unsigned wx_backdrop_error(const void* d,unsigned size){
    if(!d||size<sizeof(WxBackdropHeader)||size>WX_BACKDROP_LIMIT)return __LINE__;
    const WxBackdropHeader* h=d;
    if(memcmp(h->magic,"WXB1",4)||h->version<1||h->version>3||h->file_size!=size||!h->duration_ms||h->duration_ms>600000)return __LINE__;
    if(!h->vertices||h->vertices>16384||!h->indices||h->indices>196608||h->indices%3||!h->bones||h->bones>WX_BACKDROP_BONES||!h->batches||h->batches>WX_BACKDROP_BATCHES||!h->textures||h->textures>64||h->tracks>2048||h->keys>65536)return __LINE__;
    if(!span(h->vertex_offset,h->vertices,sizeof(WxVertex),size)||!span(h->skin_offset,h->vertices,sizeof(WxSkinVertex),size)||!span(h->index_offset,h->indices,2,size)||!span(h->bone_offset,h->bones,sizeof(WxBackdropBone),size)||!span(h->batch_offset,h->batches,sizeof(WxBackdropBatch),size)||!span(h->texture_offset,h->textures,sizeof(WxBackdropTexture),size)||!span(h->track_offset,h->tracks,sizeof(WxBackdropTrack),size)||!span(h->key_offset,h->keys,sizeof(WxBackdropKey),size))return __LINE__;
    if(!finite_values(h->camera,9)||h->fov<=.1f||h->fov>=3.1f||h->near_clip<.01f||h->far_clip<=h->near_clip||h->far_clip>10000)return __LINE__;
    float dx=h->target[0]-h->camera[0],dy=h->target[1]-h->camera[1];if(dx*dx+dy*dy<.000001f)return __LINE__;
    const WxVertex* v=PTR(d,WxVertex,h->vertex_offset);const WxSkinVertex* skin=PTR(d,WxSkinVertex,h->skin_offset);
    for(unsigned i=0;i<h->vertices;i++){
        if(!finite_values(v[i].p,8))return __LINE__;unsigned weight=0;
        for(unsigned k=0;k<4;k++){weight+=skin[i].weights[k];if(skin[i].weights[k]&&skin[i].bones[k]>=h->bones)return __LINE__;}if(!weight)return __LINE__;
    }
    const uint16_t* idx=PTR(d,uint16_t,h->index_offset);for(unsigned i=0;i<h->indices;i++)if(idx[i]>=h->vertices)return __LINE__;
    const WxBackdropTrack* tracks=PTR(d,WxBackdropTrack,h->track_offset);const WxBackdropKey* keys=PTR(d,WxBackdropKey,h->key_offset);
    for(unsigned i=0;i<h->tracks;i++){
        const WxBackdropTrack* t=tracks+i;if(!t->count||t->first>h->keys||t->count>h->keys-t->first||!t->period_ms||t->period_ms>600000||t->interpolation>1||t->kind>2)return __LINE__;
        for(unsigned j=0;j<t->count;j++){
            const WxBackdropKey* k=keys+t->first+j;
            if(k->time_ms>t->period_ms||(j&&k->time_ms<=k[-1].time_ms)||!finite_values(k->value,4))return __LINE__;
            if(t->kind==2){float n=0;for(unsigned c=0;c<4;c++)n+=k->value[c]*k->value[c];if(fabsf(n-1)>.001f)return __LINE__;}
        }
    }
    const WxBackdropBone* bones=PTR(d,WxBackdropBone,h->bone_offset);
    for(unsigned i=0;i<h->bones;i++)if(bones[i].parent < -1||bones[i].parent>=(int)i||!finite_values(bones[i].pivot,3)||!track_kind(h,d,bones[i].translation,1)||!track_kind(h,d,bones[i].rotation,2)||!track_kind(h,d,bones[i].scale,1))return __LINE__;
    const WxBackdropBatch* batches=PTR(d,WxBackdropBatch,h->batch_offset);
    for(unsigned i=0;i<h->batches;i++){
        const WxBackdropBatch* b=batches+i;if(!b->count||b->count%3||b->first>h->indices||b->count>h->indices-b->first||b->texture>=h->textures||b->blend>7||b->flags>255||!finite_values(b->color,4)||!track_kind(h,d,b->alpha,0)||!track_kind(h,d,b->weight,0))return __LINE__;
    }
    unsigned gpu=0;const WxBackdropTexture* textures=PTR(d,WxBackdropTexture,h->texture_offset);
    for(unsigned i=0;i<h->textures;i++){
        const WxBackdropTexture* t=textures+i;unsigned n=t->dimension,bytes=0,levels=0;
        if(!n||n>256||(n&(n-1))||t->flags>3)return __LINE__;
        while(n){bytes+=n*n*4;levels++;n/=2;}
        if(t->size!=bytes||t->levels!=levels||!span(t->offset,t->size,1,size)||t->gpu_offset!=gpu)return __LINE__;
        gpu+=(bytes+127)&~127u;
    }
    if(!wx_backdrop_effect_validate(d,size))return __LINE__;
    if(h->version==3){
        if(!span(h->reserved,1,sizeof(WxBackdropEffects)+sizeof(WxBackdropAnchor),size))return __LINE__;
        const WxBackdropAnchor* a=PTR(d,WxBackdropAnchor,h->reserved+sizeof(WxBackdropEffects));
        if(a->present>1||(a->bone!=WX_BACKDROP_NONE&&a->bone>=h->bones)||!finite_values(a->position,3))return __LINE__;
    }
    WxBackdropEffects empty={0};const WxBackdropEffects* e=h->version>=2?PTR(d,WxBackdropEffects,h->reserved):&empty;
    return size+gpu+h->vertices*sizeof(WxVertex)+wx_backdrop_effect_bytes(h,e)<=WX_BACKDROP_LIMIT?0:__LINE__;
}
int wx_backdrop_validate(const void* data,unsigned size){return wx_backdrop_error(data,size)==0;}
void wx_backdrop_close(WxBackdrop* s){
    if(s->pending)fclose(s->pending);s->pending=NULL;s->load_phase=s->load_read=s->load_texture=s->load_offset=0;
    if(s->vertices)wx_gpu_free(s->vertices);if(s->pixels)wx_gpu_free(s->pixels);free(s->data);
    if(s->effect_vertices)wx_gpu_free(s->effect_vertices);if(s->lighting)wx_gpu_free(s->lighting);free(s->simulation);free(s->light_state);
    s->effect_vertices=NULL;s->lighting=NULL;s->simulation=NULL;s->light_state=NULL;memset(&s->effects,0,sizeof s->effects);
    s->data=NULL;s->vertices=NULL;s->pixels=NULL;s->ready=s->bytes=s->draws=s->triangles=s->geometry_ready=0;
}
int wx_backdrop_begin(WxBackdrop* s,const char* path){
    wx_backdrop_close(s);s->pending=fopen(path,"rb");if(!s->pending)goto fail;
    WxBackdropHeader h;
    if(fread(&h,1,sizeof h,s->pending)!=sizeof h||memcmp(h.magic,"WXB1",4)||h.version<1||h.version>3||
       h.file_size<sizeof h||h.file_size>WX_BACKDROP_LIMIT||wx_free_memory()<h.file_size+8*1024*1024+65536)goto fail;
    s->data=malloc(h.file_size);if(!s->data)goto fail;
    memcpy(s->data,&h,sizeof h);s->h=h;s->bytes=h.file_size;s->load_read=sizeof h;s->load_phase=1;return 1;
fail:wx_backdrop_close(s);s->failures++;return 0;
}
int wx_backdrop_pump(WxBackdrop* s,unsigned time){
    if(!s->load_phase)return s->ready!=0;
    WxBackdropHeader h=s->h;
    if(s->load_phase==1){
        unsigned n=h.file_size-s->load_read;if(n>WX_BACKDROP_IO_SLICE)n=WX_BACKDROP_IO_SLICE;
        if(fread((uint8_t*)s->data+s->load_read,1,n,s->pending)!=n)goto fail;s->load_read+=n;
        if(s->load_read==h.file_size){if(fgetc(s->pending)!=EOF)goto fail;fclose(s->pending);s->pending=NULL;s->load_phase=2;}
        return 0;
    }
    if(s->load_phase==2){
        if(!wx_backdrop_validate(s->data,h.file_size))goto fail;
        const WxBackdropTexture* textures=PTR(s->data,WxBackdropTexture,h.texture_offset);unsigned gpu=0;
        for(unsigned i=0;i<h.textures;i++)gpu+=(textures[i].size+127)&~127u;
        if(h.version>=2)s->effects=*PTR(s->data,WxBackdropEffects,h.reserved);
        unsigned vertices=h.vertices*sizeof(WxVertex),extra=wx_backdrop_effect_bytes(&h,&s->effects);
        if(wx_free_memory()<gpu+vertices+extra+8*1024*1024+65536)goto fail;
        s->vertices=wx_gpu_alloc(vertices);s->pixels=wx_gpu_alloc(gpu);if(!s->vertices||!s->pixels)goto fail;memset(s->vertices,0,vertices);
        if(s->effects.emitters){s->simulation=calloc(1,sizeof(WxBackdropSimulation));s->effect_vertices=wx_gpu_alloc(s->effects.capacity*4*sizeof(WxEffectVertex));if(!s->simulation||!s->effect_vertices)goto fail;}
        if(s->effects.lights){
            s->lighting=wx_gpu_alloc(h.vertices*16);s->light_state=calloc(1,sizeof(WxBackdropLighting)+h.vertices);if(!s->lighting||!s->light_state)goto fail;
            for(unsigned i=0;i<h.vertices;i++)for(unsigned j=0;j<4;j++)s->lighting[i][j]=1;
            uint8_t* dirty=(void*)(s->light_state+1);const uint16_t* indices=PTR(s->data,uint16_t,h.index_offset);const WxBackdropBatch* batches=PTR(s->data,WxBackdropBatch,h.batch_offset);
            for(unsigned i=0;i<h.batches;i++)if(!(batches[i].flags&1))for(unsigned j=0;j<batches[i].count;j++)dirty[indices[batches[i].first+j]]=3;
        }
        s->bytes=h.file_size+gpu+vertices+extra;s->load_phase=3;return 0;
    }
    if(s->load_phase==3){
        const WxBackdropTexture* textures=PTR(s->data,WxBackdropTexture,h.texture_offset);unsigned left=WX_BACKDROP_IO_SLICE;
        while(left&&s->load_texture<h.textures){
            const WxBackdropTexture* t=textures+s->load_texture;unsigned n=t->size-s->load_offset;if(n>left)n=left;
            memcpy(s->pixels+t->gpu_offset+s->load_offset,(const uint8_t*)s->data+t->offset+s->load_offset,n);
            left-=n;s->load_offset+=n;if(s->load_offset==t->size){s->load_texture++;s->load_offset=0;}
        }
        if(s->load_texture==h.textures)s->load_phase=4;return 0;
    }
    s->load_phase=0;s->ready=1;wx_backdrop_view(s,0,0,640,480);wx_backdrop_update(s,time);
    if(s->ready)s->loads++;return s->ready!=0;
fail:wx_backdrop_close(s);s->failures++;return 0;
}
int wx_backdrop_open(WxBackdrop* s,const char* path){
    if(!wx_backdrop_begin(s,path))return 0;
    while(s->load_phase)wx_backdrop_pump(s,0);return s->ready!=0;
}
int wx_backdrop_anchor(const WxBackdrop* s,float position[3]){
    if(!s->ready||s->h.version<3)return 0;
    const WxBackdropAnchor* a=PTR(s->data,WxBackdropAnchor,s->h.reserved+sizeof(WxBackdropEffects));
    if(!a->present)return 0;
    if(a->bone==WX_BACKDROP_NONE)memcpy(position,a->position,12);
    else {const float* m=s->matrices[a->bone];for(unsigned i=0;i<3;i++)position[i]=m[i*4]*a->position[0]+m[i*4+1]*a->position[1]+m[i*4+2]*a->position[2]+m[i*4+3];}
    return 1;
}
void wx_backdrop_sample(const WxBackdrop* s,unsigned id,unsigned time,const float fallback[4],float out[4]){
    memcpy(out,fallback,16);if(id>=s->h.tracks)return;
    const WxBackdropTrack* t=PTR(s->data,WxBackdropTrack,s->h.track_offset)+id;
    const WxBackdropKey* keys=PTR(s->data,WxBackdropKey,s->h.key_offset)+t->first;
    time%=t->period_ms;unsigned lo=0,hi=t->count;
    while(lo+1<hi){unsigned mid=(lo+hi)/2;if(keys[mid].time_ms<=time)lo=mid;else hi=mid;}
    const float* a=keys[lo].value;memcpy(out,a,16);
    if(!t->interpolation||lo+1==t->count||time<=keys[lo].time_ms)return;
    const float* b=keys[lo+1].value;float f=(float)(time-keys[lo].time_ms)/(keys[lo+1].time_ms-keys[lo].time_ms),wa=1-f,wb=f;
    if(t->kind==2){float dot=0;for(unsigned k=0;k<4;k++)dot+=a[k]*b[k];float sign=dot<0?-1.f:1.f;dot=fabsf(dot);
        if(dot<.9995f){float theta=acosf(dot),den=sinf(theta);wa=sinf((1-f)*theta)/den;wb=sinf(f*theta)/den;}wb*=sign;
    }
    for(unsigned k=0;k<4;k++)out[k]=wa*a[k]+wb*b[k];
    if(t->kind==2){float n=0;for(unsigned k=0;k<4;k++)n+=out[k]*out[k];n=1/sqrtf(n);for(unsigned k=0;k<4;k++)out[k]*=n;}
}
void wx_backdrop_update(WxBackdrop* s,unsigned time){
    if(!s->ready)return;s->time_ms=time;s->updates++;
    const float zero[4]={0,0,0,0},one[4]={1,1,1,1},identity[4]={0,0,0,1};
    const WxBackdropBone* bones=PTR(s->data,WxBackdropBone,s->h.bone_offset);
    unsigned changed[WX_BACKDROP_BONES];
    for(unsigned i=0;i<s->h.bones;i++){
        const WxBackdropBone* b=bones+i;float tr[4],q[4],scale[4],m[12],previous[12];memcpy(previous,s->matrices[i],sizeof previous);
        wx_backdrop_sample(s,b->translation,time,zero,tr);wx_backdrop_sample(s,b->rotation,time,identity,q);wx_backdrop_sample(s,b->scale,time,one,scale);
        float x=q[0],y=q[1],z=q[2],w=q[3];
        m[0]=(1-2*(y*y+z*z))*scale[0];m[1]=2*(x*y-z*w)*scale[1];m[2]=2*(x*z+y*w)*scale[2];
        m[4]=2*(x*y+z*w)*scale[0];m[5]=(1-2*(x*x+z*z))*scale[1];m[6]=2*(y*z-x*w)*scale[2];
        m[8]=2*(x*z-y*w)*scale[0];m[9]=2*(y*z+x*w)*scale[1];m[10]=(1-2*(x*x+y*y))*scale[2];
        for(unsigned j=0;j<3;j++)m[j*4+3]=b->pivot[j]+tr[j]-m[j*4]*b->pivot[0]-m[j*4+1]*b->pivot[1]-m[j*4+2]*b->pivot[2];
        if(b->parent<0)memcpy(s->matrices[i],m,sizeof m);
        else{const float* p=s->matrices[b->parent];for(unsigned row=0;row<3;row++)for(unsigned col=0;col<4;col++)s->matrices[i][row*4+col]=p[row*4]*m[col]+p[row*4+1]*m[4+col]+p[row*4+2]*m[8+col]+(col==3?p[row*4+3]:0);}
        if(!finite_values(s->matrices[i],12)){wx_backdrop_close(s);s->failures++;return;}
        changed[i]=!s->geometry_ready||memcmp(previous,s->matrices[i],sizeof previous)!=0;
    }
    const WxVertex* source=PTR(s->data,WxVertex,s->h.vertex_offset);const WxSkinVertex* skin=PTR(s->data,WxSkinVertex,s->h.skin_offset);
    for(unsigned i=0;i<s->h.vertices;i++){
        unsigned dirty=!s->geometry_ready;for(unsigned k=0;k<4;k++)if(skin[i].weights[k]&&changed[skin[i].bones[k]])dirty=1;
        if(!dirty)continue;
        WxVertex computed;WxVertex* v=&computed;memset(v,0,sizeof *v);memcpy(v->uv,source[i].uv,sizeof v->uv);unsigned sum=0;
        for(unsigned k=0;k<4;k++)sum+=skin[i].weights[k];
        for(unsigned k=0;k<4;k++)if(skin[i].weights[k]){const float* m=s->matrices[skin[i].bones[k]];float weight=(float)skin[i].weights[k]/sum;
            for(unsigned j=0;j<3;j++){v->p[j]+=weight*(m[j*4]*source[i].p[0]+m[j*4+1]*source[i].p[1]+m[j*4+2]*source[i].p[2]+m[j*4+3]);v->n[j]+=weight*(m[j*4]*source[i].n[0]+m[j*4+1]*source[i].n[1]+m[j*4+2]*source[i].n[2]);}
        }
        if(!finite_values(v->p,6)){wx_backdrop_close(s);s->failures++;return;}
        if(s->light_state&&memcmp(s->vertices+i,v,6*sizeof(float)))((uint8_t*)(s->light_state+1))[i]|=2;
        s->vertices[i]=computed;
    }
    s->geometry_ready=1;
    const WxBackdropBatch* batches=PTR(s->data,WxBackdropBatch,s->h.batch_offset);
    for(unsigned i=0;i<s->h.batches;i++){float alpha[4],weight[4];wx_backdrop_sample(s,batches[i].alpha,time,one,alpha);wx_backdrop_sample(s,batches[i].weight,time,one,weight);
        memcpy(s->colors[i],batches[i].color,16);s->colors[i][3]=fminf(1,fmaxf(0,s->colors[i][3]*alpha[0]*weight[0]));}
    wx_backdrop_effect_update(s,time);
}
