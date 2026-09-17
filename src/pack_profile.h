/* Included by pack.c. Shared by synchronous tools and staged native profiles. */
void wx_pack_open_cancel(WxPackOpen* p){
    if(p->file)fclose(p->file);free(p->entries);free(p->animations);memset(p,0,sizeof *p);
}
int wx_pack_open_begin(WxPackOpen* p,const char* path,unsigned lane){
    wx_pack_open_cancel(p);
    if(!path||strlen(path)>=sizeof p->path||lane>=WX_STREAM_LANES)return 0;
    strcpy(p->path,path);p->lane=lane;p->phase=1;p->contiguous=1;return 1;
}
static int profile_read(WxPackOpen* p,unsigned at,void* data,unsigned size){
    unsigned n=size-p->offset;if(n>32768)n=32768;
    if(n>65536-p->read_bytes)n=65536-p->read_bytes;if(p->read_ops==8)return 0;
    n=wx_stream_read_grant(p->lane,n);if(!n)return 0;
    if(!p->file){p->file=fopen(p->path,"rb");if(!p->file)return -1;}
    p->read_bytes+=n;p->read_ops++;
    if(fseek(p->file,at+p->offset,SEEK_SET)||fread((uint8_t*)data+p->offset,1,n,p->file)!=n)return -1;
    p->offset+=n;return p->offset==size?1:0;
}
static int profile_scan(WxPackOpen* p){
    if(p->scan_bytes+sizeof(WxEntry)>65536||!wx_stream_scan_grant(p->lane,sizeof(WxEntry)))return 0;
    p->scan_bytes+=sizeof(WxEntry);return 1;
}
static int animation_valid(const WxAnimation* a,const WxEntry* e,const WxPackHeader* h){
    unsigned start=sizeof *h+h->count*sizeof(WxEntry);
    return a->frames>=2&&a->frames<=120&&a->bones&&a->bones<=256&&a->duration_ms&&a->duration_ms<=60000&&
        a->skin_offset>=start&&a->poses_offset>=start&&range(a->skin_offset,e->vertex_count,sizeof(WxSkinVertex),h->file_size)&&range(a->poses_offset,a->frames*a->bones,48,h->file_size);
}
int wx_pack_open_pump(WxScene* s,WxPackOpen* p){
    if(!p->phase||s->file||s->entries)return -1;
    p->read_bytes=p->read_ops=p->scan_bytes=0;WxPackHeader* h=&p->header;
    if(p->phase==1){
        int r=profile_read(p,0,h,sizeof *h);if(r<0)goto bad;if(!r)return 0;
        if(memcmp(h->magic,"WXP1",4)||(h->version<4||h->version>WX_PACK_VERSION)||!h->count||h->count>WX_MAX_ENTRIES||h->entry_size!=sizeof(WxEntry)||
           h->file_size>=1024u*1024u*1024u||!range(sizeof *h,h->count,sizeof(WxEntry),h->file_size)||
           fseek(p->file,0,SEEK_END)||ftell(p->file)!=(long)h->file_size)goto bad;
        for(unsigned i=0;i<3;i++)if(!isfinite(h->spawn[i]))goto bad;
        p->offset=0;p->phase=2;
    }
    if(p->phase==2){
        unsigned bytes=h->count*sizeof(WxEntry);
        if(!p->entries){
            if(wx_free_memory()<HEADROOM+bytes+65536)goto bad;
            if(!wx_stream_allocation_grant(p->lane))return 0;
            p->entries=malloc(bytes);if(!p->entries)goto bad;
        }
        int r=profile_read(p,sizeof *h,p->entries,bytes);if(r<0)goto bad;if(!r)return 0;
        p->offset=0;p->phase=3;
    }
    if(p->phase==3){
        while(p->checked<h->count){
            if(!profile_scan(p))return 0;const WxEntry* e=p->entries+p->checked++;
            if(!entry_valid(e,h))goto bad;
            if(e->flags&WX_ANIMATED){
                unsigned at=e->reserved[0];if(!p->animation_count)p->animation_first=at;
                if(at<sizeof *h+h->count*sizeof(WxEntry)||!range(at,1,sizeof(WxAnimation),h->file_size))goto bad;
                if(at<p->animation_first||at-p->animation_first!=p->animation_count*sizeof(WxAnimation))p->contiguous=0;
                p->animation_count++;
            }
        }
        if(p->animation_count*sizeof(WxAnimation)>65536)p->contiguous=0;
        p->phase=4;p->checked=0;
    }
    if(p->phase==4){
        if(p->contiguous&&p->animation_count){
            unsigned bytes=p->animation_count*sizeof(WxAnimation);
            if(!p->animations){
                if(wx_free_memory()<HEADROOM+bytes+65536)goto bad;
                if(!wx_stream_allocation_grant(p->lane))return 0;
                p->animations=malloc(bytes);if(!p->animations)goto bad;
            }
            unsigned before=p->read_ops;int r=profile_read(p,p->animation_first,p->animations,bytes);
            p->animation_reads+=p->read_ops-before;if(r<0)goto bad;if(!r)return 0;
            p->offset=0;
        }p->phase=5;
    }
    if(p->phase==5){
        while(p->checked<h->count){
            const WxEntry* e=p->entries+p->checked;
            if(p->scan_bytes+sizeof(WxEntry)>65536||stream_budget_scan_room(p->lane,sizeof(WxEntry))<sizeof(WxEntry))return 0;
            if(e->flags&WX_ANIMATED){
                WxAnimation a;
                if(p->animations)a=p->animations[p->animation_at];
                else{
                    // A complete small header is one read. Never retain a partial
                    // header on the stack between calls.
                    if(p->read_ops==8||p->read_bytes+sizeof a>65536||stream_budget_read_room(p->lane,sizeof a)<sizeof a)return 0;
                    stream_budget_read(p->lane,sizeof a);p->read_bytes+=sizeof a;p->read_ops++;p->animation_reads++;
                    if(fseek(p->file,e->reserved[0],SEEK_SET)||fread(&a,1,sizeof a,p->file)!=sizeof a)goto bad;
                }
                if(!animation_valid(&a,e,h))goto bad;p->animation_at++;
            }
            if(!profile_scan(p))return 0;p->checked++;
        }
        s->file=p->file;s->header=*h;s->entries=p->entries;s->index_bytes=h->count*sizeof(WxEntry);
        index_changed(s);
        s->sources[0]=(WxPackSource){p->file,0,h->count,h->file_size,h->version};s->animation_index_reads=p->animation_reads;
        p->entries=NULL;p->file=NULL;free(p->animations);p->animations=NULL;p->phase=0;return 1;
    }return 0;
bad:wx_pack_open_cancel(p);snprintf(s->error,sizeof s->error,"Invalid/unavailable profile pack");return -1;
}
