#include "wx_lighting.h"
#include <math.h>
#include <string.h>
int wx_clock_apply(WxWorldClock* c,uint16_t opcode,const void* bytes,unsigned size,uint32_t now){
    if(opcode!=0x42)return -1;
    if(!c||!bytes||size!=8)return 0;
    uint32_t packed;float speed;memcpy(&packed,bytes,4);memcpy(&speed,(const char*)bytes+4,4);
    unsigned minute=packed&63,hour=(packed>>6)&31,weekday=(packed>>11)&7,day=(packed>>14)&63,month=(packed>>20)&15;
    if(minute>59||hour>23||weekday>6||day>30||month>11||!isfinite(speed)||speed<0||speed>1440)return 0;
    *c=(WxWorldClock){now,packed,1,(float)(hour*60+minute),speed};return 1;
}
float wx_clock_half_minutes(const WxWorldClock* c,uint32_t now){
    if(!c||!c->valid)return 1440; /* Explicit noon fallback until server login clock. */
    double elapsed=(uint32_t)(now-c->anchor_ms)/1000.0;
    return (float)fmod((c->minutes+elapsed*c->speed)*2.0,2880.0);
}
