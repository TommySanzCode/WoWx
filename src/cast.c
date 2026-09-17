#include "wx_cast.h"
#include "wx_spell_wire.h"
#include <string.h>
static void begin(WxCastState* s,unsigned spell,unsigned phase,unsigned time,unsigned now){
    unsigned revision=s->revision+1;*s=(WxCastState){spell,phase,now,time,0,time==UINT32_MAX,revision};
}
static int active(const WxCastState* s){return s->phase==WX_CAST_PREPARING||s->phase==WX_CAST_CHANNEL;}
int wx_cast_apply(WxCastState* s,uint16_t op,const uint8_t* data,size_t size,uint64_t player,uint32_t now){
    if(op!=0x131&&op!=0x132&&op!=0x130&&op!=0x2a6&&op!=0x139&&op!=0x13a&&op!=0x1e2)return -1;
    if(!s||(!data&&size)||size>65533)return 0;
    WxReader r={data,size,0,1};unsigned spell=0,time=0;
    if(op==0x131||op==0x132){
        WxSpellEvent event;if(!wx_spell_event(op,data,size,&event))return 0;
        if(event.unit!=player)return 1;
        if(op==0x131){if(event.time)begin(s,event.spell,WX_CAST_PREPARING,event.time,now);}
        else if(s->spell==event.spell&&s->phase==WX_CAST_PREPARING)begin(s,s->spell,WX_CAST_COMPLETE,350,now);
        return 1;
    }
    if(op==0x130){
        spell=wx_read(&r,4);unsigned result=wx_read(&r,1);
        if(result==2){wx_read(&r,1);if(size-r.at!=0&&size-r.at!=4&&size-r.at!=8)return 0;while(r.ok&&r.at<size)wx_read(&r,4);}
        else if(result!=0)return 0;
        if(!spell||spell>65535||!r.ok||r.at!=size)return 0;
        if(result==2&&s->spell==spell&&active(s))begin(s,spell,WX_CAST_FAILED,800,now);
    }else if(op==0x2a6){
        uint64_t caster=wx_guid(&r,0);spell=wx_read(&r,4);
        if(!spell||spell>65535||!r.ok||r.at!=size)return 0;
        if(caster==player&&s->spell==spell&&active(s))begin(s,spell,WX_CAST_INTERRUPTED,800,now);
    }else if(op==0x139||op==0x13a){
        if(op==0x139){spell=wx_read(&r,4);if(!spell||spell>65535)return 0;}
        time=wx_read(&r,4);if((time>INT32_MAX&&time!=UINT32_MAX)||!r.ok||r.at!=size)return 0;
        if(op==0x139){if(time)begin(s,spell,WX_CAST_CHANNEL,time,now);}
        else if(s->phase==WX_CAST_CHANNEL){
            if(!time)begin(s,s->spell,WX_CAST_COMPLETE,350,now);
            else if(time==UINT32_MAX){s->infinite=1;s->revision++;}
            else {if(s->infinite||time>s->duration)s->duration=time;s->infinite=0;s->start=now-(s->duration-time);s->revision++;}
        }
    }else{
        uint64_t caster=wx_guid(&r,0);time=wx_read(&r,4);
        if(time>INT32_MAX||!r.ok||r.at!=size)return 0;
        if(caster==player&&s->phase==WX_CAST_PREPARING){
            if(time>INT32_MAX-s->duration||time>INT32_MAX-s->delay)return 0;
            s->duration+=time;s->delay+=time;s->revision++;
        }
    }
    return 1;
}
WxCastView wx_cast_query(const WxCastState* s,uint32_t now){
    WxCastView v={0};if(!s)return v;v.revision=s->revision;
    if(!s->phase)return v;
    unsigned elapsed=now-s->start;if(!s->infinite&&elapsed>=s->duration)return v;
    v.spell=s->spell;v.phase=s->phase;v.elapsed=elapsed;v.duration=s->duration;
    v.remaining=s->infinite?UINT32_MAX:s->duration-elapsed;v.delay=s->delay;v.infinite=s->infinite;return v;
}
int wx_cast_cancel(const WxCastView* v,WxCommand* c){
    if(!v||!c||!v->spell||(v->phase!=WX_CAST_PREPARING&&v->phase!=WX_CAST_CHANNEL))return 0;
    *c=(WxCommand){v->phase==WX_CAST_CHANNEL?0x13b:0x12f,0,v->spell,0};return 1;
}
