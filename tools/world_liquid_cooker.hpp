#pragma once
#include "world_liquid_mesh.hpp"
static void worldLiquidParts(Archives& archives,std::vector<Part>& parts,const WorldEnvironment& environment,size_t first,const std::string& path,bool terrainSource=false){
    std::map<unsigned,Part> textures;
    for(size_t i=first;i<environment.items.size();i++){
        const auto& item=environment.items[i];if(item.record.kind!=WX_ENV_LIQUID)continue;
        auto patches=worldLiquidMesh(item,terrainSource);if(patches.empty())continue;
        unsigned type=item.record.liquid_type;auto found=textures.find(type);
        if(found==textures.end()){
            Part t;t.e.kind=WX_KIND_STATIC;t.e.flags=WX_LIQUID|WX_TEXTURE_SEQUENCE|WX_TEX_SWIZZLED|WX_MIPMAPPED;
            t.e.width=t.e.height=64;t.e.reserved[1]=(30u<<16)|7u;t.textureName=worldLiquidTexture(type);
            for(unsigned frame=1;frame<=30;frame++){
                char name[128];snprintf(name,sizeof name,t.textureName.c_str(),frame);
                const auto& image=texture(archives,name);Part p;p.e.width=p.e.height=64;p.tex.resize(64*64);
                for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++)p.tex[y*64+x]=bgra(sample(image,(x+.5f)/64,(y+.5f)/64));
                swizzle(p);while(p.tex.size()&31u)p.tex.push_back(0);t.tex.insert(t.tex.end(),p.tex.begin(),p.tex.end());
            }
            // Initial opacity follows the pinned reusable water policy. Original
            // per-vertex depth opacity/reflection/refraction are still absent.
            if(type==3)t.e.flags|=WX_UNLIT|WX_UNFOGGED;
            else {t.e.flags|=(2u<<WX_BLEND_SHIFT)|WX_MATERIAL_MOTION;
                t.materialMotion.magic=WX_MOTION_MAGIC;t.materialMotion.weight=1;
                t.materialMotion.color[0]=t.materialMotion.color[1]=t.materialMotion.color[2]=1;
                t.materialMotion.color[3]=type==1?.48f:type==2?.72f:.65f;
            }
            found=textures.emplace(type,std::move(t)).first;
        }
        for(auto& patch:patches){Part p=found->second;p.sourceName=path+" liquid grid "+std::to_string(item.record.group);
            p.v=std::move(patch.vertices);p.i=std::move(patch.indices);parts.push_back(std::move(p));}
    }
}
