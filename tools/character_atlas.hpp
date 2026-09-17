#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>

// Vanilla's 256-square body atlas. These coordinates follow WoWee's
// character_renderer.cpp; compositing is independent of the desktop GPU API.
namespace wx_atlas {
struct Region {unsigned x,y,w,h;};
constexpr Region regions[]={{0,0,128,64},{0,64,128,64},{0,128,128,32},
    {128,0,128,64},{128,64,128,32},{128,96,128,64},{128,160,128,64},{128,224,128,32}};
inline void blend(std::vector<uint8_t>& destination,const std::vector<uint8_t>& source,
        unsigned width,unsigned height,Region r,bool colorKey=true) {
    if(destination.size()!=256*256*4||!width||!height||width>4096||height>4096||
       source.size()!=size_t(width)*height*4||!r.w||!r.h||r.w>256||r.h>256||r.x>256-r.w||r.y>256-r.h)
        throw std::runtime_error("Invalid character atlas region");
    for(unsigned y=0;y<r.h;y++)for(unsigned x=0;x<r.w;x++){
        const auto* s=&source[((y*height/r.h)*width+x*width/r.w)*4];
        auto* d=&destination[((r.y+y)*256+r.x+x)*4];
        unsigned alpha=s[3];if(colorKey&&s[0]==255&&s[1]==0&&s[2]==255)alpha=0;
        for(unsigned c=0;c<3;c++)d[c]=uint8_t((s[c]*alpha+d[c]*(255-alpha)+127)/255);
        d[3]=uint8_t(alpha+(d[3]*(255-alpha)+127)/255);
    }
}
}
