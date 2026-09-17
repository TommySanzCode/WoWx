// Fixed-capacity adaptation of WoWee's Classic update/movement/value layouts.
#include "wx_entities.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
typedef struct Reader {const uint8_t* data;size_t size,at;int ok;} Reader;
static uint32_t readn(Reader* r,unsigned n){
    if(!r->ok||r->at>r->size||n>r->size-r->at){r->ok=0;return 0;}
    uint32_t out=0;for(unsigned i=0;i<n;i++)out|=(uint32_t)r->data[r->at++]<<(i*8);return out;
}
static uint64_t guid(Reader* r,int packed){
    uint64_t out=0;unsigned mask=packed?readn(r,1):255;
    for(unsigned i=0;i<8;i++)if(mask&(1u<<i))out|=(uint64_t)readn(r,1)<<(i*8);return out;
}
static float real(Reader* r){uint32_t n=readn(r,4);float out;memcpy(&out,&n,4);if(!isfinite(out))r->ok=0;return out;}
static void floats(Reader* r,unsigned n){while(n--&&r->ok)real(r);}
static WxEntity* find(WxEntities* s,uint64_t id,int create){
    WxEntity* free_slot=0;for(unsigned i=0;i<WX_MAX_ENTITIES;i++){
        if(s->items[i].guid==id)return &s->items[i];if(!s->items[i].guid&&!free_slot)free_slot=&s->items[i];}
    if(create&&free_slot){memset(free_slot,0,sizeof *free_slot);free_slot->guid=id;s->count++;return free_slot;}return 0;
}
void wx_entities_destroy(WxEntities* s,uint64_t id){if(!s||!id)return;WxEntity* entity=find(s,id,0);if(entity){memset(&s->paths[entity-s->items],0,sizeof(WxSpline));memset(entity,0,sizeof *entity);s->count--;}}
static void point(Reader* r,float* p){for(unsigned i=0;i<3;i++){p[i]=real(r);if(fabsf(p[i])>20000)r->ok=0;}}
static void distances(WxSpline* path){
    path->distance[0]=0;
    for(unsigned i=1;i<path->count;i++){
        float sum=0;for(unsigned j=0;j<3;j++){float d=path->points[i][j]-path->points[i-1][j];sum+=d*d;}
        path->distance[i]=path->distance[i-1]+sqrtf(sum);
    }
}
static void position(Reader* r,WxEntity* e){
    e->x=real(r);e->y=real(r);e->z=real(r);e->orientation=real(r);e->positioned=1;
    if(fabsf(e->x)>20000||fabsf(e->y)>20000||fabsf(e->z)>20000)r->ok=0;
}
static void movement(Reader* r,WxEntity* e,WxSpline* path,uint32_t now){
    memset(path,0,sizeof *path);
    unsigned update=readn(r,1);if(update&0x80){r->ok=0;return;}
    if(update&0x20){
        uint32_t flags=readn(r,4);readn(r,4);position(r,e);
        if(flags&0x02000000){guid(r,0);floats(r,4);}
        if(flags&0x00200000)floats(r,1);
        readn(r,4);if(flags&0x2000)floats(r,4);if(flags&0x04000000)floats(r,1);
        floats(r,6);
        if(flags&0x00400000){
            path->flags=readn(r,4);
            if(path->flags&0x10000){path->facing_type=2;point(r,path->facing);}
            else if(path->flags&0x20000){path->facing_type=3;path->target=guid(r,0);}
            else if(path->flags&0x40000){path->facing_type=4;path->angle=real(r);}
            unsigned elapsed=readn(r,4);path->duration=readn(r,4);readn(r,4);path->count=readn(r,4);
            if(path->count>256||path->duration>3600000){r->ok=0;return;}
            path->started=now-elapsed;
            for(unsigned i=0;i<path->count;i++)point(r,path->points[i]);floats(r,3);distances(path);
        }
    }else if(update&0x40)position(r,e);
    if(update&8)readn(r,4);if(update&16)readn(r,4);if(update&4)guid(r,1);if(update&2)readn(r,4);
}
static void fields(Reader* r,WxEntity* e){
    unsigned blocks=readn(r,1);uint32_t mask[64];if(blocks>64){r->ok=0;return;}
    for(unsigned i=0;i<blocks;i++)mask[i]=readn(r,4);
    for(unsigned i=0;i<blocks&&r->ok;i++)for(unsigned bit=0;bit<32;bit++)if(mask[i]&(1u<<bit)){
        uint32_t value=readn(r,4);unsigned field=i*32+bit;if(field<WX_UNIT_FIELDS)e->fields[field]=value;
        if(e->type==4){
            if(field==193)e->player_bytes=value;
            else if(field==194)e->player_bytes2=value;
            else if(field>=260&&field<=476&&(field-260)%12==0)e->visible_items[(field-260)/12]=value;
            else if(field>=198&&field<258)e->quests[field-198]=value;
            else if(field>=486&&field<564)e->inventory[field-486]=value;
            else if(field>=0x457&&field<0x457+64)e->explored[field-0x457]=value;
            else if(field==716)e->xp=value;else if(field==717)e->next_xp=value;else if(field==1176)e->money=value;
        }
    }
}
int wx_entities_apply(WxEntities* live,WxEntities* scratch,const uint8_t* data,size_t size,uint32_t now){
    if(!live||!scratch||live==scratch||!data||size<5||size>WX_UPDATE_LIMIT)return 0;
    memcpy(scratch,live,sizeof *scratch);Reader r={data,size,0,1};unsigned blocks=readn(&r,4);
    if(blocks>1024||readn(&r,1)>1)return 0;
    for(unsigned i=0;i<blocks&&r.ok;i++){
        unsigned type=readn(&r,1);
        if(type==4||type==5){unsigned count=readn(&r,4);if(count>4096)return 0;
            for(unsigned j=0;j<count&&r.ok;j++){uint64_t id=guid(&r,1);if(type==4)wx_entities_destroy(scratch,id);}continue;}
        if(type>3)return 0;uint64_t id=guid(&r,type!=1);if(!id||!r.ok)return 0;
        WxEntity dummy={0},*e=find(scratch,id,type>=2);WxSpline dummy_path={0};
        WxSpline* path=e?&scratch->paths[e-scratch->items]:&dummy_path;
        if(!e){if(type>=2)return 0;e=&dummy;scratch->unknown_updates++;}
        if(type>=2){uint8_t object_type=(uint8_t)readn(&r,1);if(object_type>7)return 0;
            memset(e,0,sizeof *e);memset(path,0,sizeof *path);e->guid=id;e->type=object_type;}
        if(type)movement(&r,e,path,now);if(type!=1)fields(&r,e);
    }
    if(!r.ok||r.at!=r.size)return 0;scratch->packets++;memcpy(live,scratch,sizeof *live);return 1;
}

// Vanilla 5875: packed GUID, start float3, spline ID, facing, flags, duration,
// point count, then a destination and deltas relative to that destination.
// The midpoint convention used by later clients produces wrong Vanilla paths.
int wx_entities_monster_move(WxEntities* s,const uint8_t* data,size_t size,uint32_t now){
    if(!s||!data||size>65533)return 0;
    Reader r={data,size,0,1};uint64_t id=guid(&r,1);WxSpline path={0};
    point(&r,path.points[0]);readn(&r,4);unsigned facing=readn(&r,1);if(!id||facing>4)return 0;
    path.facing_type=(uint8_t)facing;
    if(facing==2)point(&r,path.facing);else if(facing==3)path.target=guid(&r,0);else if(facing==4)path.angle=real(&r);
    if(facing!=1){
        path.flags=readn(&r,4);path.duration=readn(&r,4);unsigned count=readn(&r,4);
        if(count>256||path.duration>3600000)return 0;path.count=count+1;path.started=now;
        if(path.flags&0x200){for(unsigned i=1;i<=count;i++)point(&r,path.points[i]);}
        else if(count){
            point(&r,path.points[count]);
            for(unsigned i=1;i<count;i++){
                uint32_t packed=readn(&r,4);
                int delta[3]={(int)(packed&2047),(int)((packed>>11)&2047),(int)((packed>>22)&1023)};
                for(unsigned j=0;j<3;j++){
                    int sign=j==2?512:1024;if(delta[j]&sign)delta[j]-=sign*2;
                    path.points[i][j]=path.points[count][j]-delta[j]*.25f;
                    if(fabsf(path.points[i][j])>20000)r.ok=0;
                }
            }
        }
        distances(&path);
    }
    if(!r.ok||r.at!=r.size)return 0;
    WxEntity* e=find(s,id,0);if(!e){s->unknown_updates++;return 1;}
    e->x=path.points[0][0];e->y=path.points[0][1];e->z=path.points[0][2];e->positioned=1;
    s->paths[e-s->items]=path;return 1;
}

int wx_relocation_decode(const uint8_t* data,size_t size,WxEntity* output){
    if(!output||!data||size>128)return 0;
    Reader r={data,size,0,1};uint64_t id=guid(&r,1);uint32_t flags=readn(&r,4);readn(&r,4);WxEntity pose={0};position(&r,&pose);
    if(flags&0x02000000){guid(&r,0);floats(&r,4);}
    if(flags&0x00200000)floats(&r,1);
    readn(&r,4);if(flags&0x2000)floats(&r,4);if(flags&0x04000000)floats(&r,1);
    if(!id||!r.ok||r.at!=r.size)return 0;
    pose.guid=id;*output=pose;return 1;
}
int wx_entities_relocate(WxEntities* s,const uint8_t* data,size_t size,uint64_t* moved){
    WxEntity pose;if(!s||!wx_relocation_decode(data,size,&pose))return 0;uint64_t id=pose.guid;
    WxEntity* e=find(s,id,0);if(e){e->x=pose.x;e->y=pose.y;e->z=pose.z;e->orientation=pose.orientation;e->positioned=1;e->animation=0;memset(&s->paths[e-s->items],0,sizeof(WxSpline));}
    else s->unknown_updates++;
    if(moved)*moved=id;return 1;
}
void wx_entities_sample(const WxEntities* s,unsigned index,uint32_t now,WxEntity* out){
    if(!s||!out||index>=WX_MAX_ENTITIES)return;*out=s->items[index];const WxSpline* path=&s->paths[index];
    if(path->count<2||!path->duration)return;
    uint32_t elapsed=now-path->started;
    // Cyclic, falling, and flying require specialized interpolation. Their full
    // path is retained; this phase samples ground paths using distance and time.
    if(path->flags&0x100000)elapsed%=path->duration;
    float fraction=fminf((float)elapsed/path->duration,1.f);
    if(path->flags&0x400000)fraction=0;
    float distance=path->distance[path->count-1]*fraction;unsigned next=1;
    while(next+1<path->count&&path->distance[next]<distance)next++;
    float span=path->distance[next]-path->distance[next-1];
    float t=span>.00001f?(distance-path->distance[next-1])/span:1;
    const float* a=path->points[next-1];const float* b=path->points[next];
    out->x=a[0]+(b[0]-a[0])*t;out->y=a[1]+(b[1]-a[1])*t;out->z=a[2]+(b[2]-a[2])*t;
    if(span>.00001f)out->orientation=atan2f(b[1]-a[1],b[0]-a[0]);
    out->animation=(elapsed<path->duration&&span>.00001f)?((path->flags&0x100)?5:4):0;
    if(elapsed>=path->duration){
        if(path->facing_type==4)out->orientation=path->angle;
        else if(path->facing_type==2)out->orientation=atan2f(path->facing[1]-out->y,path->facing[0]-out->x);
        else if(path->facing_type==3)for(unsigned i=0;i<WX_MAX_ENTITIES;i++)if(s->items[i].guid==path->target){
            out->orientation=atan2f(s->items[i].y-out->y,s->items[i].x-out->x);break;}
    }
}
typedef struct EntityChoice {uint64_t guid;float distance;unsigned index,rank;} EntityChoice;
static int choice_order(const void* left,const void* right){
    const EntityChoice *a=left,*b=right;
    if(a->rank!=b->rank)return a->rank<b->rank?-1:1;
    if(a->distance!=b->distance)return a->distance<b->distance?-1:1;
    return a->guid<b->guid?-1:a->guid>b->guid;
}
unsigned wx_entities_nearby(const WxEntities* s,uint64_t self,uint64_t target,
    const float* p,uint32_t now,WxEntity* output,unsigned capacity){
    if(!s||!p||!output||!capacity||!isfinite(p[0])||!isfinite(p[1])||!isfinite(p[2]))return 0;
    if(capacity>WX_VISIBLE_ENTITIES)capacity=WX_VISIBLE_ENTITIES;
    EntityChoice choices[WX_MAX_ENTITIES];unsigned count=0;
    for(unsigned i=0;i<WX_MAX_ENTITIES;i++){
        const WxEntity* e=&s->items[i];if(!e->guid)continue;
        int own_corpse=e->type==7&&self&&(((uint64_t)e->fields[7]<<32)|e->fields[6])==self;
        if(e->guid!=self&&!own_corpse&&(!e->positioned||(e->type!=3&&e->type!=4&&e->type!=5&&e->type!=7)))continue;
        WxEntity sampled;wx_entities_sample(s,i,now,&sampled);
        float dx=sampled.x-p[0],dy=sampled.y-p[1],dz=sampled.z-p[2],distance=dx*dx+dy*dy+dz*dz;
        if(!isfinite(distance))continue;
        unsigned rank=e->guid==self?0:e->guid==target?1:own_corpse?2:3;
        choices[count++]=(EntityChoice){e->guid,distance,i,rank};
    }
    qsort(choices,count,sizeof *choices,choice_order);
    if(count>capacity)count=capacity;
    for(unsigned i=0;i<count;i++)wx_entities_sample(s,choices[i].index,now,&output[i]);
    return count;
}
