extern "C" {
#include "wx_runtime.h"
}
#include "texture_compress.hpp"
#include "pipeline/dxt_block.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){std::fprintf(stderr,"Texture check failed at line %u\n",__LINE__);std::exit(1);}}while(0)
static unsigned alphaAt(const uint8_t* b,unsigned pixel){
    unsigned a[8]={b[0],b[1]};
    if(a[0]>a[1])for(unsigned i=2;i<8;i++)a[i]=((8-i)*a[0]+(i-1)*a[1])/7;
    else {for(unsigned i=2;i<6;i++)a[i]=((6-i)*a[0]+(i-1)*a[1])/5;a[6]=0;a[7]=255;}
    uint64_t bits=0;for(unsigned i=0;i<6;i++)bits|=uint64_t(b[i+2])<<(8*i);return a[(bits>>(pixel*3))&7];
}
static void reject(WxEntry e,std::vector<uint8_t> bytes){try{compressTexture(e,bytes);CHECK(false);}catch(const std::runtime_error&){checks++;}}
int main(){
    WxEntry e{};e.kind=WX_KIND_STATIC;e.width=e.height=4;
    std::vector<uint8_t> red(64);for(unsigned i=0;i<16;i++){red[i*4+2]=255;red[i*4+3]=255;}
    auto c=compressTexture(e,red);CHECK(c.encoding==WX_TEX_DXT1&&c.bytes.size()==8);
    auto d=wowee::pipeline::decodeDxtColorBlock(c.bytes.data(),true);
    for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++){unsigned index=d.indexAt(x,y);CHECK(d.rgb[index][0]>=248&&d.rgb[index][1]==0&&d.rgb[index][2]==0&&(!d.index3IsTransparent||index!=3));}
    auto alpha=red;for(unsigned i=0;i<16;i++)alpha[i*4+3]=(i%4)*85;
    c=compressTexture(e,alpha);CHECK(c.encoding==WX_TEX_DXT5&&c.bytes.size()==16);
    for(unsigned i=0;i<16;i++)CHECK(std::abs(int(alphaAt(c.bytes.data(),i))-int(alpha[i*4+3]))<=19);
    d=wowee::pipeline::decodeDxtColorBlock(c.bytes.data()+8,false);for(unsigned i=0;i<4;i++)CHECK(d.rgb[i][0]>=248&&d.rgb[i][1]==0&&d.rgb[i][2]==0);
    e.flags=WX_TEX_SWIZZLED|WX_MIPMAPPED;e.reserved[1]=3;std::vector<uint8_t> mips(84,255);unsigned base=0;
    for(unsigned n=4;n;n/=2){for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++){
        unsigned address=0;for(unsigned bit=0;(1u<<bit)<n;bit++)address|=((x>>bit)&1)<<(2*bit)|((y>>bit)&1)<<(2*bit+1);
        unsigned value=n==4?x*85:n==2?85:170;for(unsigned k=0;k<3;k++)mips[base+address*4+k]=uint8_t(value);
    }base+=n*n*4;}
    c=compressTexture(e,mips);CHECK(c.encoding==WX_TEX_DXT1&&c.bytes.size()==24);
    for(unsigned mip=0;mip<3;mip++){d=wowee::pipeline::decodeDxtColorBlock(c.bytes.data()+mip*8,true);
        for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++)for(unsigned k=0;k<3;k++){
            unsigned expected=mip==0?x*85:mip==1?85:170;CHECK(std::abs(int(d.rgb[d.indexAt(x,y)][k])-int(expected))<=8);
        }}
    auto truncated=mips;truncated.pop_back();reject(e,truncated);e.reserved[1]=4;reject(e,mips);e.reserved[1]=3;
    // Frame padding is excluded from alpha detection. All frames must share
    // one encoding even if only the last authored frame has transparency.
    std::vector<uint8_t> sequence;
    for(unsigned f=0;f<30;f++){sequence.insert(sequence.end(),mips.begin(),mips.end());sequence.resize((f+1)*128,0);}
    e.flags|=WX_TEXTURE_SEQUENCE;e.reserved[1]=(30u<<16)|3;
    c=compressTexture(e,sequence);CHECK(c.encoding==WX_TEX_DXT1&&c.bytes.size()==30*128);
    for(unsigned f=0;f<30;f++)CHECK(!memcmp(c.bytes.data(),c.bytes.data()+f*128,128));
    sequence[29*128+3]=0;c=compressTexture(e,sequence);CHECK(c.encoding==WX_TEX_DXT5&&c.bytes.size()==30*128);
    CHECK(alphaAt(c.bytes.data(),0)==255&&alphaAt(c.bytes.data()+29*128,0)==0);
    auto shortSequence=sequence;shortSequence.pop_back();reject(e,shortSequence);
    e.reserved[1]=(33u<<16)|3;reject(e,sequence);e.reserved[1]=(1u<<16)|3;reject(e,sequence);
    e.flags=WX_TEX_SWIZZLED|WX_MIPMAPPED;e.reserved[1]=3;
    e.flags|=WX_TEX_DXT1;reject(e,mips);e.flags=0;e.width=3;reject(e,mips);
    std::printf("DXT color, alpha, mip-order and input-boundary checks: %u passed\n",checks);return 0;
}
