#pragma once
#include "wx_map.h"
// Vanilla 1.12: WorldMapArea core eight columns; WorldMapOverlay columns 0..16.
// All 12 base tiles form 1024x768; the map's coordinate rectangle is 1002x668.
// Downsample by two, retaining the 501x334 rectangle in a 512-square texture.
static bool mapPath(const std::string& s){return !s.empty()&&std::all_of(s.begin(),s.end(),[](unsigned char c){return std::isalnum(c)||c=='_';});}
static std::vector<uint32_t> mapImage(Archives& a,const std::string& prefix,unsigned width,unsigned height){
    if(!width||!height||width>1024||height>768)throw std::runtime_error("Map image bounds");
    std::vector<uint32_t> out(width*height);unsigned cols=(width+255)/256,rows=(height+255)/256;
    for(unsigned ty=0;ty<rows;ty++)for(unsigned tx=0;tx<cols;tx++){
        auto name=prefix+std::to_string(ty*cols+tx+1)+".blp";auto im=BLPLoader::load(a.read(name));
        unsigned w=std::min(256u,width-tx*256),h=std::min(256u,height-ty*256);
        if(!im.isValid()||im.width>256||im.height>256||im.data.size()!=size_t(im.width)*im.height*4)throw std::runtime_error("Map tile decode: "+name);
        // Some Vanilla overlay edge tiles are smaller than their DBC rectangle.
        // Preserve transparent padding instead of reading beyond decoded pixels.
        w=std::min(w,unsigned(im.width));h=std::min(h,unsigned(im.height));
        for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){auto p=&im.data[(y*im.width+x)*4];out[(ty*256+y)*width+tx*256+x]=(uint32_t(p[3])<<24)|(uint32_t(p[0])<<16)|(uint32_t(p[1])<<8)|p[2];}
    }return out;
}
static std::vector<uint32_t> mapHalf(const std::vector<uint32_t>& src,unsigned width,unsigned height,unsigned ox,unsigned oy,unsigned x,unsigned y,unsigned w,unsigned h){
    std::vector<uint32_t> out(w*h);
    for(unsigned j=0;j<h;j++)for(unsigned i=0;i<w;i++){
        unsigned alpha=0,red=0,green=0,blue=0;
        for(unsigned dy=0;dy<2;dy++)for(unsigned dx=0;dx<2;dx++){
            int px=int((i+x)*2+dx)-int(ox),py=int((j+y)*2+dy)-int(oy);
            if(px<0||py<0||unsigned(px)>=width||unsigned(py)>=height)continue;
            uint32_t c=src[py*width+px],a=c>>24;alpha+=a;red+=((c>>16)&255)*a;green+=((c>>8)&255)*a;blue+=(c&255)*a;
        }out[j*w+i]=alpha?(((alpha+2)/4)<<24)|((red/alpha)<<16)|((green/alpha)<<8)|(blue/alpha):0;
    }return out;
}
static void writeMaps(Archives& a,const fs::path& directory){
    DbcTable maps(a.read("DBFilesClient\\WorldMapArea.dbc")),overlays(a.read("DBFilesClient\\WorldMapOverlay.dbc")),areas(a.read("DBFilesClient\\AreaTable.dbc"));
    if(maps.columns()<8||overlays.columns()<17||areas.columns()<12||maps.size()>WX_MAP_LIMIT)throw std::runtime_error("Unsupported Vanilla map tables");
    fs::create_directories(directory);std::vector<WxMapEntry> catalog;
    for(unsigned i=0;i<maps.size();i++){
        auto folder=maps.text(i,3);if(!mapPath(folder))throw std::runtime_error("Unsafe map folder");
        WxMapEntry e{};e.id=maps.value(i,0);e.map=maps.value(i,1);e.area=maps.value(i,2);e.left=maps.real(i,4);e.right=maps.real(i,5);e.top=maps.real(i,6);e.bottom=maps.real(i,7);
        if(!e.id||e.id>9999999||!std::isfinite(e.left)||!std::isfinite(e.right)||!std::isfinite(e.top)||!std::isfinite(e.bottom)||e.left<=e.right||e.top<=e.bottom)throw std::runtime_error("Invalid map bounds");
        std::string label=folder;if(e.area)label=areas.text(areas.row(e.area),11);
        if(label.empty())label=folder;snprintf(e.name,sizeof e.name,"%s",label.c_str());
        auto prefix="Interface\\WorldMap\\"+folder+"\\";
        auto base=mapImage(a,prefix+folder,1024,768);auto reduced=mapHalf(base,1024,768,0,0,0,0,512,512);
        std::vector<WxMapPatch> patches;std::vector<std::vector<uint32_t>> images;
        for(unsigned k=0;k<overlays.size();k++)if(overlays.value(k,1)==e.id){
            auto name=overlays.text(k,8);unsigned w=overlays.value(k,9),h=overlays.value(k,10),x=overlays.value(k,11),y=overlays.value(k,12);
            if(name.empty()||!w||!h)continue;if(!mapPath(name)||w>1024||h>768||x>=1002||y>=668)throw std::runtime_error("Invalid overlay dimensions/path");
            WxMapPatch p{};std::fill(std::begin(p.bits),std::end(p.bits),UINT32_MAX);
            for(unsigned b=0;b<4;b++){unsigned area=overlays.value(k,2+b);if(area){unsigned bit=areas.value(areas.row(area),3);if(bit>=2048)throw std::runtime_error("Invalid exploration bit");p.bits[b]=bit;}}
            p.x=x/2;p.y=y/2;p.width=(std::min(x+w,1002u)+1)/2-p.x;p.height=(std::min(y+h,668u)+1)/2-p.y;
            auto full=mapImage(a,prefix+name,w,h);images.push_back(mapHalf(full,w,h,x,y,p.x,p.y,p.width,p.height));patches.push_back(p);
        }
        if(patches.size()>WX_MAP_LIMIT)throw std::runtime_error("Too many map overlays");
        uint32_t size=16+patches.size()*sizeof(WxMapPatch)+WX_MAP_BYTES;
        for(unsigned p=0;p<patches.size();p++){patches[p].offset=size;size+=images[p].size()*4;}
        char filename[24];snprintf(filename,sizeof filename,"Z%07u.WMP",e.id);std::ofstream f(directory/filename,std::ios::binary|std::ios::trunc);
        uint32_t head[]={WX_MAP_MAGIC,e.id,(unsigned)patches.size(),size};f.write((char*)head,sizeof head);f.write((char*)patches.data(),patches.size()*sizeof(WxMapPatch));f.write((char*)reduced.data(),WX_MAP_BYTES);
        for(const auto& im:images)f.write((char*)im.data(),im.size()*4);if(!f)throw std::runtime_error("Map file write failed");
        catalog.push_back(e);std::cout<<filename<<" "<<e.name<<" overlays="<<patches.size()<<" bytes="<<size<<"\n";
    }
    uint32_t head[]={WX_MAP_INDEX_MAGIC,1,(unsigned)catalog.size(),sizeof(WxMapEntry)};std::ofstream f(directory/"MAPS.WMI",std::ios::binary|std::ios::trunc);
    f.write((char*)head,sizeof head);f.write((char*)catalog.data(),catalog.size()*sizeof(WxMapEntry));if(!f)throw std::runtime_error("Map catalog write failed");
}
