#include "wx_avatar.h"
#include <stdlib.h>
#include <string.h>
static int apply_look(WxAvatar* a,const uint32_t look[7]){
    if(a->header.version!=2||!wx_looks_valid(&a->looks,look))return 0;
    const WxLookRow* hair=wx_looks_find(&a->looks,5,look[4],0),*facial=wx_looks_find(&a->looks,6,look[6],0);
    // Some shipped styles (e.g. Orc female 7) have only scalp textures. The
    // pinned assembler uses bald geoset 1 when no hair-mesh mapping exists.
    memcpy(a->header.look,look,7*sizeof(uint32_t));a->header.scalp=hair?hair->geoset[0]:1;
    memcpy(a->header.facial,facial->geoset,sizeof a->header.facial);return 1;
}

int wx_avatar_path(char* output,unsigned capacity,const char* directory,const uint32_t look[7],int metadata){
    if(!output||!capacity||!directory||!look||!look[0]||look[0]>8||look[1]>1)return 0;
    for(unsigned i=2;i<7;i++)if(look[i]>255)return 0;
    size_t n=strlen(directory);const char* separator=n&&directory[n-1]!='/'&&directory[n-1]!='\\'?"/":"";
    int size=snprintf(output,capacity,"%s%sA%02X%02X%02X%02X%02X%02X%02X.WX%c",directory,separator,
        look[0],look[1],look[2],look[3],look[4],look[5],look[6],metadata?'A':'P');
    return size>=0&&(unsigned)size<capacity;
}
static int avatar_select_begin(WxAvatar* a,WxAvatarSelection* s,const char* directory,int exact){
    char pack[256],metadata[256];uint32_t base[]={s->look[0],s->look[1],0,0,0,0,0};const uint32_t* look=exact?s->look:base;
    if(!wx_avatar_path(pack,sizeof pack,directory,look,0)||!wx_avatar_path(metadata,sizeof metadata,directory,look,1)||!wx_avatar_open_begin(a,pack,metadata))return 0;
    s->pending=exact?2:1;return 1;
}
int wx_avatar_select(WxAvatar* a,WxAvatarSelection* s,const WxWorldView* world,const char* directory){
    a->opening.read_bytes=a->opening.read_ops=a->opening.scan_bytes=0;
    if(!world->active||!world->race){
        if(s->pending){s->cancelled++;s->pending=s->attempted=0;wx_avatar_close(a);}return 0;
    }
    uint32_t look[]={world->race,world->gender,world->skin,world->face,world->hair_style,world->hair_color,world->facial_hair};
    if(s->pending&&memcmp(s->look,look,sizeof look)){
        if(s->pending==1&&s->look[0]==look[0]&&s->look[1]==look[1]){memcpy(s->look,look,sizeof look);s->world_revision=world->revision;}
        else{s->cancelled++;s->pending=s->attempted=0;wx_avatar_close(a);}
    }
    if(a->profile_ready&&a->file&&!a->failures&&!memcmp(a->header.look,look,sizeof look))return 1;
    if(s->pending){
        int result=wx_avatar_open_pump(a);if(!result)return 0;
        if(s->pending==1&&(result<0||(a->header.version==1&&memcmp(a->header.look,look,sizeof look)))){
            uint32_t base[]={look[0],look[1],0,0,0,0,0};
            if(memcmp(base,look,sizeof look)){wx_avatar_close(a);if(avatar_select_begin(a,s,directory,1))return 0;goto fail;}
        }
        if(result<0||(a->header.version==2?!apply_look(a,look):memcmp(a->header.look,look,sizeof look)))goto fail;
        s->pending=0;s->changes++;return 1;
    }
    if(s->attempted&&s->world_revision==world->revision&&!memcmp(s->look,look,sizeof look))return 0;
    s->attempted=1;s->world_revision=world->revision;memcpy(s->look,look,sizeof look);s->attempts++;
    if(a->profile_ready&&a->file&&!a->failures&&apply_look(a,look)){s->changes++;return 1;}
    if(a->profile_ready&&a->header.version==2&&a->header.look[0]==look[0]&&a->header.look[1]==look[1])goto fail;
    // A new profile immediately hides/releases the previous identity, including
    // any unpublished composition buffers. Reads begin on subsequent pumps.
    wx_avatar_close(a);if(avatar_select_begin(a,s,directory,0))return 0;
fail:
    wx_avatar_close(a);s->pending=0;a->failures=1;s->failures++;
    snprintf(a->error,sizeof a->error,"Appearance profile unavailable or invalid");return 0;
}

static const unsigned regions[10][4]={{0,0,64,32},{0,32,64,32},{0,64,64,16},
    {64,0,64,32},{64,32,64,16},{64,48,64,32},{64,80,64,32},{64,112,64,16},{0,96,64,32},{0,80,64,16}};
static int span(const WxAvatarHeader* h,unsigned offset,unsigned bytes){
    unsigned first=h->items_offset+h->item_count*sizeof(WxAvatarItem);
    return offset>=first&&offset<=h->file_size&&bytes<=h->file_size-offset;
}
static int read_at(WxAvatar* a,unsigned offset,void* data,unsigned bytes){
    return span(&a->header,offset,bytes)&&!fseek(a->file,offset,SEEK_SET)&&fread(data,1,bytes,a->file)==bytes;
}
int wx_avatar_blend(uint8_t* canvas,const uint8_t* pixels,unsigned region){
    if(!canvas||!pixels||region>=10)return 0;const unsigned* r=regions[region];
    for(unsigned y=0;y<r[3];y++)for(unsigned x=0;x<r[2];x++){
        const uint8_t* s=pixels+(y*r[2]+x)*4;uint8_t* d=canvas+((y+r[1])*128+x+r[0])*4;
        unsigned alpha=s[3];if(s[0]==255&&s[1]==0&&s[2]==255)alpha=0;
        for(unsigned c=0;c<3;c++)d[c]=(uint8_t)((s[c]*alpha+d[c]*(255-alpha)+127)/255);
        d[3]=(uint8_t)(alpha+(d[3]*(255-alpha)+127)/255);
    }
    return 1;
}
void wx_avatar_mips(uint8_t* canvas,uint32_t* destination){
    // Destructive reduction of scratch RGBA; GPU output is BGRA Morton order.
    unsigned base=0;
    for(unsigned size=128;size;size/=2){
        for(unsigned y=0;y<size;y++)for(unsigned x=0;x<size;x++){
            const uint8_t* s=canvas+(y*size+x)*4;unsigned address=0;
            for(unsigned bit=0;(1u<<bit)<size;bit++)address|=((x>>bit)&1u)<<(2*bit)|((y>>bit)&1u)<<(2*bit+1);
            destination[base+address]=((uint32_t)s[3]<<24)|((uint32_t)s[0]<<16)|((uint32_t)s[1]<<8)|s[2];
        }
        base+=size*size;if(size==1)break;unsigned next=size/2;
        for(unsigned y=0;y<next;y++)for(unsigned x=0;x<next;x++)for(unsigned c=0;c<4;c++){
            unsigned sum=0;for(unsigned j=0;j<2;j++)for(unsigned i=0;i<2;i++)sum+=canvas[((2*y+j)*size+2*x+i)*4+c];
            canvas[(y*next+x)*4+c]=(uint8_t)((sum+2)/4);
        }
    }
}
void wx_avatar_close(WxAvatar* a){
    wx_pack_open_cancel(&a->opening.pack);if(a->opening.looks_file)fclose(a->opening.looks_file);
    memset(&a->opening,0,sizeof a->opening);a->profile_ready=0;memset(a->family_bits,0,sizeof a->family_bits);
    wx_pack_close(&a->scene);if(a->file)fclose(a->file);a->file=NULL;
    free(a->items);free(a->canvas);free(a->scratch);wx_gpu_free(a->body);wx_gpu_free(a->cape);
    wx_looks_close(&a->looks);wx_gpu_free(a->hair);wx_gpu_free(a->extra);a->hair=a->extra=NULL;
    wx_gpu_free(a->compose.body);wx_gpu_free(a->compose.cape);wx_gpu_free(a->compose.hair);wx_gpu_free(a->compose.extra);
    memset(&a->compose,0,sizeof a->compose);a->appearance_ready=0;a->complete_clip=UINT32_MAX;
    a->items=NULL;a->canvas=a->scratch=NULL;a->body=a->cape=NULL;a->bytes=a->count=a->ready=a->matched=0;
}
static int has(const WxAvatar* a,unsigned family){
    return family<4352&&(a->family_bits[family/32]&(1u<<(family%32)))!=0;
}
#include "avatar_open.h"
int wx_avatar_open(WxAvatar* a,const char* pack,const char* metadata){
    // Offline callers retain the original initial-atlas contract. Production
    // selection pumps metadata, then the existing staged compositor builds it.
    if(wx_stream_frame_active())return 0;memset(a,0,sizeof *a);wx_scene_init(&a->scene);
    if(!wx_avatar_open_begin(a,pack,metadata))return 0;
    int result;do{result=wx_avatar_open_pump(a);}while(!result);
    if(result<0)return 0;
    if(!read_at(a,a->header.base_offset,a->canvas,WX_AVATAR_ATLAS_BYTES)){
        wx_avatar_close(a);a->failures=1;return 0;
    }
    wx_avatar_mips(a->canvas,a->body);memset(a->cape,255,WX_AVATAR_MIP_BYTES);return 1;
}
static unsigned resolve(const WxAvatar* a,unsigned desired){
    if(has(a,desired))return desired;
    if(desired%100<=1)return 0;unsigned best=UINT32_MAX;
    unsigned first=desired/100*100;
    if(first<4096)for(unsigned id=first;id<first+100&&id<4096;id++)if(has(a,id)){best=id;break;}
    return best==UINT32_MAX?0:best;
}
static int add(WxAvatarCompose* j,unsigned family){
    for(unsigned i=0;i<j->count;i++)if(j->families[i]==family)return 1;
    if(j->count==WX_AVATAR_PARTS)return 0;j->families[j->count++]=family;return 1;
}
static unsigned group(const WxAvatarItem* item,unsigned column){return item?item->geoset[column]:0;}
#include "avatar_compose.h"
void wx_avatar_update(WxAvatar* a,const WxWorldView* world,unsigned clip,unsigned time_ms){
    a->scene.animating=(WxAnimationMetrics){0};
    WxAvatarCompose* job=&a->compose;job->read_bytes=job->read_ops=job->work_pixels=0;
    a->ready=a->drawn=a->equipment_mask=a->pose_hash=0;a->matched=0;if(!a->profile_ready||!a->file||a->failures)return;
    unsigned look[]={world->race,world->gender,world->skin,world->face,world->hair_style,world->hair_color,world->facial_hair};
    if(!world->active||memcmp(look,a->header.look,sizeof look)||(world->display_id&&world->native_display_id&&world->display_id!=world->native_display_id)){
        if(job->phase&&job->phase!=8)job->cancelled++;job->phase=0;
        wx_stream_animations(&a->scene,NULL,0);a->appearance_ready=0;a->complete_clip=UINT32_MAX;return;
    }a->matched=1;
    int changing=!a->appearance_ready||memcmp(a->published_look,a->header.look,sizeof a->published_look)||
        memcmp(a->equipment,world->equipment_display,sizeof a->equipment)||a->flags!=world->appearance_flags;
    if(job->phase&&!compose_key(job,a,world)){
        if(job->phase!=8)job->cancelled++;job->phase=0;
    }
    if(changing||job->phase){
        int failed=0;
        if(!job->phase)failed=!compose_plan(a,world);
        if(!failed&&job->phase!=8)failed=compose_pump(a)<0;
        if(failed){job->failures++;job->phase=8;snprintf(a->error,sizeof a->error,"Avatar composition failed");}
        if(job->phase==7){
            wx_stream_avatar_transition(&a->scene,job->ids,job->count,a->ids,a->appearance_ready?a->count:0);
            unsigned i=0;while(i<job->count&&wx_animation_resident(&a->scene,job->ids[i]))i++;
            if(i==job->count){compose_publish(a);a->error[0]=0;}
        }else wx_stream_avatar_animations(&a->scene,a->ids,a->appearance_ready?a->count:0,a->complete_clip);
        if(!a->appearance_ready)return;
        goto animate_published;
    }
    // All selected components share one sequence, so body and equipment never
    // use different bone poses while a new animation is still streaming.
    int supported=1;for(unsigned i=0;i<a->count;i++){
        unsigned id=(a->families[i]<<8)|clip;int found=0;
        for(unsigned j=0;j<a->scene.header.count;j++)if(a->scene.entries[j].id==id){found=1;break;}if(!found)supported=0;
    }if(!supported)clip=0;
    for(unsigned i=0;i<a->count;i++)a->ids[i]=(a->families[i]<<8)|clip;
    wx_stream_avatar_animations(&a->scene,a->ids,a->count,a->complete_clip);
    unsigned selected=clip;
    for(unsigned i=0;i<a->count;i++)if(!wx_animation_resident(&a->scene,a->ids[i])){
        if(a->complete_clip==UINT32_MAX)return;selected=a->complete_clip;break;
    }
    for(unsigned i=0;i<a->count;i++){a->ids[i]=(a->families[i]<<8)|selected;if(!wx_animation_resident(&a->scene,a->ids[i]))return;}
    a->complete_clip=selected;
animate_published:
    for(unsigned i=0;i<a->count;i++)if(!wx_animation_resident(&a->scene,a->ids[i]))return;
    wx_animate_ids(&a->scene,time_ms,a->ids,a->count);a->ready=1;
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++)if(a->scene.slots[i].entry>=0){
        const WxResident* r=&a->scene.slots[i];const WxEntry* e=&a->scene.entries[r->entry];if(e->id!=a->ids[0])continue;
        unsigned hash=2166136261u,count=e->vertex_count<32?e->vertex_count:32;
        for(unsigned j=0;j<count;j++){const uint8_t* p=(const uint8_t*)r->vertices[j].p;for(unsigned k=0;k<12;k++)hash=(hash^p[k])*16777619u;}a->pose_hash=hash;break;
    }
}
uint32_t* wx_avatar_texture(WxAvatar* a,const WxEntry* e,uint32_t* original){
    if(e->texture_offset==a->header.body_texture)return a->body;
    if(a->hair&&e->texture_offset==a->bindings.hair_texture)return a->hair;
    if(a->extra&&e->texture_offset==a->bindings.extra_texture)return a->extra;
    if((e->id>>8)/100==15)return a->cape;return original;
}
