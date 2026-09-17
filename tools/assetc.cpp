// Xbox asset preparation using the pinned WoWee parsers; see NOTICE.md.
#include <StormLib.h>
#include "wx_pack.h"
#include "wx_avatar.h"
#include "dbc_table.hpp"
#include "spell_catalog.hpp"
#include "cooldown_cooker.hpp"
#include "world_catalog.hpp"
#include "pipeline/wdt_loader.hpp"
#include "pipeline/adt_loader.hpp"
#include "pipeline/terrain_mesh.hpp"
#include "pipeline/blp_loader.hpp"
#include "pipeline/m2_loader.hpp"
#include "pipeline/wmo_loader.hpp"
#include "rendering/m2_track_sampler.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <tuple>
#include <set>
#include <glm/gtc/matrix_transform.hpp>
#include "world_transform.hpp"
#include "world_environment.hpp"
#include "terrain_liquid.hpp"
using namespace wowee::pipeline;
namespace fs = std::filesystem;

struct Archives {
    std::vector<HANDLE> handles;
    std::map<std::string,unsigned> members;
    explicit Archives(const fs::path& dir) {
        // Vanilla archives are replacement overlays, in ascending patch order.
        const char* names[]={"base.MPQ","dbc.MPQ","fonts.MPQ","interface.MPQ","misc.MPQ",
          "model.MPQ","sound.MPQ","speech.MPQ","terrain.MPQ","texture.MPQ","wmo.MPQ","patch.MPQ","patch-2.MPQ"};
        for(auto name:names) { HANDLE h=nullptr; auto p=(dir/name).string();
            if(!SFileOpenArchive(p.c_str(),0,MPQ_OPEN_READ_ONLY,&h)) throw std::runtime_error("Cannot open archive: "+p);
            handles.push_back(h);
        }
        std::cout << "Validated " << handles.size() << " MPQ archive headers/tables\n";
    }
    ~Archives() { for(auto h:handles) SFileCloseArchive(h); }
    std::vector<uint8_t> read(const std::string& name, bool required=true) {
        for(auto it=handles.rbegin();it!=handles.rend();++it) {
            HANDLE f=nullptr; if(!SFileOpenFileEx(*it,name.c_str(),SFILE_OPEN_FROM_MPQ,&f)) continue;
            DWORD high=0, n=SFileGetFileSize(f,&high), actual=0;
            if(high||n>128*1024*1024) { SFileCloseFile(f); throw std::runtime_error("Asset exceeds host input limit: "+name); }
            std::vector<uint8_t> bytes(n);
            bool ok=SFileReadFile(f,bytes.data(),n,&actual,nullptr); SFileCloseFile(f);
            if(!ok||actual!=n) throw std::runtime_error("Corrupt/truncated MPQ member: "+name);
            members[name]=n;
            return bytes;
        }
        if(required) throw std::runtime_error("Missing asset: "+name);
        return {};
    }
};
#include "world_material.hpp"
#include "world_vertex_lighting.hpp"
#include "material_motion_cooker.hpp"
struct Part {
    WxEntry e{};
    std::string sourceName;
    std::string textureName;
    unsigned textureRole=0;
    std::vector<WxVertex> v;
    std::vector<uint16_t> i;
    std::vector<uint32_t> tex;
    std::vector<uint32_t> vertexColors;
    WxAnimation animation{};
    std::vector<WxSkinVertex> skin;
    std::vector<float> poses;
    WxMaterialMotion materialMotion{};
};
static std::map<std::string,BLPImage> images;
static const BLPImage& texture(Archives& a,const std::string& name) {
    auto it=images.find(name); if(it!=images.end()) return it->second;
    auto im=BLPLoader::load(a.read(name));
    if(!im.isValid()||im.data.size()!=size_t(im.width)*im.height*4) throw std::runtime_error("Texture decode failed: "+name);
    return images.emplace(name,std::move(im)).first->second;
}
static std::array<float,4> sample(const BLPImage& im,float u,float v) {
    u-=floorf(u); v-=floorf(v);
    unsigned x=std::min(unsigned(u*im.width),unsigned(im.width-1));
    unsigned y=std::min(unsigned(v*im.height),unsigned(im.height-1));
    const auto* p=&im.data[(y*im.width+x)*4];
    return {float(p[0]),float(p[1]),float(p[2]),float(p[3])};
}
static uint32_t bgra(const std::array<float,4>& c) {
    return (uint32_t(std::clamp(c[3],0.f,255.f))<<24)|(uint32_t(std::clamp(c[0],0.f,255.f))<<16)|
           (uint32_t(std::clamp(c[1],0.f,255.f))<<8)|uint32_t(std::clamp(c[2],0.f,255.f));
}
static void bounds(Part& p) {
    if(p.v.empty()) throw std::runtime_error("Empty mesh");
    float lo[3]={1e9f,1e9f,1e9f},hi[3]={-1e9f,-1e9f,-1e9f};
    for(auto& v:p.v){
        unsigned field=0;for(float component:{v.p[0],v.p[1],v.p[2],v.n[0],v.n[1],v.n[2],v.uv[0],v.uv[1]}){
            if(!std::isfinite(component))throw std::runtime_error("Invalid vertex component "+std::to_string(field)+" at "+std::to_string(&v-p.v.data())+" from "+p.sourceName+" texture "+p.textureName);
            field++;
        }
        for(int k=0;k<3;++k){lo[k]=std::min(lo[k],v.p[k]);hi[k]=std::max(hi[k],v.p[k]);}
    }
    for(int k=0;k<3;++k)p.e.center[k]=(lo[k]+hi[k])*.5f;
    float r=0;for(int k=0;k<3;++k)r+=(hi[k]-lo[k])*(hi[k]-lo[k]);p.e.radius=sqrtf(r)*.5f;
    p.e.vertex_count=uint32_t(p.v.size());p.e.index_count=uint32_t(p.i.size());
    for(auto i:p.i)if(i>=p.v.size())throw std::runtime_error("Out of range mesh index");
    if(p.e.flags&WX_ANIMATED) {
        // Every baked bone pose contributes to conservative world-space bounds.
        float al[3]={1e9f,1e9f,1e9f},ah[3]={-1e9f,-1e9f,-1e9f};
        for(unsigned f=0;f<p.animation.frames;f++)for(size_t i=0;i<p.v.size();i++) {
            float xyz[3]={};unsigned sum=0;
            for(int k=0;k<4;k++)sum+=p.skin[i].weights[k];
            if(!sum)throw std::runtime_error("Unweighted animated vertex");
            for(int k=0;k<4;k++)if(p.skin[i].weights[k]) {
                auto* m=&p.poses[(f*p.animation.bones+p.skin[i].bones[k])*12];
                float w=float(p.skin[i].weights[k])/sum;
                for(int j=0;j<3;j++)xyz[j]+=w*(m[j*4]*p.v[i].p[0]+m[j*4+1]*p.v[i].p[1]+m[j*4+2]*p.v[i].p[2]+m[j*4+3]);
            }
            for(int j=0;j<3;j++){al[j]=std::min(al[j],xyz[j]);ah[j]=std::max(ah[j],xyz[j]);}
        }
        float r2=0;for(int j=0;j<3;j++){p.e.center[j]=(al[j]+ah[j])*.5f;r2+=(ah[j]-al[j])*(ah[j]-al[j]);}p.e.radius=sqrtf(r2)*.5f;
    }
}
static void swizzle(Part& p) {
    // Each mip is an independent Morton square, packed largest to smallest.
    auto linear=p.tex;p.tex.clear();p.e.reserved[1]=0;
    for(unsigned size=p.e.width;size;size/=2){
        size_t base=p.tex.size();p.tex.resize(base+size*size);p.e.reserved[1]++;
        for(unsigned y=0;y<size;y++)for(unsigned x=0;x<size;x++){
            unsigned address=0;for(unsigned bit=0;(1u<<bit)<size;bit++)address|=((x>>bit)&1)<<(bit*2)|((y>>bit)&1)<<(bit*2+1);
            p.tex[base+address]=linear[y*size+x];
        }
        if(size==1)break;
        unsigned next=size/2;std::vector<uint32_t> reduced(next*next);
        for(unsigned y=0;y<next;y++)for(unsigned x=0;x<next;x++){
            uint32_t pixel=0;for(unsigned channel=0;channel<4;channel++){
                unsigned sum=0;for(unsigned j=0;j<2;j++)for(unsigned i=0;i<2;i++)sum+=(linear[(y*2+j)*size+x*2+i]>>(channel*8))&255;
                pixel|=((sum+2)/4)<<(channel*8);
            }reduced[y*next+x]=pixel;
        }linear=std::move(reduced);
    }
    p.e.flags|=WX_TEX_SWIZZLED|WX_MIPMAPPED;
}
static void modelTexture(Archives& a,Part& p,const std::string& path){
    p.textureName=path;
    p.textureRole=path=="__player_body__"?1:path=="__player_hair__"?2:path=="__player_extra__"?3:0;
    const auto& im=texture(a,path);p.e.width=p.e.height=128;p.tex.resize(128*128);
    for(unsigned y=0;y<128;y++)for(unsigned x=0;x<128;x++)p.tex[y*128+x]=bgra(sample(im,(x+.5f)/128,(y+.5f)/128));
    swizzle(p);
}
static glm::mat4 placement(const float* pos,const float* rot,float scale){
    auto m=glm::translate(glm::mat4(1),wowee::core::coords::adtToWorld(pos[0],pos[1],pos[2]));
    m=glm::rotate(m,glm::radians(rot[1]+180),glm::vec3(0,0,1));
    m=glm::rotate(m,glm::radians(rot[0]),glm::vec3(0,1,0));
    m=glm::rotate(m,glm::radians(rot[2]),glm::vec3(1,0,0));
    return glm::scale(m,glm::vec3(scale));
}
static WxVertex vertex(glm::vec3 pos,glm::vec3 normal,glm::vec2 uv,const glm::mat4& m){
    WxVertex v{};pos=glm::vec3(m*glm::vec4(pos,1));normal=glm::mat3(m)*normal;
    float length=glm::length(normal);
    // Zero normals occur on authored degenerate/hidden M2 vertices. Give these
    // a finite lighting normal; do not normalize zero into NaN.
    normal=length>1e-8f?normal/length:glm::vec3(0,0,1);
    for(int k=0;k<3;k++){v.p[k]=pos[k];v.n[k]=normal[k];}v.uv[0]=uv.x;v.uv[1]=uv.y;return v;
}
static void m2(Archives& a,std::vector<Part>& parts,const std::string& path,const glm::mat4& m,std::set<std::string>* effectOnly=nullptr,std::set<std::string>* extraUV=nullptr){
    auto bytes=a.read(modelPathToM2(path));auto model=M2Loader::load(bytes);
    // Some legitimate Vanilla doodads consist entirely of particle emitters.
    // Preserve an explicit missing-effect dependency instead of inventing a mesh.
    if(effectOnly&&model.version==256&&model.vertices.empty()&&model.indices.empty()&&bytes.size()>=0x144){
        uint32_t count,offset;memcpy(&count,bytes.data()+0x13c,4);memcpy(&offset,bytes.data()+0x140,4);
        if(count&&count<=256&&offset<=bytes.size()&&count<=(bytes.size()-offset)/504){
            effectOnly->insert(modelPathToM2(path));return;
        }
    }
    if(!model.isValid())throw std::runtime_error("M2 invalid: "+path);
    VanillaScene vanilla(bytes);unsigned idle=0;
    for(unsigned i=0;i<model.sequences.size();i++)if(model.sequences[i].id==0){idle=i;break;}
    if(!model.collisionVertices.empty()&&!model.collisionIndices.empty()){
        Part p;p.sourceName=path;p.e.kind=WX_KIND_COLLISION;p.e.width=p.e.height=1;p.tex={0xffffffff};
        for(auto& v:model.collisionVertices)p.v.push_back(vertex(v,{0,0,1},{0,0},m));p.i=model.collisionIndices;
        parts.push_back(std::move(p));
    }
    for(const auto& batch:model.batches){
        if(!batch.indexCount||batch.submeshLevel>0)continue;
        if(batch.indexStart>model.indices.size()||batch.indexCount>model.indices.size()-batch.indexStart)throw std::runtime_error("M2 batch invalid");
        if(batch.textureIndex>=model.textureLookup.size())continue;
        auto ti=model.textureLookup[batch.textureIndex];if(ti>=model.textures.size()||model.textures[ti].filename.empty())continue;
        Part p;p.sourceName=path;p.e.kind=WX_KIND_STATIC;
        if(batch.materialIndex>=model.materials.size())throw std::runtime_error("M2 material invalid: "+path);
        p.e.flags|=worldMaterial(model.materials[batch.materialIndex].blendMode,model.materials[batch.materialIndex].flags,true);
        if(!(model.textures[ti].flags&1))p.e.flags|=WX_CLAMP_U;
        if(!(model.textures[ti].flags&2))p.e.flags|=WX_CLAMP_V;
        try{p.materialMotion=materialMotion(model,vanilla,batch,idle);}
        catch(const std::runtime_error& error){throw std::runtime_error(std::string(error.what())+" in "+path);}
        if(materialMotionNeeded(p.materialMotion))p.e.flags|=WX_MATERIAL_MOTION;
        if(extraUV&&batch.textureAnimIndex<model.textureTransformLookup.size()){
            unsigned index=model.textureTransformLookup[batch.textureAnimIndex];
            if(index<model.textureTransforms.size()){const auto& transform=model.textureTransforms[index];
                for(const auto& seq:transform.rotation.sequences)for(const auto& q:seq.quatValues)
                    if(fabsf(q.x)+fabsf(q.y)+fabsf(q.z)>.00001f)extraUV->insert(path);
                for(const auto& seq:transform.scale.sequences)for(const auto& s:seq.vec3Values)
                    if(glm::length(s-glm::vec3(1))>.00001f)extraUV->insert(path);
            }
        }
        std::map<unsigned,uint16_t> remap;
        for(unsigned i=0;i<batch.indexCount;i++){unsigned old=model.indices[batch.indexStart+i];if(old>=model.vertices.size())throw std::runtime_error("M2 index invalid");
            if(!remap.count(old)){auto& v=model.vertices[old];remap[old]=uint16_t(p.v.size());p.v.push_back(vertex(v.position,v.normal,v.texCoords[0],m));}p.i.push_back(remap.at(old));}
        modelTexture(a,p,model.textures[ti].filename);parts.push_back(std::move(p));
    }
}
#include "world_liquid_cooker.hpp"
struct WorldModelCoverage {unsigned groups=0,doodads=0,liquidGroups=0,lights=0,portals=0,fogs=0,complexMaterials=0;std::set<std::string> effectOnly,extraUV;};
static void wmo(Archives& a,std::vector<Part>& parts,const std::string& path,const ADTTerrain::WMOPlacement& instance,WorldModelCoverage* coverage=nullptr,WorldEnvironment* environment=nullptr){
    size_t environmentFirst=environment?environment->items.size():0;
    auto rootBytes=a.read(path);auto model=WMOLoader::load(rootBytes);auto rootEnvironment=WorldEnvironment::root(rootBytes);auto m=coverage?globalWorldPlacement(instance.position,instance.rotation):placement(instance.position,instance.rotation,1);
    if(!model.isValid()||model.nGroups!=model.groups.size()||model.nGroups>4096)throw std::runtime_error("Invalid WMO root: "+path);
    if(coverage){coverage->groups=model.nGroups;coverage->lights=(unsigned)model.lights.size();coverage->portals=(unsigned)model.portals.size();coverage->fogs=(unsigned)model.fogs.size();
        for(const auto& material:model.materials)if(material.shader)coverage->complexMaterials++;}
    for(unsigned g=0;g<model.nGroups;g++){
        char suffix[32];snprintf(suffix,sizeof suffix,"_%03u.wmo",g);
        auto groupPath=path.substr(0,path.size()-4)+suffix;
        auto groupBytes=a.read(groupPath);
        if(!WMOLoader::loadGroup(groupBytes,model,g))throw std::runtime_error("Invalid WMO group: "+groupPath);
        const auto& group=model.groups[g];
        if(environment)try{environment->add(group,groupBytes,m,rootEnvironment.first,rootEnvironment.second);}
        catch(const std::exception& e){throw std::runtime_error(groupPath+": "+e.what());}
        bool hasColors=validateWmoVertexColors(groupBytes,(unsigned)group.vertices.size());
        bool indoor=!!(group.flags&0x2000);
        if(coverage&&group.liquid.hasLiquid())coverage->liquidGroups++;
        if(!group.indices.empty()){
            Part p;p.sourceName=groupPath;p.e.kind=WX_KIND_COLLISION;p.e.width=p.e.height=1;p.tex={0xffffffff};
            for(auto& v:group.vertices)p.v.push_back(vertex(v.position,{0,0,1},{0,0},m));
            for(unsigned i=0;i+2<group.indices.size();i+=3){
                unsigned flags=i/3<group.triFlags.size()?group.triFlags[i/3]:0;
                if((flags&8)||((flags&0x20)&&!(flags&4))){p.i.push_back(group.indices[i]);p.i.push_back(group.indices[i+1]);p.i.push_back(group.indices[i+2]);}
            }
            if(!p.i.empty())parts.push_back(std::move(p));
        }
        for(auto& batch:group.batches){
            if(!batch.indexCount)continue;
            if(batch.materialId>=model.materials.size()||batch.startIndex>group.indices.size()||batch.indexCount>group.indices.size()-batch.startIndex)throw std::runtime_error("Invalid WMO batch");
            const auto& material=model.materials[batch.materialId];auto ti=model.textureOffsetToIndex.find(material.texture1);
            if(ti==model.textureOffsetToIndex.end()||ti->second>=model.textures.size())throw std::runtime_error("Invalid WMO material texture");
            Part p;p.sourceName=groupPath;p.e.kind=WX_KIND_STATIC;p.e.flags|=worldMaterial(material.blendMode,material.flags,false);std::map<unsigned,uint16_t> remap;
            if(indoor)p.e.flags|=WX_BAKED_LIGHT;
            if(hasColors)p.e.flags|=WX_VERTEX_COLOR;
            if(material.flags&0x40)p.e.flags|=WX_CLAMP_U;
            if(material.flags&0x80)p.e.flags|=WX_CLAMP_V;
            for(unsigned i=0;i<batch.indexCount;i++){auto old=group.indices[batch.startIndex+i];if(old>=group.vertices.size())throw std::runtime_error("WMO index invalid");
                if(!remap.count(old)){auto& v=group.vertices[old];remap[old]=uint16_t(p.v.size());p.v.push_back(vertex(v.position,v.normal,v.texCoord,m));
                    if(hasColors)p.vertexColors.push_back(worldVertexLighting(v.color,model.ambientColor,indoor,!!(material.flags&1)));}p.i.push_back(remap.at(old));}
            modelTexture(a,p,model.textures[ti->second]);parts.push_back(std::move(p));
        }
    }
    for(unsigned set=0;set<model.doodadSets.size();set++){if(set!=0&&set!=instance.doodadSet)continue;const auto& s=model.doodadSets[set];
        if(coverage&&(s.startIndex>model.doodads.size()||s.count>model.doodads.size()-s.startIndex))throw std::runtime_error("Invalid WMO doodad set range");
        for(unsigned i=0;i<s.count&&s.startIndex+i<model.doodads.size();i++){const auto& d=model.doodads[s.startIndex+i];auto name=model.doodadNames.find(d.nameIndex);if(name==model.doodadNames.end()){if(coverage)throw std::runtime_error("Missing WMO doodad name");continue;}
            if(coverage)coverage->doodads++;
            auto dm=m*glm::translate(glm::mat4(1),d.position)*glm::mat4_cast(d.rotation)*glm::scale(glm::mat4(1),glm::vec3(d.scale));m2(a,parts,name->second,dm,coverage?&coverage->effectOnly:nullptr,coverage?&coverage->extraUV:nullptr);}}
    if(environment)worldLiquidParts(a,parts,*environment,environmentFirst,path);
}
#include "humanoid.hpp"
#include "player_appearance.hpp"
static void animatedCreature(Archives& a,std::vector<Part>& parts,const std::string& path,
        const std::array<std::string,3>& skins,float scale,uint32_t display,unsigned animationId,HumanoidTables* humanoids=nullptr,unsigned extraId=0,const HumanoidLook* playerLook=nullptr,const M2Model* prepared=nullptr,bool components=false) {
    namespace sample=wowee::rendering::m2_track;
    auto model=prepared?*prepared:M2Loader::load(a.read(path));
    if(!model.isValid()||model.bones.empty()||model.bones.size()>256)throw std::runtime_error("Invalid animated model: "+path);
    if(extraId)assembleHumanoid(a,*humanoids,model,extraId);
    if(playerLook)assemblePlayer(a,*humanoids,model,*playerLook);
    int sequence=-1;
    for(size_t i=0;i<model.sequences.size();i++)if(model.sequences[i].id==animationId&&model.sequences[i].duration){sequence=int(i);break;}
    bool corpse=false;
    if(sequence<0&&animationId==6)for(size_t i=0;i<model.sequences.size();i++)if(model.sequences[i].id==1&&model.sequences[i].duration){sequence=int(i);corpse=true;break;}
    if(sequence<0){std::cout<<"Missing sequence "<<animationId<<" for display "<<display<<'\n';return;}
    auto duration=model.sequences[sequence].duration;
    unsigned frames=std::clamp((duration+49)/50,2u,120u);
    if(corpse)frames=2;
    auto place=glm::scale(glm::mat4(1),glm::vec3(scale));
    std::vector<float> poses;
    for(unsigned f=0;f<frames;f++) {
        float t=corpse?float(duration-1):float(f)*duration/frames;std::vector<glm::mat4> bones(model.bones.size());
        for(size_t b=0;b<model.bones.size();b++) {
            const auto& bone=model.bones[b];
            if(bone.parentBone>=int(b)||bone.parentBone < -1)throw std::runtime_error("Invalid bone hierarchy");
            auto tr=sample::sampleVec3(bone.translation,sequence,t,t,model.globalSequenceDurations,glm::vec3(0));
            auto rot=sample::sampleQuat(bone.rotation,sequence,t,t,model.globalSequenceDurations);
            auto scale=sample::sampleVec3(bone.scale,sequence,t,t,model.globalSequenceDurations,glm::vec3(1));
            auto m=glm::translate(glm::mat4(1),bone.pivot+tr)*glm::mat4_cast(rot)*glm::scale(glm::mat4(1),scale)*glm::translate(glm::mat4(1),-bone.pivot);
            bones[b]=bone.parentBone<0?m:bones[bone.parentBone]*m;
            m=place*bones[b];for(int row=0;row<3;row++)for(int col=0;col<4;col++)poses.push_back(m[col][row]);
        }
    }
    std::map<std::tuple<std::string,unsigned,unsigned>,size_t> combined;
    for(const auto& batch:model.batches) {
        if(!batch.indexCount||batch.submeshLevel>0)continue;
        if(batch.indexStart>model.indices.size()||batch.indexCount>model.indices.size()-batch.indexStart)throw std::runtime_error("Invalid animated batch");
        if(batch.textureIndex>=model.textureLookup.size())throw std::runtime_error("Invalid creature texture lookup");
        auto ti=model.textureLookup[batch.textureIndex];if(ti>=model.textures.size())throw std::runtime_error("Invalid creature texture");
        const auto& tex=model.textures[ti];std::string texturePath=tex.filename;
        if(tex.type>=11&&tex.type<=13){
            const auto& skin=skins[tex.type-11];
            if(skin.empty())throw std::runtime_error("Missing replaceable creature skin for "+path);
            texturePath=path.substr(0,path.find_last_of('\\')+1)+skin;
            if(texturePath.size()<4||texturePath.substr(texturePath.size()-4)!=".blp")texturePath+=".blp";
        }
        if(texturePath.empty())throw std::runtime_error("Unsupported creature texture type "+std::to_string(tex.type)+" in "+path);
        Part p;p.e.kind=WX_KIND_CHARACTER;p.e.id=((components?batch.submeshId:display)<<8)|animationId;p.e.flags=WX_ANIMATED;
        if(batch.materialIndex>=model.materials.size())throw std::runtime_error("Animated material invalid: "+path);
        p.e.flags|=worldMaterial(model.materials[batch.materialIndex].blendMode,model.materials[batch.materialIndex].flags,true);
        p.animation={frames,uint32_t(model.bones.size()),duration,0,0};p.poses=poses;
        std::map<unsigned,uint16_t> remap;
        for(unsigned i=0;i<batch.indexCount;i++) {
            auto old=model.indices[batch.indexStart+i];if(old>=model.vertices.size())throw std::runtime_error("Invalid animated index");
            if(!remap.count(old)) {
                const auto& v=model.vertices[old];WxSkinVertex skin{};
                for(int k=0;k<4;k++){skin.bones[k]=v.boneIndices[k];skin.weights[k]=v.boneWeights[k];if(skin.weights[k]&&skin.bones[k]>=model.bones.size())throw std::runtime_error("Invalid skin bone");}
                remap[old]=uint16_t(p.v.size());p.v.push_back(vertex(v.position,v.normal,v.texCoords[0],glm::mat4(1)));p.skin.push_back(skin);
            }p.i.push_back(remap.at(old));
        }
        auto key=std::make_tuple(texturePath,p.e.flags,p.e.id);auto found=combined.find(key);
        if(found!=combined.end()&&parts[found->second].v.size()+p.v.size()<=65535&&parts[found->second].i.size()+p.i.size()<=300000){
            auto& dst=parts[found->second];unsigned base=dst.v.size();
            dst.v.insert(dst.v.end(),p.v.begin(),p.v.end());dst.skin.insert(dst.skin.end(),p.skin.begin(),p.skin.end());
            for(auto index:p.i)dst.i.push_back(uint16_t(base+index));
        }else{modelTexture(a,p,texturePath);combined[key]=parts.size();parts.push_back(std::move(p));}
    }
    std::cout<<"Display "<<display<<" sequence "<<animationId<<": "<<path<<", "<<model.bones.size()<<" bones, "<<frames<<" poses, "<<duration<<" ms\n";
}
static void writePack(const fs::path& path,std::vector<Part>& parts,bool keepIds=false,const std::array<float,3>& spawn={-8949.95f,-132.493f,83.531f},const WorldEnvironment* environment=nullptr) {
    if(parts.empty()||parts.size()>WX_MAX_ENTRIES)throw std::runtime_error("Scene entry count exceeds bounded index: "+std::to_string(parts.size()));
    fs::create_directories(path.parent_path());
    std::ofstream f(path,std::ios::binary|std::ios::trunc); if(!f)throw std::runtime_error("Cannot create pack");
    WxPackHeader h{{'W','X','P','1'},environment?10u:8u,uint32_t(parts.size()),sizeof(WxEntry),{-8949.95f,-132.493f,83.531f},0};
    std::copy(spawn.begin(),spawn.end(),h.spawn);
    f.write((char*)&h,sizeof h);
    std::vector<WxEntry> entries(parts.size());f.write((char*)entries.data(),entries.size()*sizeof(WxEntry));
    // Keep animation headers contiguous for one bounded index read at runtime.
    // Payload offsets retain the existing WXP v5 ABI and old readers still work.
    uint32_t animationOffset=uint32_t(f.tellp());std::vector<WxAnimation> animations;
    for(auto& p:parts)if(p.e.flags&WX_ANIMATED){p.e.reserved[0]=animationOffset+uint32_t(animations.size()*sizeof(WxAnimation));animations.push_back({});}
    f.write((char*)animations.data(),animations.size()*sizeof(WxAnimation));unsigned animationIndex=0;
    std::map<std::vector<uint32_t>,uint32_t> textureOffsets;
    std::map<std::vector<float>,uint32_t> poseOffsets;
    auto offset=[&](){auto pos=f.tellp();if(pos<0||pos>=1024*1024*1024)throw std::runtime_error("Pack size limit exceeded");return uint32_t(pos);};
    for(size_t n=0;n<parts.size();++n){auto& p=parts[n];bounds(p);if(!keepIds&&!(p.e.flags&WX_PLACEMENT_ID))p.e.id=uint32_t(n);
        if(p.e.flags&WX_VERTEX_COLOR){if(p.vertexColors.size()!=p.v.size())throw std::runtime_error("Vertex lighting count mismatch");f.write((char*)p.vertexColors.data(),p.vertexColors.size()*4);}
        if(p.e.flags&WX_MATERIAL_MOTION)f.write((char*)&p.materialMotion,sizeof p.materialMotion);
        p.e.vertex_offset=offset();f.write((char*)p.v.data(),p.v.size()*sizeof(WxVertex));
        p.e.index_offset=offset();f.write((char*)p.i.data(),p.i.size()*sizeof(uint16_t));
        std::vector<uint32_t> key={p.e.width,p.e.height,p.e.reserved[1],p.e.flags&WX_TEX_SWIZZLED,p.textureRole};key.insert(key.end(),p.tex.begin(),p.tex.end());
        auto shared=textureOffsets.find(key);
        if(shared!=textureOffsets.end())p.e.texture_offset=shared->second;
        else{p.e.texture_offset=offset();f.write((char*)p.tex.data(),p.tex.size()*4);textureOffsets.emplace(std::move(key),p.e.texture_offset);}
        if(p.e.flags&WX_ANIMATED) {
            p.animation.skin_offset=offset();f.write((char*)p.skin.data(),p.skin.size()*sizeof(WxSkinVertex));
            auto found=poseOffsets.find(p.poses);
            if(found!=poseOffsets.end())p.animation.poses_offset=found->second;
            else {p.animation.poses_offset=offset();f.write((char*)p.poses.data(),p.poses.size()*4);poseOffsets.emplace(p.poses,p.animation.poses_offset);}
            animations[animationIndex++]=p.animation;
        }
        entries[n]=p.e;
    }
    if(environment)environment->write(f,offset());
    h.file_size=offset();f.seekp(0);f.write((char*)&h,sizeof h);f.write((char*)entries.data(),entries.size()*sizeof(WxEntry));
    f.write((char*)animations.data(),animations.size()*sizeof(WxAnimation));
    f.close();if(!f)throw std::runtime_error("Pack write failed");
    std::cout<<"Wrote "<<path<<": "<<parts.size()<<" entries, "<<h.file_size<<" bytes\n";
    std::cout<<"Shared textures: "<<textureOffsets.size()<<" unique mip chains\n";
}
#include "outfit_catalog.hpp"
#include "look_cooker.hpp"
#include "look_reference.hpp"
#include "avatar_cooker.hpp"
static TerrainLiquidCoverage terrain(Archives& a,std::vector<Part>& parts,int tx,int ty,const std::string& map="Azeroth",bool fullTile=false,WorldEnvironment* environment=nullptr) {
    auto name="World\\Maps\\"+map+"\\"+map+"_"+std::to_string(tx)+"_"+std::to_string(ty)+".adt";
    auto raw=a.read(name);auto adt=ADTLoader::load(raw); adt.coord={tx,ty};
    if(!adt.loaded||adt.version!=18)throw std::runtime_error("Invalid Vanilla ADT: "+name);
    auto mesh=TerrainMeshGenerator::generate(adt);
    if(!mesh.validChunkCount)throw std::runtime_error("ADT has no valid terrain: "+name);
    for(auto& chunk:mesh.chunks) {
        if(!chunk.isValid()) continue;
        Part p; p.e.kind=WX_KIND_TERRAIN;p.e.width=p.e.height=128;
        for(auto& v:chunk.vertices){WxVertex w{};std::copy_n(v.position,3,w.p);std::copy_n(v.normal,3,w.n);
            w.uv[0]=v.layerUV[0];w.uv[1]=v.layerUV[1];p.v.push_back(w);}
        for(auto i:chunk.indices)p.i.push_back(uint16_t(i));
        std::vector<const BLPImage*> layers;
        for(auto& l:chunk.layers){if(l.textureId>=mesh.textures.size())throw std::runtime_error("Invalid terrain texture index");layers.push_back(&texture(a,mesh.textures[l.textureId]));}
        if(layers.empty())throw std::runtime_error("Terrain has no texture layers");
        p.tex.resize(128*128);
        for(int y=0;y<128;++y)for(int x=0;x<128;++x){
            float u=(x+.5f)/128*8,v=(y+.5f)/128*8;auto c=sample(*layers[0],u,v);
            for(size_t l=1;l<layers.size();++l){auto d=sample(*layers[l],u,v);auto& alpha=chunk.layers[l].alphaData;
                float t=alpha.empty()?1.f:alpha[(y/2)*64+x/2]/255.f;for(int k=0;k<3;++k)c[k]=c[k]*(1-t)+d[k]*t;}
            c[3]=255;p.tex[y*128+x]=bgra(c);
        }
        swizzle(p);
        parts.push_back(std::move(p));
    }
    std::cout<<name<<": "<<mesh.validChunkCount<<" chunks, "<<adt.wmoPlacements.size()<<" WMO placements, "<<adt.doodadPlacements.size()<<" doodads\n";
    // Legacy Northshire preview keeps its original footprint. General tile builds
    // include every referenced placement, independently of zone names/spawn points.
    auto nearSpawn=[fullTile](const float* pos){if(fullTile)return true;auto p=wowee::core::coords::adtToWorld(pos[0],pos[1],pos[2]);return glm::distance(glm::vec2(p),glm::vec2(-8949.95f,-132.493f))<350.f;};
    auto markPlacement=[&](size_t start,uint32_t id,bool building){if(!fullTile)return;
        for(size_t i=start;i<parts.size();i++){auto& e=parts[i].e;e.flags|=WX_PLACEMENT_ID;e.reserved[0]=id;e.id=uint32_t(i-start)|(building?0x80000000u:0);}};
    for(auto& p:adt.wmoPlacements)if(p.nameId<adt.wmoNames.size()&&nearSpawn(p.position)){
        size_t start=parts.size();std::cout<<"  WMO "<<adt.wmoNames[p.nameId]<<'\n';wmo(a,parts,adt.wmoNames[p.nameId],p,nullptr,environment);markPlacement(start,p.uniqueId,true);}
    unsigned count=0;
    for(auto& p:adt.doodadPlacements)if(p.nameId<adt.doodadNames.size()&&nearSpawn(p.position)){
        size_t start=parts.size();m2(a,parts,adt.doodadNames[p.nameId],placement(p.position,p.rotation,p.scale/1024.f));markPlacement(start,p.uniqueId,false);count++;}
    std::cout<<"  Converted "<<count<<" nearby M2 placements\n";
    TerrainLiquidCoverage liquids;
    if(environment){size_t first=environment->items.size();liquids=terrainLiquidEnvironment(raw,adt,*environment);
        worldLiquidParts(a,parts,*environment,first,name,true);}
    return liquids;
}
#include "map_export.hpp"
#include "backdrop_cooker.hpp"
#include "portrait_cooker.hpp"
#include "icon_cooker.hpp"
#include "lighting_cooker.hpp"
#include "global_world.hpp"
int main(int argc,char** argv) {
    try {
        if(argc==6&&!strcmp(argv[1],"--world-model")){Archives a(argv[2]);globalWorld(a,argv[3],worldNumber(argv[4],999),argv[5]);return 0;}
        if(argc==4&&!strcmp(argv[1],"--lighting")){Archives a(argv[2]);prepareLighting(a,argv[3]);return 0;}
        if(argc==4&&!strcmp(argv[1],"--icons")){Archives a(argv[2]);prepareIcons(a,argv[3]);return 0;}
        if(argc==5&&!strcmp(argv[1],"--portraits")){Archives a(argv[2]);preparePortraits(a,argv[3],argv[4]);return 0;}
        if(argc==4&&!strcmp(argv[1],"--outfits")){Archives a(argv[2]);writeOutfits(a,argv[3]);return 0;}
        if(argc==5&&!strcmp(argv[1],"--backdrop")){Archives a(argv[2]);backdrop(a,argv[3],argv[4]);return 0;}
        if(argc==4&&!strcmp(argv[1],"--list-assets")){
            Archives a(argv[2]);std::set<std::string> names;
            for(auto archive:a.handles){SFILE_FIND_DATA data{};HANDLE search=SFileFindFirstFile(archive,argv[3],&data,nullptr);
                if(search&&search!=INVALID_HANDLE_VALUE){do{names.insert(data.cFileName);}while(SFileFindNextFile(search,&data));SFileFindClose(search);}}
            for(const auto& name:names)std::cout<<name<<'\n';return 0;
        }
        if(argc==5&&!strcmp(argv[1],"--extract")){
            Archives a(argv[2]);auto bytes=a.read(argv[3]);std::ofstream out(argv[4],std::ios::binary|std::ios::trunc);
            out.write((const char*)bytes.data(),bytes.size());out.close();if(!out)throw std::runtime_error("Asset extraction write failed");
            std::cout<<"Extracted "<<argv[3]<<": "<<bytes.size()<<" bytes\n";return 0;
        }
        if(argc==5&&!strcmp(argv[1],"--texture")){
            Archives a(argv[2]);auto image=BLPLoader::load(a.read(argv[3]));
            if(!image.isValid()||!image.width||!image.height||image.width>8192||image.height>8192||image.data.size()!=size_t(image.width)*image.height*4)
                throw std::runtime_error("Invalid UI texture");
            unsigned char header[18]={0,0,2};header[12]=image.width&255;header[13]=image.width>>8;
            header[14]=image.height&255;header[15]=image.height>>8;header[16]=32;header[17]=0x28;
            for(size_t i=0;i<image.data.size();i+=4)std::swap(image.data[i],image.data[i+2]);
            std::ofstream out(argv[4],std::ios::binary|std::ios::trunc);out.write((const char*)header,sizeof header);
            out.write((const char*)image.data.data(),image.data.size());out.close();if(!out)throw std::runtime_error("UI texture write failed");return 0;
        }
        if(argc==4&&!strcmp(argv[1],"--maps")){Archives a(argv[2]);writeMaps(a,argv[3]);return 0;}
        if(argc==3&&!strcmp(argv[1],"--map-info")){
            Archives a(argv[2]);DbcTable maps(a.read("DBFilesClient\\WorldMapArea.dbc"));
            if(maps.columns()<8)throw std::runtime_error("WorldMapArea requires eight core fields");
            std::cout<<"WorldMapArea records="<<maps.size()<<" columns="<<maps.columns()<<"\n";
            std::cout<<"id\tmap\tarea\tfolder\tleft\tright\ttop\tbottom\tbase_tiles_mask\n";
            for(unsigned row=0;row<maps.size();row++){
                auto name=maps.text(row,3);unsigned mask=0;
                if(!name.empty()&&std::all_of(name.begin(),name.end(),[](unsigned char c){return std::isalnum(c)||c=='_';})){
                    for(unsigned tile=1;tile<=12;tile++)if(!a.read("Interface\\WorldMap\\"+name+"\\"+name+std::to_string(tile)+".blp",false).empty())mask|=1u<<(tile-1);
                }
                for(unsigned field=4;field<8;field++)if(!std::isfinite(maps.real(row,field)))throw std::runtime_error("Non-finite map bounds");
                std::cout<<maps.value(row,0)<<'\t'<<maps.value(row,1)<<'\t'<<maps.value(row,2)<<'\t'<<name;
                for(unsigned field=4;field<8;field++)std::cout<<'\t'<<maps.real(row,field);
                std::cout<<'\t'<<mask<<'\n';
            }return 0;
        }
        if(argc==4&&!strcmp(argv[1],"--cooldowns")){
            Archives a(argv[2]);DbcTable spells(a.read("DBFilesClient\\Spell.dbc"));DbcTable categories(a.read("DBFilesClient\\SpellCategory.dbc"));writeCooldownCatalog(spells,categories,argv[3]);
            std::cout<<"Cooldown catalog: "<<spells.size()<<" Vanilla entries\n";return 0;
        }
        if(argc==4&&!strcmp(argv[1],"--spells")){
            Archives a(argv[2]);DbcTable spells(a.read("DBFilesClient\\Spell.dbc"));DbcTable ranges(a.read("DBFilesClient\\SpellRange.dbc"));writeSpellCatalog(spells,ranges,argv[3]);
            std::cout<<"Spell catalog: "<<spells.size()<<" Vanilla entries\n";return 0;
        }
        if(argc==6&&!strcmp(argv[1],"--look-reference")){
            unsigned race=std::stoul(argv[4]),sex=std::stoul(argv[5]);if(race<1||race>8||sex>1)throw std::runtime_error("Invalid reference identity");
            Archives a(argv[2]);HumanoidTables tables(a);lookReferences(a,tables,argv[3],race,sex);return 0;
        }
        if((argc==5||argc==6)&&!strcmp(argv[1],"--avatar")){
            auto look=readPlayerLook(argv[4]);Archives a(argv[2]);HumanoidTables tables(a);
            prepareAvatar(a,tables,look,argv[3],argc==6?argv[5]:nullptr);return 0;
        }
        if(argc==4&&!strcmp(argv[1],"--model-info")){
            Archives a(argv[2]);auto model=M2Loader::load(a.read(modelPathToM2(argv[3])));
            std::cout<<model.name<<" bones="<<model.bones.size()<<" vertices="<<model.vertices.size()<<'\n';
            for(unsigned i=0;i<model.textures.size();i++)std::cout<<"Texture "<<i<<" type="<<model.textures[i].type<<" file="<<model.textures[i].filename<<'\n';
            for(const auto& b:model.batches)if(!b.submeshLevel)std::cout<<"Batch geo="<<b.submeshId<<" texture="<<model.textureLookup.at(b.textureIndex)<<" indices="<<b.indexCount<<'\n';
            for(const auto& at:model.attachments)std::cout<<"Attachment "<<at.id<<" bone="<<at.bone<<" position="<<at.position.x<<","<<at.position.y<<","<<at.position.z<<'\n';return 0;
        }
        if(argc==4&&!strcmp(argv[1],"--catalog")){
            Archives a(argv[2]);DbcTable maps(a.read("DBFilesClient\\Map.dbc"));
            std::ofstream out(argv[3],std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot write map catalog");
            out<<"{\"version\":2,\"maps\":[";unsigned available=0,total=0;
            for(unsigned i=0;i<maps.size();i++){
                auto name=maps.text(i,1);auto id=maps.value(i,0);
                if(name.empty()||name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_ '-")!=std::string::npos)throw std::runtime_error("Unsafe map directory: ["+name+"] id="+std::to_string(id));
                auto bytes=a.read("World\\Maps\\"+name+"\\"+name+".wdt",false);
                if(i)out<<',';out<<"{\"id\":"<<id<<",\"directory\":\""<<name<<"\",\"available\":"<<(bytes.empty()?"false":"true");
                if(!bytes.empty()){
                    try{auto layout=worldLayout(bytes);available++;total+=(unsigned)layout.tiles.size();
                        out<<",\"valid\":true,\"map_type\":"<<maps.value(i,2)<<",\"kind\":\""<<(layout.global()?"world_model":"terrain")<<"\",\"tiles\":[";
                        for(unsigned t=0;t<layout.tiles.size();t++){if(t)out<<',';out<<'['<<layout.tiles[t].first<<','<<layout.tiles[t].second<<']';}out<<']';
                        if(layout.global())out<<",\"root\":"<<worldJson(layout.root);
                    }catch(const std::exception& e){out<<",\"valid\":false,\"error\":"<<worldJson(e.what());}
                }out<<'}';
            }out<<"]}\n";out.close();if(!out)throw std::runtime_error("Map catalog write failed");
            std::cout<<"Catalog: "<<maps.size()<<" maps, "<<available<<" available WDTs, "<<total<<" terrain tiles\n";return 0;
        }
        if(argc==8&&!strcmp(argv[1],"--tile")){
            auto number=[](const char* text,unsigned limit){size_t used=0;auto value=std::stoul(text,&used);if(used!=strlen(text)||value>limit)throw std::runtime_error("Invalid tile parameter");return(unsigned)value;};
            unsigned mapId=number(argv[4],999),tx=number(argv[6],63),ty=number(argv[7],63);
            Archives a(argv[2]);DbcTable maps(a.read("DBFilesClient\\Map.dbc"));std::string map=maps.text(maps.row(mapId),1);
            if(map!=argv[5])throw std::runtime_error("Map ID/directory mismatch");
            auto tiles=worldTiles(a.read("World\\Maps\\"+map+"\\"+map+".wdt"));
            if(std::find(tiles.begin(),tiles.end(),std::make_pair(tx,ty))==tiles.end())throw std::runtime_error("Tile is not present in the map index");
            std::vector<Part> parts;WorldEnvironment environment;auto liquids=terrain(a,parts,tx,ty,map,true,&environment);
            std::array<float,3> spawn={(31.5f-ty)*wowee::core::coords::TILE_SIZE,(31.5f-tx)*wowee::core::coords::TILE_SIZE,0};
            float nearest=1e20f;for(const auto& p:parts)if(p.e.kind==WX_KIND_TERRAIN)for(const auto& v:p.v){float x=v.p[0]-spawn[0],y=v.p[1]-spawn[1],d=x*x+y*y;if(d<nearest){nearest=d;spawn[2]=v.p[2];}}
            writePack(argv[3],parts,false,spawn,&environment);
            std::ofstream report(std::string(argv[3])+".coverage.json");
            report<<"{\"version\":1,\"kind\":\"terrain\",\"map\":"<<mapId<<",\"x\":"<<tx<<",\"y\":"<<ty
                  <<",\"chunks\":"<<liquids.chunks<<",\"terrain_liquid_grids\":"<<liquids.grids<<",\"terrain_liquid_cells\":"<<liquids.visibleCells
                  <<",\"deep_water_cells\":"<<liquids.deepCells<<",\"unused_height_sentinels\":"<<liquids.unusedSentinels<<",\"unused_flow_nonzero_bytes\":"<<liquids.flowBytes<<",\"upstream_liquid_layers\":"<<liquids.upstreamLayers
                  <<",\"environment_records\":"<<environment.items.size()<<",\"environment_bytes\":"<<environment.encode().size()
                  <<",\"limitations\":[\"Original MCLQ heights/masks retained; depth opacity and flow UV/tail are not implemented\",\"Camera columns have no cave/ground exclusion; swimming and physical traversal are unverified\",\"Preparation does not establish full visual or gameplay parity\"]}\n";
            report.close();if(!report)throw std::runtime_error("Terrain coverage report write failed");return 0;
        }
        if(argc==5&&!strcmp(argv[1],"--player")){
            auto look=readPlayerLook(argv[4]);Archives a(argv[2]);HumanoidTables humanoids(a);std::vector<Part> parts;
            for(unsigned animation:{0u,4u,5u,6u,16u})animatedCreature(a,parts,playerModelPath(look),{},1,1,animation,&humanoids,0,&look);
            writePack(argv[3],parts,true);return 0;
        }
        if(argc==5&&!strcmp(argv[1],"--actors")){
            Archives a(argv[2]);DbcTable displays(a.read("DBFilesClient\\CreatureDisplayInfo.dbc")),models(a.read("DBFilesClient\\CreatureModelData.dbc"));
            HumanoidTables humanoids(a);
            std::ifstream list(argv[4]);if(!list)throw std::runtime_error("Missing creature display list");
            std::vector<Part> parts;std::string line;std::map<uint32_t,bool> seen;
            while(std::getline(list,line)){
                if(line.empty()||line[0]=='#')continue;
                size_t used=0;auto id=std::stoul(line,&used);if(used!=line.size()||!id||id>0xffffff||!seen.emplace(id,true).second)throw std::runtime_error("Invalid display list entry");
                auto d=displays.row(id),m=models.row(displays.value(d,1));
                unsigned extraId=displays.value(d,3);
                auto path=modelPathToM2(models.text(m,2));float ds=displays.real(d,4),ms=models.real(m,4);
                if(!std::isfinite(ds)||!std::isfinite(ms)||ds<0||ms<0||ds>100||ms>100)throw std::runtime_error("Invalid creature scale");
                float scale=(ds?ds:1)*(ms?ms:1);std::array<std::string,3> skins;
                for(unsigned j=0;j<3;j++)skins[j]=displays.text(d,j+6);
                // vMaNGOS 5875 publishes the absolute model scale in OBJECT_FIELD_SCALE_X.
                // Keep templates in raw model units; applying this DBC base again shrinks them twice.
                std::cout<<"Display "<<id<<" DBC native scale "<<scale<<"; runtime scale comes from the server\n";
                for(unsigned animation:{0u,4u,5u,6u,16u})animatedCreature(a,parts,path,skins,1,id,animation,&humanoids,extraId);
            }
            writePack(argv[3],parts,true);return 0;
        }
        if(argc!=3){std::cerr<<"Usage: wowx_assetc <Vanilla Data directory> <output pack>\n       wowx_assetc --actors <Data directory> <output pack> <display list>\n       wowx_assetc --player <Data directory> <output pack> <appearance profile>\n";return 2;}
        Archives a(argv[1]);std::vector<Part> parts;terrain(a,parts,32,48);writePack(argv[2],parts);return 0;
    }catch(const std::exception& e){std::cerr<<"assetc: "<<e.what()<<'\n';return 1;}
}
