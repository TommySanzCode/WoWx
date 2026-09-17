#pragma once
#include "world_environment.hpp"
#include "pipeline/adt_loader.hpp"
#include "core/coordinates.hpp"
#include <array>
#include <algorithm>

// Vanilla MCLQ offsets are relative to the MCNK tag, including its 8-byte
// header. The pinned WoWee ADT parser remains the terrain/model foundation;
// this strict adapter reads its omitted liquid data from the original bytes.
// vMaNGOS extractor/loadlib/adt.h defines this layout. In particular, 0x80 is
// deep water, not a hidden tile; only the low-nibble 0x0f sentinel is dry.
struct TerrainLiquidCoverage {
    unsigned chunks=0,grids=0,visibleCells=0,deepCells=0,flowBytes=0,upstreamLayers=0,unusedSentinels=0;
};
inline TerrainLiquidCoverage terrainLiquidEnvironment(const std::vector<uint8_t>& raw,
        const wowee::pipeline::ADTTerrain& adt,WorldEnvironment& environment){
    if(!adt.loaded||adt.version!=18||adt.coord.x<0||adt.coord.x>63||adt.coord.y<0||adt.coord.y>63)
        throw std::runtime_error("Invalid Vanilla terrain liquid source");
    TerrainLiquidCoverage stats;std::array<bool,256> seen{};std::vector<WorldEnvironment::Item> added;
    environmentChunks(raw,0,raw.size(),[&](unsigned tag,size_t at,unsigned size){
        if(tag!=0x4d434e4b)return;
        if(size<128)throw std::runtime_error("Truncated MCNK liquid header");
        unsigned flags=environmentU32(raw,at),cx=environmentU32(raw,at+4),cy=environmentU32(raw,at+8);
        if(cx>15||cy>15||seen[cy*16+cx])throw std::runtime_error("Invalid/duplicate MCNK liquid coordinates");
        unsigned id=cy*16+cx;seen[id]=true;stats.chunks++;
        const auto& chunk=adt.getChunk(cx,cy);
        if(chunk.indexX!=cx||chunk.indexY!=cy||chunk.flags!=flags)throw std::runtime_error("WoWee/MCNK identity disagreement");
        stats.upstreamLayers+=(unsigned)adt.waterData[id].layers.size();
        unsigned off=environmentU32(raw,at+96),length=environmentU32(raw,at+100);
        if(!off&&!length){if(flags&0x3c)throw std::runtime_error("Missing declared MCLQ");return;}
        if(off<136||length<8||off>size+8||length>size+8-off)throw std::runtime_error("MCLQ exceeds its MCNK");
        size_t header=at-8+off,data=header+8;
        if(environmentU32(raw,header)!=0x4d434c51)throw std::runtime_error("Invalid MCLQ tag/offset");
        unsigned declared=environmentU32(raw,header+4);
        // Vanilla's inner length field is commonly zero; MCNK.sizeMCLQ is
        // authoritative. Preserve the optional 84-byte flow tail as a counted
        // unsupported input, never interpret it as additional surface cells.
        if(declared&&declared!=length-8)throw std::runtime_error("MCLQ length disagreement");
        if(length==8){if(flags&0x3c)throw std::runtime_error("Empty declared MCLQ");return;}
        if(length!=728&&length!=812)throw std::runtime_error("Unsupported MCLQ payload length");
        WorldEnvironment::Item item;auto& r=item.record;r.kind=WX_ENV_LIQUID;r.group=0x80000000u|id;r.flags=flags;
        r.x_tiles=r.y_tiles=8;r.tile_size=4.1666625f;
        unsigned type=flags&0x3c;
        if(!type||(type&(type-1)))throw std::runtime_error("Ambiguous MCLQ liquid family");
        r.liquid_type=type==4?1:type==8?2:type==16?3:4;
        float minimum,maximum;memcpy(&minimum,raw.data()+data,4);memcpy(&maximum,raw.data()+data+4,4);
        if(!std::isfinite(minimum)||!std::isfinite(maximum)||fabsf(minimum)>100000||fabsf(maximum)>100000||minimum>maximum)
            throw std::runtime_error("Invalid MCLQ height bounds");
        item.flags.assign(raw.begin()+data+656,raw.begin()+data+720);
        item.heights.resize(81);for(unsigned i=0;i<81;i++){
            memcpy(&item.heights[i],raw.data()+data+12+i*8,4);
            if(environmentU32(raw,data+12+i*8)==0x7f7fffff){
                unsigned x=i%9,y=i/9;bool used=false;
                for(unsigned yy=y?y-1:y;yy<=y&&yy<8;yy++)for(unsigned xx=x?x-1:x;xx<=x&&xx<8;xx++)used|=(item.flags[yy*8+xx]&15)!=15;
                if(used)throw std::runtime_error("Visible MCLQ vertex has a dry sentinel");
                // Original FLT_MAX denotes an unused grid corner. No enabled
                // triangle or depth query references it. Encode a finite zero
                // in that unreferenced slot; preserve every visible height.
                item.heights[i]=0;stats.unusedSentinels++;
            }else if(!std::isfinite(item.heights[i])||fabsf(item.heights[i])>100000)throw std::runtime_error("Invalid MCLQ vertex height");
        }
        unsigned visible=0;for(auto flag:item.flags)if((flag&15)!=15){visible++;stats.deepCells+=!!(flag&128);}
        if(length==812)for(size_t i=data+720;i<data+804;i++)stats.flowBytes+=raw[i]!=0;
        if(!visible)return;
        stats.grids++;stats.visibleCells+=visible;
        // Match the existing terrain mesh: source columns travel -worldY,
        // rows travel -worldX. A rigid reflection keeps exact original heights
        // and mask indexing; worldLiquidMesh corrects triangle winding.
        const float tile=wowee::core::coords::TILE_SIZE,step=tile/16;
        float wx=(32.f-adt.coord.y)*tile-cy*step,wy=(32.f-adt.coord.x)*tile-cx*step;
        r.inverse[1]=-1;r.inverse[3]=wy;r.inverse[4]=-1;r.inverse[7]=wx;r.inverse[10]=1;
        r.hi[0]=r.hi[1]=8*r.tile_size;r.lo[2]=-100000;
        r.hi[2]=*std::max_element(item.heights.begin(),item.heights.end());
        // Bounds describe a water column. Ground/cave exclusion and swimming
        // remain separate gameplay/collision work; no walkable floor is added.
        added.push_back(std::move(item));
    });
    if(stats.chunks!=256||environment.items.size()+added.size()>WX_ENV_MAX_RECORDS)
        throw std::runtime_error("Incomplete terrain or environment budget exceeded");
    environment.items.insert(environment.items.end(),std::make_move_iterator(added.begin()),std::make_move_iterator(added.end()));
    return stats;
}
