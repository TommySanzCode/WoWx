#include "wx_material.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static float factor(unsigned f,float source,float dest,float alpha){
    const float v[]={0,1,alpha,1-alpha,dest,source};return v[f];
}
int main(void){
    // Independently known framebuffer results for source .4, destination .6,
    // alpha .25: regular alpha, additive, alpha-add, modulate, double-modulate.
    const float expected[]={0,0,.55f,1,.7f,.24f,.48f};
    for(unsigned mode=0;mode<7;mode++){
        unsigned flags=mode==1?WX_ALPHA_TEST:mode<<WX_BLEND_SHIFT;WxMaterial m=wx_material(flags);
        CHECK(m.blend==mode&&m.depth_write==(mode<2)&&m.depth_test);
        CHECK(wx_material_valid(flags,6,WX_KIND_STATIC));
        if(mode>=2){float out=.4f*factor(m.src,.4f,.6f,.25f)+.6f*factor(m.dst,.4f,.6f,.25f);CHECK(fabsf(out-expected[mode])<.00001f);}
        if(mode>=3){float f=m.fog_neutral;float out=f*factor(m.src,f,.6f,.25f)+.6f*factor(m.dst,f,.6f,.25f);CHECK(fabsf(out-.6f)<.00001f);}
    }
    CHECK(wx_material(WX_ALPHA_TEST).blend==1);CHECK(wx_material(0).blend==0);
    CHECK(!wx_material_valid(7<<WX_BLEND_SHIFT,6,2));CHECK(!wx_material_valid(1<<WX_BLEND_SHIFT,6,2));
    CHECK(!wx_material_valid(WX_ALPHA_TEST|(2<<WX_BLEND_SHIFT),6,2));
    for(unsigned f=1<<10;f<=1<<13;f<<=1){CHECK(!wx_material_valid(f,5,2));CHECK(!wx_material_valid(f,4,2));CHECK(!wx_material_valid(f,6,4));CHECK(wx_material_valid(f,6,2));}
    WxMaterial special=wx_material(WX_UNLIT|WX_UNFOGGED|WX_NO_DEPTH_TEST|WX_NO_DEPTH_WRITE);
    CHECK(special.unlit&&special.unfogged&&!special.depth_test&&!special.depth_write);
    WxDrawOrder order[WX_CACHE_SLOTS+1];unsigned n=0;
    n=wx_material_order(order,n,7,3,2<<WX_BLEND_SHIFT);n=wx_material_order(order,n,8,9,2<<WX_BLEND_SHIFT);
    n=wx_material_order(order,n,9,9,2<<WX_BLEND_SHIFT);n=wx_material_order(order,n,10,1,0);n=wx_material_order(order,n,11,2,WX_ALPHA_TEST);
    unsigned ids[]={10,11,8,9,7};for(unsigned i=0;i<5;i++)CHECK(order[i].slot==ids[i]);
    while(n<WX_CACHE_SLOTS)n=wx_material_order(order,n,n,(float)n,2<<WX_BLEND_SHIFT);
    order[WX_CACHE_SLOTS].slot=65535;CHECK(wx_material_order(order,n,0,0,0)==WX_CACHE_SLOTS);CHECK(order[WX_CACHE_SLOTS].slot==65535);
    for(unsigned i=3;i<n;i++)CHECK(order[i-1].depth>=order[i].depth);
    printf("World material blending, fog identities and bounded ordering: %u checks passed\n",checks);return 0;
}
