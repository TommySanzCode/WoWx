/* One production streaming tick per simulated frame; no GPU/emulator timing claim. */
#include "wx_stream_scenario.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned allocated;
unsigned wx_free_memory(void){return 44u*1024u*1024u-allocated;}
void* wx_gpu_alloc(unsigned n){unsigned* p=malloc(n+sizeof *p);if(!p)return NULL;*p=n;allocated+=n;return p+1;}
void wx_gpu_free(void* p){if(p){unsigned* b=(unsigned*)p-1;allocated-=*b;free(b);}}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    WxRegion region;WxScene scene;WxStreamScenario scenario;wx_scene_init(&scene);wx_stream_scenario_init(&scenario);
    if(!wx_region_open(&region,argv[1])){puts(region.error);return 1;}
    unsigned last=99,maximum=0;int good=1;
    while(scenario.frame<15000&&!scenario.cycles&&!scenario.errors){
        wx_region_update(&region,&scene,0,scenario.position);wx_stream(&scene,scenario.position);
        if(scene.streaming.read_bytes>WX_STREAM_READ_BYTES||scene.streaming.read_ops>WX_STREAM_READ_OPS||scene.streaming.scan_bytes>WX_STREAM_SCAN_BYTES||region.pending.read_bytes>32768||region.pending.checked_frame>512){puts("quota exceeded");good=0;break;}
        wx_stream_scenario_step(&scenario,&region,&scene);if(scene.bytes>maximum)maximum=scene.bytes;
        if(last!=scenario.phase){printf("frame=%u phase=%u x=%.3f y=%.3f z=%.3f loads=%u entries=%u tiles=%u transitions=%u waits=%u blocker=%u failures=%u %s %s\n",scenario.frame,scenario.phase,scenario.position[0],scenario.position[1],scenario.position[2],scene.loads,scene.header.count,region.loaded,region.transitions,scenario.waits,scenario.blocker,scene.failures+region.failures,scene.error,region.error);last=scenario.phase;}
    }
    printf("cycles=%u frames=%u errors=%u failures=%u cache_peak=%u max_read=%u max_ops=%u max_scan=%u pending=%u cancelled=%u\n",scenario.cycles,scenario.frame,scenario.errors,scene.failures+region.failures,maximum,scene.streaming.max_read_bytes,scene.streaming.max_read_ops,scene.streaming.max_scan_bytes,scene.streaming.pending_bytes,scene.streaming.cancelled);
    good=good&&scenario.cycles&&!scenario.errors&&!scene.failures&&!region.failures;
    wx_region_close(&region);wx_pack_close(&scene);if(allocated){puts("GPU allocation leak");good=0;}return good?0:1;
}
