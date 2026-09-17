#pragma once
#include "wx_portrait.h"
// Reuse WoWee's model parser and portrait framing: authored camera for creatures,
// head-key-bone for players. No character pixels or appearance are cached here.
static WxPortraitEntry portraitEntry(const M2Model& model,uint32_t key,bool player){
    if(!model.isValid()||model.vertices.empty())throw std::runtime_error("Invalid portrait model");
    WxPortraitEntry e{};e.key=key;e.fov=.785398163f;e.near_clip=.01f;e.far_clip=1000;
    if(!player)for(const auto& cam:model.cameras)if(cam.type==0){
        e.source=1;
        for(unsigned i=0;i<3;i++){e.target[i]=cam.targetBase[i];e.camera[i]=cam.targetBase[i]+1.35f*(cam.positionBase[i]-cam.targetBase[i]);}
        if(cam.fov>.01f&&cam.fov<3.1f)e.fov=cam.fov;
        if(wx_portrait_valid(&e))return e;
    }
    float low=model.vertices[0].position.z,high=low;
    for(const auto& v:model.vertices){low=std::min(low,v.position.z);high=std::max(high,v.position.z);}
    float height=std::max(high-low,.1f);glm::vec3 focus(0,0,low+height*.82f);e.source=3;e.fov=.785398163f;
    // Vanilla's standing animation can move the head substantially from its bind
    // pivot (notably the hunched races). Use the same sampled skeleton as WXP.
    namespace sample=wowee::rendering::m2_track;
    int sequence=-1;for(unsigned i=0;i<model.sequences.size();i++)if(model.sequences[i].id==0&&model.sequences[i].duration){sequence=int(i);break;}
    std::vector<glm::mat4> pose(model.bones.size(),glm::mat4(1));
    for(unsigned i=0;i<model.bones.size();i++){
        const auto& b=model.bones[i];if(b.parentBone>=int(i)||b.parentBone < -1)throw std::runtime_error("Invalid portrait bone hierarchy");
        if(sequence>=0){auto tr=sample::sampleVec3(b.translation,sequence,0,0,model.globalSequenceDurations,glm::vec3(0));
            auto rot=sample::sampleQuat(b.rotation,sequence,0,0,model.globalSequenceDurations);
            auto scale=sample::sampleVec3(b.scale,sequence,0,0,model.globalSequenceDurations,glm::vec3(1));
            auto m=glm::translate(glm::mat4(1),b.pivot+tr)*glm::mat4_cast(rot)*glm::scale(glm::mat4(1),scale)*glm::translate(glm::mat4(1),-b.pivot);
            pose[i]=b.parentBone<0?m:pose[b.parentBone]*m;}
        if(b.keyBoneId==6&&std::isfinite(b.pivot.z)&&b.pivot.z>.001f){focus=glm::vec3(pose[i]*glm::vec4(b.pivot,1));e.source=2;}
    }
    for(unsigned i=0;i<3;i++)e.target[i]=focus[i];
    // Raw M2s face +X. WoWee rotates that axis onto its +Y preview camera.
    e.camera[0]=focus.x+std::max(1.0f,height*.50f);e.camera[1]=focus.y;e.camera[2]=focus.z;
    if(!wx_portrait_valid(&e))throw std::runtime_error("Invalid portrait framing");return e;
}
static void preparePortraits(Archives& a,const fs::path& output,const char* listPath){
    std::map<uint32_t,WxPortraitEntry> entries;
    for(unsigned race=1;race<=8;race++)for(unsigned sex=0;sex<2;sex++){
        HumanoidLook look{};look.race=race;look.sex=sex;auto path=playerModelPath(look);
        unsigned key=WX_PORTRAIT_PLAYER|(race<<1)|sex;entries.emplace(key,portraitEntry(M2Loader::load(a.read(path)),key,true));
    }
    DbcTable displays(a.read("DBFilesClient\\CreatureDisplayInfo.dbc")),models(a.read("DBFilesClient\\CreatureModelData.dbc"));
    std::ifstream list(listPath);if(!list)throw std::runtime_error("Missing portrait display list");std::string line;
    while(std::getline(list,line)){
        if(line.empty()||line[0]=='#')continue;size_t used=0;unsigned key=std::stoul(line,&used);
        if(used!=line.size()||!key||key>0xffffff||entries.count(key))throw std::runtime_error("Invalid portrait display list");
        auto row=displays.row(key);auto path=modelPathToM2(models.text(models.row(displays.value(row,1)),2));
        entries.emplace(key,portraitEntry(M2Loader::load(a.read(path)),key,false));
    }
    if(entries.size()>WX_PORTRAIT_LIMIT)throw std::runtime_error("Portrait catalog exceeds bounded resident limit");
    auto temporary=output;temporary+=".tmp";std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
    uint32_t header[]={0x54505857,1,uint32_t(entries.size()),sizeof(WxPortraitEntry)};out.write((char*)header,sizeof header);
    unsigned sources[4]={0};for(const auto& [key,e]:entries){out.write((const char*)&e,sizeof e);sources[e.source]++;}
    out.close();if(!out)throw std::runtime_error("Portrait catalog write failed");fs::rename(temporary,output);
    std::cout<<"Portraits: "<<entries.size()<<" entries, cameras="<<sources[1]<<", head bones="<<sources[2]<<", bounds="<<sources[3]<<'\n';
}
