#include "wx_death.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Death FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
    WxDeath d={0};WxWorldView w={0};w.active=1;w.revision=1;w.guid=5;
    WxEntity self={0},body={0};self.guid=5;self.fields[22]=100;float p[3]={-8800,-177,82};WxGame g={0};
    CHECK(!wx_death_update(&d,&w,&self,NULL,0,p,&g,100)&&!d.state&&!wx_death_action(&d));
    self.fields[22]=0;wx_death_update(&d,&w,&self,NULL,0,p,&g,101);CHECK(d.state==1&&wx_death_action(&d)==0x15a);
    self.fields[190]=16;self.fields[22]=1;g.corpse_delay_revision=1;g.corpse_delay_ms=30000;
    CHECK(wx_death_update(&d,&w,&self,NULL,0,p,&g,102)&&d.query_attempts==1&&d.remaining_ms==30000&&wx_death_action(&d)==0x216);
    CHECK(!wx_death_update(&d,&w,&self,NULL,0,p,&g,5101));CHECK(wx_death_update(&d,&w,&self,NULL,0,p,&g,5102));
    CHECK(wx_death_update(&d,&w,&self,NULL,0,p,&g,10102));CHECK(!wx_death_update(&d,&w,&self,NULL,0,p,&g,15102)&&d.query_attempts==3);
    g.corpse_known=1;memcpy(g.corpse_position,p,12);wx_death_update(&d,&w,&self,NULL,0,p,&g,20000);CHECK(d.known&&!d.ready&&!d.corpse);
    body.type=7;body.guid=99;body.fields[6]=6;memcpy(body.fields+9,p,12);
    wx_death_update(&d,&w,&self,&body,1,p,&g,30102);CHECK(!d.corpse&&!d.ready);
    body.fields[6]=5;wx_death_update(&d,&w,&self,&body,1,p,&g,30102);CHECK(d.corpse==99&&d.ready&&d.in_range&&wx_death_action(&d)==0x1d2);
    float far[3]={p[0]+39.1f,p[1],p[2]};wx_death_update(&d,&w,&self,&body,1,far,&g,31000);CHECK(!d.ready&&wx_death_action(&d)==0);
    far[0]=p[0];far[2]+=40;wx_death_update(&d,&w,&self,&body,1,far,&g,31000);CHECK(!d.in_range); // full 3D range
    body.positioned=1;body.x=p[0];body.y=p[1];body.z=p[2];wx_death_update(&d,&w,&self,&body,1,p,&g,31001);CHECK(d.ready);
    body.x=NAN;wx_death_update(&d,&w,&self,&body,1,p,&g,31002);CHECK(!d.corpse&&!d.ready);
    g.corpse_map=1;wx_death_update(&d,&w,&self,NULL,0,p,&g,31003);CHECK(d.known&&!d.in_range);
    // Reconnect resets old timer state and issues a fresh query, even when known.
    w.revision=2;g.corpse_delay_revision=0;g.corpse_delay_ms=0;CHECK(wx_death_update(&d,&w,&self,NULL,0,p,&g,32000)&&!d.remaining_ms);
    // Millisecond counter rollover must not expire a fresh delay prematurely.
    g.corpse_delay_revision=1;g.corpse_delay_ms=1000;wx_death_update(&d,&w,&self,NULL,0,p,&g,UINT32_MAX-500);CHECK(d.remaining_ms==1000);
    wx_death_update(&d,&w,&self,NULL,0,p,&g,499);CHECK(!d.remaining_ms);
    self.fields[190]=0;self.fields[22]=50;wx_death_update(&d,&w,&self,NULL,0,p,&g,500);CHECK(!d.state&&!d.ready&&d.resurrections==1);
    wx_death_update(&d,&w,&self,NULL,0,p,&g,501);CHECK(d.resurrections==1);
    w.active=0;wx_death_update(&d,&w,NULL,NULL,0,p,&g,502);CHECK(!d.state&&!d.ready);
    printf("Corpse state/range/timer/query checks: %u passed\n",checks);return 0;
}
