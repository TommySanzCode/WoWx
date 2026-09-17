#include "wx_trail.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"trail line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
    WxTrail t;float p[3]={-8803,-177,81.8f};wx_trail_reset(&t,p);
    CHECK(t.count==1&&!t.failed);CHECK(!memcmp(p,t.points[0],sizeof p));
    for(unsigned i=0;i<120;i++)CHECK(wx_trail_record(&t,p,0));CHECK(t.count==1);
    p[0]+=.2f;CHECK(wx_trail_record(&t,p,0));CHECK(t.count==2);
    CHECK(wx_trail_record(&t,p,1));CHECK(t.count==2);CHECK(!memcmp(t.points[1],p,sizeof p));
    p[1]+=.5f;CHECK(wx_trail_record(&t,p,0));CHECK(t.count==3);
    p[2]=NAN;CHECK(!wx_trail_record(&t,p,0));CHECK(t.failed&&t.count==3);
    p[2]=81.8f;CHECK(!wx_trail_record(&t,p,0));wx_trail_reset(&t,p);CHECK(!t.failed&&t.count==1);
    p[0]+=3;CHECK(!wx_trail_record(&t,p,1));CHECK(t.failed&&t.count==1);
    for(unsigned axis=0;axis<3;axis++){
        float q[3]={0,0,0};q[axis]=INFINITY;wx_trail_reset(&t,q);CHECK(t.failed&&t.count==0);
        q[axis]=20001;wx_trail_reset(&t,q);CHECK(t.failed&&t.count==0);
    }
    p[0]=p[1]=p[2]=0;wx_trail_reset(&t,p);
    for(unsigned i=1;i<WX_TRAIL_CAPACITY;i++){p[0]+=.5f;CHECK(wx_trail_record(&t,p,0));}
    CHECK(t.count==WX_TRAIL_CAPACITY);CHECK(wx_trail_record(&t,p,1));
    p[0]+=.5f;CHECK(!wx_trail_record(&t,p,0));CHECK(t.failed&&t.count==WX_TRAIL_CAPACITY);
    CHECK(t.points[WX_TRAIL_CAPACITY-1][0]==(WX_TRAIL_CAPACITY-1)*.5f);
    printf("%u bounded trail checks passed\n",checks);return 0;
}
