#include "wx_fog.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Fog FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void){
    WxFog f,t;wx_fog_fallback(&f,WX_FOG_OUTDOOR);CHECK(sizeof f==28&&wx_fog_valid(&f));CHECK(wx_fog_background(&f)==0xff7196b5);
    CHECK(wx_fog_visibility(&f,0)==1&&wx_fog_visibility(&f,90)==1&&wx_fog_visibility(&f,145)==0&&wx_fog_visibility(&f,160)==0);
    CHECK(fabsf(wx_fog_visibility(&f,117.5f)-.5f)<.00001f);
    float prior=1;for(unsigned i=0;i<1700;i++){float amount=wx_fog_visibility(&f,i*.1f);CHECK(isfinite(amount)&&amount<=prior&&amount>=0);prior=amount;}
    t=f;t.color[0]=NAN;CHECK(!wx_fog_valid(&t));t=f;t.end=t.start;CHECK(!wx_fog_valid(&t));t=f;t.end=161;CHECK(!wx_fog_valid(&t));
    CHECK(wx_fog_visibility(NULL,120)==1&&wx_fog_visibility(&f,INFINITY)==1);
    for(unsigned env=1;env<=2;env++){wx_fog_fallback(&t,env);CHECK(wx_fog_valid(&t)&&t.enabled==(env==2)&&wx_fog_visibility(&t,200)==(env==1));}
    t=f;t.start=70;t.end=120;t.color[0]=.9f;wx_fog_blend(&f,&t,.25f);CHECK(f.start<90&&f.start>70&&f.end<145&&f.end>120&&f.color[0]>.44f);
    WxFog saved=f;wx_fog_blend(&f,&t,NAN);CHECK(!memcmp(&f,&saved,sizeof f));
    for(unsigned i=0;i<100;i++)wx_fog_blend(&f,&t,.1f);CHECK(fabsf(f.end-120)<.001f&&wx_fog_valid(&f));
    wx_fog_fallback(&t,WX_FOG_UNDERWATER);CHECK(t.enabled&&t.start==5&&t.end==60&&wx_fog_valid(&t));
    wx_fog_fallback(&t,WX_FOG_INDOOR);wx_fog_blend(&f,&t,.01f);CHECK(!f.enabled&&f.environment==WX_FOG_INDOOR);
    printf("Fog bounds, distance visibility, color, smoothing and environment isolation: %u checks pass\n",checks);return 0;
}
