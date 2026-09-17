#include "wx_avatar.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
static unsigned free_bytes=60u*1024u*1024u,outstanding,checks;
static int fail_after=-1;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"Avatar FAIL line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return free_bytes;}
void* wx_gpu_alloc(unsigned size){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;unsigned* p=malloc(size+4);if(!p)return NULL;*p=size;free_bytes-=size;outstanding++;return p+1;}
void wx_gpu_free(void* value){if(value){unsigned* p=(unsigned*)value-1;free_bytes+=*p;outstanding--;free(p);}}
typedef struct Pack {
    WxPackHeader h;WxEntry entries[21];WxVertex v[3];uint16_t indices[4];
    uint32_t pixels[WX_AVATAR_MIP_BYTES/4];WxSkinVertex skin[3];float poses[24];WxAnimation animation;
} Pack;
typedef struct Meta {WxAvatarHeader h;WxAvatarItem items[3];uint8_t base[WX_AVATAR_ATLAS_BYTES],overlay[8192];} Meta;
static Pack pack;static Meta meta;
static void fixture(void){
    memset(&pack,0,sizeof pack);memset(&meta,0,sizeof meta);
    pack.h=(WxPackHeader){{'W','X','P','1'},8 /* geometry/profile fixture without v9 world metadata */,7,sizeof(WxEntry),{0},sizeof pack};
    unsigned families[]={0,401,501,502,801,1301,4096};
    pack.v[1].p[0]=1;pack.v[2].p[1]=1;pack.indices[1]=1;pack.indices[2]=2;
    for(unsigned i=0;i<3;i++){pack.skin[i].weights[0]=255;pack.v[i].n[2]=1;}
    for(unsigned i=0;i<2;i++)pack.poses[12*i]=pack.poses[12*i+5]=pack.poses[12*i+10]=1;
    pack.animation=(WxAnimation){2,1,1000,offsetof(Pack,skin),offsetof(Pack,poses)};
    for(unsigned i=0;i<7;i++){
        WxEntry* e=&pack.entries[i];e->kind=WX_KIND_CHARACTER;e->id=families[i]<<8;e->vertex_count=e->index_count=3;
        e->vertex_offset=offsetof(Pack,v);e->index_offset=offsetof(Pack,indices);e->texture_offset=offsetof(Pack,pixels);
        e->width=e->height=128;e->flags=WX_ANIMATED|WX_MIPMAPPED|WX_TEX_SWIZZLED;e->reserved[0]=offsetof(Pack,animation);e->reserved[1]=8;
    }
    memcpy(meta.h.magic,"WXAV",4);meta.h.version=1;meta.h.file_size=sizeof meta;meta.h.item_count=3;meta.h.item_size=sizeof(WxAvatarItem);
    meta.h.look[0]=1;meta.h.scalp=1;meta.h.body_texture=offsetof(Pack,pixels);meta.h.base_offset=offsetof(Meta,base);
    meta.h.items_offset=offsetof(Meta,items);meta.h.pack_size=sizeof pack;
    meta.items[0].display=100;meta.items[0].slot=16;meta.items[0].component[0]=4096;
    meta.items[1].display=101;meta.items[1].slot=3;meta.items[1].overlay[3]=offsetof(Meta,overlay);
    meta.items[2].display=102;meta.items[2].slot=7;meta.items[2].geoset[0]=1;
    for(unsigned i=0;i<WX_AVATAR_ATLAS_BYTES;i+=4){meta.base[i]=10;meta.base[i+1]=20;meta.base[i+2]=30;meta.base[i+3]=255;}
    for(unsigned i=0;i<8192;i+=4){meta.overlay[i]=100;meta.overlay[i+3]=128;}
}
static void write_files(void){FILE* f=fopen("avatar-fixture.wxp","wb");CHECK(f);CHECK(fwrite(&pack,1,sizeof pack,f)==sizeof pack);fclose(f);
    f=fopen("avatar-fixture.wxa","wb");CHECK(f);CHECK(fwrite(&meta,1,sizeof meta,f)==sizeof meta);fclose(f);}
static void reject(void){write_files();WxAvatar a;CHECK(!wx_avatar_open(&a,"avatar-fixture.wxp","avatar-fixture.wxa"));CHECK(a.failures==1&&!a.file&&!a.items&&!a.body&&!outstanding);}
static int selected(const WxAvatar* a,unsigned id){for(unsigned i=0;i<a->count;i++)if(a->families[i]==id)return 1;return 0;}
static void settle(WxAvatar* a,const WxWorldView* w,unsigned clip,unsigned now){
    for(unsigned frame=0;frame<120;frame++){
        wx_stream_frame_begin(8);wx_avatar_update(a,w,clip,now+frame*33);
        const WxStreamFrame* m=wx_stream_frame_metrics();
        CHECK(m->total.read_bytes<=65536&&m->total.read_ops<=16&&m->total.allocations<=3);
        CHECK(a->compose.read_bytes<=65536&&a->compose.read_ops<=8&&a->compose.work_pixels<=WX_AVATAR_COMPOSE_PIXELS);
        wx_stream_frame_end();CHECK(!a->compose.failures);
        if(!a->compose.phase&&a->ready&&a->appearance_ready)return;
    }CHECK(0);
}
static void animation_switch_tests(void){
    fixture();pack.h.count=21;
    for(unsigned i=0;i<7;i++){
        pack.entries[i+7]=pack.entries[i];pack.entries[i+7].id|=5;
        pack.entries[i+14]=pack.entries[i];pack.entries[i+14].id|=16;
    }
    write_files();WxAvatar a;CHECK(wx_avatar_open(&a,"avatar-fixture.wxp","avatar-fixture.wxa"));
    WxWorldView world={0};world.active=1;world.race=1;
    world.equipment_display[16]=100;world.equipment_display[7]=102;
    settle(&a,&world,0,0);
    CHECK(a.ready&&a.count==6&&a.complete_clip==0);unsigned idle_bytes=a.scene.bytes;
    for(unsigned frame=0;frame<3;frame++){
        unsigned loads=a.scene.loads;wx_avatar_update(&a,&world,5,500+frame*33);
        CHECK(a.ready&&a.scene.loads==loads+2&&a.complete_clip==(frame<2?0:5));
        for(unsigned i=0;i<a.count;i++)CHECK((a.ids[i]&255)==a.complete_clip);
    }
    unsigned paired_bytes=a.scene.bytes,loads=a.scene.loads;
    CHECK(paired_bytes>idle_bytes&&paired_bytes<a.scene.budget_bytes);
    for(unsigned frame=0;frame<100;frame++){
        unsigned clip=frame%2?5:0;wx_avatar_update(&a,&world,clip,700+frame*33);
        CHECK(a.ready&&a.complete_clip==clip&&a.scene.loads==loads&&a.scene.bytes==paired_bytes);
    }
    // A third sequence drops the optional clip and keeps all parts of the last
    // drawn sequence, including body and held equipment, until the new one fits.
    for(unsigned frame=0;frame<3;frame++){
        wx_avatar_update(&a,&world,16,5000+frame*33);
        CHECK(a.ready&&a.complete_clip==(frame<2?5:16)&&a.scene.bytes<=paired_bytes);
        for(unsigned i=0;i<a.count;i++)CHECK((a.ids[i]&255)==a.complete_clip);
    }
    wx_avatar_update(&a,&world,16,5200);CHECK(a.ready&&a.scene.bytes==idle_bytes);
    for(unsigned i=0;i<4;i++)wx_avatar_update(&a,&world,0,5300+i*33);
    CHECK(a.ready&&a.complete_clip==0&&a.scene.bytes==idle_bytes);
    // An allocation failure keeps a complete drawable fallback, and retry can
    // recover without leaks or exceeding the unchanged budget.
    fail_after=0;wx_avatar_update(&a,&world,5,5500);
    CHECK(a.ready&&a.complete_clip==0&&a.scene.failures==1&&a.scene.bytes==idle_bytes);fail_after=-1;
    for(unsigned i=0;i<4;i++)wx_avatar_update(&a,&world,5,5600+i*33);
    CHECK(a.ready&&a.complete_clip==5&&a.scene.bytes==paired_bytes);
    unsigned before=free_bytes,low=11u*1024u*1024u;free_bytes=low;
    wx_avatar_update(&a,&world,0,5800);
    CHECK(a.ready&&a.complete_clip==0&&a.scene.bytes==idle_bytes);free_bytes=before+(free_bytes-low);
    a.scene.budget_bytes=idle_bytes;
    wx_avatar_update(&a,&world,5,5900);
    CHECK(a.ready&&a.complete_clip==0&&a.scene.bytes==idle_bytes&&a.scene.failures==2);
    wx_avatar_close(&a);CHECK(!outstanding&&free_bytes==60u*1024u*1024u);
}
static void atomic_composition_tests(void){
    fixture();write_files();WxAvatar a;CHECK(wx_avatar_open(&a,"avatar-fixture.wxp","avatar-fixture.wxa"));
    WxWorldView w={0};w.active=1;w.race=1;w.equipment_display[3]=101;settle(&a,&w,0,0);
    uint32_t saved[WX_AVATAR_MIP_BYTES/4],families[WX_AVATAR_PARTS];memcpy(saved,a.body,sizeof saved);memcpy(families,a.families,sizeof families);
    unsigned revision=a.revision,count=a.count,accounted=a.bytes;uint32_t* published=a.body;
    // A queued equipment change cannot touch published pixels, geometry, or key.
    w.equipment_display[3]=0;w.equipment_display[16]=100;w.equipment_display[7]=102;
    for(unsigned i=0;i<4;i++){
        wx_stream_frame_begin(0);wx_avatar_update(&a,&w,0,i*33);wx_stream_frame_end();
        CHECK(a.ready&&a.revision==revision&&a.body==published&&!memcmp(saved,a.body,sizeof saved));
        CHECK(a.count==count&&!memcmp(families,a.families,sizeof families)&&!a.compose.read_bytes&&!a.compose.read_ops);
    }
    for(unsigned i=0;i<120;i++){
        wx_stream_frame_begin(8);wx_avatar_update(&a,&w,0,200+i*33);wx_stream_frame_end();
        CHECK(a.ready&&a.bytes==accounted&&a.compose.work_pixels<=WX_AVATAR_COMPOSE_PIXELS);
        if(a.revision!=revision)break;
        CHECK(a.body==published&&!memcmp(saved,a.body,sizeof saved)&&!memcmp(families,a.families,sizeof families));
    }
    CHECK(a.revision==revision+1&&a.body!=published&&a.body[4096]==0xff0a141e&&selected(&a,4096)&&selected(&a,502));
    CHECK(!memcmp(a.equipment,w.equipment_display,sizeof a.equipment));
    // Superseded work reuses only the unpublished set, even after partial reads.
    revision=a.revision;published=a.body;memcpy(saved,a.body,sizeof saved);
    for(unsigned change=0;change<24;change++){
        w.equipment_display[3]=change&1?101:0;w.equipment_display[7]=change&1?0:102;
        wx_stream_frame_begin(8);wx_avatar_update(&a,&w,0,5000+change*33);wx_stream_frame_end();
        CHECK(a.ready&&a.revision==revision&&a.body==published&&!memcmp(a.body,saved,sizeof saved));
    }
    CHECK(a.compose.cancelled>=10&&!a.compose.failures);settle(&a,&w,0,6000);
    CHECK(a.ready&&a.body[4096]==0xff370a0f&&!selected(&a,502)&&selected(&a,501));
    // If new geometry cannot fit, the prior complete appearance remains pinned.
    wx_avatar_update(&a,&w,0,6999); // Retire the no-longer-published families.
    revision=a.revision;published=a.body;unsigned budget=a.scene.budget_bytes;a.scene.budget_bytes=a.scene.bytes;
    w.equipment_display[7]=102;
    for(unsigned i=0;i<40;i++){wx_stream_frame_begin(8);wx_avatar_update(&a,&w,0,7000+i*33);wx_stream_frame_end();CHECK(a.ready);}
    CHECK(a.revision==revision&&a.body==published&&a.compose.phase==7&&a.scene.failures>0);
    a.scene.budget_bytes=budget;settle(&a,&w,0,9000);CHECK(a.revision==revision+1&&selected(&a,502));
    // A rejected layer never publishes half-written pixels. A new request recovers.
    revision=a.revision;published=a.body;w.equipment_display[3]=0;
    wx_stream_frame_begin(0);wx_avatar_update(&a,&w,0,10000);wx_stream_frame_end();a.compose.layers[0].offset=UINT32_MAX;
    wx_stream_frame_begin(8);wx_avatar_update(&a,&w,0,10033);wx_stream_frame_end();
    CHECK(a.ready&&a.body==published&&a.revision==revision&&a.compose.phase==8&&a.compose.failures==1);
    w.equipment_display[3]=101;wx_avatar_update(&a,&w,0,10066);CHECK(a.ready&&!a.compose.phase&&a.revision==revision);
    wx_avatar_close(&a);CHECK(!outstanding&&free_bytes==60u*1024u*1024u);
    // Every allocation in a fresh unpublished texture set rolls back together.
    for(int fail=0;fail<2;fail++){
        CHECK(wx_avatar_open(&a,"avatar-fixture.wxp","avatar-fixture.wxa"));unsigned before=outstanding;
        fail_after=fail;wx_stream_frame_begin(8);wx_avatar_update(&a,&w,0,0);wx_stream_frame_end();fail_after=-1;
        CHECK(a.compose.phase==8&&a.compose.failures==1&&!a.ready&&outstanding==before);
        CHECK(!a.compose.body&&!a.compose.cape);wx_avatar_close(&a);CHECK(!outstanding);
    }
}
static void profile_fixture(unsigned gender){
    fixture();meta.h.look[1]=gender;write_files();char pack_path[80],meta_path[80];
    CHECK(wx_avatar_path(pack_path,sizeof pack_path,"",meta.h.look,0));CHECK(wx_avatar_path(meta_path,sizeof meta_path,"",meta.h.look,1));
    remove(pack_path);remove(meta_path);CHECK(!rename("avatar-fixture.wxp",pack_path));CHECK(!rename("avatar-fixture.wxa",meta_path));
}
static int select_ready(WxAvatar* a,WxAvatarSelection* selection,const WxWorldView* world,const char* dir){
    for(unsigned i=0;i<512;i++){
        wx_stream_frame_begin(8);int result=wx_avatar_select(a,selection,world,dir);
        const WxStreamFrame* budget=wx_stream_frame_metrics();
        CHECK(budget->total.read_bytes<=65536&&budget->total.read_ops<=16&&budget->total.scan_bytes<=65536&&budget->total.allocations<=3);
        wx_stream_frame_end();
        if(result||!selection->pending)return result;
        CHECK(!a->profile_ready&&!a->ready);
    }CHECK(0);return 0;
}
static void profile_open_tests(void){
    fixture();write_files();WxAvatar a={0};wx_scene_init(&a.scene);
    CHECK(wx_avatar_open_begin(&a,"avatar-fixture.wxp","avatar-fixture.wxa"));
    for(unsigned i=0;i<8;i++){
        wx_stream_frame_begin(0);CHECK(wx_avatar_open_pump(&a)==0);wx_stream_frame_end();
        CHECK(!a.file&&!a.opening.pack.file&&!a.scene.file&&!a.ready&&!a.profile_ready&&!outstanding);
    }
    wx_stream_frame_begin(8);CHECK(wx_avatar_open_pump(&a)==0);wx_stream_frame_end();
    CHECK(a.opening.pack.file||a.scene.file);wx_avatar_close(&a);
    CHECK(!a.file&&!a.opening.pack.file&&!a.opening.pack.entries&&!a.profile_ready&&!outstanding);
    // Close safely at each reachable phase; verify no stale render identity.
    for(unsigned stop=1;stop<=6;stop++){
        CHECK(wx_avatar_open_begin(&a,"avatar-fixture.wxp","avatar-fixture.wxa"));
        for(unsigned tick=0;tick<stop;tick++){
            wx_stream_frame_begin(8);int r=wx_avatar_open_pump(&a);wx_stream_frame_end();CHECK(r>=0);if(r)break;
        }
        wx_avatar_close(&a);CHECK(!outstanding&&!a.opening.looks_file&&!a.scene.file&&!a.ready);
    }
    CHECK(wx_avatar_open_begin(&a,"avatar-fixture.wxp","avatar-fixture.wxa"));int result=0;
    for(unsigned tick=0;tick<100&&!result;tick++){
        wx_stream_frame_begin(8);result=wx_avatar_open_pump(&a);
        CHECK(a.opening.read_bytes<=65536&&a.opening.read_ops<=8&&a.opening.scan_bytes<=65536);wx_stream_frame_end();
    }
    CHECK(result==1&&a.profile_ready&&!a.ready&&!memcmp(&a.scene.header,&pack.h,sizeof pack.h));
    CHECK(a.family_bits[4096/32]&1u);WxWorldView w={0};w.active=1;w.race=1;settle(&a,&w,0,0);CHECK(a.ready);
    uint32_t* saved_body=a.body;wx_stream_frame_begin(8);
    CHECK(!wx_avatar_open(&a,"avatar-fixture.wxp","avatar-fixture.wxa")&&a.ready&&a.body==saved_body);
    CHECK(!wx_pack_open(&a.scene,"avatar-fixture.wxp")&&a.scene.file&&a.scene.entries);wx_stream_frame_end();
    wx_avatar_close(&a);CHECK(!outstanding);
    profile_fixture(0);profile_fixture(1);WxAvatarSelection selection={0};
    CHECK(select_ready(&a,&selection,&w,"")&&a.profile_ready);settle(&a,&w,0,0);
    w.gender=1;CHECK(!wx_avatar_select(&a,&selection,&w,"")&&selection.pending&&!a.ready&&!a.profile_ready&&!outstanding);
    wx_stream_frame_begin(8);CHECK(!wx_avatar_select(&a,&selection,&w,""));wx_stream_frame_end();
    w.gender=0;CHECK(!wx_avatar_select(&a,&selection,&w,"")&&selection.cancelled==1&&!a.ready&&!a.profile_ready);
    w.active=0;CHECK(!wx_avatar_select(&a,&selection,&w,"")&&selection.cancelled==2&&!selection.pending&&!a.opening.phase&&!outstanding);
    w.active=1;CHECK(select_ready(&a,&selection,&w,"")&&a.header.look[1]==0);wx_avatar_close(&a);
    remove("A01000000000000.WXP");remove("A01000000000000.WXA");remove("A01010000000000.WXP");remove("A01010000000000.WXA");
}
static void selection_tests(void){
    uint32_t look[7]={1,0,2,3,4,5,6};char path[80];
    CHECK(wx_avatar_path(path,sizeof path,"D:\\",look,0)&&!strcmp(path,"D:\\A01000203040506.WXP"));
    CHECK(wx_avatar_path(path,sizeof path,"profiles",look,1)&&!strcmp(path,"profiles/A01000203040506.WXA"));
    CHECK(!wx_avatar_path(path,5,"profiles",look,0));CHECK(!wx_avatar_path(NULL,0,"",look,0));
    look[0]=0;CHECK(!wx_avatar_path(path,sizeof path,"",look,0));look[0]=9;CHECK(!wx_avatar_path(path,sizeof path,"",look,0));
    look[0]=1;look[1]=2;CHECK(!wx_avatar_path(path,sizeof path,"",look,0));look[1]=0;look[6]=256;CHECK(!wx_avatar_path(path,sizeof path,"",look,0));
    profile_fixture(0);profile_fixture(1);
    WxAvatar a={0};wx_scene_init(&a.scene);WxAvatarSelection selection={0};WxWorldView world={0};world.race=1;world.revision=1;
    CHECK(!select_ready(&a,&selection,&world,"")&&!selection.attempts);
    world.active=1;CHECK(select_ready(&a,&selection,&world,"")&&selection.attempts==1&&selection.changes==1&&!selection.failures);
    for(unsigned i=0;i<100;i++){CHECK(select_ready(&a,&selection,&world,""));wx_avatar_update(&a,&world,0,i*33);}
    CHECK(selection.attempts==1&&a.ready&&a.matched);
    unsigned first_bytes=free_bytes;world.gender=1;CHECK(select_ready(&a,&selection,&world,""));
    CHECK(selection.attempts==2&&selection.changes==2&&a.header.look[1]==1);
    for(unsigned i=0;i<20;i++)wx_avatar_update(&a,&world,0,i*33);
    CHECK(a.ready&&a.matched&&free_bytes==first_bytes);
    world.skin=1;CHECK(!select_ready(&a,&selection,&world,"")&&!a.file&&!outstanding&&a.failures==1);
    for(unsigned i=0;i<100;i++)CHECK(!select_ready(&a,&selection,&world,""));
    CHECK(selection.attempts==3&&selection.failures==1);
    world.revision++;CHECK(!select_ready(&a,&selection,&world,"")&&selection.attempts==4&&selection.failures==2);
    world.skin=0;world.gender=0;CHECK(select_ready(&a,&selection,&world,"")&&selection.changes==3);
    world.active=0;wx_avatar_update(&a,&world,0,0);CHECK(!a.ready&&!a.matched);
    world.active=1;world.revision++;CHECK(select_ready(&a,&selection,&world,"")&&selection.attempts==5);
    wx_avatar_close(&a);CHECK(!outstanding);
    // A valid companion pair stored under the wrong profile name is rejected.
    fixture();write_files();remove("A01010000000000.WXA");CHECK(!rename("avatar-fixture.wxa","A01010000000000.WXA"));
    world.gender=1;CHECK(!select_ready(&a,&selection,&world,"")&&!a.file&&!outstanding);
    world.gender=0;fail_after=1;CHECK(!select_ready(&a,&selection,&world,"")&&!outstanding);fail_after=-1;
    world.revision++;CHECK(select_ready(&a,&selection,&world,""));wx_avatar_close(&a);CHECK(!outstanding);
    remove("A01000000000000.WXP");remove("A01000000000000.WXA");remove("A01010000000000.WXP");remove("A01010000000000.WXA");
}
int main(int argc,char** argv){
    if(argc==3){WxAvatar a;CHECK(wx_avatar_open(&a,argv[1],argv[2]));CHECK(wx_pack_verify(&a.scene));WxWorldView world={0};
        world.active=1;world.race=a.header.look[0];world.gender=a.header.look[1];world.skin=a.header.look[2];world.face=a.header.look[3];
        world.hair_style=a.header.look[4];world.hair_color=a.header.look[5];world.facial_hair=a.header.look[6];
        for(unsigned i=0;i<a.header.item_count;i++)world.equipment_display[a.items[i].slot]=a.items[i].display;
        const unsigned clips[]={0,5,0,16,0,6,0};
        for(unsigned c=0;c<sizeof clips/sizeof *clips;c++){
            for(unsigned i=0;i<100;i++)wx_avatar_update(&a,&world,clips[c],(c*100+i)*33);
            CHECK(a.ready&&a.matched&&!a.failures&&!a.scene.failures&&!a.missing&&a.scene.bytes<=a.scene.budget_bytes);
        }
        printf("Prepared avatar: %u selected components, %u resident bytes, %u auxiliary bytes\n",a.count,a.scene.bytes,a.bytes);
        wx_avatar_close(&a);CHECK(!outstanding);return 0;
    }
    fixture();write_files();WxAvatar a;CHECK(wx_avatar_open(&a,"avatar-fixture.wxp","avatar-fixture.wxa"));
    CHECK(a.body[0]==0xff0a141e&&a.body[WX_AVATAR_MIP_BYTES/4-1]==0xff0a141e);
    WxWorldView world={0};world.active=1;world.race=1;world.equipment_ready_mask=0x7ffff;
    world.equipment_display[16]=100;world.equipment_display[3]=101;world.equipment_display[7]=102;
    settle(&a,&world,0,0);
    CHECK(a.ready&&a.matched&&!a.missing&&a.revision==1&&a.count==6&&!a.failures&&!a.scene.failures);
    CHECK(selected(&a,4096)&&selected(&a,502)&&!selected(&a,501));
    CHECK(a.body[4096]==0xff370a0f); // x=64,y=0, alpha-over shirt on the torso
    unsigned loads=a.scene.loads,bytes=a.scene.bytes;world.equipment_display[16]=0;
    settle(&a,&world,0,400);wx_avatar_update(&a,&world,0,432);CHECK(a.ready&&!selected(&a,4096)&&a.revision==2&&a.scene.bytes<bytes);
    CHECK(a.scene.loads==loads&&a.body[4096]==0xff370a0f);
    world.equipment_display[16]=100;settle(&a,&world,0,433);CHECK(a.ready&&selected(&a,4096)&&a.scene.loads==loads+1);
    world.equipment_display[7]=0;settle(&a,&world,0,466);CHECK(a.ready&&selected(&a,501)&&!selected(&a,502));
    world.equipment_display[3]=0;settle(&a,&world,4,500);CHECK(a.ready&&a.body[4096]==0xff0a141e&&a.ids[0]==0);
    world.equipment_display[16]=999;settle(&a,&world,0,533);CHECK(a.ready&&a.missing==1&&!selected(&a,4096));
    world.skin=1;wx_avatar_update(&a,&world,0,566);CHECK(!a.matched&&!a.ready&&!a.scene.bytes);
    world.skin=0;settle(&a,&world,0,600);CHECK(a.ready&&a.matched);
    world.display_id=2;world.native_display_id=1;wx_avatar_update(&a,&world,0,900);CHECK(!a.matched&&!a.ready);
    wx_avatar_close(&a);CHECK(!outstanding);
    for(unsigned i=0;i<2;i++){fixture();fail_after=i;reject();}fail_after=-1;
    fixture();free_bytes=8u*1024u*1024u;reject();free_bytes=60u*1024u*1024u;
    fixture();meta.h.file_size--;reject();fixture();meta.h.item_count=UINT32_MAX;reject();
    fixture();meta.h.item_size--;reject();fixture();meta.h.items_offset=UINT32_MAX;reject();
    fixture();meta.h.base_offset=UINT32_MAX;reject();fixture();meta.h.pack_size--;reject();
    fixture();meta.h.body_texture=0;reject();fixture();meta.h.look[0]=9;reject();
    fixture();meta.h.facial[0]=100;reject();fixture();meta.items[0].slot=19;reject();
    fixture();meta.items[1]=meta.items[0];reject();fixture();meta.items[0].component[0]=4098;reject();
    fixture();meta.items[0].overlay[0]=sizeof meta-1;reject();fixture();meta.items[0].cape=meta.h.base_offset;reject();
    fixture();meta.items[2].geoset[0]=99;reject();fixture();pack.entries[0].id=5000<<8;reject();
    fixture();memset(meta.overlay,0,sizeof meta.overlay);for(unsigned i=0;i<8192;i+=4){meta.overlay[i]=255;meta.overlay[i+2]=255;meta.overlay[i+3]=255;}
    CHECK(wx_avatar_blend(meta.base,meta.overlay,3)&&meta.base[64*4]==10);CHECK(!wx_avatar_blend(meta.base,meta.overlay,10));
    CHECK(!wx_avatar_blend(NULL,meta.overlay,3));CHECK(!wx_avatar_blend(meta.base,NULL,3));
    selection_tests();animation_switch_tests();atomic_composition_tests();profile_open_tests();remove("avatar-fixture.wxp");remove("avatar-fixture.wxa");printf("Avatar asset, composition, equipment and allocation checks: %u passed\n",checks);return 0;
}
