#pragma once
#include "wx_pack.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#define STB_DXT_IMPLEMENTATION
#define STB_DXT_STATIC
#include "../third_party/stb/stb_dxt.h"
struct CompressedTexture {uint32_t encoding;std::vector<uint8_t> bytes;};
inline CompressedTexture compressTexture(const WxEntry& e,const std::vector<uint8_t>& source){
    if((e.flags&(WX_TEX_DXT1|WX_TEX_DXT5))||e.width<4||e.width>256||e.width!=e.height||(e.width&(e.width-1)))
        throw std::runtime_error("Texture compression requires an uncompressed power-of-two square");
    unsigned levels=wx_texture_levels(&e),frames=wx_texture_frames(&e),expected=0;
    if(levels>9||!frames||frames>32||((e.flags&WX_TEXTURE_SEQUENCE)&&(frames<2||!(e.reserved[1]&65535u))))throw std::runtime_error("Invalid mip/frame count");
    for(unsigned i=0,n=e.width;i<levels;i++,n/=2){if(!n)throw std::runtime_error("Mip chain exceeds dimensions");expected+=n*n*4;}
    unsigned stride=(e.flags&WX_TEXTURE_SEQUENCE)?(expected+127u)&~127u:expected;
    if(source.size()!=stride*frames)throw std::runtime_error("Truncated texture frame/mip chain");
    bool alpha=false;for(unsigned f=0;f<frames;f++)for(unsigned i=3;i<expected;i+=4)if(source[f*stride+i]!=255){alpha=true;break;}
    CompressedTexture out{alpha?WX_TEX_DXT5:WX_TEX_DXT1,{}};unsigned base=0,blockBytes=alpha?16:8;
    for(unsigned frame=0;frame<frames;frame++){base=frame*stride;
    for(unsigned mip=0,n=e.width;mip<levels;mip++,n/=2){
        for(unsigned by=0;by<(n+3)/4;by++)for(unsigned bx=0;bx<(n+3)/4;bx++){
            uint8_t rgba[64];
            for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++){
                unsigned px=std::min(bx*4+x,n-1),py=std::min(by*4+y,n-1),address=py*n+px;
                if(e.flags&WX_TEX_SWIZZLED){address=0;for(unsigned bit=0;(1u<<bit)<n;bit++)address|=((px>>bit)&1)<<(2*bit)|((py>>bit)&1)<<(2*bit+1);}
                const uint8_t* p=&source[base+address*4];uint8_t* d=rgba+(y*4+x)*4;
                d[0]=p[2];d[1]=p[1];d[2]=p[0];d[3]=p[3];
            }
            size_t at=out.bytes.size();out.bytes.resize(at+blockBytes);
            stb_compress_dxt_block(out.bytes.data()+at,rgba,alpha?1:0,STB_DXT_HIGHQUAL);
        }base+=n*n*4;
    }if(e.flags&WX_TEXTURE_SEQUENCE)while(out.bytes.size()&127u)out.bytes.push_back(0);
    }return out;
}
