#include "wx_runtime.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
static unsigned checks,allocated,objects;static int fail_after=-1;
static unsigned available=48u*1024u*1024u;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Stream FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return available-allocated;}
void* wx_gpu_alloc(unsigned bytes){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;unsigned* p=malloc(bytes+4);if(!p)return NULL;*p=bytes;allocated+=bytes;objects++;return p+1;}
void wx_gpu_free(void* v){if(v){unsigned* p=(unsigned*)v-1;allocated-=*p;objects--;free(p);}}
typedef struct Large {
    WxPackHeader header;WxEntry entry;WxVertex vertices[6000];uint16_t indices[18000];
    uint32_t pixels[256*256];WxSkinVertex skin[6000];float poses[2*256*12];WxAnimation animation;
} Large;
static Large pack;static WxScene scene;
static void fixture(unsigned bad,int animated){
    memset(&pack,0,sizeof pack);pack.header=(WxPackHeader){{'W','X','P','1'},5,1,sizeof(WxEntry),{0},sizeof pack};
    pack.entry=(WxEntry){0};pack.entry.kind=WX_KIND_TERRAIN;pack.entry.vertex_count=6000;pack.entry.index_count=18000;
    pack.entry.width=pack.entry.height=256;pack.entry.radius=3;pack.entry.vertex_offset=offsetof(Large,vertices);
    pack.entry.index_offset=offsetof(Large,indices);pack.entry.texture_offset=offsetof(Large,pixels);
    for(unsigned i=0;i<6000;i++){pack.vertices[i].p[0]=i%3==1;pack.vertices[i].p[1]=i%3==2;pack.vertices[i].n[2]=1;pack.skin[i].weights[0]=255;}
    for(unsigned i=0;i<18000;i++)pack.indices[i]=i%6000;
    for(unsigned i=0;i<256*256;i++)pack.pixels[i]=0xff000000|i;
    for(unsigned i=0;i<2*256;i++)pack.poses[i*12]=pack.poses[i*12+5]=pack.poses[i*12+10]=1;
    pack.animation=(WxAnimation){2,256,1000,offsetof(Large,skin),offsetof(Large,poses)};
    if(animated){pack.entry.flags=WX_ANIMATED;pack.entry.reserved[0]=offsetof(Large,animation);}
    if(bad==1)pack.vertices[5999].uv[1]=NAN;if(bad==2)pack.indices[17999]=6000;
    if(bad==3)pack.skin[5999].weights[0]=0;if(bad==4)pack.poses[2*256*12-1]=INFINITY;
    FILE* f=fopen("stream-fixture.wxp","wb");CHECK(f);CHECK(fwrite(&pack,1,sizeof pack,f)==sizeof pack);CHECK(!fclose(f));
}
static unsigned pump(void){
    for(unsigned frame=0;frame<200;frame++){
        wx_stream(&scene,pack.header.spawn);
        CHECK(scene.streaming.read_bytes<=WX_STREAM_READ_BYTES&&scene.streaming.read_ops<=WX_STREAM_READ_OPS&&scene.streaming.scan_bytes<=WX_STREAM_SCAN_BYTES);
        if(scene.failures||wx_stream_ready(&scene))return frame+1;
        CHECK(scene.stream_job.phase&&scene.streaming.pending_bytes&&scene.bytes==scene.streaming.pending_bytes);
        CHECK(scene.slots[scene.stream_job.slot].entry<0);float z=0;CHECK(!wx_ground(&scene,.1f,.1f,&z));
    }CHECK(0);return 0;
}
static void index_fixture(unsigned bad){
    fixture(0,0);unsigned delta=1024*sizeof(WxEntry);WxPackHeader h=pack.header;h.count=1025;h.file_size+=delta;
    FILE* f=fopen("stream-index.wxp","wb");CHECK(f);CHECK(fwrite(&h,1,sizeof h,f)==sizeof h);
    for(unsigned i=0;i<h.count;i++){WxEntry e=pack.entry;e.id=i;e.vertex_offset+=delta;e.index_offset+=delta;e.texture_offset+=delta;
        if(bad&&i==1024)e.radius=NAN;CHECK(fwrite(&e,1,sizeof e,f)==sizeof e);}
    unsigned skip=sizeof(WxPackHeader)+sizeof(WxEntry);CHECK(fwrite((unsigned char*)&pack+skip,1,sizeof pack-skip,f)==sizeof pack-skip);CHECK(!fclose(f));
}
static void staged_index(void){
    WxPackPending pending={0};
    for(unsigned bad=0;bad<2;bad++){
        index_fixture(bad);wx_scene_init(&scene);CHECK(wx_pack_attach_begin(&pending,"stream-index.wxp",sizeof pack+1024*sizeof(WxEntry)));
        int result=0;unsigned calls=0;
        while(!result&&calls++<20){result=wx_pack_attach_pump(&scene,&pending);
            CHECK(pending.read_bytes<=WX_STREAM_CHUNK_BYTES&&pending.read_ops<=WX_STREAM_READ_OPS&&pending.checked_frame<=512);
            if(!result)CHECK(!scene.file&&!scene.header.count&&!scene.entries);}
        CHECK(calls>=4&&calls<20&&result==(bad?-1:1));
        if(!bad)CHECK(scene.header.count==1025&&scene.sources[0].bytes==sizeof pack+1024*sizeof(WxEntry)&&!pending.entries&&!pending.file);
        else CHECK(!scene.entries&&!scene.file&&!pending.file&&!pending.entries);
        wx_pack_close(&scene);wx_pack_attach_cancel(&pending);
    }
    index_fixture(0);CHECK(wx_pack_attach_begin(&pending,"stream-index.wxp",sizeof pack+1024*sizeof(WxEntry)));
    CHECK(!wx_pack_attach_pump(&scene,&pending));wx_pack_attach_cancel(&pending);CHECK(!pending.file&&!pending.entries&&!pending.phase);
    CHECK(!wx_pack_attach_begin(&pending,"stream-index.wxp",sizeof pack)&&!pending.file&&!pending.entries);
    available=8u*1024u*1024u;CHECK(!wx_pack_attach_begin(&pending,"stream-index.wxp",sizeof pack+1024*sizeof(WxEntry))&&!pending.file&&!pending.entries);available=48u*1024u*1024u;
    remove("stream-index.wxp");
}
static void actor_stages(void){
    fixture(0,1);unsigned delta=sizeof(WxEntry);WxPackHeader h=pack.header;h.count=2;h.file_size+=delta;
    WxEntry e=pack.entry;e.kind=WX_KIND_CHARACTER;e.id=77<<8;e.vertex_offset+=delta;e.index_offset+=delta;e.texture_offset+=delta;e.reserved[0]+=delta;
    pack.animation.skin_offset+=delta;pack.animation.poses_offset+=delta;
    FILE* f=fopen("stream-fixture.wxp","wb");CHECK(f);CHECK(fwrite(&h,1,sizeof h,f)==sizeof h);
    CHECK(fwrite(&e,1,sizeof e,f)==sizeof e);e.id|=5;CHECK(fwrite(&e,1,sizeof e,f)==sizeof e);
    unsigned skip=sizeof(WxPackHeader)+sizeof(WxEntry);CHECK(fwrite((unsigned char*)&pack+skip,1,sizeof pack-skip,f)==sizeof pack-skip);CHECK(!fclose(f));
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));uint32_t idle=77<<8,run=idle|5;unsigned calls=0;
    while(calls++<100&&!wx_animation_resident(&scene,idle)){
        wx_stream_avatar_animations(&scene,&idle,1,UINT32_MAX);
        CHECK(scene.streaming.read_bytes<=WX_STREAM_READ_BYTES&&scene.streaming.read_ops<=WX_STREAM_READ_OPS&&scene.streaming.scan_bytes<=WX_STREAM_SCAN_BYTES);
        if(scene.stream_job.phase)CHECK(!wx_animation_resident(&scene,idle));
    }CHECK(calls>5&&calls<100&&scene.loads==1&&!scene.failures);
    unsigned bytes=scene.bytes;
    wx_stream_avatar_animations(&scene,&run,1,0);CHECK(scene.stream_job.phase&&wx_animation_fallback(&scene,run)==idle);
    // Rapid idle/run changes finish the pending partner instead of starving it.
    for(unsigned i=0;i<40;i++){
        uint32_t wanted=i%2?idle:run;wx_stream_avatar_animations(&scene,&wanted,1,0);
        CHECK(wx_animation_resident(&scene,idle)&&scene.streaming.read_bytes<=WX_STREAM_READ_BYTES&&scene.streaming.read_ops<=WX_STREAM_READ_OPS&&scene.streaming.scan_bytes<=WX_STREAM_SCAN_BYTES);
    }
    CHECK(wx_animation_resident(&scene,run)&&scene.loads==2&&!scene.failures&&!scene.streaming.cancelled&&scene.bytes>bytes);
    wx_stream_animations(&scene,NULL,0);CHECK(!scene.bytes&&!scene.stream_job.phase&&!objects);
    wx_stream_animations(&scene,&idle,1);CHECK(scene.stream_job.phase&&!wx_animation_resident(&scene,idle));
    wx_stream_animations(&scene,NULL,0);CHECK(!scene.bytes&&!scene.stream_job.phase&&scene.streaming.cancelled==1&&!objects);
    wx_pack_close(&scene);CHECK(!allocated&&!objects);
}
static void npc_coalescing(void){
    // Large reads deliberately span frames: changing idle/run must neither
    // restart the pending clip forever nor discard a complete visible fallback.
    fixture(0,1);unsigned delta=5*sizeof(WxEntry);WxPackHeader h=pack.header;h.count=6;h.file_size+=delta;
    WxEntry e=pack.entry;e.kind=WX_KIND_CHARACTER;e.id=77<<8;e.vertex_offset+=delta;e.index_offset+=delta;e.texture_offset+=delta;e.reserved[0]+=delta;
    pack.animation.skin_offset+=delta;pack.animation.poses_offset+=delta;
    FILE* f=fopen("stream-fixture.wxp","wb");CHECK(f);CHECK(fwrite(&h,1,sizeof h,f)==sizeof h);
    for(unsigned clip=0;clip<3;clip++){
        e.id=clip==0?77<<8:clip==1?(77<<8)|5:78<<8;
        for(unsigned part=0;part<2;part++)CHECK(fwrite(&e,1,sizeof e,f)==sizeof e);
    }
    unsigned skip=sizeof(WxPackHeader)+sizeof(WxEntry);CHECK(fwrite((unsigned char*)&pack+skip,1,sizeof pack-skip,f)==sizeof pack-skip);CHECK(!fclose(f));
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));uint32_t idle=77<<8,run=idle|5,second=78<<8;
    for(unsigned i=0;i<80&&!wx_animation_resident(&scene,idle);i++)wx_stream_animations(&scene,&idle,1);
    CHECK(wx_animation_resident(&scene,idle));unsigned single=scene.bytes;
    wx_stream_animations(&scene,&run,1);CHECK(scene.stream_job.phase&&wx_animation_fallback(&scene,run)==idle);
    for(unsigned i=0;i<80;i++){
        uint32_t id=i&1?idle:run;wx_stream_animations(&scene,&id,1);
        CHECK(wx_animation_fallback(&scene,id)!=UINT32_MAX);
        CHECK(scene.streaming.read_bytes<=WX_STREAM_READ_BYTES&&scene.streaming.read_ops<=WX_STREAM_READ_OPS&&scene.streaming.scan_bytes<=WX_STREAM_SCAN_BYTES);
    }
    CHECK(wx_animation_resident(&scene,idle)&&wx_animation_resident(&scene,run)&&scene.loads==4&&!scene.streaming.cancelled&&!scene.failures);
    unsigned paired=scene.bytes;
    // A new template takes precedence over a cached optional run clip.
    scene.budget_bytes=paired;uint32_t wants[]={idle,second};
    for(unsigned i=0;i<80&&!wx_animation_resident(&scene,second);i++)wx_stream_animations(&scene,wants,2);
    CHECK(wx_animation_resident(&scene,idle)&&wx_animation_resident(&scene,second)&&!wx_animation_resident(&scene,run)&&!scene.failures);
    wx_stream_animations(&scene,&idle,1);CHECK(scene.bytes==single);
    // Optional retention is disabled under low system headroom.
    scene.budget_bytes=32u*1024u*1024u;
    for(unsigned i=0;i<80&&!wx_animation_resident(&scene,run);i++)wx_stream_animations(&scene,&run,1);
    CHECK(wx_animation_resident(&scene,idle)&&wx_animation_resident(&scene,run));
    available=12u*1024u*1024u;wx_stream_animations(&scene,&idle,1);CHECK(scene.bytes==single&&!wx_animation_resident(&scene,run));available=48u*1024u*1024u;
    wx_stream_animations(&scene,NULL,0);CHECK(!scene.bytes&&!scene.stream_job.phase);
    wx_stream_animations(&scene,&run,1);CHECK(scene.stream_job.phase);
    wx_stream_animations(&scene,&second,1);CHECK(scene.streaming.cancelled==1&&scene.stream_job.phase&&scene.entries[scene.stream_job.entry].id==second);
    wx_pack_close(&scene);CHECK(!allocated&&!objects);
}
static WxScene budget_npc,budget_player,budget_index;
static void check_frame_budget(void){
    const WxStreamFrame* f=wx_stream_frame_metrics();CHECK(f->enabled);
    CHECK(f->total.read_bytes<=WX_STREAM_FRAME_BYTES&&f->total.read_ops<=WX_STREAM_FRAME_OPS&&f->total.scan_bytes<=WX_STREAM_FRAME_SCANS&&f->total.allocations<=WX_STREAM_FRAME_ALLOCATIONS);
    WxStreamUse sum={0};for(unsigned i=0;i<4;i++){
        sum.read_bytes+=f->lane[i].read_bytes;sum.read_ops+=f->lane[i].read_ops;sum.scan_bytes+=f->lane[i].scan_bytes;sum.allocations+=f->lane[i].allocations;
    }CHECK(!memcmp(&sum,&f->total,sizeof sum));
}
static void shared_frame_budget(void){
    index_fixture(0);unsigned index_bytes=sizeof pack+1024*sizeof(WxEntry);
    fixture(0,1);pack.entry.kind=WX_KIND_CHARACTER;pack.entry.id=77<<8;
    FILE* file=fopen("stream-fixture.wxp","wb");CHECK(file);CHECK(fwrite(&pack,1,sizeof pack,file)==sizeof pack);CHECK(!fclose(file));
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));CHECK(wx_pack_open(&budget_npc,"stream-fixture.wxp"));CHECK(wx_pack_open(&budget_player,"stream-fixture.wxp"));wx_scene_init(&budget_index);
    WxPackPending pending={0};uint32_t id=77<<8;unsigned calls=0,used[4]={0},ready=0;
    for(;calls<200;calls++){
        wx_stream_frame_begin(15);
        if(!calls){CHECK(wx_stream_index_ready());CHECK(wx_pack_attach_begin(&pending,"stream-index.wxp",index_bytes));}
        if(pending.phase)CHECK(wx_pack_attach_pump(&budget_index,&pending)>=0);
        wx_stream(&scene,pack.header.spawn);wx_stream_animations(&budget_npc,&id,1);wx_stream_avatar_animations(&budget_player,&id,1,UINT32_MAX);
        check_frame_budget();const WxStreamFrame* b=wx_stream_frame_metrics();for(unsigned i=0;i<4;i++)used[i]+=b->lane[i].read_bytes;
        CHECK(!scene.failures&&!budget_npc.failures&&!budget_player.failures);
        CHECK(scene.bytes<=scene.budget_bytes&&budget_npc.bytes<=budget_npc.budget_bytes&&budget_player.bytes<=budget_player.budget_bytes);
        ready=wx_stream_ready(&scene)&&wx_animation_resident(&budget_npc,id)&&wx_animation_resident(&budget_player,id)&&!pending.phase;
        wx_stream_frame_end();if(ready)break;
    }
    CHECK(ready&&calls>20&&calls<120&&budget_index.header.count==1025);
    CHECK(!budget_npc.stream_needed&&!budget_player.stream_needed);
    for(unsigned i=0;i<4;i++)CHECK(used[i]>0);
    CHECK(!memcmp(scene.slots[0].vertices,pack.vertices,sizeof pack.vertices));
    CHECK(!memcmp(budget_npc.slots[0].skin,pack.skin,sizeof pack.skin)&&!memcmp(budget_player.slots[0].poses,pack.poses,sizeof pack.poses));
    wx_pack_close(&scene);wx_pack_close(&budget_npc);wx_pack_close(&budget_player);wx_pack_close(&budget_index);CHECK(!allocated&&!objects);
    // With no consumers, jobs defer without an error or allocation. Repeating a
    // scene call must not replenish the frame quota or let the verifier drain it.
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));wx_stream_frame_begin(0);
    CHECK(!wx_stream_index_ready());for(unsigned i=0;i<4;i++)wx_stream(&scene,pack.header.spawn);
    CHECK(scene.selection.phase&&!scene.selection.progress&&!scene.stream_job.phase&&!scene.bytes&&!scene.failures);
    CHECK(!wx_stream_selection_entries());check_frame_budget();CHECK(!wx_pack_verify(&scene));wx_stream_frame_end();
    wx_stream_frame_begin(1u<<WX_STREAM_WORLD);
    for(unsigned i=0;i<20;i++)wx_stream(&scene,pack.header.spawn);
    check_frame_budget();CHECK(wx_stream_frame_metrics()->total.read_bytes==65536&&scene.stream_job.phase&&!scene.failures);wx_stream_frame_end();wx_pack_close(&scene);CHECK(!allocated&&!objects);
    // Disabled lanes do not steal shares; unused earlier shares flow forward.
    CHECK(wx_pack_open(&budget_player,"stream-fixture.wxp"));wx_stream_frame_begin(15);
    wx_stream_avatar_animations(&budget_player,&id,1,UINT32_MAX);check_frame_budget();
    CHECK(wx_stream_frame_metrics()->lane[WX_STREAM_AVATAR].read_bytes==65536);wx_stream_frame_end();wx_pack_close(&budget_player);
    // A newly requested idle queue needs no speculative reservation. Its first
    // denied tick advertises demand; following ticks get guaranteed progress.
    CHECK(wx_pack_open(&budget_npc,"stream-fixture.wxp"));budget_npc.loads=1;
    unsigned frames=0;
    do{
        wx_stream_frame_begin(1u|(budget_npc.stream_needed?4u:0u));
        wx_stream_animations(&budget_npc,&id,1);check_frame_budget();
        if(!frames)CHECK(budget_npc.stream_needed&&!budget_npc.streaming.read_bytes&&!budget_npc.bytes);
        wx_stream_frame_end();frames++;
    }while(budget_npc.stream_needed&&frames<100);
    CHECK(frames>5&&frames<100&&wx_animation_resident(&budget_npc,id)&&!budget_npc.failures);wx_pack_close(&budget_npc);
    CHECK(wx_pack_attach_begin(&pending,"stream-index.wxp",index_bytes));wx_stream_frame_begin(1u<<WX_STREAM_WORLD);
    CHECK(!wx_pack_attach_pump(&budget_index,&pending)&&pending.phase&&!pending.read_bytes);wx_stream_frame_end();wx_pack_attach_cancel(&pending);
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));available=8u*1024u*1024u;wx_stream_frame_begin(15);wx_stream(&scene,pack.header.spawn);
    CHECK(scene.failures==1&&!scene.bytes&&wx_stream_frame_metrics()->total.allocations==1);check_frame_budget();wx_stream_frame_end();available=48u*1024u*1024u;
    wx_pack_close(&scene);CHECK(!allocated&&!objects);remove("stream-index.wxp");
}
static unsigned fake_ms,fake_calls;
static unsigned manual_clock(void){return fake_ms;}
static unsigned work_clock(void){return fake_ms+fake_calls++*2;}
static void timed_frame_budget(void){
    wx_stream_set_clock(manual_clock);fake_ms=100;wx_stream_frame_begin(15);
    CHECK(wx_stream_time_metrics()->enabled);
    CHECK(wx_stream_read_grant(WX_STREAM_INDEX,1024)==1024);
    fake_ms+=2;CHECK(!wx_stream_index_ready());
    CHECK(!wx_stream_scan_grant(WX_STREAM_INDEX,64));
    CHECK(wx_stream_time_metrics()->yield_mask==1);
    WxPackPending pending={0};CHECK(!wx_pack_attach_begin(&pending,"missing.wxp",0));
    CHECK(pending.deferred&&!pending.file&&!pending.phase);wx_pack_attach_cancel(&pending);
    // An exhausted early lane cannot starve any later active consumer.
    CHECK(wx_stream_read_grant(WX_STREAM_WORLD,1024)==1024);
    fake_ms+=6;CHECK(!wx_stream_allocation_grant(WX_STREAM_WORLD));
    CHECK(wx_stream_read_grant(WX_STREAM_ACTORS,1024)==1024);
    fake_ms+=2;CHECK(!wx_stream_scan_grant(WX_STREAM_ACTORS,64));
    CHECK(wx_stream_read_grant(WX_STREAM_AVATAR,1024)==1024);
    fake_ms+=2;CHECK(!wx_stream_read_grant(WX_STREAM_AVATAR,1024));
    CHECK(wx_stream_time_metrics()->yield_mask==15);check_frame_budget();
    wx_stream_set_clock(NULL);CHECK(!wx_stream_read_grant(WX_STREAM_WORLD,1)); // No mid-frame reset.
    wx_stream_frame_end();CHECK(wx_stream_read_grant(WX_STREAM_WORLD,1024)==1024);
    wx_stream_frame_begin(2);CHECK(wx_stream_time_metrics()->limit_ms[WX_STREAM_WORLD]==WX_STREAM_FRAME_MS);
    CHECK(wx_stream_read_grant(WX_STREAM_WORLD,1)==1);fake_ms+=11;
    CHECK(wx_stream_scan_grant(WX_STREAM_WORLD,64));fake_ms++;
    CHECK(!wx_stream_read_grant(WX_STREAM_WORLD,1));wx_stream_frame_end();
    fake_ms=UINT32_MAX-1;wx_stream_frame_begin(15);CHECK(wx_stream_index_ready());fake_ms=0;
    CHECK(!wx_stream_index_ready()&&wx_stream_time_metrics()->observed_ms[0]==2);wx_stream_frame_end();
    wx_stream_frame_begin(0);CHECK(!wx_stream_read_grant(WX_STREAM_WORLD,1)&&!wx_stream_time_metrics()->yield_mask);wx_stream_frame_end();
    // Exercise real pending payloads across timed yields, then compare every
    // published buffer and release all pending allocations.
    fixture(0,1);CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));wx_stream_set_clock(work_clock);
    unsigned frames=0,yields=0;
    for(;frames<200;frames++){
        fake_ms=frames*33;fake_calls=0;wx_stream_frame_begin(15);
        wx_stream(&scene,pack.header.spawn);check_frame_budget();
        yields+=!!wx_stream_time_metrics()->yield_mask;
        wx_stream_frame_end();CHECK(!scene.failures);if(wx_stream_ready(&scene))break;
    }
    printf("Timed streaming: %u frames, %u yield frames, %u published payloads\n",frames+1,yields,scene.loads);
    CHECK(frames<200&&yields>0&&scene.loads==1);
    CHECK(!memcmp(scene.slots[0].vertices,pack.vertices,sizeof pack.vertices));
    CHECK(!memcmp(scene.slots[0].indices,pack.indices,sizeof pack.indices));
    CHECK(!memcmp(scene.slots[0].texture,pack.pixels,sizeof pack.pixels));
    CHECK(!memcmp(scene.slots[0].skin,pack.skin,sizeof pack.skin));
    CHECK(!memcmp(scene.slots[0].poses,pack.poses,sizeof pack.poses));
    wx_pack_close(&scene);wx_stream_set_clock(NULL);CHECK(!allocated&&!objects);
}
int main(void){
    staged_index();
    actor_stages();
    npc_coalescing();
    shared_frame_budget();
    timed_frame_budget();
    for(unsigned animated=0;animated<2;animated++){
        fixture(0,animated);CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));unsigned frames=pump();
        CHECK(frames>5&&frames<30&&scene.loads==1&&!scene.failures&&scene.streaming.completed==1);
        CHECK(!scene.stream_job.phase&&!scene.streaming.pending_bytes&&scene.streaming.deferred>0);
        WxResident* r=&scene.slots[0];CHECK(!memcmp(r->vertices,pack.vertices,sizeof pack.vertices)&&!memcmp(r->indices,pack.indices,sizeof pack.indices)&&!memcmp(r->texture,pack.pixels,sizeof pack.pixels));
        if(animated)CHECK(!memcmp(r->bind_vertices,pack.vertices,sizeof pack.vertices)&&!memcmp(r->skin,pack.skin,sizeof pack.skin)&&!memcmp(r->poses,pack.poses,sizeof pack.poses));
        CHECK(scene.bytes==sizeof pack.vertices+sizeof pack.indices+sizeof pack.pixels+(animated?sizeof pack.vertices+sizeof pack.skin+sizeof pack.poses:0));
        float z=9;CHECK(wx_ground(&scene,.1f,.1f,&z)&&z==0);unsigned loads=scene.loads;
        float edge[3]={180,0,0};wx_stream(&scene,edge);CHECK(scene.loads==loads&&scene.slots[0].entry==0); // retained beyond prefetch radius
        edge[0]=210;wx_stream(&scene,edge);CHECK(scene.bytes==0&&!allocated&&!objects);
        wx_pack_close(&scene);CHECK(!scene.bytes&&!allocated&&!objects);
    }
    for(unsigned bad=1;bad<=4;bad++){
        fixture(bad,1);CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));CHECK(pump()>5);
        CHECK(scene.failures==1&&!scene.loads&&!scene.stream_job.phase&&!scene.bytes&&!objects);wx_pack_close(&scene);
    }
    fixture(0,1);CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));wx_stream(&scene,pack.header.spawn);
    CHECK(scene.stream_job.phase&&scene.streaming.pending_bytes&&!scene.loads&&scene.bytes>0);
    float far[3]={1000,0,0};wx_stream(&scene,far);CHECK(!scene.stream_job.phase&&!scene.bytes&&!objects&&scene.streaming.cancelled==1);
    wx_stream(&scene,pack.header.spawn);CHECK(scene.stream_job.phase);wx_pack_close(&scene);CHECK(!scene.bytes&&!objects&&!allocated);
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));CHECK(wx_pack_attach(&scene,"stream-fixture.wxp")==1);
    wx_stream(&scene,pack.header.spawn);wx_pack_detach(&scene,0);CHECK(!scene.stream_job.phase&&!scene.bytes&&!objects&&scene.header.count==1);
    CHECK(pump()>5&&scene.loads==1&&!scene.failures);wx_pack_close(&scene);CHECK(!allocated&&!objects);
    // Pending job in a later file retains its identity when earlier indices move.
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));scene.entries[0].center[0]=1000;
    CHECK(wx_pack_attach(&scene,"stream-fixture.wxp")==1);wx_stream(&scene,pack.header.spawn);CHECK(scene.stream_job.entry==1);
    wx_pack_detach(&scene,0);CHECK(scene.stream_job.phase&&scene.stream_job.entry==0);CHECK(scene.index_bytes==2*sizeof(WxEntry));CHECK(pump()>1&&scene.loads==1&&!scene.failures);wx_pack_close(&scene);CHECK(!scene.index_bytes);
    for(int fail=0;fail<2;fail++){
        CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));fail_after=fail;wx_stream(&scene,pack.header.spawn);fail_after=-1;
        CHECK(scene.failures==1&&!scene.bytes&&!objects);wx_pack_close(&scene);
    }
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));scene.budget_bytes=100;wx_stream(&scene,pack.header.spawn);CHECK(scene.failures==1&&!scene.bytes&&!objects);wx_pack_close(&scene);
    CHECK(wx_pack_open(&scene,"stream-fixture.wxp"));available=8u*1024u*1024u;wx_stream(&scene,pack.header.spawn);CHECK(scene.failures==1&&!scene.bytes&&!objects);available=48u*1024u*1024u;wx_pack_close(&scene);
    remove("stream-fixture.wxp");printf("Staged reads/validation, atomic publication, cancellation, detach and memory bounds: %u checks pass\n",checks);return 0;
}
