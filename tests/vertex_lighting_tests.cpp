extern "C" {
#include "wx_runtime.h"
}
#include "world_vertex_lighting.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <filesystem>
static unsigned checks,gpuBytes,gpuObjects;static int failAfter=-1;
#define CHECK(x) do{checks++;if(!(x))throw std::runtime_error(std::to_string(__LINE__)+": " #x);}while(0)
extern "C" unsigned wx_free_memory(void){return 48u*1024u*1024u-gpuBytes;}
extern "C" void* wx_gpu_alloc(unsigned n){if(failAfter==0)return nullptr;if(failAfter>0)failAfter--;auto p=(unsigned*)malloc(n+4);if(p){*p=n;gpuBytes+=n;gpuObjects++;return p+1;}return nullptr;}
extern "C" void wx_gpu_free(void* p){if(p){auto b=(unsigned*)p-1;gpuBytes-=*b;gpuObjects--;free(b);}}
static void colorTests(){
    CHECK(worldVertexLighting({1,0,0,0},{0,0,0},true,false)==0xff2626ff);
    CHECK(worldVertexLighting({0,0,1,0},{0,0,0},false,false)==0xffff4040);
    CHECK(worldVertexLighting({0,0,0,0},{0,0,0},false,true)==0xffffffff);
    for(unsigned i=0;i<256;i++)for(unsigned indoor=0;indoor<2;indoor++){
        glm::vec4 color(i/255.f,(255-i)/255.f,(i%71)/255.f,0);glm::vec3 ambient(.3f,.2f,.1f);
        auto packed=worldVertexLighting(color,ambient,indoor,false);
        for(unsigned k=0;k<3;k++){float reference=indoor?std::max(color[k],std::max(ambient[k],.15f)):std::max(color[k],.25f);
            CHECK(std::fabs(float((packed>>(8*k))&255)/255.f-reference)<=.5001f/255.f);}
        CHECK((packed>>24)==255); // MOCV alpha never punches holes in a wall.
    }
    for(float bad:{NAN,INFINITY,-.1f,1.1f}){bool rejected=false;try{worldVertexLighting({bad,0,0,1},{0,0,0},true,false);}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);}
}
static void chunkTests(){
    std::vector<uint8_t> b(8+68+8+12);auto put=[&](size_t at,uint32_t x){memcpy(b.data()+at,&x,4);};
    put(0,0x4d4f4750);put(4,(unsigned)b.size()-8);put(76,0x4d4f4356);put(80,12);
    CHECK(validateWmoVertexColors(b,3));
    for(unsigned bad=0;bad<5;bad++){
        auto x=b;if(bad==0)x.pop_back();if(bad==1){uint32_t huge=0xffffffff;memcpy(x.data()+80,&huge,4);}if(bad==2)x.resize(7);
        if(bad==3)x[0]=0;if(bad==4){uint32_t n=8;memcpy(x.data()+80,&n,4);}
        bool rejected=false;try{validateWmoVertexColors(x,3);}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
    }
    bool rejected=false;try{validateWmoVertexColors(b,4);}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
    b.resize(76);put(4,68);CHECK(!validateWmoVertexColors(b,0));
}
static std::vector<uint8_t> fixture(unsigned count=3,bool motion=false){
    WxPackHeader h={{'W','X','P','1'},8,1,64,{0},0};WxEntry e{};
    e.kind=WX_KIND_STATIC;e.flags=WX_VERTEX_COLOR|WX_BAKED_LIGHT|WX_TEX_SWIZZLED|(motion?WX_MATERIAL_MOTION:0);
    e.vertex_count=count;e.index_count=3;e.width=e.height=2;e.radius=2;
    std::vector<uint8_t> b(96);auto append=[&](const void* p,size_t size){auto at=(unsigned)b.size();b.insert(b.end(),(const uint8_t*)p,(const uint8_t*)p+size);return at;};
    std::vector<uint32_t> colors(count);for(unsigned i=0;i<count;i++)colors[i]=0xff000000u|i;append(colors.data(),count*4);
    if(motion){WxMaterialMotion m{};m.magic=WX_MOTION_MAGIC;m.color[0]=.5f;m.color[1]=m.color[2]=m.color[3]=m.weight=1;append(&m,sizeof m);}
    std::vector<WxVertex> v(count);for(auto& x:v)x.n[2]=1;v[1].p[0]=v[2].p[1]=1;e.vertex_offset=append(v.data(),count*sizeof(WxVertex));
    uint16_t indices[3]={0,1,2};e.index_offset=append(indices,sizeof indices);uint32_t pixels[4]={~0u,~0u,~0u,~0u};e.texture_offset=append(pixels,sizeof pixels);
    h.file_size=(unsigned)b.size();memcpy(b.data(),&h,32);memcpy(b.data()+32,&e,64);return b;
}
static void write(const std::vector<uint8_t>& b){FILE* f=fopen("vertex-lighting-fixture.wxp","wb");CHECK(f);CHECK(fwrite(b.data(),1,b.size(),f)==b.size());CHECK(!fclose(f));}
static void runtimeTests(){
    WxScene s{};float p[3]={0};
    for(unsigned trial=0;trial<9;trial++){
        auto b=fixture(3,true);WxPackHeader* h=(WxPackHeader*)b.data();WxEntry* e=(WxEntry*)(b.data()+32);
        if(trial==1)h->version=7;if(trial==2)e->vertex_offset=96+sizeof(WxMaterialMotion)+8;
        if(trial==3)e->kind=WX_KIND_COLLISION;if(trial==4)e->flags|=WX_ANIMATED;
        if(trial==5)failAfter=1;if(trial==6)failAfter=2;
        write(b);bool opened=wx_pack_open(&s,"vertex-lighting-fixture.wxp");
        if(trial>=1&&trial<=4)CHECK(!opened);else{
            CHECK(opened);if(trial==7)s.budget_bytes=3*32+6+16+sizeof(WxMaterialMotion); // Excludes only colours.
            if(trial==8)std::filesystem::resize_file("vertex-lighting-fixture.wxp",96+4);
            wx_stream(&s,p);
            if(trial)CHECK(s.failures==1&&s.loads==0&&s.bytes==0);
            else{CHECK(s.loads==1&&s.slots[0].vertex_colors&&s.slots[0].vertex_colors[2]==0xff000002);
                CHECK(s.slots[0].material_motion&&s.slots[0].material_motion->color[0]==.5f);
                CHECK(s.bytes==3*36+6+16+sizeof(WxMaterialMotion));}
        }
        wx_pack_close(&s);CHECK(!gpuBytes&&!gpuObjects&&!s.bytes);failAfter=-1;
    }
    for(unsigned cancel=0;cancel<2;cancel++){
        write(fixture(20000));CHECK(wx_pack_open(&s,"vertex-lighting-fixture.wxp"));bool sawColorRead=false;
        for(unsigned frame=0;frame<100&&!s.loads;frame++){
            wx_stream(&s,p);CHECK(s.streaming.read_bytes<=65536&&s.streaming.read_ops<=8&&!s.failures);
            if(s.stream_job.phase==14){sawColorRead=true;CHECK(s.slots[0].entry==-1&&s.streaming.pending_bytes>0);if(cancel)break;}
        }
        CHECK(sawColorRead);if(!cancel){CHECK(s.loads==1&&s.slots[0].vertex_colors[19999]==(0xff000000u|19999));CHECK(s.bytes==20000*36+6+16);}
        wx_pack_close(&s);CHECK(!gpuBytes&&!gpuObjects&&!s.bytes);
    }
    std::remove("vertex-lighting-fixture.wxp");
}
int main(){try{colorTests();chunkTests();runtimeTests();std::cout<<checks<<" vertex lighting, colour-chunk, staged publication and rollback checks passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
