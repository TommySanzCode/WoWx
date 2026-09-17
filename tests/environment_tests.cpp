extern "C" {
#include "wx_runtime.h"
}
#include "world_environment.hpp"
#include "terrain_liquid.hpp"
#include "world_liquid_mesh.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
static unsigned checks,gpuBytes,freeBytes=48u*1024u*1024u;
#define CHECK(x) do{checks++;if(!(x))throw std::runtime_error(std::to_string(__LINE__)+": " #x);}while(0)
extern "C" unsigned wx_free_memory(void){return freeBytes-gpuBytes;}
extern "C" void* wx_gpu_alloc(unsigned n){auto p=(unsigned*)malloc(n+4);if(p){*p=n;gpuBytes+=n;return p+1;}return nullptr;}
extern "C" void wx_gpu_free(void* p){if(p){auto b=(unsigned*)p-1;gpuBytes-=*b;free(b);}}
static WorldEnvironment::Item volume(bool liquid=false,unsigned side=2){
    WorldEnvironment::Item item;auto& r=item.record;r.kind=liquid?2:1;r.flags=0x2000;r.group=100;
    r.inverse[0]=r.inverse[5]=r.inverse[10]=1;
    for(unsigned k=0;k<3;k++){r.lo[k]=-100;r.hi[k]=400;}
    if(liquid){r.x_tiles=r.y_tiles=side;r.tile_size=4.1666625f;r.liquid_type=1;
        item.heights.assign((side+1)*(side+1),10);item.flags.assign(side*side,0);}
    return item;
}
static std::vector<uint8_t> pack(const WorldEnvironment& env){
    WxPackHeader h={{'W','X','P','1'},9,1,64,{0},0};WxEntry e{};
    e.kind=2;e.vertex_count=e.index_count=3;e.vertex_offset=96;e.index_offset=192;e.texture_offset=198;e.width=e.height=1;e.radius=2;
    std::vector<uint8_t> out(202);memcpy(out.data()+32,&e,64);WxVertex v[3]={};v[1].p[0]=1;v[2].p[1]=1;
    memcpy(out.data()+96,v,96);uint16_t indices[3]={0,1,2};memcpy(out.data()+192,indices,6);
    auto blob=env.encode();WxEnvironmentFooter footer={{'W','X','E','1'},(unsigned)env.items.size(),(unsigned)blob.size(),202};
    out.insert(out.end(),blob.begin(),blob.end());auto f=(const uint8_t*)&footer;out.insert(out.end(),f,f+16);
    h.file_size=(unsigned)out.size();memcpy(out.data(),&h,32);return out;
}
static void write(const std::vector<uint8_t>& b){FILE* f=fopen("environment-fixture.wxp","wb");CHECK(f);CHECK(fwrite(b.data(),1,b.size(),f)==b.size());fclose(f);}
static void pump(WxScene& scene){float p[3]={0};for(unsigned frame=0;frame<400&&!wx_stream_ready(&scene)&&!scene.failures;frame++){
    wx_stream_frame_begin(3);wx_stream(&scene,p);wx_stream_frame_end();const auto* budget=wx_stream_frame_metrics();
    CHECK(budget->total.read_bytes<=65536&&budget->total.read_ops<=16&&budget->total.scan_bytes<=65536&&budget->total.allocations<=3);
    CHECK(scene.streaming.read_bytes<=65536&&scene.streaming.read_ops<=8&&scene.streaming.scan_bytes<=65536);
}}
static WxEnvironmentSample sample(WxScene& scene,float x,float y,float z){float p[3]={x,y,z};WxEnvironmentSample result;wx_environment_sample(&scene,p,&result);return result;}
static void sampling(){
    WorldEnvironment env;env.items.push_back(volume());auto water=volume(true);water.flags[1]=15;
    // A tilted original surface: triangulated height must remain distinct from a
    // flat waterline. The hidden tile must remain dry.
    water.heights={10,12,14,12,14,16,14,16,18};env.items.push_back(water);write(pack(env));
    WxScene scene;CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));CHECK(!sample(scene,1,1,0).known);pump(scene);
    CHECK(!scene.failures&&wx_stream_ready(&scene));auto s=sample(scene,1,1,0);CHECK(s.known&&s.environment==2&&s.liquid_type==1&&s.depth>10.9f&&s.depth<11);
    CHECK(sample(scene,1,1,30).environment==1);CHECK(sample(scene,5,1,0).environment==1);CHECK(sample(scene,401,1,0).environment==0);
    CHECK(!sample(scene,NAN,0,0).known);CHECK(scene.bytes>scene.sources[0].environment.bytes);
    auto at=sample(scene,4.1666625f*.5f,4.1666625f*.5f,12);CHECK(at.environment==1); // Exactly at surface.
    wx_pack_detach(&scene,0);CHECK(!sample(scene,1,1,0).known&&!scene.bytes&&!gpuBytes);wx_pack_close(&scene);
    // Four region sources retain independent metadata, including detach/reload.
    write(pack(env));CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));
    for(unsigned i=1;i<4;i++)CHECK(wx_pack_attach(&scene,"environment-fixture.wxp")==int(i));
    pump(scene);CHECK(sample(scene,1,1,0).known&&sample(scene,1,1,0).records==8);
    wx_pack_detach(&scene,1);CHECK(sample(scene,1,1,0).records==6);
    CHECK(wx_pack_attach(&scene,"environment-fixture.wxp")==1);CHECK(!sample(scene,1,1,0).known);pump(scene);
    CHECK(sample(scene,1,1,0).records==8);wx_pack_close(&scene);CHECK(!scene.bytes&&!gpuBytes);
    // A nonplanar cell follows the server's two triangles, not a bilinear patch.
    auto nonplanar=env;nonplanar.items[1].heights={0,0,0,0,8,0,0,0,0};write(pack(nonplanar));
    CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));pump(scene);
    CHECK(fabsf(sample(scene,4.1666625f*.75f,4.1666625f*.25f,1).depth-1)<.001f);
    CHECK(fabsf(sample(scene,4.1666625f*.25f,4.1666625f*.75f,1).depth-1)<.001f);wx_pack_close(&scene);
    // Rotation and translation are evaluated in local coordinates, without a
    // guessed world-axis box. Two overlapping interiors choose the smaller one.
    env.items.clear();auto rotated=volume();auto& r=rotated.record;
    r.inverse[0]=r.inverse[5]=0;r.inverse[1]=1;r.inverse[4]=-1;r.inverse[3]=-20;r.inverse[7]=10;
    for(unsigned k=0;k<3;k++){r.lo[k]=-2;r.hi[k]=2;}r.group=200;
    env.items={volume(),rotated};write(pack(env));CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));pump(scene);
    CHECK(sample(scene,10,20,0).group==200);CHECK(sample(scene,13,20,0).group==100);wx_pack_close(&scene);
    for(unsigned type:{3u,4u,21u,0u,99u}){env.items={volume(),volume(true)};env.items[1].record.liquid_type=type;write(pack(env));
        CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));pump(scene);s=sample(scene,1,1,0);
        CHECK(s.environment==1&&s.liquid_type==type);wx_pack_close(&scene);}
    env.items.clear();write(pack(env));CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));pump(scene);CHECK(sample(scene,0,0,0).known);wx_pack_close(&scene);
    auto old=pack(env);old.resize(202);((WxPackHeader*)old.data())->version=8;((WxPackHeader*)old.data())->file_size=202;write(old);
    CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));pump(scene);CHECK(!sample(scene,0,0,0).known&&wx_stream_ready(&scene));wx_pack_close(&scene);
}
static unsigned clockCalls;
static unsigned expiryClock(){return clockCalls++?6:0;}
static void staging(){
    // Time may expire between checking room for a fixed footer and reserving
    // the read. Deferral must neither consume a partial header nor report I/O.
    WorldEnvironment small;small.items.push_back(volume(true));write(pack(small));
    WxScene timed;CHECK(wx_pack_open(&timed,"environment-fixture.wxp"));clockCalls=0;
    wx_stream_set_clock(expiryClock);wx_stream_frame_begin(15);float origin[3]={0};wx_stream(&timed,origin);
    CHECK(!timed.failures&&!timed.sources[0].environment.phase&&!timed.bytes);
    CHECK(!wx_stream_frame_metrics()->total.read_bytes&&wx_stream_time_metrics()->yield_mask==2);
    wx_stream_frame_end();wx_stream_set_clock(nullptr);pump(timed);
    CHECK(wx_stream_ready(&timed)&&sample(timed,1,1,0).environment==2);wx_pack_close(&timed);CHECK(!gpuBytes);
    WorldEnvironment env;for(unsigned i=0;i<70;i++)env.items.push_back(volume(true,64));auto b=pack(env);
    for(unsigned trial=0;trial<4;trial++){
        write(b);WxScene scene;CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));float p[3]={0};
        if(trial==2)scene.budget_bytes=1024;if(trial==3)freeBytes=8u*1024u*1024u;
        if(trial<2){for(unsigned i=0;i<3;i++){wx_stream_frame_begin(3);wx_stream(&scene,p);wx_stream_frame_end();}
            CHECK(!sample(scene,1,1,0).known&&!wx_stream_ready(&scene));CHECK(scene.sources[0].environment.bytes>0);
            if(trial==0){pump(scene);CHECK(!scene.failures&&sample(scene,1,1,0).known&&sample(scene,1,1,0).records==70);}
        }else{pump(scene);CHECK(scene.failures==1&&!sample(scene,1,1,0).known&&!scene.sources[0].environment.bytes);}
        freeBytes=48u*1024u*1024u;wx_pack_close(&scene);CHECK(!scene.bytes&&!gpuBytes);
    }
    // Truncation after the index was opened must fail the staged read without
    // publication, even when the recorded header size had been valid.
    write(b);WxScene scene;CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));FILE* f=fopen("environment-fixture.wxp","wb");CHECK(f);fclose(f);pump(scene);
    CHECK(scene.failures==1&&!sample(scene,0,0,0).known&&!scene.bytes);wx_pack_close(&scene);
}
static void malformed(){
    WorldEnvironment env;env.items={volume(),volume(true)};auto base=pack(env);
    for(unsigned trial=0;trial<16;trial++){
        auto b=base;auto f=(WxEnvironmentFooter*)(b.data()+b.size()-16);auto r=(WxEnvironmentRecord*)(b.data()+202);
        switch(trial){case 0:f->magic[0]=0;break;case 1:f->count=UINT32_MAX;break;case 2:f->bytes++;break;case 3:f->offset=96;break;
        case 4:r->kind=3;break;case 5:r->inverse[0]=0;break;case 6:r->lo[0]=NAN;break;case 7:r->hi[0]=-101;break;
        case 8:r->reserved[0]=1;break;case 9:r[1].heights_offset=0;break;case 10:r[1].x_tiles=257;break;case 11:r[1].flags_offset=UINT32_MAX;break;
        case 12:r[1].tile_size=INFINITY;break;case 13:*(float*)(b.data()+202+r[1].heights_offset)=NAN;break;
        case 14:((WxEntry*)(b.data()+32))->texture_offset=f->offset;break;case 15:f->count=0;break;}
        write(b);WxScene scene;CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));CHECK(!wx_pack_verify(&scene));
        CHECK(scene.failures==1&&!sample(scene,0,0,0).known);wx_pack_close(&scene);CHECK(!scene.bytes&&!gpuBytes);
    }
}
static void sourceAdapter(){
    CHECK(WorldEnvironment::liquidEntry(0,1,0,0,{})==1);CHECK(WorldEnvironment::liquidEntry(0,1,0x80000,0,{})==2);
    CHECK(WorldEnvironment::liquidEntry(0,1,0,15,{15,2})==3);CHECK(WorldEnvironment::liquidEntry(4,4489,0,4,{})==21);
    for(unsigned family=0;family<4;family++)for(unsigned variation=0;variation<5;variation++)
        CHECK(WorldEnvironment::liquidEntry(0,1,0,family+variation*4,{(uint8_t)family})==family+1);
    using namespace wowee::pipeline;WMOGroup g{};g.boundingBoxMin={-5,-5,-5};g.boundingBoxMax={5,5,5};g.flags=0x2000;
    g.liquid.xVerts=g.liquid.yVerts=2;g.liquid.xTiles=g.liquid.yTiles=1;g.liquid.heights={1,2,3,4};g.liquid.flags={0};
    std::vector<uint8_t> b(8+68+8+30+32+1);auto put=[&](unsigned at,unsigned value){memcpy(b.data()+at,&value,4);};
    put(0,0x4d4f4750);put(4,(unsigned)b.size()-8);put(76,0x4d4c4951);put(80,63);
    put(84,2);put(88,2);put(92,1);put(96,1);for(unsigned i=0;i<4;i++)memcpy(b.data()+114+i*8+4,&g.liquid.heights[i],4);
    WorldEnvironment env;env.add(g,b,glm::mat4(1),0,1);CHECK(env.items.size()==2&&env.items[1].heights==g.liquid.heights);CHECK(!env.encode().empty());
    for(unsigned trial=0;trial<5;trial++){auto raw=b;if(trial==0)raw.pop_back();if(trial==1)raw[84]=3;if(trial==2)raw[80]=62;if(trial==3)raw[0]=0;if(trial==4)raw[114+4]=1;
        bool failed=false;try{WorldEnvironment e;e.add(g,raw,glm::mat4(1),0,1);}catch(const std::exception&){failed=true;}CHECK(failed);}
}
static void terrainAdapter(){
    wowee::pipeline::ADTTerrain adt;adt.loaded=true;adt.version=18;adt.coord={32,48};
    std::vector<uint8_t> raw;
    auto put=[&](unsigned at,unsigned value){memcpy(raw.data()+at,&value,4);};
    auto real=[&](unsigned at,float value){memcpy(raw.data()+at,&value,4);};
    for(unsigned id=0;id<256;id++){
        auto& c=adt.chunks[id];c.indexX=id%16;c.indexY=id/16;c.flags=id?0:5;
        unsigned at=(unsigned)raw.size(),size=128+(id?0:812);raw.resize(at+8+size);
        put(at,0x4d434e4b);put(at+4,size);put(at+8,c.flags);put(at+12,c.indexX);put(at+16,c.indexY);
        if(!id){put(at+104,136);put(at+108,812);put(at+136,0x4d434c51);
            real(144,10);real(148,20);
            for(unsigned y=0;y<9;y++)for(unsigned x=0;x<9;x++)real(156+(y*9+x)*8,10+x*.5f+y*.25f);
            memset(raw.data()+800,4,64);raw[800]=0x84;raw[801]=0x8f;raw[864]=1;raw[947]=2;}
    }
    WorldEnvironment env;auto stats=terrainLiquidEnvironment(raw,adt,env);
    CHECK(stats.chunks==256&&stats.grids==1&&stats.visibleCells==63&&stats.deepCells==1&&stats.flowBytes==2);
    CHECK(env.items.size()==1&&env.items[0].record.group==0x80000000u&&env.items[0].flags[0]==0x84);
    CHECK(env.encode().size()==516);auto patches=worldLiquidMesh(env.items[0]);CHECK(patches.size()==1&&patches[0].indices.size()==63*6);
    const auto& p=patches[0];for(unsigned i=0;i<p.indices.size();i+=3){
        const auto *a=p.vertices[p.indices[i]].p,*b=p.vertices[p.indices[i+1]].p,*c=p.vertices[p.indices[i+2]].p;
        CHECK((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])>0);
    }
    for(const auto& v:p.vertices){float x=-v.p[1]/4.1666625f,y=(-16*wowee::core::coords::TILE_SIZE-v.p[0])/4.1666625f;
        CHECK(fabsf(v.p[2]-(10+x*.5f+y*.25f))<.001f);CHECK(v.n[2]>0);}
    write(pack(env));WxScene scene;CHECK(wx_pack_open(&scene,"environment-fixture.wxp"));pump(scene);
    float wx=-16*wowee::core::coords::TILE_SIZE,step=4.1666625f;
    auto s=sample(scene,wx-.5f*step,-.5f*step,10);CHECK(s.known&&s.environment==2&&s.liquid_type==1&&fabsf(s.depth-.375f)<.001f);
    CHECK(sample(scene,wx-.5f*step,-1.5f*step,0).environment==0); // dry 0x8f
    CHECK(sample(scene,wx-.5f*step,-.5f*step,12).environment==0);
    wx_pack_close(&scene);CHECK(!gpuBytes);
    auto good=raw;
    for(unsigned n:{0u,1u,135u,143u,799u,863u,947u,(unsigned)raw.size()-1}){
        auto shortRaw=raw;shortRaw.resize(n);WorldEnvironment target;target.items.push_back(volume());bool failed=false;
        try{terrainLiquidEnvironment(shortRaw,adt,target);}catch(const std::runtime_error&){failed=true;}CHECK(failed&&target.items.size()==1);
    }
    for(unsigned trial=0;trial<12;trial++){
        raw=good;auto model=adt;
        switch(trial){case 0:put(104,UINT32_MAX);break;case 1:put(108,811);break;case 2:put(140,803);break;
        case 3:put(136,0);break;case 4:real(156,NAN);break;case 5:real(144,30);break;
        case 6:put(12,16);break;case 7:put(8,13);model.chunks[0].flags=13;break;
        case 8:put(104,0);put(108,0);break;case 9:put(108,8);break;
        case 10:put(948+12,0);break;case 11:model.coord.x=64;break;}
        WorldEnvironment target;target.items.push_back(volume());bool failed=false;
        try{terrainLiquidEnvironment(raw,model,target);}catch(const std::runtime_error&){failed=true;}CHECK(failed&&target.items.size()==1);
    }
    // Preserve sea level zero and deep-water tiles, unlike the forgiving
    // upstream fallback/hidden-bit interpretation. Each source family maps once.
    for(unsigned type=0;type<4;type++){
        raw=good;put(8,1|(4u<<type));adt.chunks[0].flags=1|(4u<<type);
        for(unsigned i=0;i<81;i++)real(156+i*8,0);
        WorldEnvironment target;terrainLiquidEnvironment(raw,adt,target);
        CHECK(target.items[0].record.liquid_type==type+1&&target.items[0].heights[0]==0&&target.items[0].heights[80]==0);
    }
    raw=good;adt.chunks[0].flags=5;memset(raw.data()+800,15,64);WorldEnvironment empty;
    CHECK(terrainLiquidEnvironment(raw,adt,empty).grids==0&&empty.items.empty());
    // Only original FLT_MAX in corners unused by every visible cell can be
    // canonicalized; these slots never create triangles or influence normals.
    raw[800]=4;for(unsigned i=0;i<81;i++)if(i!=0&&i!=1&&i!=9&&i!=10)put(156+i*8,0x7f7fffff);
    WorldEnvironment isolated;CHECK(terrainLiquidEnvironment(raw,adt,isolated).unusedSentinels==77);
    auto isolatedMesh=worldLiquidMesh(isolated.items[0],true);CHECK(isolatedMesh.size()==1&&isolatedMesh[0].indices.size()==6);
    for(auto& v:isolatedMesh[0].vertices)CHECK(v.n[2]>.99f&&v.p[2]>=10&&v.p[2]<=10.75f);
    put(156,0x7f7fffff);bool failed=false;try{WorldEnvironment e;terrainLiquidEnvironment(raw,adt,e);}catch(const std::runtime_error&){failed=true;}CHECK(failed);
}
int main(){try{sampling();staging();malformed();sourceAdapter();terrainAdapter();std::remove("environment-fixture.wxp");std::cout<<checks<<" environment classification, bounds, liquid masks/heights, staged publication and rollback checks passed\n";}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
