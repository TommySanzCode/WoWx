#pragma once
#include "wx_icons.h"
static void prepareIcons(Archives& archives,const fs::path& output){
    if(fs::exists(output))throw std::runtime_error("Use a new icon catalog output");
    DbcTable spells(archives.read("DBFilesClient\\Spell.dbc")),names(archives.read("DBFilesClient\\SpellIcon.dbc")),items(archives.read("DBFilesClient\\ItemDisplayInfo.dbc"));
    if(spells.columns()!=173||names.columns()!=2||items.columns()<6)throw std::runtime_error("Unexpected Vanilla icon DBC layout");
    std::map<unsigned,std::string> spellPaths;for(unsigned i=0;i<names.size();i++)spellPaths[names.value(i,0)]=names.text(i,1);
    std::map<std::string,unsigned> textures;std::map<unsigned,unsigned> index;std::vector<std::array<uint32_t,1024>> images;std::set<std::string> missing;
    auto image=[&](std::string path)->unsigned {
        if(path.empty())return 0;
        std::replace(path.begin(),path.end(),'/','\\');for(char& c:path)c=(char)std::tolower((unsigned char)c);
        if(path.find('\\')==std::string::npos)path="interface\\icons\\"+path;
        if(path.ends_with(".tga"))path.resize(path.size()-4);
        if(!path.ends_with(".blp"))path+=".blp";
        if(auto it=textures.find(path);it!=textures.end())return it->second;
        auto bytes=archives.read(path,false);if(bytes.empty()){
            if(images.empty())throw std::runtime_error("Missing fallback icon");missing.insert(path);textures[path]=0;return 0;
        }
        auto source=BLPLoader::load(bytes);
        if(!source.isValid()||!source.width||!source.height||source.width>1024||source.height>1024||source.data.size()!=size_t(source.width)*source.height*4)throw std::runtime_error("Malformed icon: "+path);
        std::array<uint32_t,1024> pixels{};
        for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
            uint64_t channels[3]={0},alpha=0,count=0;
            unsigned left=x*source.width/32,right=std::max(left+1,(x+1)*source.width/32),top=y*source.height/32,bottom=std::max(top+1,(y+1)*source.height/32);
            for(unsigned sy=top;sy<bottom;sy++)for(unsigned sx=left;sx<right;sx++){
                const uint8_t* p=source.data.data()+(sy*source.width+sx)*4;for(unsigned c=0;c<3;c++)channels[c]+=unsigned(p[c])*p[3];alpha+=p[3];count++;
            }
            uint32_t color=uint32_t(alpha/count)<<24;if(alpha)for(unsigned c=0;c<3;c++)color|=uint32_t(channels[c]/alpha)<<((2-c)*8);
            unsigned at=0;for(unsigned b=0;b<5;b++)at|=((x>>b)&1)<<(2*b)|((y>>b)&1)<<(2*b+1);pixels[at]=color;
        }
        if(images.size()==WX_ICON_IMAGE_LIMIT)throw std::runtime_error("Icon image limit exceeded");
        unsigned id=images.size();images.push_back(pixels);textures[path]=id;return id;
    };
    index[0]=image("Interface\\Icons\\INV_Misc_QuestionMark.blp");
    for(unsigned i=0;i<spells.size();i++){
        unsigned id=spells.value(i,0);if(!id||id>65535)throw std::runtime_error("Invalid Vanilla spell icon key");
        auto found=spellPaths.find(spells.value(i,117));index[id]=found==spellPaths.end()?0:image(found->second);
    }
    for(unsigned i=0;i<items.size();i++){
        unsigned id=items.value(i,0);if(!id)continue;if(id>0xffffff)throw std::runtime_error("Invalid item display icon key");
        index[WX_ICON_ITEM|id]=image(items.text(i,5));
    }
    if(index.size()>WX_ICON_INDEX_LIMIT)throw std::runtime_error("Icon index exceeds bounded resident limit");
    auto temporary=output;temporary+=".tmp";std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
    uint32_t offset=24+uint32_t(index.size())*8,header[]={0x43495857,1,uint32_t(index.size()),uint32_t(images.size()),offset,offset+uint32_t(images.size())*4096};
    out.write((char*)header,sizeof header);for(const auto& [key,icon]:index){uint32_t row[]={key,icon};out.write((char*)row,sizeof row);}
    for(const auto& pixels:images)out.write((const char*)pixels.data(),4096);out.close();if(!out)throw std::runtime_error("Icon pack write failed");fs::rename(temporary,output);
    std::ofstream report(output.string()+".missing.txt");for(const auto& path:missing)report<<path<<'\n';
    std::cout<<"Icons: "<<index.size()<<" mappings, "<<images.size()<<" unique images, "<<header[5]<<" bytes; "<<missing.size()<<" missing source paths use original question mark\n";
}
