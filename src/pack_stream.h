/* Included by pack.c. One unpublished job per scene; no temporary bulk buffer.
   The synchronous verifier drains the same checked stages. */
static void stream_cancel(WxScene* s){
    if(!s->stream_job.phase)return;
    unload(s,&s->slots[s->stream_job.slot]);memset(&s->stream_job,0,sizeof s->stream_job);
    s->streaming.pending_bytes=0;s->streaming.cancelled++;
}
static int stream_fail(WxScene* s){
    int entry=s->stream_job.entry;stream_cancel(s);s->streaming.cancelled--;s->failures++;
    snprintf(s->error,sizeof s->error,"Asset %d read/allocation/validation failure",entry);return -1;
}
static int stream_allocate(WxScene* s){
    WxStreamJob* j=&s->stream_job;WxResident* r=&s->slots[j->slot];const WxEntry* e=&s->entries[j->entry];
    unsigned vb=e->vertex_count*sizeof(WxVertex),ib=e->index_count*2,tb=wx_texture_bytes(e),source=source_of(s,j->entry);
    unsigned ab=r->animation.frames*r->animation.bones*12*sizeof(float),sb=ab?e->vertex_count*sizeof(WxSkinVertex):0;
    unsigned levels=e->reserved[1]?e->reserved[1]:1;int shared=-1,empty=-1,shared_pose=-1,empty_pose=-1;
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++){WxTexture* t=&s->textures[i];
        if(!t->references){if(empty<0)empty=(int)i;continue;}
        if(t->source==source&&t->offset==e->texture_offset&&t->bytes==tb&&t->width==e->width&&t->height==e->height&&t->levels==levels&&t->encoding==(e->flags&WX_TEXTURE_ENCODING)){shared=(int)i;break;}}
    if(ab)for(unsigned i=0;i<WX_CACHE_SLOTS;i++){WxPoseBuffer* p=&s->poses[i];
        if(!p->references){if(empty_pose<0)empty_pose=(int)i;continue;}
        if(p->source==source&&p->offset==r->animation.poses_offset&&p->bytes==ab){shared_pose=(int)i;break;}}
    unsigned mb=(e->flags&WX_MATERIAL_MOTION)?sizeof(WxMaterialMotion):0;
    unsigned cb=wx_vertex_color_bytes(e);
    unsigned own=vb+ib+(ab?vb+sb:0)+mb+cb,total=own+(shared<0?tb:0)+(shared_pose<0?ab:0);
    if(total>s->budget_bytes||s->bytes>s->budget_bytes-total||wx_free_memory()<HEADROOM+total+65536)return 0;
    // Reserve before allocating; rollback uses the same accounting even when
    // only a prefix of the allocations succeeds. Pending bytes count in budget.
    r->bytes=own;s->bytes+=own;s->streaming.pending_bytes=total;
    r->vertices=wx_gpu_alloc(vb);r->indices=malloc(ib);
    if(!r->vertices||!r->indices)return 0;
    if(mb){r->material_motion=malloc(mb);if(!r->material_motion)return 0;}
    if(cb){r->vertex_colors=wx_gpu_alloc(cb);if(!r->vertex_colors)return 0;}
    if(shared<0){
        if(empty<0)return 0;uint32_t* pixels=wx_gpu_alloc(tb);if(!pixels)return 0;
        shared=empty;s->textures[shared]=(WxTexture){pixels,e->texture_offset,tb,0,e->width,e->height,levels,e->flags&WX_TEXTURE_ENCODING,source};s->bytes+=tb;j->new_texture=1;
    }
    r->texture_slot=shared;r->texture=s->textures[shared].pixels;s->textures[shared].references++;
    if(ab){
        r->bind_vertices=malloc(vb);r->skin=malloc(sb);if(!r->bind_vertices||!r->skin)return 0;
        if(shared_pose<0){
            if(empty_pose<0)return 0;float* poses=malloc(ab);if(!poses)return 0;
            shared_pose=empty_pose;s->poses[shared_pose]=(WxPoseBuffer){poses,source,r->animation.poses_offset,ab,0};s->bytes+=ab;j->new_pose=1;
        }
        r->pose_slot=shared_pose;r->poses=s->poses[shared_pose].data;s->poses[shared_pose].references++;
    }
    if(s->bytes>s->peak_bytes)s->peak_bytes=s->bytes;return 1;
}
static void stream_advance(WxStreamJob* j){j->phase++;j->offset=0;}
// Result: -1 rejected/rolled back, 0 quota exhausted, 1 committed, 2 progressed.
static int stream_step(WxScene* s,unsigned bytes,unsigned reads,unsigned scans,unsigned lane){
    if(!stream_budget_time_ready(lane))return 0;
    WxStreamJob* j=&s->stream_job;WxResident* r=&s->slots[j->slot];const WxEntry* e=&s->entries[j->entry];
    unsigned vb=e->vertex_count*sizeof(WxVertex),ib=e->index_count*2,ab=r->animation.frames*r->animation.bones*12*sizeof(float);
    if(j->phase==1){
        if(e->flags&WX_ANIMATED){
            if(s->streaming.read_ops>=reads||bytes-s->streaming.read_bytes<sizeof r->animation||stream_budget_read_room(lane,sizeof r->animation)<sizeof r->animation)return 0;
            stream_budget_read(lane,sizeof r->animation);
            s->streaming.read_ops++;s->streaming.read_bytes+=sizeof r->animation;
            if(!read_at(s,j->entry,e->reserved[0],&r->animation,sizeof r->animation))return stream_fail(s);
            const WxAnimation* a=&r->animation;unsigned source=source_of(s,j->entry),length=s->sources[source].bytes;
            if(a->frames<2||a->frames>120||!a->bones||a->bones>256||!a->duration_ms||a->duration_ms>60000||
               !range(a->skin_offset,e->vertex_count,sizeof(WxSkinVertex),length)||!range(a->poses_offset,a->frames*a->bones,48,length))return stream_fail(s);
        }
        stream_advance(j);return 2;
    }
    if(j->phase==2){if(!stream_budget_allocate(lane))return 0;if(!stream_allocate(s))return stream_fail(s);stream_advance(j);return 2;}
    if(j->phase>=3&&j->phase<=7){
        unsigned size=0,at=0;void* destination=NULL;
        if(j->phase==3&&j->new_texture){size=wx_texture_bytes(e);at=e->texture_offset;destination=r->texture;}
        if(j->phase==4){size=vb;at=e->vertex_offset;destination=r->vertices;}
        if(j->phase==5){size=ib;at=e->index_offset;destination=r->indices;}
        if(j->phase==6&&ab){size=e->vertex_count*sizeof(WxSkinVertex);at=r->animation.skin_offset;destination=r->skin;}
        if(j->phase==7&&j->new_pose){size=ab;at=r->animation.poses_offset;destination=r->poses;}
        if(j->offset==size){stream_advance(j);return 2;}
        if(s->streaming.read_bytes==bytes||s->streaming.read_ops==reads)return 0;
        unsigned n=size-j->offset;if(n>bytes-s->streaming.read_bytes)n=bytes-s->streaming.read_bytes;if(n>WX_STREAM_CHUNK_BYTES)n=WX_STREAM_CHUNK_BYTES;
        n=stream_budget_read_room(lane,n);if(!n)return 0;stream_budget_read(lane,n);
        s->streaming.read_bytes+=n;s->streaming.read_ops++;
        if(!read_at(s,j->entry,at+j->offset,(uint8_t*)destination+j->offset,n))return stream_fail(s);
        j->offset+=n;return 2;
    }
    if(j->phase>=8&&j->phase<=11){
        unsigned size=j->phase==8?ib:j->phase==9?vb:j->phase==10&&ab?e->vertex_count*sizeof(WxSkinVertex):j->phase==11&&j->new_pose?ab:0;
        if(j->offset==size){stream_advance(j);return 2;}
        unsigned unit=j->phase==8?2:j->phase==10?sizeof(WxSkinVertex):4,n=size-j->offset;
        if(n>scans-s->streaming.scan_bytes)n=scans-s->streaming.scan_bytes;n=stream_budget_scan_room(lane,n);n-=n%unit;if(!n)return 0;
        stream_budget_scan(lane,n);
        s->streaming.scan_bytes+=n;
        if(j->phase==8){for(unsigned i=j->offset/2;i<(j->offset+n)/2;i++)if(r->indices[i]>=e->vertex_count)return stream_fail(s);}
        else if(j->phase==9){
            const float* values=(const float*)r->vertices;
            for(unsigned i=j->offset/4;i<(j->offset+n)/4;i++)if(!isfinite(values[i]))return stream_fail(s);
            if(ab)memcpy((uint8_t*)r->bind_vertices+j->offset,(uint8_t*)r->vertices+j->offset,n);
        }else if(j->phase==10){
            for(unsigned i=j->offset/unit;i<(j->offset+n)/unit;i++){unsigned sum=0;
                for(unsigned k=0;k<4;k++){sum+=r->skin[i].weights[k];if(r->skin[i].weights[k]&&r->skin[i].bones[k]>=r->animation.bones)return stream_fail(s);}
                if(!sum)return stream_fail(s);}
        }else for(unsigned i=j->offset/4;i<(j->offset+n)/4;i++)if(!isfinite(r->poses[i]))return stream_fail(s);
        j->offset+=n;return 2;
    }
    if(j->phase==12){
        if(r->material_motion){
            unsigned size=sizeof(WxMaterialMotion);if(j->offset<size){
                if(s->streaming.read_ops==reads)return 0;
                unsigned n=size-j->offset;if(n>bytes-s->streaming.read_bytes)n=bytes-s->streaming.read_bytes;
                n=stream_budget_read_room(lane,n);if(!n)return 0;stream_budget_read(lane,n);
                s->streaming.read_ops++;s->streaming.read_bytes+=n;
                if(!read_at(s,j->entry,e->vertex_offset-size+j->offset,(uint8_t*)r->material_motion+j->offset,n))return stream_fail(s);
                j->offset+=n;return 2;
            }
        }stream_advance(j);return 2;
    }
    if(j->phase==13){
        if(r->material_motion){unsigned n=sizeof(WxMaterialMotion);
            if(n>scans-s->streaming.scan_bytes||stream_budget_scan_room(lane,n)<n)return 0;
            stream_budget_scan(lane,n);s->streaming.scan_bytes+=n;if(!wx_motion_valid(r->material_motion))return stream_fail(s);
        }stream_advance(j);return 2;
    }
    if(j->phase==14){
        unsigned size=wx_vertex_color_bytes(e);
        if(j->offset<size){
            if(s->streaming.read_ops==reads)return 0;
            unsigned n=size-j->offset;if(n>bytes-s->streaming.read_bytes)n=bytes-s->streaming.read_bytes;
            if(n>WX_STREAM_CHUNK_BYTES)n=WX_STREAM_CHUNK_BYTES;
            n=stream_budget_read_room(lane,n);if(!n)return 0;stream_budget_read(lane,n);
            s->streaming.read_ops++;s->streaming.read_bytes+=n;
            unsigned at=e->vertex_offset-size-((e->flags&WX_MATERIAL_MOTION)?sizeof(WxMaterialMotion):0);
            if(!read_at(s,j->entry,at+j->offset,(uint8_t*)r->vertex_colors+j->offset,n))return stream_fail(s);
            j->offset+=n;return 2;
        }stream_advance(j);return 2;
    }
    r->entry=j->entry;memset(j,0,sizeof *j);s->loads++;s->streaming.completed++;s->streaming.pending_bytes=0;return 1;
}
static int load(WxScene* s,WxResident* r,int id){
    stream_cancel(s);s->stream_job=(WxStreamJob){id,(int)(r-s->slots),1,0,0,0};
    // Offline verifier only. Runtime world/actor/avatar paths use step() with
    // per-call quotas and never drain an entire large asset synchronously.
    for(;;){s->streaming.read_bytes=s->streaming.read_ops=s->streaming.scan_bytes=0;
        int result=stream_step(s,WX_STREAM_READ_BYTES,WX_STREAM_READ_OPS,WX_STREAM_SCAN_BYTES,WX_STREAM_WORLD);
        if(result<0)return 0;if(result==1)return 1;}
}
