#include "wx_death.h"
#include <string.h>
#include <math.h>
static int finite_position(const float* p){return isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2])&&fabsf(p[0])<=20000&&fabsf(p[1])<=20000&&fabsf(p[2])<=20000;}
int wx_death_update(WxDeath* d,const WxWorldView* w,const WxEntity* self,const WxEntity* entities,unsigned count,const float* p,const WxGame* g,unsigned now){
    unsigned previous=d->state;int query=0;
    if(!w->active||w->revision!=d->world_revision){d->world_revision=w->revision;d->state=0;d->delay_revision=0;d->ready_at=now;d->query_attempts=0;previous=0;}
    d->state=w->active&&self?((self->fields[190]&16)?2:!self->fields[22]):0;
    if(previous==2&&!d->state&&self&&w->active)d->resurrections++;
    if(d->state==2&&previous!=2)d->query_attempts=0;
    if(g->corpse_delay_revision!=d->delay_revision){d->delay_revision=g->corpse_delay_revision;d->ready_at=now+g->corpse_delay_ms;}
    int32_t wait=(int32_t)(d->ready_at-now);d->remaining_ms=d->state==2&&wait>0?(unsigned)wait:0;
    d->corpse=0;d->known=d->in_range=d->ready=0;d->distance=0;
    if(d->state!=2)return 0;
    if(g->corpse_known&&finite_position(g->corpse_position)){d->known=1;d->map=g->corpse_map;memcpy(d->position,g->corpse_position,12);}
    if(count>WX_VISIBLE_ENTITIES)count=WX_VISIBLE_ENTITIES;
    for(unsigned i=0;i<count;i++){const WxEntity* e=entities+i;
        if(e->type!=7||!e->guid||(((uint64_t)e->fields[7]<<32)|e->fields[6])!=w->guid)continue;
        float where[3];if(e->positioned){where[0]=e->x;where[1]=e->y;where[2]=e->z;}else memcpy(where,e->fields+9,12);
        if(!finite_position(where))continue;d->corpse=e->guid;d->known=1;d->map=w->map;memcpy(d->position,where,12);break;
    }
    if(d->known&&d->map==w->map&&finite_position(p)){
        float dx=p[0]-d->position[0],dy=p[1]-d->position[1],dz=p[2]-d->position[2];d->distance=sqrtf(dx*dx+dy*dy+dz*dz);
        // Conservative center distance; the pinned server allows 39 yards plus
        // object radii and remains authoritative for timer and resurrection.
        d->in_range=d->corpse&&d->distance<=39;d->ready=d->in_range&&!d->remaining_ms;
    }
    if((previous!=2||!d->known)&&d->query_attempts<3&&(!d->query_attempts||(unsigned)(now-d->query_at)>=5000)){
        d->query_attempts++;d->query_at=now;query=1;
    }return query;
}
unsigned wx_death_action(const WxDeath* d){return d->state==1?0x15a:d->state==2&&d->ready?0x1d2:d->state==2&&!d->known?0x216:0;}
