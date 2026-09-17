/* Bounded terrain-index preparation; unpublished entries never reach selection. */
void wx_pack_attach_cancel(WxPackPending* p){
    if(p->file)fclose(p->file);free(p->entries);free(p->joined);memset(p,0,sizeof *p);
}
unsigned wx_pack_pending_bytes(const WxPackPending* p){
    return p?(p->entries?p->header.count*sizeof(WxEntry):0)+(p->joined?p->join_total:0):0;
}
static int index_publish(WxScene* s,WxPackPending* p,unsigned slot,WxEntry* entries,unsigned bytes){
    WxEntry* old=s->entries;s->entries=entries;s->index_bytes=bytes;
    s->sources[slot]=(WxPackSource){p->file,s->header.count,p->header.count,p->header.file_size,p->header.version};
    s->file=p->file;s->header.count+=p->header.count;s->wanted_ready=0;s->error[0]=0;
    if(entries==p->entries)p->entries=NULL;p->joined=NULL;p->file=NULL;
    free(old);free(p->entries);p->entries=NULL;p->phase=0;index_changed(s);return (int)slot+1;
}
int wx_pack_attach_begin(WxPackPending* p,const char* path,unsigned expected_bytes){
    wx_pack_attach_cancel(p);stream_budget_enter(WX_STREAM_INDEX);
    if(!wx_stream_index_ready()){p->deferred=1;return 0;}
    p->file=fopen(path,"rb");if(!p->file)return 0;
    WxPackHeader* h=&p->header;
    stream_budget_read(WX_STREAM_INDEX,sizeof *h);
    if(fread(h,1,sizeof *h,p->file)!=sizeof *h||memcmp(h->magic,"WXP1",4)||(h->version<4||h->version>WX_PACK_VERSION)||
       !h->count||h->count>WX_MAX_ENTRIES||h->entry_size!=sizeof(WxEntry)||h->file_size!=expected_bytes||h->file_size>=1024u*1024u*1024u||
       !range(sizeof *h,h->count,sizeof(WxEntry),h->file_size))goto fail;
    for(unsigned i=0;i<3;i++)if(!isfinite(h->spawn[i]))goto fail;
    if(fseek(p->file,0,SEEK_END)||ftell(p->file)!=(long)h->file_size||fseek(p->file,sizeof *h,SEEK_SET))goto fail;
    if(wx_free_memory()<HEADROOM+h->count*sizeof(WxEntry)+65536)goto fail;
    p->entries=malloc(h->count*sizeof(WxEntry));if(!p->entries)goto fail;
    p->phase=1;p->read_bytes=sizeof *h;p->read_ops=1;return 1;
fail:wx_pack_attach_cancel(p);return 0;
}
int wx_pack_attach_pump(WxScene* s,WxPackPending* p){
    if(!p->phase)return -1;
    stream_budget_enter(WX_STREAM_INDEX);
    p->read_bytes=p->read_ops=p->checked_frame=0;
    // Existing sources can detach, close or be replaced while a join is being
    // copied. Discard only the unpublished join; the validated new index stays.
    if(p->phase>=4&&(p->join_scene!=s||p->join_revision!=s->index_revision)){
        free(p->joined);p->joined=NULL;p->join_total=p->join_progress=0;p->join_restarts++;p->phase=3;return 0;
    }
    if(p->phase==1){
        unsigned size=p->header.count*sizeof(WxEntry),n=size-p->offset;
        if(n>WX_STREAM_CHUNK_BYTES)n=WX_STREAM_CHUNK_BYTES;
        n=stream_budget_read_room(WX_STREAM_INDEX,n);if(!n)return 0;
        stream_budget_read(WX_STREAM_INDEX,n);
        if(fseek(p->file,sizeof(WxPackHeader)+p->offset,SEEK_SET)||fread((uint8_t*)p->entries+p->offset,1,n,p->file)!=n)goto fail;
        p->offset+=n;p->read_bytes=n;p->read_ops=1;
        if(p->offset<size)return 0;p->phase=2;
    }
    while(p->checked<p->header.count&&p->checked_frame<512){
        if(stream_budget_scan_room(WX_STREAM_INDEX,sizeof(WxEntry))<sizeof(WxEntry))return 0;
        const WxEntry* e=&p->entries[p->checked];if(!entry_valid(e,&p->header))goto fail;
        if(e->flags&WX_ANIMATED){
            WxAnimation a;unsigned data_start=sizeof(WxPackHeader)+p->header.count*sizeof(WxEntry);
            if(p->read_ops==WX_STREAM_READ_OPS||p->read_bytes+sizeof a>WX_STREAM_CHUNK_BYTES||stream_budget_read_room(WX_STREAM_INDEX,sizeof a)<sizeof a)return 0;
            stream_budget_read(WX_STREAM_INDEX,sizeof a);
            if(!range(e->reserved[0],1,sizeof a,p->header.file_size)||e->reserved[0]<data_start||
               fseek(p->file,e->reserved[0],SEEK_SET)||fread(&a,1,sizeof a,p->file)!=sizeof a)goto fail;
            p->read_ops++;p->read_bytes+=sizeof a;
            if(a.frames<2||a.frames>120||!a.bones||a.bones>256||!a.duration_ms||a.duration_ms>60000||a.skin_offset<data_start||a.poses_offset<data_start||
               !range(a.skin_offset,e->vertex_count,sizeof(WxSkinVertex),p->header.file_size)||!range(a.poses_offset,a.frames*a.bones,48,p->header.file_size))goto fail;
        }
        stream_budget_scan(WX_STREAM_INDEX,sizeof(WxEntry));p->checked++;p->checked_frame++;
    }
    if(p->checked<p->header.count)return 0;
    if(p->phase==2){p->phase=3;if(s->header.count)return 0;}
    if(!stream_budget_time_ready(WX_STREAM_INDEX))return 0;
    unsigned slot=0;while(slot<WX_REGION_FILES&&s->sources[slot].file)slot++;
    unsigned count=s->header.count+p->header.count,bytes=count*sizeof(WxEntry);
    if(slot==WX_REGION_FILES||count>WX_REGION_FILES*WX_MAX_ENTRIES)goto fail;
    if(p->phase==3){
        // A first source already owns its complete table: transfer it directly.
        if(!s->header.count)return index_publish(s,p,slot,p->entries,bytes);
        if(wx_free_memory()<HEADROOM+bytes+65536)goto fail;
        p->joined=malloc(bytes);if(!p->joined)goto fail;
        p->join_scene=s;p->join_revision=s->index_revision;p->join_count=s->header.count;
        p->join_total=bytes;p->join_progress=0;p->phase=4;return 0;
    }
    if(p->phase==4){
        unsigned copied=0,old_bytes=p->join_count*sizeof(WxEntry);
        while(p->join_progress<p->join_total&&copied<WX_STREAM_FRAME_COPY_BYTES){
            unsigned at=p->join_progress,end=at<old_bytes?old_bytes:p->join_total;
            unsigned n=end-at;if(n>WX_STREAM_FRAME_COPY_BYTES-copied)n=WX_STREAM_FRAME_COPY_BYTES-copied;
            n=stream_budget_index_copy(n);if(!n)return 0;
            const uint8_t* from=at<old_bytes?(const uint8_t*)s->entries+at:(const uint8_t*)p->entries+at-old_bytes;
            memcpy((uint8_t*)p->joined+at,from,n);p->join_progress+=n;copied+=n;
        }
        if(p->join_progress==p->join_total)p->phase=5;
        return 0; // Publication never shares a frame with the final copy batch.
    }
    if(p->phase==5){
        if(frame_active&&frame_copy_bytes)return 0;
        return index_publish(s,p,slot,p->joined,p->join_total);
    }
    goto fail;
fail:wx_pack_attach_cancel(p);snprintf(s->error,sizeof s->error,"Invalid/unavailable terrain index");return -1;
}
