// Bounded ground/jump subset of WoWee's ClassicPacketParsers wire layout.
// No WotLK packed player GUID or flags2. Other movement modes remain future work.
#include "wx_world.h"
#include <math.h>
#include <string.h>
int wx_movement_valid(const WxMovement* m){
    if(!m||(m->flags&~(15u|WX_MOVE_JUMP)))return 0;
    if((m->flags&3)==3||(m->flags&12)==12)return 0;
    const float values[]={m->x,m->y,m->z,m->orientation,m->jump_speed,m->jump_cos,m->jump_sin,m->jump_xy};
    for(unsigned i=0;i<8;i++)if(!isfinite(values[i]))return 0;
    return fabsf(m->x)<20000&&fabsf(m->y)<20000&&fabsf(m->z)<20000&&
        m->orientation>=0&&m->orientation<6.283186f&&m->fall_ms<600000&&
        fabsf(m->jump_speed)<100&&fabsf(m->jump_cos)<=1.001f&&fabsf(m->jump_sin)<=1.001f&&m->jump_xy>=0&&m->jump_xy<100;
}
static void word(uint8_t* p,uint32_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);p[2]=(uint8_t)(n>>16);p[3]=(uint8_t)(n>>24);}
static void real(uint8_t* p,float n){uint32_t bits;memcpy(&bits,&n,4);word(p,bits);}
unsigned wx_movement_encode(const WxMovement* m,uint8_t* out,unsigned capacity){
    if(!out||!wx_movement_valid(m))return 0;
    unsigned size=(m->flags&WX_MOVE_JUMP)?44:28;if(capacity<size)return 0;
    word(out,m->flags);word(out+4,m->time_ms);real(out+8,m->x);real(out+12,m->y);real(out+16,m->z);real(out+20,m->orientation);word(out+24,m->fall_ms);
    if(size==44){real(out+28,m->jump_speed);real(out+32,m->jump_cos);real(out+36,m->jump_sin);real(out+40,m->jump_xy);}
    return size;
}
uint16_t wx_movement_opcode(uint32_t previous,uint32_t flags){
    if(!(previous&WX_MOVE_JUMP)&&(flags&WX_MOVE_JUMP))return 0xbb;
    if((previous&WX_MOVE_JUMP)&&!(flags&WX_MOVE_JUMP))return 0xc9;
    if((previous&15)&&!(flags&15))return 0xb7;
    if(!(previous&15)&&(flags&1))return 0xb5;
    if(!(previous&15)&&(flags&2))return 0xb6;
    if(!(previous&15)&&(flags&4))return 0xb8;
    if(!(previous&15)&&(flags&8))return 0xb9;
    return 0xee;
}
