extern "C" {
#include "wx_runtime.h"
}
#include "world_liquid_mesh.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
static unsigned checks,allocated,available=48u*1024u*1024u;static int failAfter=-1;
#define CHECK(x) do{checks++;if(!(x))throw std::runtime_error(std::to_string(__LINE__)+": " #x);}while(0)
extern "C" unsigned wx_free_memory(void){return available-allocated;}
extern "C" void* wx_gpu_alloc(unsigned n){if(failAfter==0)return nullptr;if(failAfter>0)failAfter--;auto p=(unsigned*)malloc(n+4);if(p){*p=n;allocated+=n;return p+1;}return nullptr;}
extern "C" void wx_gpu_free(void* v){if(v){auto p=(unsigned*)v-1;allocated-=*p;free(p);}}
template<class F> static void rejects(F f){bool rejected=false;try{f();}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);}
static void meshes(){
    WorldEnvironment::Item item;auto& r=item.record;r.kind=WX_ENV_LIQUID;r.x_tiles=34;r.y_tiles=68;r.tile_size=4.1666625f;
    // Rotated and translated placement, matching the runtime inverse convention.
    r.inverse[1]=1;r.inverse[4]=-1;r.inverse[10]=1;r.inverse[3]=-20;r.inverse[7]=10;r.inverse[11]=-30;
    r.corner[0]=2;r.corner[1]=3;r.corner[2]=100;
    for(unsigned y=0;y<=r.y_tiles;y++)for(unsigned x=0;x<=r.x_tiles;x++)item.heights.push_back(7+x*.3f+y*.2f+(x*y%3)*.1f);
    item.flags.resize(r.x_tiles*r.y_tiles);unsigned holes=0;
    for(unsigned i=0;i<item.flags.size();i++)if(i%13==0){item.flags[i]=0x8f;holes++;}else item.flags[i]=0x80;
    auto patches=worldLiquidMesh(item);CHECK(patches.size()==15);unsigned triangles=0;
    std::map<std::pair<unsigned,unsigned>,std::array<float,5>> seams;
    for(const auto& p:patches){CHECK(p.vertices.size()<=289&&p.indices.size()<=1536);triangles+=(unsigned)p.indices.size()/3;
        for(auto i:p.indices)CHECK(i<p.vertices.size());
        for(const auto& v:p.vertices){
            float local[3]={};for(unsigned k=0;k<3;k++){local[k]=r.inverse[k*4+3];for(unsigned j=0;j<3;j++)local[k]+=r.inverse[k*4+j]*v.p[j];}
            unsigned x=(unsigned)lroundf((local[0]-r.corner[0])/r.tile_size),y=(unsigned)lroundf((local[1]-r.corner[1])/r.tile_size);
            CHECK(x<=r.x_tiles&&y<=r.y_tiles);CHECK(fabsf(local[2]-item.heights[y*(r.x_tiles+1)+x])<.0001f);
            CHECK(fabsf(v.n[0]*v.n[0]+v.n[1]*v.n[1]+v.n[2]*v.n[2]-1)<.0001f);
            std::array<float,5> value={v.n[0],v.n[1],v.n[2],v.uv[0],v.uv[1]};auto key=std::make_pair(x,y);
            if(seams.count(key))CHECK(seams.at(key)==value);else seams[key]=value;
        }
    }CHECK(triangles==2*(r.x_tiles*r.y_tiles-holes));
    std::fill(item.flags.begin(),item.flags.end(),15);CHECK(worldLiquidMesh(item).empty());
    auto good=item;item.flags.pop_back();rejects([&]{worldLiquidMesh(item);});item=good;
    item.heights[10]=std::numeric_limits<float>::quiet_NaN();rejects([&]{worldLiquidMesh(item);});item=good;
    item.record.x_tiles=257;rejects([&]{worldLiquidMesh(item);});
    for(auto type:{1u,2u,3u,4u,21u})CHECK(worldLiquidTexture(type));rejects([]{worldLiquidTexture(7);});
}
static WxEntry entry(){WxEntry e{};e.kind=WX_KIND_STATIC;e.vertex_count=e.index_count=3;e.width=e.height=64;e.radius=2;
    e.flags=WX_TEXTURE_SEQUENCE|WX_LIQUID|WX_TEX_SWIZZLED|WX_MIPMAPPED;e.reserved[1]=(30u<<16)|7;return e;}
static std::vector<uint8_t> fixture(WxEntry e=entry(),unsigned version=10){
    WxPackHeader h{{'W','X','P','1'},version,2,64,{0},0};e.vertex_offset=32+128;e.index_offset=e.vertex_offset+96;e.texture_offset=e.index_offset+6;
    auto original=entry();unsigned bytes=wx_texture_bytes(&original);h.file_size=e.texture_offset+bytes+16;
    std::vector<uint8_t> out(h.file_size);memcpy(out.data(),&h,32);memcpy(out.data()+32,&e,64);e.id=1;memcpy(out.data()+96,&e,64);
    WxVertex v[3]={};v[1].p[0]=1;v[2].p[1]=1;for(auto& q:v)q.n[2]=1;
    uint16_t indices[3]={0,1,2};memcpy(out.data()+e.vertex_offset,v,96);memcpy(out.data()+e.index_offset,indices,6);
    unsigned stride=wx_texture_frame_bytes(&original);
    for(unsigned frame=0;frame<30;frame++)for(unsigned i=0;i<stride;i++)out[e.texture_offset+frame*stride+i]=(uint8_t)frame;
    WxEnvironmentFooter footer{{'W','X','E','1'},0,0,h.file_size-16};memcpy(out.data()+h.file_size-16,&footer,16);return out;
}
static const char* path="liquid-fixture.wxp";
static void write(const std::vector<uint8_t>& bytes){FILE* f=fopen(path,"wb");CHECK(f);CHECK(fwrite(bytes.data(),1,bytes.size(),f)==bytes.size());CHECK(!fclose(f));}
static WxScene scene;
static void invalid(std::vector<uint8_t> data){write(data);int opened=wx_pack_open(&scene,path);CHECK(!opened||!wx_pack_verify(&scene));wx_pack_close(&scene);CHECK(!allocated);}
static void sequences(){
    auto e=entry();CHECK(wx_texture_levels(&e)==7&&wx_texture_frames(&e)==30);CHECK(wx_texture_frame_bytes(&e)==21888&&wx_texture_bytes(&e)==656640);
    for(unsigned ms=0;ms<5000;ms++){double phase=(ms%1250)*30./1250.-.5;int ref=(int)std::nearbyint(phase);if(ref<0)ref=0;CHECK(wx_texture_frame(&e,ms)==(unsigned)ref);}
    CHECK(wx_texture_frame(&e,125)==2&&wx_texture_frame(&e,126)==3&&wx_texture_frame(&e,1249)==29&&wx_texture_frame(&e,1250)==0);
    e.flags=(e.flags&~WX_TEX_SWIZZLED)|WX_TEX_DXT1;CHECK(wx_texture_frame_bytes(&e)==2816&&wx_texture_bytes(&e)==84480);
    e.flags=(e.flags&~WX_TEX_DXT1)|WX_TEX_DXT5;CHECK(wx_texture_frame_bytes(&e)==5504&&wx_texture_bytes(&e)==165120);
    for(unsigned count:{0u,1u,33u,65535u}){e=entry();e.reserved[1]=(count<<16)|7;CHECK(!wx_texture_bytes(&e));invalid(fixture(e));}
    e=entry();e.reserved[1]=30u<<16;invalid(fixture(e));e=entry();e.flags&=~WX_TEXTURE_SEQUENCE;invalid(fixture(e));
    e=entry();e.flags|=WX_ANIMATED;invalid(fixture(e));e=entry();e.kind=WX_KIND_CHARACTER;invalid(fixture(e));
    invalid(fixture(entry(),9));
    auto shortFile=fixture();shortFile.erase(shortFile.end()-17);unsigned size=(unsigned)shortFile.size();memcpy(shortFile.data()+28,&size,4);size-=16;memcpy(shortFile.data()+shortFile.size()-4,&size,4);invalid(shortFile);
    write(fixture());CHECK(wx_pack_open(&scene,path));unsigned frames=0;
    while(!wx_stream_ready(&scene)&&frames++<100){wx_stream_frame_begin(2);wx_stream(&scene,scene.header.spawn);wx_stream_frame_end();
        CHECK(scene.streaming.read_bytes<=65536&&scene.streaming.read_ops<=16&&scene.streaming.scan_bytes<=65536);
        if(scene.stream_job.phase)CHECK(scene.slots[scene.stream_job.slot].entry<0);
    }
    CHECK(frames>=11&&frames<100&&!scene.failures&&scene.loads==2);
    WxResident *first=nullptr,*second=nullptr;for(auto& r:scene.slots)if(r.entry>=0){if(!first)first=&r;else second=&r;}
    CHECK(first&&second&&first->texture==second->texture&&first->texture_slot==second->texture_slot);
    CHECK(scene.textures[first->texture_slot].references==2&&scene.bytes==656640+2*(96+6));
    for(unsigned frame=0;frame<30;frame++){auto pixels=(uint8_t*)first->texture+frame*21888;CHECK(pixels[0]==frame&&pixels[21887]==frame);}
    float floor=0;CHECK(!wx_ground(&scene,.1f,.1f,&floor));wx_pack_close(&scene);CHECK(!allocated);
    // Every unpublished prefix can be abandoned without retaining a frame or reference.
    for(unsigned stop=1;stop<=12;stop++){CHECK(wx_pack_open(&scene,path));for(unsigned i=0;i<stop;i++)wx_stream(&scene,scene.header.spawn);wx_pack_close(&scene);CHECK(!allocated&&!scene.bytes);}
    for(int failure=0;failure<3;failure++){CHECK(wx_pack_open(&scene,path));failAfter=failure;
        for(unsigned i=0;i<40&&!scene.failures;i++)wx_stream(&scene,scene.header.spawn);
        CHECK(scene.failures);wx_pack_close(&scene);CHECK(!allocated);failAfter=-1;
    }
    CHECK(wx_pack_open(&scene,path));available=8u*1024u*1024u+65536;
    for(unsigned i=0;i<10&&!scene.failures;i++)wx_stream(&scene,scene.header.spawn);
    CHECK(!allocated&&!scene.loads);wx_pack_close(&scene);available=48u*1024u*1024u;remove(path);
}
int main(){try{meshes();sequences();printf("Liquid mesh, frame layout/clock, publication, shared memory, failure and cancellation: %u passed\n",checks);return 0;}
    catch(const std::exception& e){fprintf(stderr,"Liquid tests: %s\n",e.what());wx_pack_close(&scene);return 1;}}
