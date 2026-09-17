/* Compare staged production selection with an independently sorted near set. */
#include "wx_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Selection FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return 48u*1024u*1024u;}
void* wx_gpu_alloc(unsigned n){return malloc(n);}
void wx_gpu_free(void* p){free(p);}
#include "../src/pack.c"
static WxScene scene;
#define COUNT 12000u
static void fixture(void){
    wx_scene_init(&scene);scene.entries=calloc(COUNT,sizeof *scene.entries);CHECK(scene.entries);
    scene.header.count=COUNT;scene.index_bytes=COUNT*sizeof(WxEntry);scene.file=tmpfile();CHECK(scene.file);
    scene.sources[0].file=scene.file;scene.sources[0].count=COUNT;
    for(unsigned i=0;i<COUNT;i++){
        WxEntry* e=scene.entries+i;e->id=i;e->kind=i%3?WX_KIND_STATIC:WX_KIND_COLLISION;
        e->center[0]=(COUNT-i)*.1f;e->radius=(i%5)*.05f;
        if(i>=COUNT-120){e->kind=WX_KIND_STATIC;e->flags=WX_PLACEMENT_ID;e->id=i/2;e->reserved[0]=123;e->center[0]=(COUNT-i/2*2)*.1f;e->radius=0;}
    }
    // Retained candidates outside the new-prefetch radius are still eligible.
    scene.slots[0].entry=COUNT-1850;scene.slots[1].entry=COUNT-600;
}
typedef struct Choice {int index;float score;} Choice;
static int compare(const void* va,const void* vb){
    const Choice* a=va;const Choice* b=vb;
    if(a->score<b->score)return -1;if(a->score>b->score)return 1;return a->index-b->index;
}
static void expected(const float* point,int* out){
    Choice* choices=malloc(COUNT*sizeof *choices);CHECK(choices);
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)out[i]=-1;
    for(unsigned collision=0;collision<2;collision++){
        unsigned count=0;
        for(unsigned i=0;i<scene.header.count;i++){
            const WxEntry* e=scene.entries+i;if((e->kind==WX_KIND_COLLISION)!=collision)continue;
            float x=e->center[0]-point[0],y=e->center[1]-point[1],d=sqrtf(x*x+y*y)-e->radius;
            int retained=resident_entry(&scene,(int)i)||(scene.stream_job.phase&&scene.stream_job.entry==(int)i);
            if(d>(collision?(retained?65:55):(retained?195:175)))continue;
            int duplicate=0;
            if(e->flags&WX_PLACEMENT_ID)for(unsigned j=0;j<count;j++){
                const WxEntry* prev=scene.entries+choices[j].index;
                if(prev->flags&WX_PLACEMENT_ID&&prev->kind==e->kind&&prev->id==e->id&&prev->reserved[0]==e->reserved[0]){duplicate=1;break;}
            }
            if(!duplicate)choices[count++]=(Choice){(int)i,d+(d>(collision?45:165)?200:0)};
        }
        qsort(choices,count,sizeof *choices,compare);
        unsigned start=collision?WX_RENDER_SLOTS:0,capacity=collision?WX_COLLISION_SLOTS:WX_RENDER_SLOTS;
        for(unsigned i=0;i<count&&i<capacity;i++)out[start+i]=choices[i].index;
    }free(choices);
}
static unsigned finish(float* position,int moving){
    unsigned frames=0;int complete=0;
    while(!complete&&frames++<100){
        wx_stream_frame_begin(2);complete=selection_step(&scene,position);
        CHECK(wx_stream_selection_entries()<=WX_STREAM_FRAME_SELECT_ENTRIES);
        CHECK(scene.selection.progress<=scene.selection.count);
        if(!complete){unsigned at=scene.selection.progress;
            CHECK(scene.selection.phase&&!wx_stream_ready(&scene));
            for(unsigned repeat=0;repeat<4;repeat++){CHECK(!selection_step(&scene,position));CHECK(scene.selection.progress==at);}
        }
        wx_stream_frame_end();if(moving)position[0]+=.25f;
    }CHECK(complete&&frames<100);return frames;
}
static void atomic_selection(void){
    fixture();float position[3]={0};int oracle[WX_CACHE_SLOTS];expected(position,oracle);
    wx_stream_frame_begin(0);CHECK(!selection_step(&scene,position));
    CHECK(scene.selection.phase&&!scene.selection.progress&&!wx_stream_selection_entries());wx_stream_frame_end();
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)CHECK(scene.wanted[i]==-1);
    CHECK(finish(position,0)==6);CHECK(scene.wanted_ready&&!scene.selection.phase&&scene.selection.commits==1);
    CHECK(!memcmp(scene.wanted,oracle,sizeof oracle));
    // A previously published set is unchanged while the next snapshot scans.
    position[0]=3;wx_stream_frame_begin(2);CHECK(!selection_step(&scene,position));
    CHECK(!memcmp(scene.wanted,oracle,sizeof oracle)&&scene.wanted_ready&&!wx_stream_ready(&scene));wx_stream_frame_end();
    expected(position,oracle);CHECK(finish(position,0)==5);CHECK(!memcmp(scene.wanted,oracle,sizeof oracle));
    CHECK(scene.selection.commits==2);wx_pack_close(&scene);
}
static void movement_and_mutation(void){
    fixture();float position[3]={0};CHECK(finish(position,1)==6);
    CHECK(scene.selection.commits==1&&scene.wanted_position[0]==0&&!scene.selection.restarts);
    position[0]=10;wx_stream_frame_begin(2);CHECK(!selection_step(&scene,position));wx_stream_frame_end();
    position[0]=100;wx_stream_frame_begin(2);CHECK(!selection_step(&scene,position));
    CHECK(scene.selection.restarts==1&&scene.selection.position[0]==100&&scene.selection.progress==2048);wx_stream_frame_end();
    unsigned commits=scene.selection.commits;wx_pack_detach(&scene,0);
    CHECK(!scene.selection.phase&&!scene.wanted_ready&&!scene.header.count&&scene.selection.restarts==2);
    CHECK(scene.selection.commits==commits);for(unsigned i=0;i<WX_CACHE_SLOTS;i++)CHECK(scene.wanted[i]==-1);
    wx_pack_close(&scene);
}
static unsigned fake_time;
static unsigned slow_clock(void){return fake_time++;}
static void clock_deferral(void){
    fixture();float p[3]={0};wx_stream_set_clock(slow_clock);fake_time=0;wx_stream_frame_begin(2);
    CHECK(!selection_step(&scene,p));CHECK(scene.selection.progress>0&&scene.selection.progress<2048);
    unsigned at=scene.selection.progress;CHECK(!selection_step(&scene,p)&&scene.selection.progress==at);
    CHECK(wx_stream_time_metrics()->yield_mask&2);wx_stream_frame_end();wx_stream_set_clock(NULL);
    CHECK(finish(p,0)>1);wx_pack_close(&scene);
}
int main(void){atomic_selection();movement_and_mutation();clock_deferral();printf("Staged selection: %u checks; workspace %zu bytes\n",checks,sizeof(WxSelection));return 0;}
