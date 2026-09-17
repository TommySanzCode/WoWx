#pragma once
#include "dbc_table.hpp"
#include "wx_spellbook.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cmath>
inline void writeSpellCatalog(const DbcTable& dbc,const DbcTable& ranges,const std::filesystem::path& path){
    if(dbc.columns()!=173||!dbc.size()||dbc.size()>65535)throw std::runtime_error("Expected Vanilla 173-column Spell.dbc");
    std::vector<std::pair<uint16_t,WxSpellInfo>> rows;
    for(unsigned i=0;i<dbc.size();i++){
        auto id=dbc.value(i,0);if(!id||id>65535)throw std::runtime_error("Spell ID exceeds Vanilla protocol range");
        WxSpellInfo info{};info.attributes=dbc.value(i,6);
        info.metadata=1;info.attributes_ex=dbc.value(i,7);info.school=dbc.value(i,1);
        info.attributes_ex2=dbc.value(i,8);
        info.power_type=dbc.value(i,31);info.cost=dbc.value(i,32);info.cost_per_level=dbc.value(i,33);info.cost_percent=dbc.value(i,156);
        info.base_level=dbc.value(i,28);info.max_level=dbc.value(i,27);info.spell_level=dbc.value(i,29);info.range_index=dbc.value(i,36);
        info.stance_allow=dbc.value(i,11);info.stance_deny=dbc.value(i,12);
        for(unsigned effect=0;effect<3;effect++){info.target_a[effect]=dbc.value(i,82+effect);info.target_b[effect]=dbc.value(i,85+effect);}
        auto range=ranges.row(info.range_index);info.min_range=ranges.real(range,1);info.max_range=ranges.real(range,2);
        if(info.school>6||!std::isfinite(info.min_range)||!std::isfinite(info.max_range)||info.min_range<0||info.max_range<info.min_range)
            throw std::runtime_error("Invalid Vanilla spell feedback metadata: "+std::to_string(id));
        auto name=dbc.text(i,120),rank=dbc.text(i,129);
        if(name.empty()||name.size()>=sizeof info.name||rank.size()>=sizeof info.rank)throw std::runtime_error("Spell text exceeds catalog bounds: "+std::to_string(id));
        std::memcpy(info.name,name.c_str(),name.size()+1);std::memcpy(info.rank,rank.c_str(),rank.size()+1);
        rows.emplace_back(uint16_t(id),info);
    }
    std::sort(rows.begin(),rows.end(),[](const auto& a,const auto& b){return a.first<b.first;});
    auto temporary=path;temporary+=".tmp";
    if(std::filesystem::exists(path)||std::filesystem::exists(temporary))throw std::runtime_error("Spell catalog output must be new");
    std::ofstream out(temporary,std::ios::binary);if(!out)throw std::runtime_error("Cannot create spell catalog");
    uint32_t header[]={0x31535857,2,uint32_t(rows.size()),sizeof(WxSpellInfo)};
    out.write((const char*)header,sizeof header);
    for(const auto& row:rows)out.write((const char*)&row.first,2);
    for(const auto& row:rows)out.write((const char*)&row.second,sizeof row.second);
    out.close();if(!out)throw std::runtime_error("Spell catalog write failed");std::filesystem::rename(temporary,path);
}
