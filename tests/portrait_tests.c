#include "wx_portrait.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Portrait FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static WxPortraitCatalog catalog;
static void save(const void* bytes,unsigned size){FILE* f=fopen("portrait-test.tmp","wb");CHECK(f);CHECK(fwrite(bytes,1,size,f)==size);CHECK(!fclose(f));}
static void camera(const WxPortraitEntry* e){
    WxPortraitView v;CHECK(wx_portrait_view(e,&v));
    const float* axes[]={v.right,v.up,v.forward};
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++){
        float dot=0;for(unsigned k=0;k<3;k++)dot+=axes[i][k]*axes[j][k];CHECK(fabsf(dot-(i==j))<.001f);
    }
    for(unsigned j=0;j<2;j++){float dot=0;for(unsigned k=0;k<3;k++)dot+=(e->target[k]-e->camera[k])*axes[j][k];CHECK(fabsf(dot)<.001f);}
    CHECK(v.focal>0&&isfinite(v.focal)&&v.camera[3]==1);
}
int main(int argc,char** argv){
    _Static_assert(sizeof(WxPortraitEntry)==44,"Portrait catalog ABI");
    WxPortraitEntry e={328,1,{2,0,1},{0,0,1},.785398163f,.01f,1000};camera(&e);
    e.camera[0]=0;e.camera[2]=3;camera(&e);e.camera[0]=2;e.camera[2]=1;
    WxPortraitEntry bad=e;bad.fov=NAN;CHECK(!wx_portrait_valid(&bad));bad=e;bad.camera[0]=INFINITY;CHECK(!wx_portrait_valid(&bad));
    bad=e;bad.camera[0]=0;CHECK(!wx_portrait_valid(&bad));bad=e;bad.near_clip=bad.far_clip;CHECK(!wx_portrait_valid(&bad));
    bad=e;bad.key=WX_PORTRAIT_PLAYER|18;CHECK(!wx_portrait_valid(&bad));bad.key=WX_PORTRAIT_PLAYER|2;CHECK(wx_portrait_valid(&bad));
    uint8_t bytes[16+88];uint32_t h[]={0x54505857,1,2,44};memcpy(bytes,h,16);memcpy(bytes+16,&e,44);bad=e;bad.key=WX_PORTRAIT_PLAYER|2;memcpy(bytes+60,&bad,44);
    save(bytes,sizeof bytes);CHECK(wx_portrait_open(&catalog,"portrait-test.tmp"));CHECK(catalog.count==2&&wx_portrait_find(&catalog,e.key));CHECK(!wx_portrait_find(&catalog,300));
    for(unsigned n=0;n<sizeof bytes;n++){save(bytes,n);CHECK(!wx_portrait_open(&catalog,"portrait-test.tmp"));CHECK(!catalog.ready&&!catalog.count&&!wx_portrait_find(&catalog,e.key));}
    memcpy(bytes+60,&e,44);save(bytes,sizeof bytes);CHECK(!wx_portrait_open(&catalog,"portrait-test.tmp"));
    h[2]=WX_PORTRAIT_LIMIT+1;memcpy(bytes,h,16);save(bytes,sizeof bytes);CHECK(!wx_portrait_open(&catalog,"portrait-test.tmp"));CHECK(!remove("portrait-test.tmp"));
    WxPortraitSpan spans[64];unsigned n=wx_portrait_spans(spans,50,32);CHECK(n&&n<=64);unsigned char pixels[64][64]={{0}};
    for(unsigned i=0;i<n;i++){
        const WxPortraitSpan* r=&spans[i];CHECK(r->x>=50&&r->x+r->width<=114&&r->y>=32&&r->y+r->height<=96&&r->width&&r->height);
        for(unsigned y=r->y;y<r->y+r->height;y++)for(unsigned x=r->x;x<r->x+r->width;x++)CHECK(++pixels[y-32][x-50]==1);
    }
    for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){int dx=2*(int)x-63,dy=2*(int)y-63;CHECK(pixels[y][x]==(dx*dx+dy*dy<=4096));}
    CHECK(!wx_portrait_spans(spans,577,32)&&!wx_portrait_spans(spans,50,417));CHECK(wx_portrait_spans(spans,576,416)==n);
    if(argc==2){CHECK(wx_portrait_open(&catalog,argv[1]));unsigned players=0,sources[4]={0};
        for(unsigned i=0;i<catalog.count;i++){const WxPortraitEntry* row=&catalog.entries[i];camera(row);sources[row->source]++;players+=(row->key&WX_PORTRAIT_PLAYER)!=0;}
        CHECK(players==16);printf("Real source catalog: %u entries, %u players, sources %u/%u/%u\n",catalog.count,players,sources[1],sources[2],sources[3]);
    }
    printf("Portrait cameras, malformed catalog, exact circular mask: %u checks pass; %u spans, %u fixed catalog bytes, no allocations\n",checks,n,(unsigned)sizeof catalog);return 0;
}
