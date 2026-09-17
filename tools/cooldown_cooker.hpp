#pragma once
#include "dbc_table.hpp"
#include "wx_cooldown.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
inline void writeCooldownCatalog(const DbcTable& dbc,const DbcTable& categories,const std::filesystem::path& path){
    if(dbc.columns()!=173||!dbc.size()||dbc.size()>65535)throw std::runtime_error("Expected Vanilla 173-column Spell.dbc");
    if(categories.columns()!=2)throw std::runtime_error("Expected Vanilla SpellCategory.dbc");
    std::vector<WxCooldownInfo> rows;
    for(unsigned i=0;i<dbc.size();i++){
        // 5875 layout: four 9-column locale strings end at 155; family mask is
        // two words at 161/162. Do not use vMaNGOS's expanded SQL struct offsets.
        WxCooldownInfo r{dbc.value(i,0),dbc.value(i,2),dbc.value(i,19),dbc.value(i,20),dbc.value(i,6),
            dbc.value(i,157),dbc.value(i,158),dbc.value(i,160),{dbc.value(i,161),dbc.value(i,162)},dbc.value(i,8),dbc.value(i,9),dbc.value(i,164),0};
        if(r.category)r.category_flags=categories.value(categories.row(r.category),1);
        if(!r.spell||r.spell>65535||r.category>65535||r.recovery>0x7fffffffu||r.category_recovery>0x7fffffffu||
            r.gcd_category>65535||r.gcd_time>0x7fffffffu||r.family>65535||r.damage_class>3)throw std::runtime_error("Invalid Vanilla cooldown metadata");
        rows.push_back(r);
    }
    std::sort(rows.begin(),rows.end(),[](const auto& a,const auto& b){return a.spell<b.spell;});
    auto temp=path;temp+=".tmp";
    if(std::filesystem::exists(path)||std::filesystem::exists(temp))throw std::runtime_error("Cooldown output must be new");
    std::ofstream out(temp,std::ios::binary);if(!out)throw std::runtime_error("Cannot create cooldown catalog");
    uint32_t header[]={0x44435857,2,uint32_t(rows.size()),sizeof(WxCooldownInfo)};
    out.write((const char*)header,sizeof header);out.write((const char*)rows.data(),rows.size()*sizeof(rows[0]));
    out.close();if(!out)throw std::runtime_error("Cooldown write failed");std::filesystem::rename(temp,path);
}
