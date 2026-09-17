#include "wx_fog.h"
#include "wx_ui.h"
#include <pbkit/pbkit.h>
#include <stddef.h>
#include <string.h>
#define UI_PROGRAM 104
void wx_ui_draw(WxUi* u){
    wx_fog_bind(NULL);
    if(!u->ready||!u->quads)return;static int initialized;
    if(!initialized){const uint32_t program[]={
#include "font.inl"
        };uint32_t* p=pb_begin();p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_LOAD,UI_PROGRAM);pb_end(p);
        for(unsigned i=0;i<sizeof program/16;i++){p=pb_begin();pb_push(p++,NV097_SET_TRANSFORM_PROGRAM,4);memcpy(p,program+i*4,16);p+=4;pb_end(p);}initialized=1;
    }
    uint32_t* p=pb_begin();p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,UI_PROGRAM);
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,0);p=pb_push1(p,NV097_SET_DEPTH_MASK,0);p=pb_push1(p,NV097_SET_CULL_FACE_ENABLE,0);
    p=pb_push1(p,NV097_SET_BLEND_ENABLE,1);
    p=pb_push1(p,NV097_SET_BLEND_FUNC_SFACTOR,NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
    p=pb_push1(p,NV097_SET_BLEND_FUNC_DFACTOR,NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);
    p=pb_push1(p,NV097_SET_ALPHA_TEST_ENABLE,0);
    pb_push(p++,NV097_SET_VERTEX_DATA_ARRAY_FORMAT,16);for(unsigned i=0;i<16;i++)*p++=2;
    unsigned attributes[3]={0,3,9},components[3]={4,4,2};size_t offsets[3]={offsetof(WxUiVertex,position),offsetof(WxUiVertex,color),offsetof(WxUiVertex,uv)};
    for(unsigned i=0;i<3;i++){p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+attributes[i]*4,2|(components[i]<<4)|(sizeof(WxUiVertex)<<8));
        p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+attributes[i]*4,((uint32_t)u->vertices+offsets[i])&0x03ffffff);}
    p=pb_push1(p,NV097_SET_STENCIL_TEST_ENABLE,0);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_WRAP(0),0x00030303);p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0),0x4003ffc0);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_FILTER(0),0x02022000);
    for(unsigned i=1;i<4;i++)p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(i),0x0003ffc0);pb_end(p);
    for(unsigned i=0;i<u->batch_count;i++){
        const WxUiBatch* batch=&u->batches[i];unsigned logsize=0;while((1u<<logsize)<batch->size)logsize++;
        p=pb_begin();p=pb_push2(p,NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0),(uint32_t)batch->pixels&0x03ffffff,0x0001062a|(logsize<<20)|(logsize<<24));
        p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0),(batch->size*4)<<16);p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0),(batch->size<<16)|batch->size);
        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_QUADS);
        for(unsigned at=batch->first*4,count=(batch->first+batch->count)*4;at<count;){unsigned n=count-at;if(n>256)n=256;p=pb_push1(p,NV097_DRAW_ARRAYS,((n-1)<<24)|at);at+=n;}
        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);pb_end(p);
    }
}
