// Exercise the target streamer against real packs without emulating the GPU.
#include "wx_region.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
static unsigned allocated,minimum=44u*1024u*1024u;
unsigned wx_free_memory(void){unsigned free=44u*1024u*1024u-allocated;if(free<minimum)minimum=free;return free;}
void* wx_gpu_alloc(unsigned size){unsigned* block=malloc(size+sizeof *block);if(!block)return NULL;*block=size;allocated+=size;return block+1;}
void wx_gpu_free(void* p){if(p){unsigned* block=(unsigned*)p-1;allocated-=*block;free(block);}}
static int route(const char* index,unsigned map,const char* path){
    FILE* file=fopen(path,"r");if(!file)return 2;float points[128][3];unsigned count=0;char line[160],extra;
    while(fgets(line,sizeof line,file)){
        if(line[0]=='#'||line[0]=='\n'||line[0]=='\r')continue;
        if(count==128||sscanf(line,"%f %f %f %c",&points[count][0],&points[count][1],&points[count][2],&extra)!=3){fclose(file);return 2;}
        for(unsigned k=0;k<3;k++)if(!isfinite(points[count][k])){fclose(file);return 2;}count++;
    }
    fclose(file);if(count<2)return 2;WxRegion r;WxScene s;wx_scene_init(&s);
    if(!wx_region_open(&r,index))return 1;float p[3];memcpy(p,points[0],sizeof p);int good=1;
    for(unsigned leg=1;leg<count;leg++){
        float dx=points[leg][0]-p[0],dy=points[leg][1]-p[1],length=sqrtf(dx*dx+dy*dy);
        if(length>1000){good=0;break;}unsigned steps=(unsigned)ceilf(length/.2f);if(!steps)steps=1;
        float from[3];memcpy(from,p,sizeof from);
        for(unsigned i=0;i<160;i++){wx_region_update(&r,&s,map,p);wx_stream(&s,p);}
        for(unsigned step=0;step<steps;step++){
            for(unsigned i=0;i<4;i++){wx_region_update(&r,&s,map,p);wx_stream(&s,p);}
            float x=from[0]+dx*((float)(step+1)/steps),y=from[1]+dy*((float)(step+1)/steps),z=p[2];
            if(!wx_walk(&s,p,x,y,&z)){
                int entry;unsigned reason=wx_walk_blocker(&entry);
                printf("blocked leg=%u reason=%u entry=%d x=%.4f y=%.4f z=%.4f\n",leg,reason,entry,p[0],p[1],p[2]);good=0;break;
            }p[0]=x;p[1]=y;p[2]=z;
        }
        printf("leg=%u x=%.4f y=%.4f z=%.4f expected_z=%.4f loads=%u failures=%u\n",leg,p[0],p[1],p[2],points[leg][2],s.loads,s.failures+r.failures);
        if(fabsf(p[2]-points[leg][2])>3)good=0;if(!good)break;
    }
    if(s.failures||r.failures)good=0;wx_pack_close(&s);wx_region_close(&r);if(allocated)good=0;return good?0:1;
}
static int point(const char* index,unsigned map,const char* x,const char* y,const char* z){
    WxRegion r;WxScene s;wx_scene_init(&s);float p[3]={strtof(x,NULL),strtof(y,NULL),strtof(z,NULL)};
    if(!wx_region_open(&r,index))return 1;
    unsigned frame=0;int attached=0;
    for(;frame<1800;frame++){
        wx_stream_frame_begin(1u|(s.file&&!wx_stream_ready(&s)?2u:0u));
        attached=wx_region_update(&r,&s,map,p);wx_stream(&s,p);wx_stream_frame_end();
        if(attached&&wx_stream_ready(&s))break;
        if(s.failures||r.failures)break;
    }
    float floor=0;int found=wx_floor(&s,p[0],p[1],p[2]+.5f,p[2]-.5f,&floor);
    int good=attached&&wx_stream_ready(&s)&&found&&!s.failures&&!r.failures;
    printf("map=%u frames=%u ready=%d floor_found=%d z=%.6f entries=%u cache=%u loads=%u failures=%u simulated_free=%u\n",
           map,frame,wx_stream_ready(&s),found,floor,s.header.count,s.bytes,s.loads,s.failures+r.failures,minimum);
    wx_pack_close(&s);wx_region_close(&r);if(allocated)good=0;return good?0:1;
}
int main(int argc,char** argv){
    if(argc==7&&!strcmp(argv[1],"--point"))return point(argv[2],(unsigned)strtoul(argv[3],NULL,10),argv[4],argv[5],argv[6]);
    if(argc==5&&!strcmp(argv[1],"--route"))return route(argv[2],(unsigned)strtoul(argv[3],NULL,10),argv[4]);
    if(argc!=5&&argc!=6){fprintf(stderr,"Usage: regioncheck world.wxi map x y [walk_south_distance]\n");return 2;}
    WxRegion r;WxScene s;wx_scene_init(&s);if(!wx_region_open(&r,argv[1])){fprintf(stderr,"%s\n",r.error);return 1;}
    unsigned map=(unsigned)strtoul(argv[2],NULL,10);float p[3]={strtof(argv[3],NULL),strtof(argv[4],NULL),0};
    for(unsigned i=0;i<160;i++){wx_region_update(&r,&s,map,p);wx_stream(&s,p);}
    float height=0;int found=wx_ground(&s,p[0],p[1],&height);
    if(argc==6&&found){
        float start=p[0],distance=strtof(argv[5],NULL);p[2]=height;
        if(!isfinite(distance)||distance<=0||distance>1000)return 2;
        while(start-p[0]<distance){
            for(unsigned i=0;i<4;i++){wx_region_update(&r,&s,map,p);wx_stream(&s,p);}
            float z=p[2];if(!wx_walk(&s,p,p[0]-.2f,p[1],&z)){int entry;unsigned reason=wx_walk_blocker(&entry);printf("blocked_reason=%u entry=%d ",reason,entry);found=0;break;}p[0]-=.2f;p[2]=z;
        }height=p[2];printf("walked=%.3f requested=%.3f ",start-p[0],distance);
    }
    printf("map=%u x=%.3f y=%.3f floor_found=%d z=%.6f tiles=%u entries=%u cache=%u loads=%u failures=%u\n",map,p[0],p[1],found,height,r.loaded,s.header.count,s.bytes,s.loads,s.failures+r.failures);
    int passed=found&&s.failures==0&&r.failures==0;wx_pack_close(&s);wx_region_close(&r);if(allocated)passed=0;return passed?0:1;
}
