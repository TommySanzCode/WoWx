extern "C" {
#include "wx_runtime.h"
}
#include "wx_material.h"
#include "pipeline/m2_loader.hpp"
#include "rendering/m2_track_sampler.hpp"
#include <vector>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <cstddef>
using namespace wowee::pipeline;
#include "material_motion_cooker.hpp"
static unsigned checks,gpuBytes,gpuObjects;static int failAfter=-1;
#define CHECK(x) do{checks++;if(!(x))throw std::runtime_error(std::to_string(__LINE__)+": " #x);}while(0)
extern "C" unsigned wx_free_memory(void){return 48u*1024u*1024u-gpuBytes;}
extern "C" void* wx_gpu_alloc(unsigned n){if(failAfter==0)return nullptr;if(failAfter>0)failAfter--;auto p=(unsigned*)malloc(n+4);if(p){*p=n;gpuBytes+=n;gpuObjects++;return p+1;}return nullptr;}
extern "C" void wx_gpu_free(void* p){if(p){auto b=(unsigned*)p-1;gpuBytes-=*b;gpuObjects--;free(b);}}
static WxMaterialMotion base(){WxMaterialMotion m{};m.magic=WX_MOTION_MAGIC;for(float& x:m.color)x=1;m.weight=1;return m;}
static void sampleTests(){
    M2Model model;model.sequences.resize(2);model.sequences[0].duration=1000;model.sequences[1].duration=2000;model.globalSequenceDurations={600,0};
    M2AnimationTrack rgb,alpha,weight,uv;
    for(auto* t:{&rgb,&alpha}){t->sequences.resize(2);t->interpolationType=1;t->sequences[1].timestamps={0,200,700};}
    rgb.sequences[1].vec3Values={{.2,.3,.4},{1,.5,.1},{.5,.8,.9}};alpha.sequences[1].floatValues={.25,1,.5};
    for(auto* t:{&weight,&uv}){t->sequences.resize(1);t->interpolationType=1;t->globalSequence=0;t->sequences[0].timestamps={0,300,600};}
    weight.sequences[0].floatValues={.3f,.8f,.3f};uv.sequences[0].vec3Values={{0,0,0},{.5,.2,0},{0,0,0}};
    auto m=base();motionTrack(m,0,rgb,model,1);motionTrack(m,1,alpha,model,1);motionTrack(m,2,weight,model,1);motionTrack(m,3,uv,model,1);CHECK(wx_motion_valid(&m));
    for(unsigned time=0;time<7000;time+=7){
        unsigned local=time%2000,global=time+31;WxMotionState v;wx_motion_evaluate(&m,local,global,&v);
        namespace s=wowee::rendering::m2_track;
        auto color=s::sampleVec3(rgb,1,float(local),float(global),model.globalSequenceDurations,glm::vec3(1));
        auto a=s::sampleFloat(alpha,1,float(local),float(global),model.globalSequenceDurations,1);
        auto w=s::sampleFloat(weight,1,float(local),float(global),model.globalSequenceDurations,1);
        auto tex=s::sampleVec3(uv,1,float(local),float(global),model.globalSequenceDurations,glm::vec3(0));
        for(unsigned k=0;k<3;k++)CHECK(fabsf(color[k]-v.color[k])<.00001f);
        CHECK(fabsf(v.color[3]-a*w)<.00001f);CHECK(fabsf(v.uv[0]-tex.x)<.00001f&&fabsf(v.uv[1]-tex.y)<.00001f);
    }
    alpha.interpolationType=0;m=base();motionTrack(m,1,alpha,model,1);WxMotionState v;
    wx_motion_evaluate(&m,199,0,&v);CHECK(v.color[3]==.25f);wx_motion_evaluate(&m,200,0,&v);CHECK(v.color[3]==1);
    weight.globalSequence=1;m=base();motionTrack(m,2,weight,model,1);CHECK(!m.key_count&&fabsf(m.weight-.3f)<.000001f);
    wx_motion_evaluate(&m,900,999999,&v);CHECK(fabsf(v.color[3]-.3f)<.000001f);
    auto bad=m;bad.key_count=129;CHECK(!wx_motion_valid(&bad));bad=m;bad.color[0]=NAN;CHECK(!wx_motion_valid(&bad));
    bad=m;bad.tracks[0]={0,1,0,0};CHECK(!wx_motion_valid(&bad));bad=m;bad.tracks[0].flags=4;CHECK(!wx_motion_valid(&bad));
    m=base();motionTrack(m,1,alpha,model,1);bad=m;bad.keys[1].time_ms=0;CHECK(!wx_motion_valid(&bad));
    bad=m;bad.keys[1].value[0]=INFINITY;CHECK(!wx_motion_valid(&bad));bad=m;bad.keys[2].time_ms=2001;CHECK(!wx_motion_valid(&bad));
    bool rejected=false;try{alpha.interpolationType=2;motionTrack(m,1,alpha,model,1);}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
}
struct Fixture {WxPackHeader h;WxEntry e;WxMaterialMotion motion;WxVertex v[3];uint16_t indices[3],padding;uint32_t pixels[4];};
static Fixture fixture(){
    Fixture f{};f.h={{'W','X','P','1'},7,1,sizeof(WxEntry),{0},sizeof f};f.motion=base();f.motion.color[0]=.25f;
    f.e.kind=WX_KIND_STATIC;f.e.vertex_count=3;f.e.index_count=3;f.e.width=f.e.height=2;f.e.radius=2;
    f.e.flags=WX_MATERIAL_MOTION|WX_CLAMP_U|WX_CLAMP_V|WX_TEX_SWIZZLED;
    f.e.vertex_offset=offsetof(Fixture,v);f.e.index_offset=offsetof(Fixture,indices);f.e.texture_offset=offsetof(Fixture,pixels);
    f.v[0]={{0,0,0},{0,0,1},{0,0}};f.v[1]={{1,0,0},{0,0,1},{1,0}};f.v[2]={{0,1,0},{0,0,1},{0,1}};
    f.indices[1]=1;f.indices[2]=2;for(auto& c:f.pixels)c=0xffffffff;return f;
}
static void write(const Fixture& f){auto p=fopen("motion-fixture.wxp","wb");CHECK(p);CHECK(fwrite(&f,1,sizeof f,p)==sizeof f);CHECK(!fclose(p));}
static void runtimeTests(){
    static_assert(sizeof(WxMaterialMotion)==2652);auto f=fixture();WxScene s{};
    for(unsigned trial=0;trial<8;trial++){
        f=fixture();if(trial==1)f.motion.magic=0;if(trial==2)f.motion.color[0]=NAN;if(trial==3)f.motion.key_count=129;
        if(trial==4)f.h.version=6;if(trial==5)f.e.vertex_offset-=sizeof(WxMaterialMotion);
        if(trial==6)failAfter=1;write(f);int opened=wx_pack_open(&s,"motion-fixture.wxp");
        if(trial==4||trial==5)CHECK(!opened);else{
            CHECK(opened);if(trial==7)s.budget_bytes=sizeof(WxVertex)*3+6+16; // Fits old geometry, excludes material block.
            wx_stream(&s,f.h.spawn);
            if(trial){CHECK(s.failures==1&&s.loads==0&&s.bytes==0);}
            else{CHECK(s.loads==1&&s.slots[0].material_motion&&s.slots[0].material_motion->color[0]==.25f);CHECK(s.bytes==sizeof(WxVertex)*3+6+16+sizeof(WxMaterialMotion));}
        }
        wx_pack_close(&s);CHECK(!gpuBytes&&!gpuObjects&&!s.bytes);failAfter=-1;
    }
    f=fixture();write(f);CHECK(wx_pack_open(&s,"motion-fixture.wxp")&&wx_pack_verify(&s));wx_pack_close(&s);CHECK(!gpuObjects);remove("motion-fixture.wxp");
}
static void rawTests(){
    std::vector<uint8_t> b(0x144+28+12+6);memcpy(b.data(),"MD20",4);
    auto put=[&](unsigned at,auto v){memcpy(b.data()+at,&v,sizeof v);};put(4,256u);
    unsigned at=0x144;put(at,uint16_t(1));put(at+2,int16_t(-1));put(at+12,3u);put(at+16,at+28);put(at+20,3u);put(at+24,at+40);
    put(at+28,0u);put(at+32,200u);put(at+36,400u);put(at+40,int16_t(0));put(at+42,int16_t(16384));put(at+44,int16_t(32767));
    auto t=VanillaScene(b).track(at,4);CHECK(t.sequences.size()==1&&t.sequences[0].floatValues.size()==3);
    CHECK(t.sequences[0].floatValues[0]==0&&t.sequences[0].floatValues[2]==1);
    CHECK(fabsf(t.sequences[0].floatValues[1]-16384.f/32767.f)<.000001f);
    M2Model model;M2Batch batch{};batch.colorIndex=batch.transparencyIndex=65535;batch.textureAnimIndex=0;
    model.textureTransformLookup={0};auto inert=materialMotion(model,VanillaScene(b),batch,0);CHECK(!materialMotionNeeded(inert));
    model.textureTransforms.resize(1);model.textureTransformLookup[0]=2;bool badBinding=false;
    try{materialMotion(model,VanillaScene(b),batch,0);}catch(const std::runtime_error&){badBinding=true;}CHECK(badBinding);
    bool rejected=false;b.pop_back();try{VanillaScene(b).track(at,4);}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
}
int main(){sampleTests();runtimeTests();rawTests();std::cout<<"Material motion, pinned-sampler comparisons, corruption and rollback: "<<checks<<" checks passed\n";}
