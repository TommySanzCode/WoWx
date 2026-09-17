#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <utility>
#include <stdexcept>
#include <string>
#include <cmath>
#include <algorithm>
struct WorldLayout {
    uint32_t flags=0,uniqueId=0;
    std::string root;
    std::vector<std::pair<unsigned,unsigned>> tiles;
    bool global()const{return (flags&1)!=0;}
};
// Validate archive boundaries before passing placement metadata to pinned WoWee.
// A global WMO is one map-wide streamed pack, not a fictional terrain tile.
inline WorldLayout worldLayout(const std::vector<uint8_t>& data){
    bool version=false,main=false,header=false,mwmo=false,modf=false;WorldLayout result;
    std::vector<uint8_t> names,placement;
    for(size_t at=0;at<data.size();){
        if(data.size()-at<8)throw std::runtime_error("Truncated WDT chunk header");
        uint32_t magic,size;memcpy(&magic,data.data()+at,4);memcpy(&size,data.data()+at+4,4);at+=8;
        if(size>data.size()-at)throw std::runtime_error("WDT chunk exceeds archive member");
        if(magic==0x4d564552){uint32_t value=0;if(version||size!=4)throw std::runtime_error("Invalid WDT version chunk");memcpy(&value,data.data()+at,4);if(value!=18)throw std::runtime_error("Unsupported WDT version");version=true;}
        if(magic==0x4d504844){if(header||size<4)throw std::runtime_error("Invalid WDT map header");header=true;memcpy(&result.flags,data.data()+at,4);}
        if(magic==0x4d41494e){
            if(main||size!=64*64*8)throw std::runtime_error("Invalid WDT tile table");main=true;
            for(unsigned i=0;i<4096;i++){uint32_t flags;memcpy(&flags,data.data()+at+i*8,4);if(flags&1)result.tiles.emplace_back(i%64,i/64);}
        }
        if(magic==0x4d574d4f){if(mwmo)throw std::runtime_error("Duplicate WDT model names");mwmo=true;names.assign(data.begin()+at,data.begin()+at+size);}
        if(magic==0x4d4f4446){if(modf)throw std::runtime_error("Duplicate WDT model placement");modf=true;placement.assign(data.begin()+at,data.begin()+at+size);}
        at+=size;
    }
    if(!version||!header)throw std::runtime_error("Missing required WDT header");
    if(result.global()){
        if(!mwmo||!modf||names.empty()||names.size()>1024||placement.size()!=64||!result.tiles.empty())throw std::runtime_error("Invalid global WDT model metadata");
        auto end=std::find(names.begin(),names.end(),0);
        if(end==names.end()||end==names.begin()||std::any_of(end,names.end(),[](uint8_t c){return c!=0;}))throw std::runtime_error("Invalid global WDT model name table");
        result.root.assign(names.begin(),end);
        if(result.root.front()=='\\'||result.root.front()=='/'||result.root.find("..")!=std::string::npos||result.root.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_ '-.\\/")!=std::string::npos)
            throw std::runtime_error("Unsafe global WDT model path");
        auto suffix=result.root.substr(result.root.size()>4?result.root.size()-4:0);
        for(char& c:suffix)if(c>='A'&&c<='Z')c+=32;
        if(suffix!=".wmo")throw std::runtime_error("Global WDT root is not a WMO");
        uint32_t nameId;memcpy(&nameId,placement.data(),4);if(nameId)throw std::runtime_error("Unexpected global WDT name offset");
        memcpy(&result.uniqueId,placement.data()+4,4);
        for(unsigned at=8;at<56;at+=4){float value;memcpy(&value,placement.data()+at,4);if(!std::isfinite(value)||fabsf(value)>1000000)throw std::runtime_error("Invalid global WDT placement component");}
    }else if(!main)throw std::runtime_error("Missing terrain WDT tile table");
    return result;
}
inline std::vector<std::pair<unsigned,unsigned>> worldTiles(const std::vector<uint8_t>& data){return worldLayout(data).tiles;}
