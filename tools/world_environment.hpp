#pragma once
#include "wx_environment.h"
#include "pipeline/wmo_loader.hpp"
#include <vector>
#include <stdexcept>
#include <fstream>
#include <glm/glm.hpp>
// Strict original-file adapter around WoWee's forgiving WMO parser. No flat
// height fallback, waterline offsets or map-specific exclusion rectangles.
inline uint32_t environmentU32(const std::vector<uint8_t>& b,size_t at){
    if(at>b.size()||b.size()-at<4)throw std::runtime_error("Truncated WMO environment field");
    uint32_t x;memcpy(&x,b.data()+at,4);return x;
}
template<class F> void environmentChunks(const std::vector<uint8_t>& b,size_t start,size_t end,F visit){
    if(start>end||end>b.size())throw std::runtime_error("Invalid WMO environment chunk range");
    while(start<end){if(end-start<8)throw std::runtime_error("Truncated WMO environment chunk");
        auto id=environmentU32(b,start),size=environmentU32(b,start+4);start+=8;
        if(size>end-start)throw std::runtime_error("Truncated WMO environment payload");
        visit(id,start,size);start+=size;
    }
}
struct WorldEnvironment {
    struct Item {WxEnvironmentRecord record{};std::vector<float> heights;std::vector<uint8_t> flags;};
    std::vector<Item> items;
    static std::pair<unsigned,unsigned> root(const std::vector<uint8_t>& b){
        bool found=false;std::pair<unsigned,unsigned> out{};
        environmentChunks(b,0,b.size(),[&](unsigned tag,size_t at,unsigned size){if(tag==0x4d4f4844){
            if(found||size!=64)throw std::runtime_error("Invalid Vanilla MOHD environment header");found=true;
            out={environmentU32(b,at+60),environmentU32(b,at+32)};
        }});if(!found)throw std::runtime_error("Missing MOHD environment header");return out;
    }
    // Same original liquid-entry interpretation as pinned vMaNGOS's vmap
    // extractor (contrib/vmap_extractor/vmapextract/wmo.cpp). See NOTICE.md.
    static unsigned liquidEntry(unsigned rootFlags,unsigned rootId,unsigned groupFlags,unsigned groupType,const std::vector<uint8_t>& tiles){
        unsigned type=(rootFlags&4)?groupType:groupType==15?0:groupType+1;
        if(!type)for(auto flag:tiles)if((flag&15)!=15){type=(flag&15)+1;break;}
        if(type&&type<21){unsigned family=(type-1)&3;
            type=family==0?1+!!(groupFlags&0x80000):family==1?2:family==2?3:rootId==4489?21:4;
        }return type;
    }
    void add(const wowee::pipeline::WMOGroup& group,const std::vector<uint8_t>& raw,const glm::mat4& placement,unsigned rootFlags,unsigned rootId){
        Item base;auto& r=base.record;r.kind=WX_ENV_GROUP;r.group=group.groupId;r.flags=group.flags;
        auto inverse=glm::inverse(placement);
        for(unsigned k=0;k<3;k++){r.lo[k]=group.boundingBoxMin[k];r.hi[k]=group.boundingBoxMax[k];
            for(unsigned j=0;j<4;j++)r.inverse[k*4+j]=inverse[j][k];}
        if(!wx_environment_record_valid(&r,0,0))throw std::runtime_error("Invalid WMO environment transform/bounds");
        items.push_back(base);bool found=false,header=false;
        environmentChunks(raw,0,raw.size(),[&](unsigned tag,size_t at,unsigned size){if(tag==0x4d4f4750){
            if(header||size<68)throw std::runtime_error("Invalid MOGP environment header");header=true;
            environmentChunks(raw,at+68,at+size,[&](unsigned chunk,size_t data,unsigned length){if(chunk==0x4d4c4951){
                if(found||length<30)throw std::runtime_error("Invalid/duplicate MLIQ");found=true;
                unsigned xv=environmentU32(raw,data),yv=environmentU32(raw,data+4),xt=environmentU32(raw,data+8),yt=environmentU32(raw,data+12);
                if(!xt||!yt||xt>256||yt>256||xv!=xt+1||yv!=yt+1||length!=30u+xv*yv*8u+xt*yt)
                    throw std::runtime_error("Unsupported/truncated MLIQ grid: vertices="+std::to_string(xv)+"x"+std::to_string(yv)+" tiles="+std::to_string(xt)+"x"+std::to_string(yt)+" bytes="+std::to_string(length));
                Item liquid=base;auto& e=liquid.record;e.kind=WX_ENV_LIQUID;e.x_tiles=xt;e.y_tiles=yt;e.tile_size=4.1666625f;
                memcpy(e.corner,raw.data()+data+16,12);liquid.heights.resize(xv*yv);
                for(unsigned i=0;i<xv*yv;i++){memcpy(&liquid.heights[i],raw.data()+data+30+i*8+4,4);
                    if(!std::isfinite(liquid.heights[i])||fabsf(liquid.heights[i])>100000)throw std::runtime_error("Invalid MLIQ height");}
                liquid.flags.assign(raw.begin()+data+30+xv*yv*8,raw.begin()+data+length);
                // Cross-check the reusable parser before accepting the adapter.
                if(group.liquid.heights!=liquid.heights||group.liquid.flags!=liquid.flags||group.liquid.xTiles!=xt||group.liquid.yTiles!=yt)
                    throw std::runtime_error("WoWee/strict MLIQ parser disagreement");
                e.liquid_type=liquidEntry(rootFlags,rootId,group.flags,group.liquidType,liquid.flags);
                items.push_back(std::move(liquid));
            }});
        }});if(!header||found!=group.liquid.hasLiquid())throw std::runtime_error("WMO liquid presence mismatch");
        if(items.size()>WX_ENV_MAX_RECORDS)throw std::runtime_error("WMO environment record budget exceeded");
    }
    std::vector<uint8_t> encode() const {
        std::vector<uint8_t> out(items.size()*sizeof(WxEnvironmentRecord));
        for(unsigned i=0;i<items.size();i++){
            const auto& item=items[i];auto r=item.record;
            if(r.kind==WX_ENV_LIQUID){r.heights_offset=(unsigned)out.size();auto p=(const uint8_t*)item.heights.data();
                out.insert(out.end(),p,p+item.heights.size()*4);r.flags_offset=(unsigned)out.size();
                out.insert(out.end(),item.flags.begin(),item.flags.end());while(out.size()&3)out.push_back(0);}
            memcpy(out.data()+i*sizeof r,&r,sizeof r);
        }
        if(out.size()>WX_ENV_MAX_BYTES)throw std::runtime_error("WMO environment byte budget exceeded");
        unsigned next=(unsigned)items.size()*sizeof(WxEnvironmentRecord);
        for(unsigned i=0;i<items.size();i++){auto r=(const WxEnvironmentRecord*)out.data()+i;
            if(!wx_environment_record_valid(r,(unsigned)out.size(),next))throw std::runtime_error("Invalid cooked WMO environment");
            if(r->kind==WX_ENV_LIQUID)next=r->flags_offset+((r->x_tiles*r->y_tiles+3)&~3u);
        }return out;
    }
    unsigned write(std::ofstream& file,unsigned offset) const {
        auto blob=encode();WxEnvironmentFooter footer{{'W','X','E','1'},(unsigned)items.size(),(unsigned)blob.size(),offset};
        file.write((const char*)blob.data(),blob.size());file.write((const char*)&footer,sizeof footer);return footer.bytes;
    }
};
