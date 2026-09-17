#include "wx_runtime.h"
#include "wx_input.h"
#include "wx_replay.h"
#include "wx_world.h"
#include "wx_region.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static unsigned available=60u*1024u*1024u,outstanding;
static int fail_after=-1,checks;
#define CHECK(x) do { checks++; if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
unsigned wx_free_memory(void){return available;}
void* wx_gpu_alloc(unsigned bytes){
    if(fail_after==0)return NULL;if(fail_after>0)fail_after--;
    unsigned* p=malloc(bytes+sizeof(unsigned));if(!p)return NULL;
    *p=bytes;available-=bytes;outstanding++;return p+1;
}
void wx_gpu_free(void* ptr){if(ptr){unsigned* p=(unsigned*)ptr-1;available+=*p;outstanding--;free(p);}}

typedef struct Fixture {
    WxPackHeader h;WxEntry e;WxVertex v[3];uint16_t indices[3];uint16_t padding;
    uint32_t texture;WxSkinVertex skin[3];float poses[24];WxAnimation animation;
} Fixture;
#define OFFSET(member) ((uint32_t)offsetof(Fixture,member))
#include <stddef.h>
static Fixture fixture(void){
    Fixture f={0};memcpy(f.h.magic,"WXP1",4);f.h.version=8 /* geometry/profile fixture without v9 world metadata */;
    f.h.count=1;f.h.entry_size=sizeof(WxEntry);f.h.file_size=sizeof f;
    f.e.kind=WX_KIND_TERRAIN;f.e.vertex_count=3;f.e.index_count=3;f.e.width=f.e.height=1;f.e.radius=3;
    f.e.vertex_offset=OFFSET(v);f.e.index_offset=OFFSET(indices);f.e.texture_offset=OFFSET(texture);
    f.v[0]=(WxVertex){{0,0,2},{0,0,1},{0,0}};
    f.v[1]=(WxVertex){{1,0,2},{0,0,1},{1,0}};
    f.v[2]=(WxVertex){{0,1,2},{0,0,1},{0,1}};
    f.indices[1]=1;f.indices[2]=2;f.texture=0xffffffff;
    return f;
}
static void write_fixture(const Fixture* f){FILE* out=fopen("runtime-fixture.wxp","wb");CHECK(out);CHECK(fwrite(f,1,sizeof *f,out)==sizeof *f);CHECK(!fclose(out));}
static void reject_open(Fixture* f){WxScene s;write_fixture(f);CHECK(!wx_pack_open(&s,"runtime-fixture.wxp"));CHECK(!outstanding);}
static void reject_load(Fixture* f){WxScene s;write_fixture(f);CHECK(wx_pack_open(&s,"runtime-fixture.wxp"));wx_stream(&s,f->h.spawn);CHECK(s.failures==1);CHECK(s.bytes==0);CHECK(!outstanding);wx_pack_close(&s);}
static void shared_textures(void){
    struct Shared {WxPackHeader h;WxEntry e[2];WxVertex v[3];uint16_t indices[3],padding;uint32_t pixels[5];} f={0};
    Fixture source=fixture();f.h=source.h;f.h.count=2;f.h.file_size=sizeof f;
    memcpy(f.v,source.v,sizeof f.v);memcpy(f.indices,source.indices,sizeof f.indices);
    for(unsigned i=0;i<2;i++){f.e[i]=source.e;f.e[i].id=i;f.e[i].vertex_offset=offsetof(struct Shared,v);
        f.e[i].index_offset=offsetof(struct Shared,indices);f.e[i].texture_offset=offsetof(struct Shared,pixels);
        f.e[i].flags=WX_TEX_SWIZZLED|WX_MIPMAPPED;f.e[i].width=f.e[i].height=2;f.e[i].reserved[1]=2;f.e[i].center[0]=i*10.f;}
    FILE* out=fopen("shared-fixture.wxp","wb");CHECK(out);CHECK(fwrite(&f,1,sizeof f,out)==sizeof f);fclose(out);
    WxScene s;CHECK(wx_pack_open(&s,"shared-fixture.wxp"));wx_stream(&s,f.h.spawn);
    CHECK(s.loads==2&&s.failures==0&&outstanding==3);CHECK(s.slots[0].texture==s.slots[1].texture);
    CHECK(s.bytes==2*(sizeof f.v+sizeof f.indices)+sizeof f.pixels);
    float far[3]={199,0,0};wx_stream(&s,far);CHECK(outstanding==2);CHECK(s.bytes==sizeof f.v+sizeof f.indices+sizeof f.pixels);
    wx_pack_close(&s);CHECK(!outstanding&&!s.bytes);
    CHECK(wx_pack_open(&s,"shared-fixture.wxp"));fail_after=2;wx_stream(&s,f.h.spawn);fail_after=-1;
    CHECK(s.loads==1&&s.failures==1&&outstanding==2);wx_pack_close(&s);CHECK(!outstanding&&!s.bytes);
    f.h.file_size-=4;out=fopen("shared-fixture.wxp","wb");CHECK(out);fwrite(&f,1,sizeof f-4,out);fclose(out);
    CHECK(!wx_pack_open(&s,"shared-fixture.wxp"));CHECK(!outstanding);remove("shared-fixture.wxp");
}
static void compressed_textures(void){
    Fixture f=fixture();WxScene s;WxEntry e=f.e;
    e.width=e.height=128;e.flags=WX_TEX_DXT1|WX_MIPMAPPED;e.reserved[1]=8;
    CHECK(wx_texture_bytes(&e)==10936);e.flags=WX_TEX_DXT5|WX_MIPMAPPED;CHECK(wx_texture_bytes(&e)==21872);
    e.reserved[1]=9;CHECK(!wx_texture_bytes(&e));e.reserved[1]=8;
    e.flags|=WX_TEX_SWIZZLED;CHECK(!wx_texture_bytes(&e));e.flags=WX_TEX_DXT1|WX_TEX_DXT5;CHECK(!wx_texture_bytes(&e));
    e.flags=WX_TEX_DXT1;e.reserved[1]=1;e.height=64;CHECK(!wx_texture_bytes(&e));e.height=128;
    e.kind=WX_KIND_CHARACTER;CHECK(!wx_texture_bytes(&e));e.kind=WX_KIND_COLLISION;CHECK(!wx_texture_bytes(&e));
    e.kind=WX_KIND_STATIC;e.flags|=WX_ANIMATED;CHECK(!wx_texture_bytes(&e));
    f.e.width=f.e.height=4;f.e.flags=WX_TEX_DXT1|WX_MIPMAPPED;f.e.reserved[1]=3;
    CHECK(wx_texture_bytes(&f.e)==24);write_fixture(&f);CHECK(wx_pack_open(&s,"runtime-fixture.wxp"));
    wx_stream(&s,f.h.spawn);CHECK(s.loads==1&&!s.failures&&s.bytes==sizeof f.v+sizeof f.indices+24);
    CHECK(s.textures[s.slots[0].texture_slot].encoding==WX_TEX_DXT1);wx_pack_close(&s);CHECK(!outstanding);
    f.h.version=4;reject_open(&f);f.h.version=8 /* geometry/profile fixture without v9 world metadata */;
    f.e.texture_offset=sizeof f-23;reject_open(&f);f.e.texture_offset=OFFSET(texture);
    for(int n=0;n<2;n++){fail_after=n;reject_load(&f);fail_after=-1;}
    f=fixture();f.h.version=4;write_fixture(&f);CHECK(wx_pack_open(&s,"runtime-fixture.wxp"));wx_stream(&s,f.h.spawn);
    CHECK(s.loads==1&&!s.failures);wx_pack_close(&s);CHECK(!outstanding);
    // Identical offsets and byte counts with distinct encodings must not alias.
    struct Formats {WxPackHeader h;WxEntry e[2];WxVertex v[3];uint16_t indices[3],pad;uint8_t pixels[16];} both={0};
    both.h=f.h;both.h.version=8 /* geometry/profile fixture without v9 world metadata */;both.h.count=2;both.h.file_size=sizeof both;
    memcpy(both.v,f.v,sizeof f.v);memcpy(both.indices,f.indices,sizeof f.indices);
    for(unsigned i=0;i<2;i++){both.e[i]=f.e;both.e[i].id=i;both.e[i].width=both.e[i].height=2;
        both.e[i].vertex_offset=offsetof(struct Formats,v);both.e[i].index_offset=offsetof(struct Formats,indices);
        both.e[i].texture_offset=offsetof(struct Formats,pixels);both.e[i].flags=i?WX_TEX_DXT5:WX_TEX_SWIZZLED;}
    FILE* out=fopen("formats-fixture.wxp","wb");CHECK(out);CHECK(fwrite(&both,1,sizeof both,out)==sizeof both);CHECK(!fclose(out));
    CHECK(wx_pack_open(&s,"formats-fixture.wxp"));wx_stream(&s,both.h.spawn);CHECK(s.loads==2&&!s.failures);
    CHECK(s.slots[0].texture!=s.slots[1].texture);CHECK(s.bytes==2*(sizeof both.v+sizeof both.indices+sizeof both.pixels));
    wx_pack_close(&s);CHECK(!outstanding);remove("formats-fixture.wxp");
}
static void walking_collision(void){
    WxScene s={0};WxEntry entries[2]={0};WxVertex ground[3]={{{0,0,0}},{{3,0,0}},{{0,3,0}}};
    WxVertex wall[4]={{{.5f,0,0}},{{.5f,2,0}},{{.5f,0,3}},{{.5f,2,3}}};
    uint16_t floor_indices[3]={0,1,2},wall_indices[6]={0,1,2,1,3,2};
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)s.slots[i].entry=-1;
    entries[0].kind=WX_KIND_TERRAIN;entries[0].index_count=3;entries[0].radius=5;
    entries[1].kind=WX_KIND_COLLISION;entries[1].index_count=6;entries[1].radius=5;
    s.entries=entries;s.header.count=2;s.slots[0].entry=0;s.slots[0].vertices=ground;s.slots[0].indices=floor_indices;
    s.slots[1].entry=1;s.slots[1].vertices=wall;s.slots[1].indices=wall_indices;
    float from[3]={.2f,.2f,0},height=99;
    CHECK(wx_floor(&s,.2f,.2f,1,-1,&height)&&height==0);
    CHECK(wx_floor(&s,-.00025f,.3f,1,-1,&height)&&height==0);
    CHECK(!wx_floor(&s,-.02f,.3f,1,-1,&height));
    CHECK(!wx_floor(&s,5,5,1,-1,&height));CHECK(!wx_floor(&s,NAN,0,1,-1,&height));
    CHECK(wx_walk(&s,from,.3f,.2f,&height));CHECK(!wx_walk(&s,from,.7f,.2f,&height));
    float reverse[3]={.7f,.2f,0};CHECK(!wx_walk(&s,reverse,.2f,.2f,&height));
    CHECK(!wx_walk(&s,from,2,.2f,&height));
    s.slots[1].entry=-1;CHECK(!wx_walk(&s,from,.3f,.2f,&height));s.slots[1].entry=1;
    float focus[3]={0,1,1},forward[3]={-1,0,0},right[3]={0,1,0},up[3]={0,0,1};
    CHECK(fabsf(wx_camera_distance(&s,focus,forward,right,up,2)-.3f)<.0001f);
    forward[0]=1;CHECK(wx_camera_distance(&s,focus,forward,right,up,2)==2);forward[0]=-1;
    s.slots[1].entry=-1;CHECK(wx_camera_distance(&s,focus,forward,right,up,2)==0);s.slots[1].entry=1;
    CHECK(wx_camera_distance(&s,focus,forward,right,up,NAN)==0);CHECK(wx_camera_distance(&s,focus,forward,right,up,13)==0);
    forward[0]=2;CHECK(wx_camera_distance(&s,focus,forward,right,up,2)==0);forward[0]=-1;
    focus[0]=NAN;CHECK(wx_camera_distance(&s,focus,forward,right,up,2)==0);focus[0]=0;
    // Reversed triangle winding must still stop the camera.
    wall_indices[0]=2;wall_indices[2]=0;wall_indices[3]=2;wall_indices[5]=1;
    CHECK(fabsf(wx_camera_distance(&s,focus,forward,right,up,2)-.3f)<.0001f);
    // A raised model floor wins over terrain without snapping to a high roof.
    wall[0]=(WxVertex){{0,0,.5f}};wall[1]=(WxVertex){{3,0,.5f}};wall[2]=(WxVertex){{0,3,.5f}};
    entries[1].index_count=3;s.slots[1].indices=floor_indices;
    CHECK(wx_walk(&s,from,.3f,.2f,&height)&&fabsf(height-.5f)<.0001f);
    for(unsigned i=0;i<3;i++)wall[i].p[2]=3;
    CHECK(wx_floor(&s,.3f,.2f,.65f,-1,&height)&&height==0);
}
static void shared_poses(void){
    struct Shared {WxPackHeader h;WxEntry e[3];WxVertex v[3];uint16_t indices[3],padding;uint32_t pixel;
        WxSkinVertex skin[3],bad_skin[3];float poses[24];WxAnimation animation[2];} f={0};
    Fixture source=fixture();f.h=source.h;f.h.count=3;f.h.file_size=sizeof f;
    memcpy(f.v,source.v,sizeof f.v);memcpy(f.indices,source.indices,sizeof f.indices);f.pixel=0xffffffff;
    for(unsigned i=0;i<3;i++)f.skin[i].weights[0]=255;
    for(unsigned i=0;i<2;i++){f.poses[i*12]=f.poses[i*12+5]=f.poses[i*12+10]=1;f.poses[i*12+3]=i*2;}
    f.animation[0]=(WxAnimation){2,1,1000,offsetof(struct Shared,skin),offsetof(struct Shared,poses)};
    f.animation[1]=f.animation[0];f.animation[1].skin_offset=offsetof(struct Shared,bad_skin);
    for(unsigned i=0;i<3;i++){f.e[i]=source.e;f.e[i].kind=WX_KIND_CHARACTER;f.e[i].flags=WX_ANIMATED;f.e[i].id=(77<<8)+(i==2?4:0);
        f.e[i].vertex_offset=offsetof(struct Shared,v);f.e[i].index_offset=offsetof(struct Shared,indices);
        f.e[i].texture_offset=offsetof(struct Shared,pixel);f.e[i].reserved[0]=offsetof(struct Shared,animation);}
    FILE* out=fopen("poses-fixture.wxp","wb");CHECK(out);CHECK(fwrite(&f,1,sizeof f,out)==sizeof f);fclose(out);
    WxScene s;uint32_t id=77<<8;CHECK(wx_pack_open(&s,"poses-fixture.wxp"));
    for(unsigned i=0;i<16&&!wx_animation_resident(&s,id);i++){wx_stream_animations(&s,&id,1);CHECK(s.streaming.read_bytes<=WX_STREAM_READ_BYTES&&s.streaming.read_ops<=WX_STREAM_READ_OPS);}
    unsigned own=2*sizeof f.v+sizeof f.indices+sizeof f.skin;
    CHECK(s.loads==2&&!s.failures&&s.bytes==2*own+sizeof f.pixel+sizeof f.poses);
    CHECK(s.slots[0].poses==s.slots[1].poses&&s.poses[s.slots[0].pose_slot].references==2);
    CHECK(wx_animation_resident(&s,id)&&!wx_animation_resident(&s,id+4));
    CHECK(wx_animation_fallback(&s,id+4)==id&&wx_animation_fallback(&s,78<<8)==UINT32_MAX);
    wx_animate(&s,250);CHECK(fabsf(s.slots[1].vertices[0].p[0]-1)<.0001f);
    CHECK(s.animating.sampled==2&&s.animating.palettes==1&&s.animating.vertices==6&&!s.animating.skipped);
    wx_animate(&s,250);CHECK(s.animating.skipped==2&&!s.animating.sampled&&!s.animating.vertices&&!s.animating.palettes);
    unsigned interval=100;wx_animate_scheduled(&s,281,&id,&interval,1);
    CHECK(s.animating.sampled==2&&s.slots[0].animated_time==200&&s.slots[1].animated_time==200);
    CHECK(fabsf(s.slots[1].vertices[0].p[0]-.8f)<.0001f);
    wx_animate_scheduled(&s,299,&id,&interval,1);CHECK(s.animating.skipped==2&&!s.animating.sampled);
    uint32_t same[]={id,id};unsigned rates[]={100,0};wx_animate_scheduled(&s,299,same,rates,2);
    CHECK(s.animating.sampled==2&&s.slots[0].animated_time==299&&s.slots[1].animated_time==299);
    wx_animate_scheduled(&s,UINT32_MAX,&id,&interval,1);CHECK(s.animating.sampled==2);
    wx_animate_scheduled(&s,0,&id,&interval,1);CHECK(s.animating.sampled==2&&s.slots[0].animated_time==0);
    CHECK(wx_animation_interval(35*35,0,0)==0&&wx_animation_interval(35*35+1,0,0)==66);
    CHECK(wx_animation_interval(80*80,5,0)==66&&wx_animation_interval(80*80+1,5,0)==100);
    CHECK(wx_animation_interval(100*100,5,1)==0&&wx_animation_interval(100*100,16,0)==0);
    CHECK(wx_animation_interval(NAN,0,0)==0);
    // A rejected third batch must release only its reference to the shared pose.
    s.entries[2].reserved[0]+=sizeof(WxAnimation);uint32_t both[]={id,id+4};wx_stream_animations(&s,both,2);
    CHECK(s.failures==1&&s.loads==2&&s.bytes==2*own+sizeof f.pixel+sizeof f.poses);
    CHECK(s.poses[s.slots[0].pose_slot].references==2);
    s.entries[2].reserved[0]-=sizeof(WxAnimation);id+=4;wx_stream_animations(&s,&id,1);
    CHECK(s.loads==3&&s.bytes==3*own+sizeof f.pixel+sizeof f.poses&&wx_animation_fallback(&s,id)==id);
    wx_stream_animations(&s,&id,1);
    CHECK(s.loads==3&&s.bytes==own+sizeof f.pixel+sizeof f.poses);
    CHECK(s.slots[2].entry==2&&s.poses[s.slots[2].pose_slot].references==1);
    wx_pack_close(&s);CHECK(!outstanding&&!s.bytes);
    // Same offsets in different files must never alias a pose buffer.
    CHECK(wx_pack_open(&s,"poses-fixture.wxp"));CHECK(wx_pack_attach(&s,"poses-fixture.wxp")==1);id=77<<8;
    for(unsigned i=0;i<16&&!wx_animation_resident(&s,id);i++){wx_stream_animations(&s,&id,1);CHECK(s.streaming.read_bytes<=WX_STREAM_READ_BYTES&&s.streaming.read_ops<=WX_STREAM_READ_OPS);}
    CHECK(s.loads==4&&!s.failures&&s.slots[0].poses!=s.slots[2].poses);
    wx_pack_detach(&s,0);CHECK(s.bytes==2*own+sizeof f.pixel+sizeof f.poses);
    wx_pack_close(&s);CHECK(!outstanding&&!s.bytes);remove("poses-fixture.wxp");
}
static void region_streaming(void){
    Fixture f=fixture();const char* names[]={"M0003131.WXP","M0003231.WXP"};
    for(unsigned i=0;i<2;i++){f.texture=i?0xff00ff00:0xffff0000;FILE* file=fopen(names[i],"wb");CHECK(file);CHECK(fwrite(&f,1,sizeof f,file)==sizeof f);fclose(file);}
    WxScene s;CHECK(wx_pack_open(&s,names[0]));CHECK(wx_pack_attach(&s,names[1])==1);wx_stream(&s,f.h.spawn);
    CHECK(s.header.count==2&&s.loads==2&&!s.failures);CHECK(s.slots[0].texture!=s.slots[1].texture);
    CHECK(*s.slots[0].texture==0xffff0000&&*s.slots[1].texture==0xff00ff00);
    wx_pack_detach(&s,0);CHECK(s.header.count==1&&s.sources[1].first==0);CHECK(s.slots[1].entry==0&&*s.slots[1].texture==0xff00ff00);
    CHECK(wx_pack_attach(&s,names[0])==0);wx_stream(&s,f.h.spawn);CHECK(s.loads==3&&!s.failures);
    available=8u*1024u*1024u;CHECK(wx_pack_attach(&s,names[0])==-1&&s.header.count==2);available=60u*1024u*1024u;
    wx_pack_close(&s);CHECK(!outstanding);
    // Duplicate static placement batches in neighboring ADTs render once.
    f.e.kind=WX_KIND_COLLISION;f.e.flags=WX_PLACEMENT_ID;f.e.reserved[0]=123;f.e.id=42;
    for(unsigned i=0;i<2;i++){FILE* file=fopen(names[i],"wb");CHECK(file);fwrite(&f,1,sizeof f,file);fclose(file);}
    CHECK(wx_pack_open(&s,names[0]));CHECK(wx_pack_attach(&s,names[1])==1);wx_stream(&s,f.h.spawn);CHECK(s.loads==1);
    float from[3]={.1f,.1f,2},floor=0;CHECK(wx_walk(&s,from,.2f,.1f,&floor)&&floor==2);
    wx_pack_detach(&s,0);wx_stream(&s,f.h.spawn);CHECK(s.loads==2&&!s.failures);wx_pack_close(&s);CHECK(!outstanding);
    uint32_t header[4]={0x31495857,1,2,32};WxRegionCell cells[2]={{0,31,31,"M0003131.WXP",sizeof f,0},{0,32,31,"M0003231.WXP",sizeof f,0}};
    FILE* index=fopen("region-fixture.wxi","wb");CHECK(index);fwrite(header,1,16,index);fwrite(cells,1,sizeof cells,index);fclose(index);
    WxRegion region;CHECK(wx_region_open(&region,"region-fixture.wxi"));wx_scene_init(&s);
    float position[3]={.2f,.2f,2};CHECK(wx_region_cell_at(0,position,&cells[0]));CHECK(!wx_region_cell_at(1,position,&cells[0]));
    CHECK(wx_region_update(&region,&s,0,position));CHECK(region.loaded==1);
    // The selected tile stays usable while the adjacent table is prepared.
    for(unsigned frame=0;frame<16&&region.loaded<2;frame++){
        CHECK(wx_region_update(&region,&s,0,position));
        CHECK((region.loaded==1&&s.header.count==1)||(region.loaded==2&&s.header.count==2));
    }
    CHECK(region.loaded==2&&s.header.count==2&&!region.pending.phase);
    position[1]=-3;CHECK(wx_region_update(&region,&s,0,position)&&region.selected==1);
    position[1]=300;CHECK(wx_region_update(&region,&s,0,position)&&region.loaded==1&&s.header.count==1);
    position[1]=-300;CHECK(wx_region_update(&region,&s,0,position)&&region.loaded==1&&region.selected==1&&s.header.count==1);
    position[1]=-600;CHECK(!wx_region_update(&region,&s,0,position)&&region.error[0]);
    position[1]=NAN;CHECK(!wx_region_update(&region,&s,0,position));wx_pack_close(&s);wx_region_close(&region);CHECK(!outstanding);
    cells[1]=cells[0];index=fopen("region-fixture.wxi","wb");CHECK(index);fwrite(header,1,16,index);fwrite(cells,1,sizeof cells,index);fclose(index);
    CHECK(!wx_region_open(&region,"region-fixture.wxi")&&!region.cells);
    strcpy(cells[1].file,"../bad.wxp");cells[1].x=32;index=fopen("region-fixture.wxi","wb");CHECK(index);fwrite(header,1,16,index);fwrite(cells,1,sizeof cells,index);fclose(index);
    CHECK(!wx_region_open(&region,"region-fixture.wxi"));remove(names[0]);remove(names[1]);remove("region-fixture.wxi");
}
static void global_region_streaming(void){
    Fixture f=fixture();f.e.kind=WX_KIND_COLLISION;const char* names[]={"M0003131.WXP","G034.WXP"};
    for(unsigned i=0;i<2;i++){FILE* file=fopen(names[i],"wb");CHECK(file);CHECK(fwrite(&f,1,sizeof f,file)==sizeof f);fclose(file);}
    uint32_t header[4]={0x31495857,2,2,32};
    WxRegionCell cells[2]={{0,31,31,"M0003131.WXP",sizeof f,0},{34,0,0,"G034.WXP",sizeof f,WX_REGION_GLOBAL_WMO}};
    FILE* file=fopen("global-region.wxi","wb");CHECK(file);fwrite(header,1,16,file);fwrite(cells,1,sizeof cells,file);fclose(file);
    WxRegion r;WxScene s;wx_scene_init(&s);CHECK(wx_region_open(&r,"global-region.wxi"));
    float p[3]={.2f,.2f,2};CHECK(wx_region_update(&r,&s,0,p)&&r.selected==0&&r.loaded==1);
    wx_stream(&s,p);CHECK(s.loads==1&&!s.failures);
    CHECK(wx_region_update(&r,&s,34,p)&&r.selected==1&&r.loaded==1&&s.header.count==1);
    wx_stream(&s,p);float z=0;CHECK(wx_floor(&s,p[0],p[1],3,1,&z)&&z==2);
    for(int x=-16000;x<=16000;x+=4000){p[0]=(float)x;p[1]=(float)-x;CHECK(wx_region_cell_at(34,p,&cells[1]));CHECK(wx_region_update(&r,&s,34,p)&&r.selected==1&&r.loaded==1);}
    p[0]=p[1]=.2f;CHECK(!wx_region_update(&r,&s,35,p)&&r.loaded==0&&s.header.count==0);
    CHECK(wx_region_update(&r,&s,34,p)&&r.loaded==1&&!r.failures);wx_pack_close(&s);wx_region_close(&r);CHECK(!outstanding);
    for(unsigned bad=0;bad<7;bad++){
        header[1]=bad==0?1:2;cells[1]=(WxRegionCell){34,0,0,"G034.WXP",sizeof f,WX_REGION_GLOBAL_WMO};
        if(bad==1)cells[1].reserved=2;if(bad==2)cells[1].x=1;
        if(bad==3)strcpy(cells[1].file,"M0340000.WXP");
        if(bad==4)cells[1].bytes=32;
        if(bad==5){cells[1].map=0;strcpy(cells[1].file,"G000.WXP");}
        if(bad==6){cells[0]=cells[1];}
        file=fopen("global-region.wxi","wb");CHECK(file);fwrite(header,1,16,file);fwrite(cells,1,sizeof cells,file);fclose(file);
        CHECK(!wx_region_open(&r,"global-region.wxi")&&!r.cells);
    }
    remove(names[0]);remove(names[1]);remove("global-region.wxi");
}
int main(void){
    compressed_textures();
    shared_poses();
    region_streaming();
    global_region_streaming();
    shared_textures();
    walking_collision();
    WxEntry texture_entry={0};texture_entry.width=texture_entry.height=128;
    texture_entry.flags=WX_TEX_SWIZZLED|WX_MIPMAPPED;texture_entry.reserved[1]=8;
    CHECK(wx_texture_bytes(&texture_entry)==87380);texture_entry.reserved[1]=9;CHECK(!wx_texture_bytes(&texture_entry));
    texture_entry.reserved[1]=8;texture_entry.height=64;CHECK(!wx_texture_bytes(&texture_entry));
    WxMovement movement={0};movement.x=1;movement.y=-2;movement.z=3;movement.time_ms=0x12345678;movement.jump_cos=1;
    uint8_t wire[44];const uint8_t golden[28]={0,0,0,0,0x78,0x56,0x34,0x12,0,0,0x80,0x3f,0,0,0,0xc0,0,0,0x40,0x40,0,0,0,0,0,0,0,0};
    CHECK(wx_movement_encode(&movement,wire,sizeof wire)==28);CHECK(!memcmp(wire,golden,28));
    CHECK(!wx_movement_encode(&movement,wire,27));movement.flags=WX_MOVE_JUMP;movement.jump_speed=8;
    CHECK(wx_movement_encode(&movement,wire,sizeof wire)==44);CHECK(wire[31]==0x41&&wire[35]==0x3f);
    CHECK(!wx_movement_encode(&movement,wire,43));movement.x=NAN;CHECK(!wx_movement_valid(&movement));movement.x=1;
    movement.flags=3;CHECK(!wx_movement_valid(&movement));movement.flags=12;CHECK(!wx_movement_valid(&movement));
    movement.flags=0x02000000;CHECK(!wx_movement_valid(&movement));movement.flags=0;movement.orientation=-1;CHECK(!wx_movement_valid(&movement));
    CHECK(wx_movement_opcode(0,1)==0xb5);CHECK(wx_movement_opcode(1,0)==0xb7);
    CHECK(wx_movement_opcode(0,WX_MOVE_JUMP)==0xbb);CHECK(wx_movement_opcode(WX_MOVE_JUMP,0)==0xc9);
    CHECK(wx_movement_opcode(1,1)==0xee);
    WxControls controls;wx_controls_default(&controls);CHECK(wx_controls_valid(&controls));
    WxRawPad raw={0};WxPad pad;unsigned previous=0;raw.connected=1;
    raw.axes[0]=2000;raw.axes[1]=-2000;wx_input_process(&raw,&controls,&previous,&pad);CHECK(pad.move_x==0&&pad.move_y==0);
    raw.axes[0]=32767;raw.axes[1]=-32768;wx_input_process(&raw,&controls,&previous,&pad);CHECK(fabsf(pad.move_x*pad.move_x+pad.move_y*pad.move_y-1)<.0001f);
    const int buttons[8]={WX_A,WX_B,WX_X,WX_Y,WX_UP,WX_DOWN,WX_LEFT,WX_RIGHT};
    for(int layer=1;layer<=3;layer++)for(int button=0;button<8;button++){
        raw.axes[4]=(layer&1)?32767:0;raw.axes[5]=(layer&2)?32767:0;raw.buttons=1u<<buttons[button];previous=0;
        wx_input_process(&raw,&controls,&previous,&pad);CHECK(pad.slot==(layer-1)*8+button&&pad.action==pad.slot);
        wx_input_process(&raw,&controls,&previous,&pad);CHECK(pad.action==-1);
    }
    raw.axes[4]=32767;raw.axes[5]=0;raw.buttons=1u<<WX_A;previous=0;controls.actions[0]=119;
    wx_input_process(&raw,&controls,&previous,&pad);CHECK(pad.action==119);
    raw.connected=0;wx_input_process(&raw,&controls,&previous,&pad);CHECK(!pad.connected&&pad.buttons==0&&pad.action==-1&&previous==0);
    controls.deadzone=NAN;CHECK(!wx_controls_valid(&controls));wx_controls_default(&controls);controls.actions[23]=255;CHECK(!wx_controls_valid(&controls));
    WxReplayHeader rh={{'W','X','R','1'},2,4};WxReplayRecord rr[2]={{2,{0,-32768,0,0,0,0},1u<<WX_A,1},{4,{0},0,1}};
    FILE* replay_file=fopen("input-fixture.rpl","wb");CHECK(replay_file);CHECK(fwrite(&rh,1,sizeof rh,replay_file)==sizeof rh);CHECK(fwrite(rr,1,sizeof rr,replay_file)==sizeof rr);fclose(replay_file);
    WxReplay replay;CHECK(wx_replay_open(&replay,"input-fixture.rpl"));CHECK(wx_replay_apply(&replay,&raw)&&raw.buttons==(1u<<WX_A)&&raw.axes[1]==-32768);
    CHECK(wx_replay_apply(&replay,&raw)&&raw.buttons==(1u<<WX_A));CHECK(wx_replay_apply(&replay,&raw)&&raw.buttons==0);
    CHECK(wx_replay_apply(&replay,&raw)&&replay.frame==4);CHECK(wx_replay_apply(&replay,&raw)&&!replay.active&&!replay.file);
    rr[1].end_frame=2;replay_file=fopen("input-fixture.rpl","wb");CHECK(replay_file);fwrite(&rh,1,sizeof rh,replay_file);fwrite(rr,1,sizeof rr,replay_file);fclose(replay_file);
    CHECK(!wx_replay_open(&replay,"input-fixture.rpl")&&replay.error);remove("input-fixture.rpl");
    Fixture f=fixture();WxScene s;write_fixture(&f);
    CHECK(wx_pack_open(&s,"runtime-fixture.wxp"));wx_stream(&s,f.h.spawn);
    CHECK(s.loads==1&&s.failures==0);float z=-1;CHECK(wx_ground(&s,.2f,.2f,&z));CHECK(fabsf(z-2)<.0001f);
    CHECK(!wx_ground(&s,4,4,&z));wx_pack_close(&s);CHECK(!outstanding);
    // Old files remain readable. New material semantics cannot be smuggled into
    // an older pack version or into collision geometry.
    for(unsigned version=4;version<=6;version++){
        f=fixture();f.h.version=version;write_fixture(&f);CHECK(wx_pack_open(&s,"runtime-fixture.wxp"));wx_pack_close(&s);
        f.e.flags=WX_UNLIT|(4u<<WX_BLEND_SHIFT);
        if(version<6)reject_open(&f);else{write_fixture(&f);CHECK(wx_pack_open(&s,"runtime-fixture.wxp"));wx_stream(&s,f.h.spawn);CHECK(s.loads==1&&!s.failures);wx_pack_close(&s);CHECK(!outstanding);}
    }
    f=fixture();f.e.flags=7u<<WX_BLEND_SHIFT;reject_open(&f);
    f.e.flags=1u<<WX_BLEND_SHIFT;reject_open(&f);
    f.e.flags=WX_ALPHA_TEST|(2u<<WX_BLEND_SHIFT);reject_open(&f);
    f.e.flags=WX_UNLIT;f.e.kind=WX_KIND_COLLISION;reject_open(&f);
    f=fixture();f.h.count=UINT32_MAX;reject_open(&f);
    f=fixture();f.h.file_size--;reject_open(&f);
    f=fixture();f.h.spawn[0]=NAN;reject_open(&f);
    f=fixture();f.e.vertex_offset=UINT32_MAX;reject_open(&f);
    f=fixture();f.e.index_count=UINT32_MAX;reject_open(&f);
    f=fixture();f.e.texture_offset=0;reject_open(&f);
    f=fixture();f.e.flags=WX_TEX_SWIZZLED;f.e.width=3;reject_open(&f);
    f=fixture();f.indices[2]=3;reject_load(&f);
    f=fixture();f.v[1].uv[0]=NAN;reject_load(&f);
    f=fixture();fail_after=0;reject_load(&f);fail_after=1;reject_load(&f);fail_after=-1;
    available=8u*1024u*1024u;reject_open(&f);available=60u*1024u*1024u;
    f.e.kind=WX_KIND_CHARACTER;f.e.flags=WX_ANIMATED;f.e.reserved[0]=OFFSET(animation);
    f.animation=(WxAnimation){2,1,1000,OFFSET(skin),OFFSET(poses)};
    for(int i=0;i<3;i++)f.skin[i].weights[0]=255;
    for(int i=0;i<2;i++){f.poses[i*12]=1;f.poses[i*12+5]=1;f.poses[i*12+10]=1;f.poses[i*12+3]=i*2;}
    write_fixture(&f);CHECK(wx_pack_open(&s,"runtime-fixture.wxp"));wx_stream(&s,f.h.spawn);CHECK(s.loads==1);
    wx_animate(&s,250);CHECK(fabsf(s.slots[0].vertices[0].p[0]-1)<.0001f);
    wx_animate(&s,1000);CHECK(fabsf(s.slots[0].vertices[0].p[0])<.0001f);
    uint32_t animation_id=UINT32_MAX;wx_animate_ids(&s,250,&animation_id,1);CHECK(fabsf(s.slots[0].vertices[0].p[0])<.0001f);
    animation_id=f.e.id;wx_animate_ids(&s,250,&animation_id,1);CHECK(fabsf(s.slots[0].vertices[0].p[0]-1)<.0001f);
    wx_pack_close(&s);CHECK(!outstanding);
    f.skin[1].bones[0]=1;reject_load(&f);f.skin[1].bones[0]=0;
    f.animation.frames=UINT32_MAX;reject_open(&f);
    f=fixture();f.e.id=77<<8;write_fixture(&f);CHECK(wx_pack_open(&s,"runtime-fixture.wxp"));
    uint32_t display=77;wx_stream_displays(&s,&display,1);CHECK(s.loads==1&&s.bytes>0);
    wx_stream_displays(&s,&display,1);CHECK(s.loads==1);
    display=78;wx_stream_displays(&s,&display,1);CHECK(s.bytes==0&&!outstanding);
    display=77;s.budget_bytes=1;wx_stream_displays(&s,&display,1);CHECK(s.loads==1&&s.failures==1&&!s.bytes);
    wx_pack_close(&s);CHECK(!outstanding);
    remove("runtime-fixture.wxp");printf("Runtime boundary, collision, allocation and animation checks: %d passed\n",checks);return 0;
}
