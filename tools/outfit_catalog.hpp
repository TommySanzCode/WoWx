#pragma once
#include "wx_outfit.h"
// Vanilla packs race/class/sex/outfit into one word. Its logical field count
// includes the four bytes separately: 41 fields, but 38 words / 152 bytes.
// Schema: https://corepunch.github.io/open-realm/games/world-of-warcraft/dbc-reference/
static std::vector<uint8_t> outfitWords(std::vector<uint8_t> bytes){
    if(bytes.size()<20)throw std::runtime_error("Truncated outfit DBC");
    uint32_t h[5];memcpy(h,bytes.data(),20);
    if(memcmp(bytes.data(),"WDBC",4)||(h[2]!=41&&h[2]!=38)||h[3]!=152)throw std::runtime_error("Expected Vanilla packed outfit layout");
    h[2]=38;memcpy(bytes.data()+8,h+2,4);return bytes;
}
template<class Source>static std::vector<WxOutfit> startOutfits(Source& a){
    DbcTable dbc(outfitWords(a.read("DBFilesClient\\CharStartOutfit.dbc")));
    if(dbc.size()>WX_OUTFIT_LIMIT)throw std::runtime_error("Outfit table exceeds fixed runtime budget");
    std::vector<WxOutfit> rows;
    for(unsigned i=0;i<dbc.size();i++){
        WxOutfit row{};row.key=dbc.value(i,1);
        for(unsigned j=0;j<12;j++){
            int display=int32_t(dbc.value(i,14+j)),type=int32_t(dbc.value(i,26+j)),slot=wx_outfit_slot(type);
            if(display<=0||slot<0)continue;
            if(row.display[slot])throw std::runtime_error("Duplicate starter equipment slot");
            row.display[slot]=display;row.type[slot]=uint8_t(type);
        }
        if(!wx_outfit_valid(&row))throw std::runtime_error("Invalid starter outfit");
        for(const auto& previous:rows)if(previous.key==row.key)throw std::runtime_error("Duplicate starter outfit key");
        rows.push_back(row);
    }
    return rows;
}
template<class Source>static void writeOutfits(Source& a,const fs::path& output){
    auto rows=startOutfits(a);WxOutfitHeader h{{'W','X','O','F'},1,uint32_t(rows.size()),sizeof(WxOutfit)};
    std::ofstream file(output,std::ios::binary|std::ios::trunc);file.write((const char*)&h,sizeof h);file.write((const char*)rows.data(),rows.size()*sizeof(WxOutfit));file.close();
    if(!file)throw std::runtime_error("Outfit write failed");std::cout<<rows.size()<<" starter outfits / "<<sizeof h+rows.size()*sizeof(WxOutfit)<<" bytes\n";
}
