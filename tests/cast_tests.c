#include "wx_cast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,at;static uint8_t wire[128];static WxCastState state;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Cast FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static void w(uint32_t n,unsigned bytes){for(unsigned i=0;i<bytes;i++)wire[at++]=(uint8_t)(n>>(i*8));}
static void start(unsigned caster,unsigned spell,unsigned time){at=0;for(unsigned i=0;i<2;i++){w(1,1);w(caster,1);}w(spell,4);w(0,2);w(time,4);w(0,2);}
static int apply(unsigned op,unsigned time){return wx_cast_apply(&state,(uint16_t)op,wire,at,1,time);}
static WxCastView view(unsigned time){return wx_cast_query(&state,time);}
static void bad(unsigned op){WxCastState saved=state;for(unsigned n=0;n<at;n++){CHECK(!wx_cast_apply(&state,(uint16_t)op,wire,n,1,123));CHECK(!memcmp(&saved,&state,sizeof state));}wire[at]=0;CHECK(!wx_cast_apply(&state,(uint16_t)op,wire,at+1,1,123));CHECK(!memcmp(&saved,&state,sizeof state));}
int main(void){
    CHECK(sizeof state==28&&sizeof(WxCastView)==32);start(1,133,3000);bad(0x131);CHECK(apply(0x131,1000)==1);
    CHECK(view(1000).phase==WX_CAST_PREPARING&&view(2500).remaining==1500&&view(2500).elapsed==1500&&!view(4000).phase);
    WxCommand c={0};WxCastView v=view(2500);CHECK(wx_cast_cancel(&v,&c)&&c.opcode==0x12f&&c.value==133);uint8_t encoded[4];CHECK(wx_command_encode(&c,encoded,4)==4&&encoded[0]==133&&!encoded[1]);CHECK(wx_command_encode(&c,encoded,3)<0);
    start(2,116,5000);CHECK(apply(0x131,1600)==1&&view(1600).spell==133);start(1,116,0);CHECK(apply(0x131,1600)==1&&view(1600).spell==133);
    at=0;w(1,4);w(0,4);w(500,4);bad(0x1e2);CHECK(apply(0x1e2,2000)==1&&view(2000).remaining==2500&&view(2000).delay==500);
    wire[0]=2;CHECK(apply(0x1e2,2000)==1&&view(2000).delay==500);
    at=0;w(133,4);w(0,1);bad(0x130);CHECK(apply(0x130,2100)==1&&view(2100).phase==WX_CAST_PREPARING);
    at=0;for(unsigned i=0;i<4;i++)w(1,1);w(133,4);w(0,2);w(0,1);w(0,1);w(0,2);bad(0x132);CHECK(apply(0x132,2500)==1&&view(2500).phase==WX_CAST_COMPLETE&&!view(2850).phase);
    at=0;w(1,4);w(0,4);w(133,4);bad(0x2a6);CHECK(apply(0x2a6,2600)==1&&view(2600).phase==WX_CAST_COMPLETE);
    start(1,133,3000);CHECK(apply(0x131,5000)==1);at=0;w(116,4);w(2,1);w(7,1);CHECK(apply(0x130,5100)==1&&view(5100).phase==WX_CAST_PREPARING);
    wire[0]=133;bad(0x130);CHECK(apply(0x130,5200)==1&&view(5200).phase==WX_CAST_FAILED&&!view(6000).phase);
    for(unsigned extra=0;extra<=2;extra++){start(1,133,3000);CHECK(apply(0x131,6000)==1);at=0;w(133,4);w(2,1);w(7,1);for(unsigned i=0;i<extra;i++)w(1,4);CHECK(apply(0x130,6100)==1&&view(6100).phase==WX_CAST_FAILED);}
    at=0;w(689,4);w(3000,4);bad(0x139);CHECK(apply(0x139,7000)==1&&view(7500).remaining==2500);v=view(7500);CHECK(wx_cast_cancel(&v,&c)&&c.opcode==0x13b&&c.value==689);
    at=0;w(1500,4);bad(0x13a);CHECK(apply(0x13a,7800)==1&&view(7800).duration==3000&&view(7800).elapsed==1500&&view(7900).remaining==1400);
    at=0;w(0,4);CHECK(apply(0x13a,8000)==1&&view(8000).phase==WX_CAST_COMPLETE);v=view(8000);CHECK(!wx_cast_cancel(&v,&c));
    at=0;w(689,4);w(UINT32_MAX,4);CHECK(apply(0x139,9000)==1&&view(900000).infinite&&view(900000).remaining==UINT32_MAX);
    at=0;w(2000,4);CHECK(apply(0x13a,900000)==1&&!view(900000).infinite&&view(900000).remaining==2000);
    at=0;w(1,4);w(0,4);w(689,4);CHECK(apply(0x2a6,900100)==1&&view(900100).phase==WX_CAST_INTERRUPTED);
    start(1,133,3000);CHECK(apply(0x131,UINT32_MAX-999)==1&&view(0).remaining==2000);
    at=0;w(1,4);w(0,4);w(INT32_MAX,4);WxCastState saved=state;CHECK(!apply(0x1e2,0)&&!memcmp(&state,&saved,sizeof state));
    start(1,133,UINT32_MAX);CHECK(!apply(0x131,0));CHECK(wx_cast_apply(&state,0x999,NULL,0,1,0)==-1);
    memset(&state,0,sizeof state);CHECK(!view(0).phase);printf("Cast lifecycle, channels, pushback, cancellation, malformed packets and wrap: %u checks pass\n",checks);return 0;
}
