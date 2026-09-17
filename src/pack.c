#include "wx_runtime.h"
#include "wx_material.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define HEADROOM (8u*1024u*1024u)
#include "stream_budget.h"
static unsigned index_serial;
static void index_changed(WxScene* s){
    if(!++index_serial)++index_serial;s->index_revision=index_serial;
    if(s->selection.phase)s->selection.restarts++;
    s->selection.phase=0;s->wanted_ready=0;
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)s->wanted[i]=-1;
}
static int range(unsigned offset,unsigned count,unsigned size,unsigned length){return offset<=length && count<=(length-offset)/size;}
unsigned wx_texture_frame_bytes(const WxEntry* e){
    if(!e||!e->width||!e->height||e->width>256||e->height>256)return 0;
    unsigned levels=wx_texture_levels(e),frames=wx_texture_frames(e);
    if((e->flags&WX_TEXTURE_SEQUENCE)&&(frames<2||frames>32||!(e->reserved[1]&65535u)||e->kind!=WX_KIND_STATIC||(e->flags&WX_ANIMATED)))return 0;
    unsigned compressed=e->flags&(WX_TEX_DXT1|WX_TEX_DXT5);
    if(compressed){
        if(compressed==(WX_TEX_DXT1|WX_TEX_DXT5)||(e->flags&(WX_TEX_SWIZZLED|WX_ANIMATED))||
           (e->kind!=WX_KIND_TERRAIN&&e->kind!=WX_KIND_STATIC)||e->width!=e->height||(e->width&(e->width-1)))return 0;
    }
    if(levels>9||(!(e->flags&WX_MIPMAPPED)&&levels!=1))return 0;
    if((e->flags&WX_MIPMAPPED)&&(!(e->flags&WX_TEXTURE_ENCODING)||levels<2||e->width!=e->height||(e->width&(e->width-1))))return 0;
    unsigned bytes=0,w=e->width,h=e->height;
    for(unsigned i=0;i<levels;i++){if(!w||!h)return 0;bytes+=compressed?((w+3)/4)*((h+3)/4)*(compressed==WX_TEX_DXT1?8:16):w*h*4;w/=2;h/=2;}
    return (e->flags&WX_TEXTURE_SEQUENCE)?(bytes+127u)&~127u:bytes;
}
unsigned wx_texture_bytes(const WxEntry* e){return e?wx_texture_frame_bytes(e)*wx_texture_frames(e):0;}
static int entry_valid(const WxEntry* e,const WxPackHeader* h){
        if(!e->vertex_count||e->vertex_count>65535||!e->index_count||e->index_count>300000||e->index_count%3||
           !e->width||!e->height||e->width>256||e->height>256||
           e->kind<WX_KIND_TERRAIN||e->kind>WX_KIND_COLLISION||e->flags&~(WX_TEXTURE_ENCODING|WX_ALPHA_TEST|WX_ANIMATED|WX_MIPMAPPED|WX_PLACEMENT_ID|WX_MATERIAL_FLAGS|WX_MATERIAL_V7|WX_MATERIAL_V8|WX_MATERIAL_V10)||
           ((e->flags&WX_MATERIAL_V10)&&(h->version<10||e->kind!=WX_KIND_STATIC||(e->flags&WX_ANIMATED)))||
           ((e->flags&WX_LIQUID)&&!(e->flags&WX_TEXTURE_SEQUENCE))||
           ((e->flags&WX_MATERIAL_V8)&&(h->version<8||e->kind!=WX_KIND_STATIC||(e->flags&WX_ANIMATED)))||
           ((e->flags&WX_MATERIAL_V7)&&(h->version<7||e->kind==WX_KIND_COLLISION))||
           !wx_material_valid(e->flags,h->version,e->kind)||
           (h->version<5&&(e->flags&(WX_TEX_DXT1|WX_TEX_DXT5)))||
           ((e->flags&WX_PLACEMENT_ID)&&((e->flags&WX_ANIMATED)||e->kind==WX_KIND_TERRAIN||e->kind==WX_KIND_CHARACTER))||
           ((e->flags&WX_TEX_SWIZZLED)&&(e->width!=e->height||(e->width&(e->width-1))))||
           !range(e->vertex_offset,e->vertex_count,sizeof(WxVertex),h->file_size)||
           !range(e->index_offset,e->index_count,2,h->file_size)||!wx_texture_bytes(e)||!range(e->texture_offset,wx_texture_bytes(e),1,h->file_size)||
           !isfinite(e->center[0])||!isfinite(e->center[1])||!isfinite(e->center[2])||!isfinite(e->radius)||e->radius<0)return 0;
        unsigned data_start=sizeof *h+h->count*sizeof(WxEntry);
        unsigned prefix=wx_vertex_color_bytes(e)+((e->flags&WX_MATERIAL_MOTION)?sizeof(WxMaterialMotion):0);
        if(e->vertex_offset<data_start+prefix)return 0;
        if(e->vertex_offset<data_start||e->index_offset<data_start||e->texture_offset<data_start)return 0;
    return 1;
}
void wx_scene_init(WxScene* s){
    memset(s,0,sizeof *s);for(int i=0;i<WX_CACHE_SLOTS;i++){s->slots[i].entry=-1;s->slots[i].texture_slot=-1;s->slots[i].pose_slot=-1;}
    s->budget_bytes=32u*1024u*1024u;
    index_changed(s);
}
#include "pack_environment.h"
#include "pack_profile.h"
int wx_pack_open(WxScene* s,const char* path){
    if(wx_stream_frame_active())return 0;wx_scene_init(s);
    WxPackOpen job={0};if(!wx_pack_open_begin(&job,path,WX_STREAM_INDEX))return 0;
    int result;do{result=wx_pack_open_pump(s,&job);}while(!result);
    wx_pack_open_cancel(&job);return result>0;
}
static void unload(WxScene* s,WxResident* r){
    wx_gpu_free(r->vertices);wx_gpu_free(r->vertex_colors);
    if(r->texture_slot>=0){WxTexture* t=&s->textures[r->texture_slot];
        if(--t->references==0){wx_gpu_free(t->pixels);s->bytes-=t->bytes;memset(t,0,sizeof *t);}}
    if(r->pose_slot>=0){WxPoseBuffer* poses=&s->poses[r->pose_slot];if(--poses->references==0){free(poses->data);s->bytes-=poses->bytes;memset(poses,0,sizeof *poses);}}
    free(r->bind_vertices);free(r->skin);free(r->material_motion);
    free(r->indices);s->bytes-=r->bytes;memset(r,0,sizeof *r);r->entry=-1;r->texture_slot=-1;r->pose_slot=-1;
}
static void stream_cancel(WxScene* s);
void wx_pack_close(WxScene* s){stream_cancel(s);for(int i=0;i<WX_CACHE_SLOTS;i++)unload(s,&s->slots[i]);free(s->entries);s->entries=NULL;s->index_bytes=0;
    for(unsigned i=0;i<WX_REGION_FILES;i++){environment_release(s,s->sources+i);if(s->sources[i].file)fclose(s->sources[i].file);memset(&s->sources[i],0,sizeof s->sources[i]);}s->file=NULL;s->header.count=0;index_changed(s);}
static unsigned source_of(const WxScene* s,unsigned id){for(unsigned i=0;i<WX_REGION_FILES;i++)if(s->sources[i].file&&id>=s->sources[i].first&&id-s->sources[i].first<s->sources[i].count)return i;return WX_REGION_FILES;}
static int read_at(WxScene* s,unsigned id,unsigned at,void* p,unsigned n){unsigned source=source_of(s,id);if(source==WX_REGION_FILES)return 0;
    if(!range(at,n,1,s->sources[source].bytes))return 0;
    FILE* file=s->sources[source].file;return !fseek(file,at,SEEK_SET)&&fread(p,1,n,file)==n;}
int wx_pack_attach(WxScene* s,const char* path){
    unsigned slot=0;while(slot<WX_REGION_FILES&&s->sources[slot].file)slot++;if(slot==WX_REGION_FILES)return -1;
    WxScene* next=malloc(sizeof *next);if(!next)return -1;
    if(!wx_pack_open(next,path)){snprintf(s->error,sizeof s->error,"%s",next->error);free(next);return -1;}
    unsigned count=s->header.count+next->header.count,bytes=count*sizeof(WxEntry);
    if(count>WX_REGION_FILES*WX_MAX_ENTRIES||wx_free_memory()<HEADROOM+bytes+65536){wx_pack_close(next);free(next);return -1;}
    WxEntry* entries=realloc(s->entries,bytes);if(!entries){wx_pack_close(next);free(next);return -1;}
    s->entries=entries;s->index_bytes=bytes;memcpy(entries+s->header.count,next->entries,next->header.count*sizeof(WxEntry));
    s->sources[slot]=next->sources[0];s->sources[slot].first=s->header.count;
    s->bytes+=next->sources[0].environment.bytes;next->bytes-=next->sources[0].environment.bytes;memset(&next->sources[0].environment,0,sizeof next->sources[0].environment);
    s->header.count=count;s->file=next->file;next->sources[0].file=NULL;next->file=NULL;index_changed(s);
    wx_pack_close(next);free(next);s->wanted_ready=0;s->error[0]=0;return (int)slot;
}
void wx_pack_detach(WxScene* s,unsigned source){
    if(source>=WX_REGION_FILES||!s->sources[source].file)return;
    unsigned first=s->sources[source].first,count=s->sources[source].count;
    if(s->stream_job.phase){if(source_of(s,(unsigned)s->stream_job.entry)==source)stream_cancel(s);
        else if(s->stream_job.entry>=(int)(first+count))s->stream_job.entry-=count;}
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++){WxResident* r=&s->slots[i];
        if(r->entry>=(int)first&&r->entry<(int)(first+count))unload(s,r);else if(r->entry>=(int)(first+count))r->entry-=count;}
    memmove(s->entries+first,s->entries+first+count,(s->header.count-first-count)*sizeof(WxEntry));
    s->header.count-=count;environment_release(s,s->sources+source);fclose(s->sources[source].file);memset(&s->sources[source],0,sizeof s->sources[source]);s->file=NULL;
    for(unsigned i=0;i<WX_REGION_FILES;i++)if(s->sources[i].file){if(s->sources[i].first>first)s->sources[i].first-=count;s->file=s->sources[i].file;}
    s->wanted_ready=0;
    index_changed(s);
}
#include "pack_open.h"
#include "pack_stream.h"
int wx_pack_verify(WxScene* s){
    if(frame_active){snprintf(s->error,sizeof s->error,"Offline verifier cannot drain a render-frame quota");return 0;}
    if(!s||!s->file)return 0;stream_cancel(s);
    int environment;do{s->streaming.read_bytes=s->streaming.read_ops=s->streaming.scan_bytes=0;environment=environment_pump(s);}while(!environment);
    if(environment<0)return 0;
    for(int i=0;i<WX_CACHE_SLOTS;i++)unload(s,&s->slots[i]);
    for(unsigned i=0;i<s->header.count;i++){
        if(!load(s,&s->slots[0],(int)i))return 0;unload(s,&s->slots[0]);
    }
    return 1;
}
static int wanted_entry(const WxScene* s,int entry){
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(s->wanted[i]==entry)return 1;return 0;
}
static int resident_entry(const WxScene* s,int entry){
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(s->slots[i].entry==entry)return 1;return 0;
}
#include "pack_selection.h"
int wx_stream_ready(const WxScene* s){
    if(!s||!s->wanted_ready||s->selection.phase||s->stream_job.phase||!wx_environment_ready(s))return 0;
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(s->wanted[i]>=0&&!resident_entry(s,s->wanted[i]))return 0;
    return 1;
}
void wx_stream(WxScene* s,const float* position){
    stream_budget_enter(WX_STREAM_WORLD);
    if(!s)return;s->streaming.read_bytes=s->streaming.read_ops=s->streaming.scan_bytes=0;
    if(!s->file||!position)return;
    for(unsigned i=0;i<3;i++)if(!isfinite(position[i]))return;
    int* wanted=s->wanted;
    s->streaming.read_bytes=s->streaming.read_ops=s->streaming.scan_bytes=0;
    if(environment_pump(s)<0)return;
    if(!selection_step(s,position)&&!s->wanted_ready)return;
    if(s->stream_job.phase&&!wanted_entry(s,s->stream_job.entry))stream_cancel(s);
    if(s->stream_job.phase&&s->entries[s->stream_job.entry].kind!=WX_KIND_COLLISION){
        for(unsigned i=WX_RENDER_SLOTS;i<WX_CACHE_SLOTS;i++)if(wanted[i]>=0&&!resident_entry(s,wanted[i])){
            const WxEntry* e=&s->entries[wanted[i]];float dx=e->center[0]-position[0],dy=e->center[1]-position[1];
            if(sqrtf(dx*dx+dy*dy)-e->radius<=45){stream_cancel(s);break;}
        }
    }
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(s->slots[i].entry>=0&&!wanted_entry(s,s->slots[i].entry))unload(s,&s->slots[i]);
    unsigned completed=0,operations=0;
    while(completed<3&&operations++<64){
        if(!s->stream_job.phase){
            int next=-1,slot=-1;
            // Collision is serviced before visuals; each class is nearest-first.
            for(unsigned pass=0;pass<2&&next<0;pass++)for(unsigned j=pass?0:WX_RENDER_SLOTS;j<(pass?WX_RENDER_SLOTS:WX_CACHE_SLOTS);j++){
                if(wanted[j]>=0&&!resident_entry(s,wanted[j])){next=wanted[j];break;}}
            if(next<0)break;
            for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(s->slots[i].entry<0){slot=(int)i;break;}
            if(slot<0)break;
            s->stream_job=(WxStreamJob){next,slot,1,0,0,0};
        }
        int result=stream_step(s,WX_STREAM_READ_BYTES,WX_STREAM_READ_OPS,WX_STREAM_SCAN_BYTES,WX_STREAM_WORLD);
        if(!result)break;if(result<0){completed++;break;}if(result==1)completed++;
    }
    if(s->stream_job.phase)s->streaming.deferred++;
    if(s->streaming.read_bytes>s->streaming.max_read_bytes)s->streaming.max_read_bytes=s->streaming.read_bytes;
    if(s->streaming.read_ops>s->streaming.max_read_ops)s->streaming.max_read_ops=s->streaming.read_ops;
    if(s->streaming.scan_bytes>s->streaming.max_scan_bytes)s->streaming.max_scan_bytes=s->streaming.scan_bytes;
}
int wx_animation_resident(const WxScene* s,uint32_t id){
    unsigned parts=0;
    for(unsigned i=0;i<s->header.count;i++)if(s->entries[i].id==id){
        unsigned j=0;while(j<WX_CACHE_SLOTS&&s->slots[j].entry!=(int)i)j++;
        if(j==WX_CACHE_SLOTS)return 0;parts++;
    }
    return parts!=0;
}
uint32_t wx_animation_fallback(const WxScene* s,uint32_t id){
    if(wx_animation_resident(s,id))return id;
    uint32_t idle=id&~255u;if(wx_animation_resident(s,idle))return idle;
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(s->slots[i].entry>=0){
        uint32_t other=s->entries[s->slots[i].entry].id;
        if((other>>8)==(id>>8)&&wx_animation_resident(s,other))return other;
    }
    return UINT32_MAX;
}
static int idle_run_pair(uint32_t a,uint32_t b){
    return (a>>8)==(b>>8)&&((a&255)==0||(a&255)==5)&&((b&255)==0||(b&255)==5);
}
static void trim_actor_pairs(WxScene* s,uint8_t optional[WX_CACHE_SLOTS],unsigned reserve,int need_slot){
    // Drop optional cached clips before a requested allocation can fail. The
    // last complete fallback is never optional. Remove whole clips together.
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(optional[i]&&s->slots[i].entry>=0){
        if(!need_slot&&reserve<=s->budget_bytes&&s->bytes<=s->budget_bytes-reserve&&wx_free_memory()>=HEADROOM+reserve+65536)break;
        uint32_t id=s->entries[s->slots[i].entry].id;
        for(unsigned j=0;j<WX_CACHE_SLOTS;j++)if(optional[j]&&s->slots[j].entry>=0&&s->entries[s->slots[j].entry].id==id){unload(s,&s->slots[j]);optional[j]=0;}
        need_slot=0;
    }
}
static void stream_actors(WxScene* s,const uint32_t* displays,unsigned count,int exact,unsigned previous_clip,const uint32_t* held,unsigned held_count){
    unsigned lane=exact==2?WX_STREAM_AVATAR:WX_STREAM_ACTORS;stream_budget_enter(lane);
    // At most 128 requested batches and two commits/frame, with the same byte
    // and validation budgets as world jobs. Partial clips remain unpublished.
    s->streaming.read_bytes=s->streaming.read_ops=s->streaming.scan_bytes=0;
    int wanted[128];unsigned used=0;if(count>32)count=32;
    uint8_t optional[WX_CACHE_SLOTS]={0};uint32_t fallback[32];
    for(unsigned i=0;exact==1&&i<count;i++)fallback[i]=wx_animation_fallback(s,displays[i]);
    for(unsigned j=0;j<count;j++)for(unsigned i=0;i<s->header.count&&used<128;i++){
        if((exact?s->entries[i].id:s->entries[i].id>>8)!=displays[j])continue;
        unsigned k=0;while(k<used&&wanted[k]!=(int)i)k++;if(k==used)wanted[used++]=(int)i;
    }
    int complete=1;for(unsigned i=0;exact==2&&i<count;i++)if(!wx_animation_resident(s,displays[i]))complete=0;
    // Retention adds no prefetches and never changes the allocation budget or
    // headroom guard. Drop optional idle/run data when system memory is tight.
    int keep_pair=exact&&wx_free_memory()>=HEADROOM+s->budget_bytes;
    if(s->stream_job.phase){
        unsigned j=0;while(j<used&&wanted[j]!=s->stream_job.entry)j++;
        int keep=j<used;
        if(!keep&&keep_pair){uint32_t pending=s->entries[s->stream_job.entry].id;
            for(unsigned k=0;k<count;k++)if(idle_run_pair(displays[k],pending)){keep=1;break;}}
        if(!keep)stream_cancel(s);
    }
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++){
        unsigned j=0;while(j<used&&wanted[j]!=s->slots[i].entry)j++;
        if(j==used&&s->slots[i].entry>=0){
            int retain=0,required=0;
            for(unsigned k=0;k<held_count;k++)if(s->entries[s->slots[i].entry].id==held[k])required=1;
            // Hold a complete prior clip while the next one's bounded queue is
            // loading, so switching walk/attack/idle never draws half a body.
            if(exact){uint32_t old=s->entries[s->slots[i].entry].id;
                for(unsigned k=0;k<count;k++)if((displays[k]>>8)==(old>>8)){
                    if(exact==2){
                        unsigned requested=displays[k]&255,prior=old&255;
                        if(!complete&&prior==previous_clip)required=1;
                        if(keep_pair&&((requested==0&&prior==5)||(requested==5&&prior==0)))retain=1;
                    }else{
                        if(!wx_animation_resident(s,displays[k])&&fallback[k]==old)required=1;
                        if(keep_pair&&idle_run_pair(displays[k],old))retain=1;
                    }
                }}
            if(!retain&&!required)unload(s,&s->slots[i]);else if(!required)optional[i]=1;
        }
    }
    unsigned completed=0,operations=0;
    while(completed<2&&operations++<64){
        if(!s->stream_job.phase){
            int next=-1,slot=-1;
            for(unsigned j=0;j<used;j++)if(!resident_entry(s,wanted[j])){next=wanted[j];break;}
            if(next<0)break;
            for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(s->slots[i].entry<0){slot=(int)i;break;}
            if(slot<0){trim_actor_pairs(s,optional,0,1);for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(s->slots[i].entry<0){slot=(int)i;break;}}
            if(slot<0)break;s->stream_job=(WxStreamJob){next,slot,1,0,0,0};
        }
        if(s->stream_job.phase==2){
            const WxEntry* e=&s->entries[s->stream_job.entry];const WxAnimation* a=&s->slots[s->stream_job.slot].animation;
            // Conservative upper bound includes unshared texture/pose bytes;
            // allocation itself still accounts for actual sharing exactly.
            unsigned reserve=e->vertex_count*sizeof(WxVertex)+e->index_count*2+wx_texture_bytes(e);
            if(a->frames)reserve+=e->vertex_count*(sizeof(WxVertex)+sizeof(WxSkinVertex))+a->frames*a->bones*48;
            trim_actor_pairs(s,optional,reserve,0);
        }
        int result=stream_step(s,WX_STREAM_READ_BYTES,WX_STREAM_READ_OPS,WX_STREAM_SCAN_BYTES,lane);
        if(!result)break;if(result<0)break;if(result==1)completed++;
    }
    s->stream_needed=s->stream_job.phase!=0;
    for(unsigned i=0;!s->stream_needed&&i<used;i++)if(!resident_entry(s,wanted[i]))s->stream_needed=1;
    if(s->stream_job.phase)s->streaming.deferred++;
    if(s->streaming.read_bytes>s->streaming.max_read_bytes)s->streaming.max_read_bytes=s->streaming.read_bytes;
    if(s->streaming.read_ops>s->streaming.max_read_ops)s->streaming.max_read_ops=s->streaming.read_ops;
    if(s->streaming.scan_bytes>s->streaming.max_scan_bytes)s->streaming.max_scan_bytes=s->streaming.scan_bytes;
}
void wx_stream_displays(WxScene* s,const uint32_t* displays,unsigned count){stream_actors(s,displays,count,0,UINT32_MAX,NULL,0);}
void wx_stream_animations(WxScene* s,const uint32_t* ids,unsigned count){stream_actors(s,ids,count,1,UINT32_MAX,NULL,0);}
void wx_stream_avatar_animations(WxScene* s,const uint32_t* ids,unsigned count,unsigned previous_clip){stream_actors(s,ids,count,2,previous_clip,NULL,0);}
void wx_stream_avatar_transition(WxScene* s,const uint32_t* ids,unsigned count,const uint32_t* held,unsigned held_count){
    stream_actors(s,ids,count,2,UINT32_MAX,held,held_count>32?32:held_count);
}
int wx_ground(WxScene* s,float x,float y,float* z){
    for(int k=0;k<WX_CACHE_SLOTS;k++){WxResident* r=&s->slots[k];if(r->entry<0)continue;WxEntry* e=&s->entries[r->entry];if(e->kind!=WX_KIND_TERRAIN)continue;
        if(fabsf(e->center[0]-x)>17||fabsf(e->center[1]-y)>17)continue;
        for(unsigned i=0;i<e->index_count;i+=3){const float* a=r->vertices[r->indices[i]].p;const float* b=r->vertices[r->indices[i+1]].p;const float* c=r->vertices[r->indices[i+2]].p;
            float det=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1]);if(fabsf(det)<.0001f)continue;
            float u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/det;
            float v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/det;
            if(u>=-.0001f&&v>=-.0001f&&u+v<=1.0001f){*z=u*a[2]+v*b[2]+(1-u-v)*c[2];return 1;}}
    }return 0;
}
