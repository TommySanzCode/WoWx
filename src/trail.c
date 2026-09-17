#include "wx_trail.h"
#include <math.h>
#include <string.h>
int wx_trail_record(WxTrail* t,const float p[3],int endpoint){
    (void)endpoint; // Every actual movement sample now preserves tight turns.
    if(t->failed)return 0;
    for(unsigned i=0;i<3;i++)if(!isfinite(p[i])||fabsf(p[i])>20000){t->failed=1;return 0;}
    if(t->count){
        const float* last=t->points[t->count-1];float dx=p[0]-last[0],dy=p[1]-last[1],dz=p[2]-last[2];
        float d=dx*dx+dy*dy+dz*dz;
        if(d<.000001f)return 1;
        /* A server relocation cannot become a supposedly walked path. */
        if(d>4){t->failed=1;return 0;}
    }
    if(t->count==WX_TRAIL_CAPACITY){t->failed=1;return 0;}
    memcpy(t->points[t->count++],p,3*sizeof(float));return 1;
}
void wx_trail_reset(WxTrail* t,const float p[3]){t->count=t->failed=0;wx_trail_record(t,p,1);}
