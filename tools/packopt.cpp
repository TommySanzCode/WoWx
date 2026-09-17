// Desktop-only repacker. Geometry, collision and placement IDs are
// copied byte-for-byte; terrain/static mip chains become GPU-native DXT blocks.
extern "C" {
#include "wx_runtime.h"
}
#include "texture_compress.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <tuple>
#include <cstdlib>
namespace fs=std::filesystem;
extern "C" void* wx_gpu_alloc(unsigned n){return std::malloc(n);}
extern "C" void wx_gpu_free(void* p){std::free(p);}
extern "C" unsigned wx_free_memory(void){return 64u*1024u*1024u;}
struct Scene {WxScene value;Scene(){wx_scene_init(&value);}~Scene(){wx_pack_close(&value);}};
int main(int argc,char** argv){
    fs::path temporary;bool created=false;
    try{
        if(argc!=3){std::cerr<<"Usage: wowx_packopt input.wxp output.wxp\n";return 2;}
        fs::path input=fs::absolute(argv[1]),output=fs::absolute(argv[2]);temporary=output;temporary+=".tmp";
        if(input==output||fs::exists(output)||fs::exists(temporary))throw std::runtime_error("Output must be a new path");
        Scene source;if(!wx_pack_open(&source.value,input.string().c_str())||!wx_pack_verify(&source.value))throw std::runtime_error("Invalid source pack");
        // Atlas metadata names byte offsets in its companion pack. Keeping that
        // pair unchanged is essential even if its textures were left as RGBA.
        for(unsigned i=0;i<source.value.header.count;i++)if(source.value.entries[i].kind==WX_KIND_CHARACTER)
            throw std::runtime_error("Use this repacker for terrain/static packs; character/atlas packs remain unchanged");
        std::ifstream in(input,std::ios::binary);fs::create_directories(output.parent_path());
        std::ofstream out(temporary,std::ios::binary);if(!out)throw std::runtime_error("Cannot create output");created=true;
        WxPackHeader h=source.value.header;h.version=std::max(source.value.header.version,8u);
        std::vector<WxEntry> entries(source.value.entries,source.value.entries+h.count);
        out.write((const char*)&h,sizeof h);out.write((const char*)entries.data(),entries.size()*sizeof(WxEntry));
        auto offset=[&](){auto n=out.tellp();if(n<0||n>=1024*1024*1024)throw std::runtime_error("Pack exceeds file limit");return uint32_t(n);};
        auto read=[&](unsigned at,unsigned n){std::vector<uint8_t> bytes(n);in.seekg(at);in.read((char*)bytes.data(),n);if(!in)throw std::runtime_error("Source read failed");return bytes;};
        auto write=[&](const std::vector<uint8_t>& bytes){unsigned at=offset();out.write((const char*)bytes.data(),bytes.size());if(!out)throw std::runtime_error("Output write failed");return at;};
        using Key=std::tuple<uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,bool>;
        struct Texture {uint32_t offset,encoding;};std::map<Key,Texture> textures;uint64_t before=0,after=0;unsigned dxt1=0,dxt5=0;
        for(unsigned i=0;i<h.count;i++){
            WxEntry old=entries[i],&e=entries[i];
            unsigned cb=wx_vertex_color_bytes(&old),mb=(old.flags&WX_MATERIAL_MOTION)?sizeof(WxMaterialMotion):0;
            if(cb)write(read(old.vertex_offset-mb-cb,cb));
            if(old.flags&WX_MATERIAL_MOTION)write(read(old.vertex_offset-sizeof(WxMaterialMotion),sizeof(WxMaterialMotion)));
            e.vertex_offset=write(read(old.vertex_offset,old.vertex_count*sizeof(WxVertex)));
            e.index_offset=write(read(old.index_offset,old.index_count*2));
            bool compress=(e.kind==WX_KIND_TERRAIN||e.kind==WX_KIND_STATIC)&&e.width>=4&&e.width==e.height&&!(e.width&(e.width-1))&&!(e.flags&(WX_ANIMATED|WX_TEX_DXT1|WX_TEX_DXT5));
            Key key{old.texture_offset,old.width,old.height,old.reserved[1],old.flags&WX_TEXTURE_ENCODING,compress};
            auto t=textures.find(key);
            if(t==textures.end()){
                auto bytes=read(old.texture_offset,wx_texture_bytes(&old));uint32_t encoding=old.flags&WX_TEXTURE_ENCODING;before+=bytes.size();
                if(compress){auto packed=compressTexture(old,bytes);encoding=packed.encoding;bytes=std::move(packed.bytes);if(encoding==WX_TEX_DXT1)dxt1++;else dxt5++;}
                after+=bytes.size();t=textures.emplace(key,Texture{write(bytes),encoding}).first;
            }
            e.texture_offset=t->second.offset;e.flags=(e.flags&~WX_TEXTURE_ENCODING)|t->second.encoding;
            if(e.flags&WX_ANIMATED)throw std::runtime_error("Unexpected animated world entry");
        }
        if(h.version>=9){auto footer=source.value.sources[0].environment.footer;auto blob=read(footer.offset,footer.bytes);footer.offset=write(blob);out.write((const char*)&footer,sizeof footer);}
        h.file_size=offset();out.seekp(0);out.write((const char*)&h,sizeof h);out.write((const char*)entries.data(),entries.size()*sizeof(WxEntry));
        out.close();if(!out)throw std::runtime_error("Cannot finalize output");
        {Scene verify;if(!wx_pack_open(&verify.value,temporary.string().c_str())||!wx_pack_verify(&verify.value))throw std::runtime_error("Output runtime verification failed");}
        fs::rename(temporary,output);created=false;
        std::cout<<"entries="<<h.count<<" textures="<<textures.size()<<" dxt1="<<dxt1<<" dxt5="<<dxt5
            <<" texture_before="<<before<<" texture_after="<<after<<" file_before="<<source.value.header.file_size<<" file_after="<<h.file_size<<'\n';return 0;
    }catch(const std::exception& e){if(created){std::error_code ignored;fs::remove(temporary,ignored);}std::cerr<<"packopt: "<<e.what()<<'\n';return 1;}
}
