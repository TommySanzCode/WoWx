#pragma once
#include "wx_lighting.h"
#include "dbc_table.hpp"
#include "rendering/light_coords.hpp"
#include "rendering/light_band_block.hpp"
/* Pinned WoWee coordinate/block interpretation, with strict Vanilla dimensions.
   Keep missing/empty source bands explicit: the runtime uses a visible fallback. */
static void prepareLighting(Archives& archives,const std::string& output){
    DbcTable lights(archives.read("DBFilesClient\\Light.dbc"));
    DbcTable colors(archives.read("DBFilesClient\\LightIntBand.dbc"));
    DbcTable floats(archives.read("DBFilesClient\\LightFloatBand.dbc"));
    if(lights.columns()!=12||colors.columns()!=34||floats.columns()!=34)throw std::runtime_error("Expected Vanilla 1.12 light DBC layouts");
    std::vector<WxLightVolume> volumes;std::map<uint32_t,WxLightProfile> profiles;std::set<uint32_t> maps;
    for(unsigned i=0;i<lights.size();i++){
        WxLightVolume v{};v.id=lights.value(i,0);v.map=lights.value(i,1);maps.insert(v.map);
        auto position=wowee::rendering::lightPositionToWorld(lights.real(i,2),lights.real(i,4),lights.real(i,3));
        for(unsigned k=0;k<3;k++)v.position[k]=position[k];
        v.inner=lights.real(i,5)/wowee::rendering::LIGHT_COORD_UNITS_PER_YARD;
        v.outer=lights.real(i,6)/wowee::rendering::LIGHT_COORD_UNITS_PER_YARD;
        for(unsigned k=0;k<3;k++){v.profile[k]=lights.value(i,7+k);if(v.profile[k])profiles[v.profile[k]].id=v.profile[k];}
        volumes.push_back(v);
    }
    unsigned reordered=0,empty=0;
    auto bands=[&](const DbcTable& dbc,unsigned channels){
        for(unsigned row=0;row<dbc.size();row++){
            auto slot=wowee::rendering::lightBandSlot(dbc.value(row,0),channels);
            auto p=profiles.find(slot.lightParamsId);if(!slot.valid||p==profiles.end())continue;
            const int color_bands[]={4,3,5,6,7,8,9,0,-1,10,-1,-1,-1,-1,-1,-1,-1,-1};
            int band=channels==18?color_bands[slot.channel]:(slot.channel<2?int(slot.channel)+1:-1);if(band<0)continue;
            auto& dest=p->second.bands[band];dest.count=dbc.value(row,1);if(dest.count>16)throw std::runtime_error("Too many light keys");
            std::vector<std::pair<uint16_t,uint32_t>> keys;
            for(unsigned k=0;k<dest.count;k++){
                unsigned time=dbc.value(row,2+k);if(time>=2880)throw std::runtime_error("Invalid light time");
                uint32_t value=dbc.value(row,18+k);
                // WoWee dbcColorToRGB ignores the high byte. Vanilla profile
                // 499's sun band actually contains FF in that byte.
                if(channels==18)value&=0xffffff;
                if(band==1){float yards=dbc.real(row,18+k)/wowee::rendering::LIGHT_COORD_UNITS_PER_YARD;memcpy(&value,&yards,4);}
                keys.emplace_back(uint16_t(time),value);
            }
            if(!std::is_sorted(keys.begin(),keys.end()))reordered++;
            std::sort(keys.begin(),keys.end());
            for(unsigned k=0;k<keys.size();k++){dest.time[k]=keys[k].first;dest.value[k]=keys[k].second;}
        }
    };
    bands(colors,wowee::rendering::LIGHT_INT_CHANNELS);bands(floats,wowee::rendering::LIGHT_FLOAT_CHANNELS);
    for(auto& [id,p]:profiles){
        if(!wx_light_profile_valid(&p))throw std::runtime_error("Invalid light profile "+std::to_string(id));
        for(unsigned b=0;b<3+WX_LIGHT_COLORS;b++)if(!p.bands[b].count)empty++;
    }
    std::sort(volumes.begin(),volumes.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    if(volumes.empty()||volumes.size()>WX_LIGHT_MAX_VOLUMES||profiles.size()>WX_LIGHT_MAX_PROFILES)throw std::runtime_error("Light catalog exceeds console limits");
    WxLightHeader h{0x314c5857,2,uint32_t(volumes.size()),uint32_t(profiles.size()),uint32_t(sizeof(WxLightHeader)+volumes.size()*sizeof(WxLightVolume)+profiles.size()*sizeof(WxLightProfile)),{}};
    const std::string temp=output+".tmp";
    std::ofstream out(temp,std::ios::binary|std::ios::trunc);out.write((const char*)&h,sizeof h);
    out.write((const char*)volumes.data(),volumes.size()*sizeof(WxLightVolume));
    for(const auto& [id,p]:profiles)out.write((const char*)&p,sizeof p);out.close();
    if(!out)throw std::runtime_error("Light catalog write failed");
    WxLighting catalog{};if(!wx_light_open(&catalog,temp.c_str()))throw std::runtime_error("Light catalog runtime validation failed");wx_light_close(&catalog);
    std::filesystem::copy_file(temp,output,std::filesystem::copy_options::overwrite_existing);std::filesystem::remove(temp);
    std::cout<<"Vanilla lighting v2: "<<volumes.size()<<" volumes, "<<maps.size()<<" maps, "<<profiles.size()<<" profiles, "<<h.bytes<<" bytes; "<<reordered<<" reordered bands, "<<empty<<" empty bands (explicit fallback).\n";
}
