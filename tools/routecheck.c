// Host-only route fixture validation using the same bounded stream and collision.
#include "wx_region.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
static unsigned allocated;
unsigned wx_free_memory(void){return 44u*1024*1024-allocated;}
void* wx_gpu_alloc(unsigned n){unsigned* p=malloc(n+4);if(!p)return NULL;*p=n;allocated+=n;return p+1;}
void wx_gpu_free(void* p){if(p){unsigned* q=(unsigned*)p-1;allocated-=*q;free(q);}}
static int line(WxRegion* region,WxScene* scene,const float* from,const float* to,float* result){
    float p[3];memcpy(p,from,12);
    for(unsigned step=0;step<3000;step++){
        float dx=to[0]-p[0],dy=to[1]-p[1],d=sqrtf(dx*dx+dy*dy);if(d<.001f){memcpy(result,p,12);return 1;}
        for(unsigned k=0;k<8;k++){wx_region_update(region,scene,0,p);wx_stream(scene,p);}
        float n=fminf(.15f,d),x=p[0]+dx*n/d,y=p[1]+dy*n/d,z=p[2];
        if(!wx_walk(scene,p,x,y,&z))return 0;p[0]=x;p[1]=y;p[2]=z;
    }return 0;
}
typedef struct RouteNode {float z,cost;int parent;unsigned state;} RouteNode;
static int find_route(WxRegion* region,WxScene* scene,const float* start,const float* goal){
    enum {N=181};const float step=.7f;static RouteNode nodes[N*N];float origin[2]={start[0]-90*step,start[1]-90*step};
    if(fabsf(goal[0]-start[0])>55||fabsf(goal[1]-start[1])>55){fprintf(stderr,"Local route search limited to55yards peraxis\n");return 2;}
    int initial=90*N+90,found=-1;nodes[initial]=(RouteNode){start[2],0,-1,1};
    for(unsigned visit=0;visit<N*N;visit++){
        int current=-1;float best=1e30f;
        for(unsigned i=0;i<N*N;i++)if(nodes[i].state==1){float dx=origin[0]+(i%N)*step-goal[0],dy=origin[1]+(i/N)*step-goal[1],f=nodes[i].cost+sqrtf(dx*dx+dy*dy);if(f<best){best=f;current=(int)i;}}
        if(current<0)break;RouteNode* q=nodes+current;q->state=2;int cx=current%N,cy=current/N;
        float p[3]={origin[0]+cx*step,origin[1]+cy*step,q->z},end[3];
        if(hypotf(p[0]-goal[0],p[1]-goal[1])<1&&line(region,scene,p,goal,end)){found=current;break;}
        for(unsigned k=0;k<12;k++){wx_region_update(region,scene,0,p);wx_stream(scene,p);}
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
            int x=cx+dx,y=cy+dy;if((!dx&&!dy)||x<0||y<0||x>=N||y>=N)continue;int i=y*N+x;
            float cost=q->cost+step*(dx&&dy?1.41421356f:1);if(nodes[i].state&&(nodes[i].state==2||cost>=nodes[i].cost))continue;
            float to[3]={origin[0]+x*step,origin[1]+y*step,p[2]},end[3];
            if(line(region,scene,p,to,end))nodes[i]=(RouteNode){end[2],cost,current,1};
        }
    }
    if(found<0){fprintf(stderr,"No local walkable route found\n");return 1;}
    int reverse[N*N];unsigned count=0;for(int i=found;i>=0;i=nodes[i].parent)reverse[count++]=i;
    float p[3];memcpy(p,start,12);printf("%.6f %.6f %.6f\n",p[0],p[1],p[2]);
    unsigned at=count-1;
    while(at){unsigned selected=at-1;float best[3]={0};
        for(unsigned n=0;n<at;n++){int i=reverse[n];float target[3]={origin[0]+(i%N)*step,origin[1]+(i/N)*step,nodes[i].z},end[3];
            if(line(region,scene,p,target,end)&&fabsf(end[2]-target[2])<.1f){selected=n;memcpy(best,end,12);break;}}
        if(best[0]==0){fprintf(stderr,"Route smoothing failed\n");return 1;}memcpy(p,best,12);printf("%.6f %.6f %.6f\n",p[0],p[1],p[2]);at=selected;
    }
    float end[3];if(!line(region,scene,p,goal,end))return 1;printf("%.6f %.6f %.6f\n",end[0],end[1],end[2]);return 0;
}
int main(int argc,char** argv){
    if(argc!=3&&(argc!=4||strcmp(argv[3],"--find"))){fprintf(stderr,"Usage: routecheck world.wxi route-xyz.txt [--find]\n");return 2;}
    FILE* f=fopen(argv[2],"r");if(!f)return 2;float points[128][3];unsigned count=0;
    while(count<128&&fscanf(f,"%f%f%f",points[count],points[count]+1,points[count]+2)==3){
        for(unsigned i=0;i<3;i++)if(!isfinite(points[count][i])||fabsf(points[count][i])>20000)return 2;count++;
    }fclose(f);if(count<2)return 2;
    WxRegion region;WxScene scene;wx_scene_init(&scene);if(!wx_region_open(&region,argv[1]))return 2;
    float p[3];memcpy(p,points[0],12);unsigned steps=0;int good=1;
    for(unsigned i=0;i<170;i++){wx_region_update(&region,&scene,0,p);wx_stream(&scene,p);}
    if(argc==4){int result=find_route(&region,&scene,p,points[count-1]);wx_pack_close(&scene);wx_region_close(&region);return result||allocated?1:0;}
    for(unsigned to=1;to<count&&good;to++){
        for(unsigned step=0;step<30000;step++){
            float dx=points[to][0]-p[0],dy=points[to][1]-p[1],distance=sqrtf(dx*dx+dy*dy);if(distance<.04f)break;
            float n=fminf(7.f*.7f/30,distance),x=p[0]+dx*n/distance,y=p[1]+dy*n/distance,z=p[2];
            for(unsigned i=0;i<8;i++){wx_region_update(&region,&scene,0,p);wx_stream(&scene,p);}
            if(!wx_walk(&scene,p,x,y,&z)){int entry;unsigned why=wx_walk_blocker(&entry);
                printf("BLOCKED segment=%u reason=%u entry=%d at %.6f %.6f %.6f next %.6f %.6f\n",to,why,entry,p[0],p[1],p[2],x,y);good=0;break;}
            p[0]=x;p[1]=y;p[2]=z;steps++;if(step==29999)good=0;
        }printf("segment=%u end=%.6f %.6f %.6f steps=%u\n",to,p[0],p[1],p[2],steps);
    }
    good=good&&!region.failures&&!scene.failures;wx_pack_close(&scene);wx_region_close(&region);
    printf("%s steps=%u remaining_allocations=%u scope=host_collision_only\n",good?"PASS":"FAIL",steps,allocated);return good&&!allocated?0:1;
}
