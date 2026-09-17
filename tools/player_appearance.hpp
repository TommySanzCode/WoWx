#pragma once
#include "character_atlas.hpp"
#include "core/character_paths.hpp"
#include "pipeline/item_textures.hpp"
#include <sstream>

// Human-readable preparation input: seven appearance numbers followed by twenty
// display-ID/inventory-type pairs, in SMSG_CHAR_ENUM slot order. No game assets.
static HumanoidLook readPlayerLook(const fs::path& path) {
    std::ifstream file(path);if(!file)throw std::runtime_error("Missing player appearance profile");
    std::string text,line;while(std::getline(file,line)){line.resize(line.find('#')==std::string::npos?line.size():line.find('#'));text+=line+' ';}
    std::istringstream input(text);HumanoidLook look;
    if(!(input>>look.race>>look.sex>>look.skin>>look.face>>look.style>>look.color>>look.facial)||
       !look.race||look.race>8||look.sex>1||look.skin>255||look.face>255||look.style>255||look.color>255||look.facial>255)
        throw std::runtime_error("Invalid Vanilla player appearance");
    for(unsigned i=0;i<20;i++)if(!(input>>look.equipment[i]>>look.inventoryType[i])||look.inventoryType[i]>28||
        (bool(look.equipment[i])!=bool(look.inventoryType[i])))throw std::runtime_error("Invalid player equipment profile");
    if(input>>line)throw std::runtime_error("Trailing player appearance data");return look;
}
static std::string playerModelPath(const HumanoidLook& look) {
    using namespace wowee::core;
    return std::string("Character\\")+raceModelFolder(look.race)+"\\"+sexModelFolder(look.sex)+"\\"+characterArtPrefix(look.race,look.sex)+".m2";
}
static void assemblePlayer(Archives& a,HumanoidTables& tables,M2Model& model,const HumanoidLook& look,bool modular=false) {
    std::vector<unsigned> textureTypes;for(const auto& tex:model.textures)textureTypes.push_back(tex.type);
    auto section=[&](unsigned kind,unsigned variation,unsigned color,bool required){
        std::array<std::string,3> paths{};
        for(unsigned i=0;i<tables.sections.size();i++)if(tables.sections.value(i,1)==look.race&&tables.sections.value(i,2)==look.sex&&
            tables.sections.value(i,3)==kind&&tables.sections.value(i,4)==variation&&tables.sections.value(i,5)==color&&!(tables.sections.value(i,9)&1)){
            for(unsigned j=0;j<3;j++)paths[j]=tables.sections.text(i,6+j);return paths;}
        if(required)throw std::runtime_error("Missing player CharSections row "+std::to_string(kind)+"/"+std::to_string(variation)+"/"+std::to_string(color));return paths;
    };
    auto skin=section(0,0,look.skin,true),face=section(1,look.face,look.skin,true);
    auto hair=section(3,look.style,look.color,false),facial=section(2,look.facial,look.color,false),underwear=section(4,0,look.skin,false);
    if(skin[0].empty())throw std::runtime_error("Missing player body texture");
    const auto& base=texture(a,skin[0]);BLPImage body;body.width=body.height=256;body.data.resize(256*256*4);
    wx_atlas::blend(body.data,base.data,base.width,base.height,{0,0,256,256},false);
    auto overlay=[&](const std::string& path,wx_atlas::Region region){if(path.empty())return;
        const auto& im=texture(a,path);wx_atlas::blend(body.data,im.data,im.width,im.height,region);};
    auto namedOverlay=[&](const std::string& path){if(path.empty())return;
        auto lower=path;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(lower.find("facelower")!=std::string::npos||lower.find("faciallower")!=std::string::npos||lower.find("scalplower")!=std::string::npos)overlay(path,{0,192,128,64});
        else if(lower.find("faceupper")!=std::string::npos||lower.find("facialupper")!=std::string::npos||lower.find("scalpupper")!=std::string::npos)overlay(path,{0,160,128,32});
        else if(lower.find("pelvis")!=std::string::npos)overlay(path,wx_atlas::regions[5]);
        else if(lower.find("torso")!=std::string::npos)overlay(path,wx_atlas::regions[3]);
        else throw std::runtime_error("Unknown character overlay region: "+path);
    };
    for(const auto& path:face)namedOverlay(path);
    for(const auto& path:underwear)if(!path.empty()&&!a.read(path,false).empty())namedOverlay(path);
    auto detailOverlay=[&](const std::string& path){if(path.empty())return;
        // Shipped Tauren rows name optional facial/scalp overlays that are not
        // present in the archives. Keep the base face and report that omission.
        if(a.read(path,false).empty()){std::cout<<"Optional character overlay absent: "<<path<<'\n';return;}namedOverlay(path);};
    for(const auto& path:facial)detailOverlay(path);
    // The hair row's remaining textures paint scalp details onto the face atlas.
    detailOverlay(hair[1]);detailOverlay(hair[2]);
    // Draw clothing from inside to outside; gloves and boots cover sleeves/legs.
    // Robes use their authored leg regions after pants and before belts/tabards.
    static constexpr unsigned order[]={3,6,4,18,5,8,9,7};
    for(auto slot:order){auto id=look.equipment[slot];if(!id||modular)continue;auto r=tables.items.row(id);
        for(unsigned region=0;region<8;region++){
            auto name=tables.items.text(r,14+region);if(name.empty())continue;
            std::string basePath=std::string("Item\\TextureComponents\\")+wowee::pipeline::itemComponentDir(region)+"\\"+withoutExtension(name),path;
            for(const auto& suffix:{look.sex?"_F.blp":"_M.blp","_U.blp",".blp"})if(!a.read(basePath+suffix,false).empty()){path=basePath+suffix;break;}
            if(path.empty())throw std::runtime_error("Missing item body texture: "+basePath);
            overlay(path,wx_atlas::regions[region]);
        }
    }
    const std::string bodyKey="__player_body__";images[bodyKey]=std::move(body);
    if(modular){BLPImage blank;blank.width=blank.height=1;blank.data={255,255,255,255};images["__player_cape__"]=blank;
        for(auto& tex:model.textures)if(tex.type==2){tex.filename="__player_cape__";tex.type=0;}}
    if(look.equipment[14]){
        auto row=tables.items.row(look.equipment[14]);std::string cape;
        for(unsigned column:{look.sex?4u:3u,look.sex?3u:4u})for(const auto& path:wowee::pipeline::capeTextureCandidates(tables.items.text(row,column),look.sex==1))
            if(cape.empty()&&!a.read(path,false).empty())cape=path;
        if(cape.empty())throw std::runtime_error("Missing player cape texture");
        for(auto& tex:model.textures)if(tex.type==2){tex.filename=cape;tex.type=0;}
    }
    for(auto& tex:model.textures)if(tex.type==8){tex.filename=skin[1].empty()?bodyKey:skin[1];tex.type=0;}
    assembleLook(a,tables,model,look,bodyKey,modular);
    if(modular){
        for(unsigned i=0;i<textureTypes.size();i++)if(textureTypes[i]==6||textureTypes[i]==8){
            const char* key=textureTypes[i]==6?"__player_hair__":"__player_extra__";
            images[key]=texture(a,model.textures[i].filename);model.textures[i].filename=key;
        }return;
    }
    // Weapons stay in the hand for this first appearance path. Sheathed variants
    // and per-weapon animation grip selection remain runtime integration work.
    for(unsigned slot:{15u,16u,17u})if(look.equipment[slot]){
        auto row=tables.items.row(look.equipment[slot]);auto name=tables.items.text(row,1),tex=tables.items.text(row,3);
        if(name.empty()){name=tables.items.text(row,2);tex=tables.items.text(row,4);}
        if(name.empty())throw std::runtime_error("Missing player weapon model");
        bool shield=look.inventoryType[slot]==14;
        const std::string dir=shield?"Item\\ObjectComponents\\Shield\\":"Item\\ObjectComponents\\Weapon\\";
        unsigned point=shield?0:slot==16?2:1;
        // Ranged equipment is stored but not drawn over the equipped main hand.
        if(slot!=17)attachEquipment(a,model,racialAsset(a,dir,name,"",".m2"),racialAsset(a,dir,tex,"",".blp"),point);
    }
}
