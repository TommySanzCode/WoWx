#include "wx_backdrop.h"
#include <pbkit/pbkit.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#define BACKDROP_PROGRAM 112
static uint32_t* material(uint32_t* p,const WxBackdrop* s,unsigned texture,unsigned blend,unsigned flags){
    const WxBackdropTexture* t=(const void*)((const uint8_t*)s->data+s->h.texture_offset);t+=texture;
    unsigned log=0;while((1u<<log)<t->dimension)log++;
    unsigned src=NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA,dst=NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA;
    if(blend==3){src=NV097_SET_BLEND_FUNC_SFACTOR_V_ONE;dst=NV097_SET_BLEND_FUNC_DFACTOR_V_ONE;}
    if(blend==4)dst=NV097_SET_BLEND_FUNC_DFACTOR_V_ONE;
    if(blend==5){src=NV097_SET_BLEND_FUNC_SFACTOR_V_DST_COLOR;dst=NV097_SET_BLEND_FUNC_DFACTOR_V_ZERO;}
    if(blend==6){src=NV097_SET_BLEND_FUNC_SFACTOR_V_DST_COLOR;dst=NV097_SET_BLEND_FUNC_DFACTOR_V_SRC_COLOR;}
    if(blend==7)src=NV097_SET_BLEND_FUNC_SFACTOR_V_ONE;
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,(flags&8)?0:1);p=pb_push1(p,NV097_SET_DEPTH_MASK,blend>=2||(flags&16)?0:1);
    p=pb_push1(p,NV097_SET_BLEND_ENABLE,blend>=2);p=pb_push1(p,NV097_SET_BLEND_FUNC_SFACTOR,src);p=pb_push1(p,NV097_SET_BLEND_FUNC_DFACTOR,dst);
    p=pb_push1(p,NV097_SET_ALPHA_TEST_ENABLE,blend==1);p=pb_push1(p,NV097_SET_ALPHA_FUNC,NV097_SET_ALPHA_FUNC_V_GREATER);p=pb_push1(p,NV097_SET_ALPHA_REF,100);
    p=pb_push2(p,NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0),((uint32_t)s->pixels+t->gpu_offset)&0x03ffffff,0x62a|(t->levels<<16)|(log<<20)|(log<<24));
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0),(t->dimension*4)<<16);p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0),(t->dimension<<16)|t->dimension);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_WRAP(0),0x00030000|((t->flags&2)?0x100:0x300)|((t->flags&1)?1:3));
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0),0x4003ffc0);return pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_FILTER(0),0x02062000);
}
void wx_backdrop_draw(WxBackdrop* s){
    s->draws=s->triangles=0;if(!s->ready)return;static int initialized;
    if(!initialized){const uint32_t program[]={
#include "backdrop.inl"
        };_Static_assert(sizeof program<=24*16,"Backdrop exceeds NV2A program storage");
        uint32_t* p=pb_begin();p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_LOAD,BACKDROP_PROGRAM);pb_end(p);
        for(unsigned i=0;i<sizeof program/16;i++){p=pb_begin();pb_push(p++,NV097_SET_TRANSFORM_PROGRAM,4);memcpy(p,program+i*4,16);p+=4;pb_end(p);}initialized=1;
    }
    const WxBackdropView* view=&s->view;
    float camera[4]={view->camera[0],view->camera[1],view->camera[2],1};
    float right[4]={view->right[0],view->right[1],view->right[2],0},up[4]={view->up[0],view->up[1],view->up[2],0};
    float forward[4]={view->forward[0],view->forward[1],view->forward[2],0};
    float projection[4]={view->center[0],view->center[1],view->focal,1};
    float depth[4]={65535*view->far_clip/(view->far_clip-view->near_clip),65535*view->far_clip*view->near_clip/(view->far_clip-view->near_clip),1,0};
    float tint[4]={1,1,1,1};
    uint32_t* p=pb_begin();p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,BACKDROP_PROGRAM);p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,96);
    const float* values[]={camera,right,up,forward,projection,depth,tint};
    for(unsigned i=0;i<7;i++){pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,4);memcpy(p,values[i],16);p+=4;}
    p=pb_push1(p,NV097_SET_CONTROL0,NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE|NV097_SET_CONTROL0_TEXTURE_PERSPECTIVE_ENABLE);
    p=pb_push1(p,NV097_SET_CULL_FACE_ENABLE,0);p=pb_push1(p,NV097_SET_DEPTH_FUNC,NV097_SET_DEPTH_FUNC_V_LEQUAL);
    for(unsigned i=1;i<4;i++)p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(i),0x0003ffc0);
    pb_push(p++,NV097_SET_VERTEX_DATA_ARRAY_FORMAT,16);for(unsigned i=0;i<16;i++)*p++=2;
    unsigned attrs[]={0,2,9},components[]={3,3,2},offsets[]={offsetof(WxVertex,p),offsetof(WxVertex,n),offsetof(WxVertex,uv)};
    for(unsigned i=0;i<3;i++){p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+attrs[i]*4,2|(components[i]<<4)|(sizeof(WxVertex)<<8));p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+attrs[i]*4,((uint32_t)s->vertices+offsets[i])&0x03ffffff);}
    if(s->lighting){p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+3*4,2|(4<<4)|(16<<8));p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+3*4,((uint32_t)s->lighting)&0x03ffffff);}pb_end(p);
    const WxBackdropBatch* batches=(const void*)((const uint8_t*)s->data+s->h.batch_offset);
    const uint16_t* indices=(const void*)((const uint8_t*)s->data+s->h.index_offset);
    for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<s->h.batches;i++){
        const WxBackdropBatch* b=batches+i;unsigned blend=b->blend;
        if((blend>=2)!=pass||s->colors[i][3]<.001f)continue;
        if(blend==0&&s->colors[i][3]<.999f)blend=2;
        p=pb_begin();p=material(p,s,b->texture,blend,b->flags);
        depth[2]=(b->flags&1)||!s->lighting?1:0;depth[3]=1-depth[2];
        p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,101);pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,4);memcpy(p,depth,16);p+=4;pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,4);memcpy(p,s->colors[i],16);p+=4;pb_end(p);
        for(unsigned at=0;at<b->count;){unsigned n=b->count-at;if(n>240)n=240;p=pb_begin();p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_TRIANGLES);
            pb_push(p++,0x40000000|NV20_TCL_PRIMITIVE_3D_INDEX_DATA,n/2);
            for(unsigned k=0;k<n/2;k++)*p++=indices[b->first+at+k*2]|((uint32_t)indices[b->first+at+k*2+1]<<16);
            if(n&1)p=pb_push1(p,NV097_ARRAY_ELEMENT32,indices[b->first+at+n-1]);
            p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);pb_end(p);at+=n;
        }s->draws++;s->triangles+=b->count/3;
    }
    if(s->simulation&&s->simulation->quads){
        const WxBackdropSimulation* f=s->simulation;const WxBackdropEmitter* emitters=(const void*)((const uint8_t*)s->data+s->effects.emitter_offset);
        p=pb_begin();pb_push(p++,NV097_SET_VERTEX_DATA_ARRAY_FORMAT,16);for(unsigned i=0;i<16;i++)*p++=2;
        unsigned attr[]={0,3,9},comp[]={3,4,2},off[]={offsetof(WxEffectVertex,p),offsetof(WxEffectVertex,color),offsetof(WxEffectVertex,uv)};
        for(unsigned i=0;i<3;i++){p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+attr[i]*4,2|(comp[i]<<4)|(sizeof(WxEffectVertex)<<8));p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+attr[i]*4,((uint32_t)s->effect_vertices+off[i])&0x03ffffff);}
        depth[2]=0;depth[3]=1;p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,101);pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,4);memcpy(p,depth,16);p+=4;pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,4);memcpy(p,tint,16);p+=4;pb_end(p);
        for(unsigned e=0;e<s->effects.emitters;e++)if(f->count[e]){
            p=pb_begin();p=material(p,s,emitters[e].texture,emitters[e].blend,16);
            p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_QUADS);
            for(unsigned at=0,n=f->count[e]*4;at<n;){unsigned count=n-at;if(count>256)count=256;p=pb_push1(p,NV097_DRAW_ARRAYS,((count-1)<<24)|(f->first[e]+at));at+=count;}
            p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);pb_end(p);s->draws++;s->triangles+=f->count[e]*2;
        }
    }
}
