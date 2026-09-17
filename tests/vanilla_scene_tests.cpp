#include "wx_backdrop.h"
#include "pipeline/m2_loader.hpp"
#include <vector>
#include <cstring>
#include <stdexcept>
#include <iostream>
using wowee::pipeline::M2AnimationTrack;
#include "vanilla_scene.hpp"
#include "world_transform.hpp"
#include "world_material.hpp"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x))throw std::runtime_error(#x);}while(0)
template<class T>static void put(std::vector<uint8_t>& b,unsigned p,T v){memcpy(b.data()+p,&v,sizeof v);}
int main(){
    CHECK(worldMaterial(4,0,true)==(4u<<WX_BLEND_SHIFT));
    CHECK(worldMaterial(1,1|2|8|16,true)==(WX_ALPHA_TEST|WX_UNLIT|WX_UNFOGGED|WX_NO_DEPTH_TEST|WX_NO_DEPTH_WRITE));
    CHECK(worldMaterial(0,1|2|8|16,false)==(WX_UNLIT|WX_UNFOGGED));
    bool invalidBlend=false;try{worldMaterial(7,0,true);}catch(const std::runtime_error&){invalidBlend=true;}CHECK(invalidBlend);
    float zero[3]={0},rotation[3]={0};
    auto matrix=globalWorldPlacement(zero,rotation);
    auto origin=matrix*glm::vec4(0,0,0,1);CHECK(glm::length(glm::vec3(origin))<.00001f);
    auto axis=matrix*glm::vec4(1,0,0,0);CHECK(fabsf(axis.x+1)<.00001f&&fabsf(axis.y)<.00001f);
    float position[3]={16000,20,17000};matrix=globalWorldPlacement(position,zero);
    CHECK(glm::length(glm::vec3(matrix[3])-wowee::core::coords::adtToWorld(16000,20,17000))<.0001f);
    rotation[0]=90;matrix=globalWorldPlacement(zero,rotation);axis=matrix*glm::vec4(0,0,1,0);CHECK(fabsf(axis.x-1)<.00001f);
    std::vector<uint8_t> b(0x144+504);memcpy(b.data(),"MD20",4);put(b,4,256u);put(b,0x13c,1u);put(b,0x140,0x144u);
    unsigned p=0x144;put(b,p+0x28,uint16_t(4));put(b,p+0x2a,uint16_t(2));put(b,p+0x150,0x80112233u);put(b,p+0x15c,.00002f);
    VanillaScene v(b);auto e=v.emitter(0);CHECK(e.type==2&&e.blend==4);CHECK(e.scale[0]==.00002f);CHECK(e.color[0][3]>0.50f&&e.color[0][3]<.51f);CHECK(e.color[0][0]<e.color[0][2]);
    put(b,p+0x2a,uint16_t(0x102));CHECK(v.emitter(0).type==0x102); // Must not truncate to the later u8 layout.
    unsigned rejected=0;try{auto bad=b;bad.pop_back();VanillaScene(bad).emitter(0);}catch(const std::runtime_error&){rejected++;}
    try{auto bad=b;put(bad,4,264u);VanillaScene later(bad);}catch(const std::runtime_error&){rejected++;}
    try{auto bad=b;put(bad,0x140,UINT32_MAX);VanillaScene(bad).emitter(0);}catch(const std::runtime_error&){rejected++;}
    try{auto bad=b;put(bad,p+0x34+12,UINT32_MAX);VanillaScene(bad).track(p+0x34,0);}catch(const std::runtime_error&){rejected++;}
    CHECK(rejected==4);
    for(unsigned flags:{0x9u,0x29u,0x131u,0x1031u}){put(b,p+4,flags);CHECK(v.emitter(0).flags==flags);}
    rejected=0;
    for(unsigned flags:{0x8u,0x40u,0x80000000u})try{put(b,p+4,flags);v.emitter(0);}catch(const std::runtime_error&){rejected++;}
    CHECK(rejected==3);
    // Adjacent texture/transparency arrays must never be interpreted as colors.
    put(b,0x54,2u);put(b,0x58,0x144u);put(b,0x64,99u);put(b,0x68,UINT32_MAX);
    auto colors=v.colors();CHECK(colors.first==2&&colors.second==0x144);
    std::cout<<"Vanilla scene adapter: "<<checks<<" checks passed\n";
}
