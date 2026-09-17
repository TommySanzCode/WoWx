#include "wx_map.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
static unsigned checks,allocated,calls,fail_at,available=40u*1024*1024;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Map FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return available;}
void* wx_gpu_alloc(unsigned size){if(++calls==fail_at)return NULL;unsigned* p=malloc(size+4);if(!p)return NULL;*p=size;allocated+=size;return p+1;}
void wx_gpu_free(void* p){if(p){unsigned* q=(unsigned*)p-1;allocated-=*q;free(q);}}
static void fixture(const WxMapEntry* e){
    FILE* f=fopen("map-test-MAPS.WMI","wb");CHECK(f);uint32_t head[]={WX_MAP_INDEX_MAGIC,1,1,sizeof *e};CHECK(fwrite(head,1,16,f)==16);CHECK(fwrite(e,1,sizeof *e,f)==sizeof *e);fclose(f);
    WxMapPatch p={{31,32,2047,UINT_MAX},10,20,2,1,16+sizeof(WxMapPatch)+WX_MAP_BYTES};
    uint32_t h[]={WX_MAP_MAGIC,30,1,p.offset+8},row[512];for(unsigned i=0;i<512;i++)row[i]=0xff204060;
    f=fopen("map-test-Z0000030.WMP","wb");CHECK(f);CHECK(fwrite(h,1,16,f)==16);CHECK(fwrite(&p,1,sizeof p,f)==sizeof p);
    for(unsigned y=0;y<512;y++)CHECK(fwrite(row,4,512,f)==512);
    uint32_t patch[]={0x80ff0000,0x0000ff00};CHECK(fwrite(patch,4,2,f)==2);fclose(f);
}
static WxPad pad(unsigned bits){WxPad p={0};p.connected=1;p.buttons=p.pressed=bits;p.action=p.slot=-1;return p;}
int main(int argc,char** argv){
    static WxMap m;uint32_t explored[64]={0};
    if(argc==2){CHECK(wx_map_open(&m,argv[1]));unsigned count=m.count;
        for(unsigned i=0;i<count;i++){
            m.open=1;m.selected=i;memset(explored,0,sizeof explored);wx_map_update(&m,explored);CHECK(m.ready&&!m.failures);
            memset(explored,255,sizeof explored);wx_map_update(&m,explored);CHECK(m.ready&&!m.failures);
            CHECK(m.bytes==WX_MAP_BYTES+96&&allocated==m.bytes);
        }m.open=0;wx_map_update(&m,NULL);CHECK(!allocated);printf("Decoded native map files: %u maps / %u checks\n",count,checks);return 0;
    }
    WxMapEntry e={30,0,12,1535.42f,-1935.42f,-7939.58f,-10254.2f,"Elwynn Forest"};fixture(&e);
    CHECK(wx_map_open(&m,"map-test-")&&m.count==1);float u,v;CHECK(wx_map_project(&e,0,-9378.81f,-77.882f,&u,&v));CHECK(fabsf(u-.4648f)<.001f&&fabsf(v-.6218f)<.001f);
    CHECK(!wx_map_project(&e,1,-9378,-77,&u,&v));CHECK(!wx_map_project(&e,0,NAN,0,&u,&v));CHECK(!wx_map_project(&e,0,0,0,&u,&v));
    CHECK(wx_map_find(&m,0,-9378,-77,0)==0&&wx_map_find(&m,0,-9378,-77,1)==-1);
    WxPad p=pad(1u<<WX_BACK);p.layer=1;CHECK(!wx_map_input(&m,&p,1,0,-9378,-77,.033f));
    p=pad(1u<<WX_BACK);CHECK(wx_map_input(&m,&p,1,0,-9378,-77,.033f)&&m.open&&!p.buttons&&m.u==.5f&&m.marker);
    wx_map_body(&m,1,0,-8800,-177);CHECK(m.body&&m.body_u>0&&m.body_u<1&&m.body_v>0&&m.body_v<1);
    wx_map_body(&m,1,1,-8800,-177);CHECK(!m.body);wx_map_body(&m,0,0,-8800,-177);CHECK(!m.body);
    wx_map_update(&m,explored);CHECK(m.ready&&m.loads==1&&m.overlays==0&&allocated==WX_MAP_BYTES+96);CHECK(m.pixels[wx_map_morton(10,20)]==0xff204060);
    wx_map_update(&m,explored);CHECK(m.loads==1);explored[0]=1u<<31;wx_map_update(&m,explored);CHECK(m.loads==2&&m.overlays==1);
    CHECK(m.pixels[wx_map_morton(10,20)]==0xff902030);CHECK(m.pixels[wx_map_morton(11,20)]==0xff204060);
    WxMapPatch patch={{UINT_MAX,32,2047,UINT_MAX},0};CHECK(!wx_map_revealed(&patch,explored));explored[1]=1;CHECK(wx_map_revealed(&patch,explored));explored[1]=0;explored[63]=0x80000000;CHECK(wx_map_revealed(&patch,explored));
    p=pad(1u<<WX_A);p.move_x=1;p.look_y=.9f;p.action=7;p.layer=3;CHECK(wx_map_input(&m,&p,1,0,-9378,-77,.05f)&&m.zoom==2&&!p.move_x&&!p.look_y&&p.action==-1&&!p.layer);
    for(unsigned i=0;i<100;i++){p=pad(0);p.move_x=1;p.move_y=-1;wx_map_input(&m,&p,1,0,-9378,-77,.05f);}CHECK(m.u==.75f&&m.v==.75f);
    p=pad(1u<<WX_X);wx_map_input(&m,&p,1,0,-9378,-77,.033f);CHECK(fabsf(m.u-.4648f)<.001f);
    p=pad(1u<<WX_B);p.move_y=1;wx_map_input(&m,&p,1,0,-9378,-77,.033f);CHECK(!m.open&&m.latched&&!p.move_y);
    p=pad(1u<<WX_X);p.action=2;wx_map_input(&m,&p,1,0,-9378,-77,.033f);CHECK(m.latched&&p.action==-1&&!p.pressed);
    p=pad(0);wx_map_input(&m,&p,1,0,-9378,-77,.033f);CHECK(!m.latched);wx_map_update(&m,NULL);CHECK(!allocated&&!m.ready&&!m.bytes);
    // Both allocation failure points roll back; low memory never allocates.
    for(unsigned fault=1;fault<=2;fault++){m.open=1;fail_at=calls+fault;wx_map_update(&m,explored);CHECK(!m.ready&&!allocated);m.open=0;wx_map_update(&m,NULL);}
    fail_at=0;available=8u*1024*1024;m.open=1;wx_map_update(&m,explored);CHECK(!m.ready&&!allocated);m.open=0;wx_map_update(&m,NULL);available=40u*1024*1024;
    // Truncated data and impossible patch bounds are rejected before display.
    FILE* f=fopen("map-test-Z0000030.WMP","wb");CHECK(f);fwrite("WX",1,2,f);fclose(f);m.open=1;wx_map_update(&m,explored);CHECK(!m.ready);m.open=0;wx_map_update(&m,NULL);CHECK(!allocated);
    fixture(&e);f=fopen("map-test-Z0000030.WMP","rb+");CHECK(f);fseek(f,16+16,SEEK_SET);uint32_t bad=UINT_MAX;fwrite(&bad,4,1,f);fclose(f);m.open=1;wx_map_update(&m,explored);CHECK(!m.ready);m.open=0;wx_map_update(&m,NULL);
    f=fopen("map-test-MAPS.WMI","ab");CHECK(f);fputc(0,f);fclose(f);CHECK(!wx_map_open(&m,"map-test-"));
    remove("map-test-MAPS.WMI");remove("map-test-Z0000030.WMP");CHECK(!allocated);printf("Map boundary/input/allocation checks: %u passed\n",checks);return 0;
}
