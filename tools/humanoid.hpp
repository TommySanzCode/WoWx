#pragma once
// Vanilla NPC appearances. Uses WoWee's shared geoset and race naming rules.
#include "core/geoset_rules.hpp"
#include "core/helm_visual.hpp"
#include <unordered_set>

struct HumanoidLook {
    unsigned race=0,sex=0,skin=0,face=0,style=0,color=0,facial=0;
    std::array<unsigned,20> equipment{},inventoryType{};
};

struct HumanoidTables {
    DbcTable extra,sections,hair,facial,items,helmVisibility;
    explicit HumanoidTables(Archives& a):extra(a.read("DBFilesClient\\CreatureDisplayInfoExtra.dbc")),
        sections(a.read("DBFilesClient\\CharSections.dbc")),hair(a.read("DBFilesClient\\CharHairGeosets.dbc")),
        facial(a.read("DBFilesClient\\CharacterFacialHairStyles.dbc"),false),items(a.read("DBFilesClient\\ItemDisplayInfo.dbc")),
        helmVisibility(a.read("DBFilesClient\\HelmetGeosetVisData.dbc")) {
        // This cooker targets the supplied 5875 layout, not expansion overlays.
        if(extra.columns()!=19||sections.columns()!=10||items.columns()!=23||helmVisibility.columns()!=6)
            throw std::runtime_error("Unsupported humanoid DBC layout");
    }
};
static std::string withoutExtension(std::string name) {
    auto dot=name.find_last_of('.');if(dot!=std::string::npos)name.resize(dot);return name;
}
static std::string racialAsset(Archives& a,const std::string& directory,const std::string& name,
        const std::string& suffix,const char* extension) {
    if(name.empty())return {};
    const auto base=directory+withoutExtension(name);
    if(!suffix.empty()&&!a.read(base+suffix+extension,false).empty())return base+suffix+extension;
    if(!a.read(base+extension,false).empty())return base+extension;
    throw std::runtime_error("Missing equipment asset: "+base+extension);
}
static void attachEquipment(Archives& a,M2Model& body,const std::string& path,const std::string& texturePath,unsigned point) {
    if(path.empty())return;
    const M2Attachment* attachment=nullptr;
    for(const auto& at:body.attachments)if(at.id==point){attachment=&at;break;}
    if(!attachment||attachment->bone>=body.bones.size())throw std::runtime_error("Missing equipment attachment "+std::to_string(point));
    const auto anchor=*attachment;
    auto item=M2Loader::load(a.read(path));
    if(!item.isValid()||body.vertices.size()+item.vertices.size()>65535)throw std::runtime_error("Invalid attached equipment: "+path);
    unsigned vertexBase=body.vertices.size(),textureBase=body.textures.size(),lookupBase=body.textureLookup.size(),materialBase=body.materials.size();
    // Head/shoulder meshes are rigid in the owner's attachment frame. Their
    // vertices use that owner's bone, so the existing pose stream animates them.
    for(auto v:item.vertices){v.position+=anchor.position;memset(v.boneWeights,0,4);memset(v.boneIndices,0,4);
        v.boneWeights[0]=255;v.boneIndices[0]=uint8_t(anchor.bone);body.vertices.push_back(v);}
    for(auto tex:item.textures){if(tex.type||tex.filename.empty())tex.filename=texturePath;tex.type=0;
        if(tex.filename.empty())throw std::runtime_error("Unresolved attached equipment texture");body.textures.push_back(tex);}
    for(auto lookup:item.textureLookup){if(lookup>=item.textures.size())throw std::runtime_error("Invalid equipment texture lookup");body.textureLookup.push_back(textureBase+lookup);}
    body.materials.insert(body.materials.end(),item.materials.begin(),item.materials.end());
    for(auto batch:item.batches){if(!batch.indexCount||batch.submeshLevel)continue;
        if(batch.indexStart>item.indices.size()||batch.indexCount>item.indices.size()-batch.indexStart||
            batch.textureIndex>=item.textureLookup.size()||batch.materialIndex>=item.materials.size())throw std::runtime_error("Invalid equipment batch");
        unsigned first=body.indices.size();
        for(unsigned i=0;i<batch.indexCount;i++){auto index=item.indices[batch.indexStart+i];if(index>=item.vertices.size())throw std::runtime_error("Invalid equipment index");body.indices.push_back(vertexBase+index);}
        batch.indexStart=first;batch.vertexStart+=vertexBase;batch.textureIndex+=lookupBase;batch.materialIndex+=materialBase;
        body.batches.push_back(batch);
    }
    std::cout<<"  Attachment "<<point<<": "<<path<<'\n';
}
static void assembleLook(Archives& a,HumanoidTables& tables,M2Model& model,const HumanoidLook& look,
        const std::string& bodyTexture,bool modular=false) {
    using namespace wowee::core;
    unsigned race=look.race,sex=look.sex,style=look.style,color=look.color,face=look.facial;
    if(!race||race>8||sex>1)throw std::runtime_error("Invalid Vanilla humanoid appearance");
    texture(a,bodyTexture); // Fail preparation if the authored bake is unavailable.
    std::string hairTexture;unsigned scalp=1,f100=0,f200=0,f300=0;
    for(unsigned i=0;i<tables.sections.size();i++)if(tables.sections.value(i,1)==race&&tables.sections.value(i,2)==sex&&
        tables.sections.value(i,3)==3&&tables.sections.value(i,4)==style&&tables.sections.value(i,5)==color){hairTexture=tables.sections.text(i,6);break;}
    for(unsigned i=0;i<tables.hair.size();i++)if(tables.hair.value(i,1)==race&&tables.hair.value(i,2)==sex&&tables.hair.value(i,3)==style){scalp=tables.hair.value(i,4);if(!scalp)scalp=1;break;}
    for(unsigned i=0;i<tables.facial.size();i++)if(tables.facial.value(i,0)==race&&tables.facial.value(i,1)==sex&&tables.facial.value(i,2)==face){
        f100=tables.facial.value(i,6);f200=tables.facial.value(i,8);f300=tables.facial.value(i,7);break;}
    auto selected=bareCharacterGeosets(scalp,f100,f200,f300,race);
    selected.insert(kGeosetNoCape);
    // Extra.dbc's ten slots map to the standard character equipment slots.
    static constexpr unsigned slots[]={0,2,3,4,5,6,7,8,9,18};
    auto item=[&](unsigned slot){return look.equipment[slots[slot]];};
    auto group=[&](unsigned slot,unsigned field){auto id=item(slot);return id?tables.items.value(tables.items.row(id),field):0;};
    auto replace=[&](unsigned base,unsigned value){selected.erase(uint16_t(base));selected.insert(equippedGeoset(base,value));};
    replace(kGeosetBareForearms,group(8,6));replace(kGeosetBareShins,group(6,6));
    replace(kGeosetBareSleeves,std::max(group(3,6),group(2,6)));
    unsigned robe=group(3,8);replace(kGeosetBarePants,robe?robe:group(5,6));
    // 5875's tabard GG=1 selects 1202 (1201 is the absent variant).
    if(item(9)&&!robe)selected.insert(equippedGeoset(kGeosetDefaultTabard,group(9,6)));
    if(look.equipment[14]){selected.erase(kGeosetNoCape);selected.insert(kGeosetWithCape);}
    if(robe){for(auto it=selected.begin();it!=selected.end();)if(geosetGroup(*it)==5||geosetGroup(*it)==9)it=selected.erase(it);else ++it;}
    std::unordered_set<uint16_t> available,visible{0};for(const auto& b:model.batches)available.insert(b.submeshId);
    for(auto id:selected){auto resolved=resolveGeoset(id,available);if(resolved)visible.insert(resolved);}
    auto helm=item(0);
    if(helm){
        auto visibility=tables.items.value(tables.items.row(helm),12+sex);
        if(visibility){auto r=tables.helmVisibility.row(visibility);
            // Vanilla stores five masks: hair, facial groups 1/2/3, ears.
            // Bits address mesh variants, not CharSections variation indices.
            for(auto it=visible.begin();it!=visible.end();){unsigned group=geosetGroup(*it),variant=geosetVariant(*it);
                unsigned column=group<=3?group+1:group==7?5:0;
                if(*it&&column&&variant<32&&(tables.helmVisibility.value(r,column)&(1u<<variant)))it=visible.erase(it);else ++it;}
        }
    }
    if(!modular)model.batches.erase(std::remove_if(model.batches.begin(),model.batches.end(),[&](const M2Batch& b){return !visible.count(b.submeshId);}),model.batches.end());
    for(auto& tex:model.textures){if(tex.type==1){tex.filename=bodyTexture;tex.type=0;}else if(tex.type==6&&!hairTexture.empty()){tex.filename=hairTexture;tex.type=0;}}
    if(modular)for(auto& tex:model.textures)if(tex.type==6&&tex.filename.empty()){tex.filename=bodyTexture;tex.type=0;}
    // A bald scalp may be represented by a body-textured batch. There must be
    // no surviving hair-textured batch when CharSections supplies no hair.
    for(const auto& batch:model.batches){if(batch.textureIndex>=model.textureLookup.size())throw std::runtime_error("Invalid humanoid texture lookup");
        auto ti=model.textureLookup[batch.textureIndex];if(ti>=model.textures.size()||model.textures[ti].filename.empty())throw std::runtime_error("Unresolved humanoid texture/geoset "+std::to_string(batch.submeshId));}
    std::cout<<"Humanoid "<<race<<'/'<<sex<<": "<<bodyTexture<<", "<<model.batches.size()<<" selected body batches\n";
    if(modular)return;
    const auto suffix=raceGenderSuffix(race,sex);
    auto equipment=[&](unsigned slot,unsigned side,unsigned attachment,const std::string& directory){auto id=item(slot);if(!id)return;
        auto r=tables.items.row(id);auto name=tables.items.text(r,1+side);if(name.empty())return;
        auto path=racialAsset(a,directory,name,suffix,".m2");
        auto tex=racialAsset(a,directory,tables.items.text(r,3+side),suffix,".blp");
        attachEquipment(a,model,path,tex,attachment);};
    equipment(0,0,kAttachHelm,"Item\\ObjectComponents\\Head\\");
    equipment(1,0,5,"Item\\ObjectComponents\\Shoulder\\");
    equipment(1,1,6,"Item\\ObjectComponents\\Shoulder\\");
}
static void assembleHumanoid(Archives& a,HumanoidTables& tables,M2Model& model,unsigned extraId) {
    const auto& e=tables.extra;auto row=e.row(extraId);HumanoidLook look;
    look.race=e.value(row,1);look.sex=e.value(row,2);look.skin=e.value(row,3);look.face=e.value(row,4);
    look.style=e.value(row,5);look.color=e.value(row,6);look.facial=e.value(row,7);
    static constexpr unsigned slots[]={0,2,3,4,5,6,7,8,9,18};
    for(unsigned i=0;i<10;i++)look.equipment[slots[i]]=e.value(row,8+i);
    assembleLook(a,tables,model,look,"Textures\\BakedNpcTextures\\"+e.text(row,18));
}
