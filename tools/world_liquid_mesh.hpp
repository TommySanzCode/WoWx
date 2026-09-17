#pragma once
#include "world_environment.hpp"
#include "wx_pack.h"
#include <array>
#include <map>
// World liquid surfaces share the exact heights, diagonal and hole mask used by
// camera classification. Small patches provide ordinary spatial residency.
struct WorldLiquidPatch {std::vector<WxVertex> vertices;std::vector<uint16_t> indices;};
inline std::vector<WorldLiquidPatch> worldLiquidMesh(const WorldEnvironment::Item& item,bool visibleNormals=false){
    const auto& r=item.record;unsigned nx=r.x_tiles,ny=r.y_tiles;
    if(r.kind!=WX_ENV_LIQUID||!nx||!ny||nx>256||ny>256||item.heights.size()!=(nx+1)*(ny+1)||item.flags.size()!=nx*ny||
       !std::isfinite(r.tile_size)||fabsf(r.tile_size-4.1666625f)>.0001f)throw std::runtime_error("Invalid liquid mesh source");
    for(auto h:item.heights)if(!std::isfinite(h)||fabsf(h)>100000)throw std::runtime_error("Invalid liquid mesh height");
    const float* m=r.inverse;
    float determinant=m[0]*(m[5]*m[10]-m[6]*m[9])-m[1]*(m[4]*m[10]-m[6]*m[8])+m[2]*(m[4]*m[9]-m[5]*m[8]);
    std::vector<WorldLiquidPatch> out;
    auto used=[&](unsigned x,unsigned y){
        for(unsigned yy=y?y-1:y;yy<=y&&yy<ny;yy++)for(unsigned xx=x?x-1:x;xx<=x&&xx<nx;xx++)if((item.flags[yy*nx+xx]&15)!=15)return true;
        return false;
    };
    for(unsigned by=0;by<ny;by+=16)for(unsigned bx=0;bx<nx;bx+=16){
        WorldLiquidPatch p;std::map<unsigned,uint16_t> remap;
        auto vertex=[&](unsigned x,unsigned y){unsigned key=y*(nx+1)+x;
            auto found=remap.find(key);if(found!=remap.end())return found->second;
            WxVertex v{};float local[3]={r.corner[0]+x*r.tile_size,r.corner[1]+y*r.tile_size,item.heights[key]};
            for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++)v.p[j]+=r.inverse[k*4+j]*(local[k]-r.inverse[k*4+3]);
            unsigned x0=x?x-1:x,x1=x<nx?x+1:x,y0=y?y-1:y,y1=y<ny?y+1:y;
            // Terrain dry FLT_MAX corners are canonicalized by the adapter.
            // Never use an undrawn corner to light an adjacent visible vertex.
            if(visibleNormals){if(!used(x0,y))x0=x;if(!used(x1,y))x1=x;if(!used(x,y0))y0=y;if(!used(x,y1))y1=y;}
            float normal[3]={x1==x0?0:-(item.heights[y*(nx+1)+x1]-item.heights[y*(nx+1)+x0])/((x1-x0)*r.tile_size),
                y1==y0?0:-(item.heights[y1*(nx+1)+x]-item.heights[y0*(nx+1)+x])/((y1-y0)*r.tile_size),1};
            float length=sqrtf(normal[0]*normal[0]+normal[1]*normal[1]+1);
            for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++)v.n[j]+=r.inverse[k*4+j]*normal[k]/length;
            // Continuous across patches. Exact authored flow UV remains open.
            v.uv[0]=x*.25f;v.uv[1]=y*.25f;
            auto id=(uint16_t)p.vertices.size();p.vertices.push_back(v);remap.emplace(key,id);return id;
        };
        for(unsigned y=by;y<std::min(by+16,ny);y++)for(unsigned x=bx;x<std::min(bx+16,nx);x++){
            if((item.flags[y*nx+x]&15)==15)continue;
            auto a=vertex(x,y),b=vertex(x+1,y),c=vertex(x+1,y+1),d=vertex(x,y+1);
            if(determinant<0){for(auto i:{a,c,b,a,d,c})p.indices.push_back(i);}
            else {for(auto i:{a,b,c,a,c,d})p.indices.push_back(i);}
        }
        if(!p.indices.empty())out.push_back(std::move(p));
    }return out;
}
inline const char* worldLiquidTexture(unsigned type){
    switch(type){case 1:return "XTextures\\river\\lake_a.%u.blp";case 2:return "XTextures\\ocean\\ocean_h.%u.blp";
        case 3:return "XTextures\\lava\\lava.%u.blp";case 4:case 21:return "XTextures\\slime\\slime.%u.blp";
        default:throw std::runtime_error("Unsupported original WMO liquid type: "+std::to_string(type));}
}
