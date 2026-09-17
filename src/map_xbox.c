#include "wx_fog.h"
#include "wx_map.h"
#include "wx_font.h"
#include <pbkit/pbkit.h>
#include <string.h>
#define MAP_PROGRAM 100
void wx_map_draw(WxMap* m){
    wx_fog_bind(NULL);
    if(!m->open)return;
    pb_fill(20,16,600,432,0xff14202b);wx_font_clear();
    const WxMapEntry* e=m->count?&m->entries[m->selected]:NULL;
    wx_font_printat(0,2,"%.42s / %ux",e?e->name:"World map",(unsigned)m->zoom);
    if(m->marker)wx_font_printat(1,2,"You: %u, %u",(unsigned)(m->player_u*100+.5f),(unsigned)(m->player_v*100+.5f));
    if(m->body)wx_font_printat(1,24,"Body: %u, %u",(unsigned)(m->body_u*100+.5f),(unsigned)(m->body_v*100+.5f));
    wx_font_printat(14,2,"A: zoom  X: player  Left stick: pan");
    wx_font_printat(15,2,"D-pad: maps/up continent/down here  Back/B: close");
    if(!m->ready){wx_font_printat(7,2,"%.54s",m->error);return;}
    static int shader_ready=0;
    if(!shader_ready){const uint32_t program[]={
#include "font.inl"
        };uint32_t* p=pb_begin();p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_LOAD,MAP_PROGRAM);pb_end(p);
        for(unsigned i=0;i<sizeof program/16;i++){p=pb_begin();pb_push(p++,NV097_SET_TRANSFORM_PROGRAM,4);memcpy(p,program+i*4,16);p+=4;pb_end(p);}shader_ready=1;
    }
    const float x=95,y=70,w=450,h=300,half=.5f/m->zoom;
    float u0=(m->u-half)*501/512.f,v0=(m->v-half)*334/512.f,u1=(m->u+half)*501/512.f,v1=(m->v+half)*334/512.f;
    WxFontVertex* v=m->vertices;
    v[0]=(WxFontVertex){{x,y,0,1},{u0,v0}};v[1]=(WxFontVertex){{x+w,y,0,1},{u1,v0}};
    v[2]=(WxFontVertex){{x+w,y+h,0,1},{u1,v1}};v[3]=(WxFontVertex){{x,y+h,0,1},{u0,v1}};
    uint32_t* p=pb_begin();p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,MAP_PROGRAM);
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,0);p=pb_push1(p,NV097_SET_DEPTH_MASK,0);
    p=pb_push1(p,NV097_SET_CULL_FACE_ENABLE,0);p=pb_push1(p,NV097_SET_BLEND_ENABLE,0);p=pb_push1(p,NV097_SET_ALPHA_TEST_ENABLE,0);
    pb_push(p++,NV097_SET_VERTEX_DATA_ARRAY_FORMAT,16);for(unsigned i=0;i<16;i++)*p++=2;
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT,2|(4<<4)|(sizeof(WxFontVertex)<<8));p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET,(uint32_t)v&0x03ffffff);
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+9*4,2|(2<<4)|(sizeof(WxFontVertex)<<8));p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+9*4,(uint32_t)v[0].uv&0x03ffffff);
    float white[4]={1,1,1,1};pb_push(p++,NV097_SET_VERTEX_DATA4F_M+3*16,4);memcpy(p,white,16);p+=4;
    p=pb_push2(p,NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0),(uint32_t)m->pixels&0x03ffffff,0x0991062a);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0),2048<<16);p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0),(512<<16)|512);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_WRAP(0),0x00030303);p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0),0x4003ffc0);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_FILTER(0),0x02022000);
    for(unsigned i=1;i<4;i++)p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(i),0x0003ffc0);
    p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_QUADS);p=pb_push1(p,NV097_DRAW_ARRAYS,3u<<24);p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);pb_end(p);
    float px=(m->player_u-m->u)*m->zoom+.5f,py=(m->player_v-m->v)*m->zoom+.5f;
    if(m->marker&&px>=0&&px<=1&&py>=0&&py<=1){unsigned sx=(unsigned)(x+px*w),sy=(unsigned)(y+py*h);
        pb_fill(sx-4,sy-4,9,9,0xff151515);pb_fill(sx-1,sy-4,3,9,0xffffff20);pb_fill(sx-4,sy-1,9,3,0xffffff20);}
    px=(m->body_u-m->u)*m->zoom+.5f;py=(m->body_v-m->v)*m->zoom+.5f;
    if(m->body&&px>=0&&px<=1&&py>=0&&py<=1){unsigned sx=(unsigned)(x+px*w),sy=(unsigned)(y+py*h);
        pb_fill(sx-4,sy-4,9,9,0xff151515);pb_fill(sx-1,sy-4,3,9,0xffff5544);pb_fill(sx-4,sy-1,9,3,0xffff5544);}
}
