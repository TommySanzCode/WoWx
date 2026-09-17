#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <glm/glm.hpp>
// Pinned WoWee wmo.frag.glsl policy: interiors use authored baked colour
// floored by max(MOHD ambient, .15); lit exteriors tint directional lighting
// with max(MOCV, .25). No shadow/specular/local-light terms are baked here.
inline uint32_t worldVertexLighting(glm::vec4 color,glm::vec3 ambient,bool indoor,bool unlit){
    uint32_t rgba=0xff000000u;
    for(unsigned k=0;k<3;k++){
        if(!std::isfinite(color[k])||!std::isfinite(ambient[k])||color[k]<0||color[k]>1||ambient[k]<0||ambient[k]>1)
            throw std::runtime_error("Invalid authored WMO lighting colour");
        float value=indoor?std::max(color[k],std::max(ambient[k],.15f)):unlit?1.f:std::max(color[k],.25f);
        rgba|=uint32_t(std::lround(value*255.f))<<(k*8);
    }return rgba;
}
// The pinned loader tolerates partial chunks. Colour extraction needs exact
// per-vertex correspondence, so reject truncation/duplicates before cooking.
inline bool validateWmoVertexColors(const std::vector<uint8_t>& b,unsigned vertices){
    auto u32=[&](size_t at){if(at>b.size()||b.size()-at<4)throw std::runtime_error("Truncated WMO colour chunk");uint32_t x;memcpy(&x,b.data()+at,4);return x;};
    bool group=false,colors=false;
    for(size_t at=0;at<b.size();){
        if(b.size()-at<8)throw std::runtime_error("Truncated WMO outer chunk");
        auto id=u32(at),size=u32(at+4);at+=8;
        if(size>b.size()-at)throw std::runtime_error("Truncated WMO outer payload");
        size_t end=at+size;
        if(id==0x4d4f4750){
            if(group||size<68)throw std::runtime_error("Invalid WMO group header");group=true;
            for(size_t sub=at+68;sub<end;){
                if(end-sub<8)throw std::runtime_error("Truncated WMO inner chunk");
                auto tag=u32(sub),n=u32(sub+4);sub+=8;
                if(n>end-sub)throw std::runtime_error("Truncated WMO inner payload");
                if(tag==0x4d4f4356){if(colors||n!=uint64_t(vertices)*4)throw std::runtime_error("WMO vertex colour count mismatch/duplicate");colors=true;}
                sub+=n;
            }
        }at=end;
    }
    if(!group)throw std::runtime_error("Missing WMO group header");return colors;
}
