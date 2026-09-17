#ifndef WX_SPELL_WIRE_H
#define WX_SPELL_WIRE_H
#include "wx_wire.h"
typedef struct WxSpellEvent {uint64_t caster,unit;uint32_t spell,time,flags;} WxSpellEvent;
/* Shared strict Vanilla START/GO layout, including packed targets and ammunition. */
static inline int wx_spell_event(uint16_t opcode,const uint8_t* data,size_t size,WxSpellEvent* out){
    if((opcode!=0x131&&opcode!=0x132)||!out||(!data&&size)||size>65533)return 0;
    WxReader r={data,size,0,1};WxSpellEvent e={0};
    e.caster=wx_guid(&r,1);e.unit=wx_guid(&r,1);e.spell=wx_read(&r,4);e.flags=wx_read(&r,2);
    if(!e.spell||e.spell>65535)return 0;
    if(opcode==0x131){e.time=wx_read(&r,4);if(e.time>INT32_MAX)return 0;}
    else{
        unsigned hits=wx_read(&r,1);for(unsigned i=0;i<hits;i++)wx_guid(&r,0);
        unsigned misses=wx_read(&r,1);for(unsigned i=0;i<misses;i++){wx_guid(&r,0);if(wx_read(&r,1)==11)wx_read(&r,1);}
    }
    unsigned mask=wx_read(&r,2);
    if(mask&(2|0x200|0x800|0x8000))wx_guid(&r,1);
    if(mask&(0x10|0x1000))wx_guid(&r,1);
    if(mask&0x20)for(unsigned i=0;i<3;i++)wx_real(&r);
    if(mask&0x40)for(unsigned i=0;i<3;i++)wx_real(&r);
    if(mask&0x2000)wx_string(&r,NULL,65534);
    if(e.flags&0x20){wx_read(&r,4);wx_read(&r,4);}
    if(!r.ok||r.at!=size)return 0;
    *out=e;return 1;
}
#endif
