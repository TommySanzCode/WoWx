/* Included by avatar.c. No previous profile is kept during a profile switch. */
unsigned wx_avatar_pending_bytes(const WxAvatar* a){
    const WxPackOpen* p=&a->opening.pack;
    return (p->entries?p->header.count*sizeof(WxEntry):0)+(p->animations?p->animation_count*sizeof(WxAnimation):0);
}
int wx_avatar_open_begin(WxAvatar* a,const char* pack,const char* metadata){
    memset(a,0,sizeof *a);wx_scene_init(&a->scene);a->complete_clip=UINT32_MAX;
    if(!metadata||strlen(metadata)>=sizeof a->opening.metadata||!wx_pack_open_begin(&a->opening.pack,pack,WX_STREAM_AVATAR))goto bad;
    strcpy(a->opening.metadata,metadata);a->opening.phase=1;return 1;
bad:a->failures=1;snprintf(a->error,sizeof a->error,"Invalid avatar profile path");return 0;
}
static int avatar_open_read(WxAvatarOpen* p,FILE** file,const char* path,unsigned start,void* destination,unsigned bytes){
    unsigned n=bytes-p->offset;if(n>32768)n=32768;if(n>65536-p->read_bytes)n=65536-p->read_bytes;
    if(p->read_ops==8)return 0;n=wx_stream_read_grant(WX_STREAM_AVATAR,n);if(!n)return 0;
    p->read_bytes+=n;p->read_ops++;
    if(!*file){*file=fopen(path,"rb");if(!*file)return -1;}
    if(fseek(*file,start+p->offset,SEEK_SET)||fread((uint8_t*)destination+p->offset,1,n,*file)!=n)return -1;
    p->offset+=n;return p->offset==bytes?1:0;
}
static int avatar_open_scan(WxAvatarOpen* p,unsigned n){
    if(n>65536-p->scan_bytes||!wx_stream_scan_grant(WX_STREAM_AVATAR,n))return 0;p->scan_bytes+=n;return 1;
}
static void avatar_open_next(WxAvatarOpen* p,unsigned phase){p->phase=phase;p->offset=p->checked=0;}
int wx_avatar_open_pump(WxAvatar* a){
    WxAvatarOpen* p=&a->opening;WxAvatarHeader* h=&a->header;
    p->read_bytes=p->read_ops=p->scan_bytes=0;
    if(!p->phase)return a->profile_ready?1:-1;
    if(p->phase==1){
        int r=wx_pack_open_pump(&a->scene,&p->pack);p->read_bytes=p->pack.read_bytes;p->read_ops=p->pack.read_ops;p->scan_bytes=p->pack.scan_bytes;
        if(r<0)goto bad;if(!r)return 0;a->scene.budget_bytes=4u*1024u*1024u;a->bytes=a->scene.index_bytes;avatar_open_next(p,2);return 0;
    }
    if(p->phase==2){
        int r=avatar_open_read(p,&a->file,p->metadata,0,h,sizeof *h);if(r<0)goto bad;if(!r)return 0;
        if(memcmp(h->magic,"WXAV",4)||(h->version!=1&&h->version!=2)||h->file_size>32u*1024u*1024u||h->item_count>WX_AVATAR_ITEMS||h->item_size!=sizeof(WxAvatarItem)||
           h->items_offset!=sizeof *h+(h->version==2?sizeof(WxAvatarBindings):0)||h->pack_size!=a->scene.header.file_size||
           fseek(a->file,0,SEEK_END)||ftell(a->file)!=(long)h->file_size||!h->look[0]||h->look[0]>8||h->look[1]>1||h->scalp>99||
           !span(h,h->base_offset,WX_AVATAR_ATLAS_BYTES))goto bad;
        for(unsigned i=2;i<7;i++)if(h->look[i]>255)goto bad;
        for(unsigned i=0;i<3;i++)if(h->facial[i]>99)goto bad;
        avatar_open_next(p,h->version==2?3:6);
    }
    if(p->phase==3){
        int r=avatar_open_read(p,&a->file,p->metadata,sizeof *h,&a->bindings,sizeof a->bindings);if(r<0)goto bad;if(!r)return 0;
        if(a->bindings.hair_texture==h->body_texture||a->bindings.extra_texture==h->body_texture||
           (a->bindings.hair_texture&&a->bindings.hair_texture==a->bindings.extra_texture))goto bad;
        avatar_open_next(p,4);
    }
    if(p->phase==4){
        char path[256];const char* slash=strrchr(p->metadata,'/'),*back=strrchr(p->metadata,'\\');if(back&&(!slash||back>slash))slash=back;
        unsigned prefix=slash?(unsigned)(slash-p->metadata+1):0;if(prefix>sizeof path-20)goto bad;
        memcpy(path,p->metadata,prefix);snprintf(path+prefix,sizeof path-prefix,"L%02X%02X.WXL",h->look[0],h->look[1]);
        int r=avatar_open_read(p,&p->looks_file,path,0,&a->looks.header,sizeof(WxLookHeader));if(r<0)goto bad;if(!r)return 0;
        if(!wx_looks_header_valid(&a->looks.header)||fseek(p->looks_file,0,SEEK_END)||ftell(p->looks_file)!=(long)a->looks.header.file_size)goto bad;
        avatar_open_next(p,5);
    }
    if(p->phase==5){
        unsigned bytes=a->looks.header.count*sizeof(WxLookRow);
        if(p->offset<bytes){int r=avatar_open_read(p,&p->looks_file,NULL,sizeof(WxLookHeader),a->looks.rows,bytes);if(r<0)goto bad;if(!r)return 0;}
        unsigned checked=0;
        while(p->checked<a->looks.header.count&&checked++<32){
            if(!avatar_open_scan(p,sizeof(WxLookRow)+p->checked*4))return 0;
            if(!wx_looks_row_valid(&a->looks,p->checked++))goto bad;
        }
        if(p->checked<a->looks.header.count)return 0;
        a->looks.file=p->looks_file;p->looks_file=NULL;
        if(!wx_looks_valid(&a->looks,h->look))goto bad;avatar_open_next(p,6);
    }
    if(p->phase==6){
        while(p->checked<a->scene.header.count){
            if(!avatar_open_scan(p,sizeof(WxEntry)))return 0;
            const WxEntry* e=a->scene.entries+p->checked++;unsigned family=e->id>>8;
            if(e->kind!=WX_KIND_CHARACTER||!(e->flags&WX_ANIMATED)||family>=4096+2*h->item_count)goto bad;
            if(!(e->id&255))a->family_bits[family/32]|=1u<<(family%32);
            if(e->texture_offset==h->body_texture||e->texture_offset==a->bindings.hair_texture||e->texture_offset==a->bindings.extra_texture){
                if(e->width!=128||e->height!=128||e->reserved[1]!=8||(e->flags&(WX_TEX_SWIZZLED|WX_MIPMAPPED))!=(WX_TEX_SWIZZLED|WX_MIPMAPPED))goto bad;
                p->binding+=e->texture_offset==h->body_texture;p->hair_binding+=e->texture_offset==a->bindings.hair_texture;p->extra_binding+=e->texture_offset==a->bindings.extra_texture;
            }
        }
        if(!p->binding||!has(a,0)||(a->bindings.hair_texture&&!p->hair_binding)||(a->bindings.extra_texture&&!p->extra_binding))goto bad;
        avatar_open_next(p,7);
    }
    if(p->phase==7){
        unsigned table=h->item_count*sizeof(WxAvatarItem);
        unsigned bytes=table+a->scene.index_bytes+WX_AVATAR_ATLAS_BYTES+8192+2*WX_AVATAR_MIP_BYTES+sizeof a->family_bits;
        if(h->version==2)bytes+=sizeof(WxLooks)+(!!p->hair_binding+!!p->extra_binding)*WX_AVATAR_MIP_BYTES;
        if(wx_free_memory()<8u*1024u*1024u+bytes+65536)goto bad;
        if(!wx_stream_allocation_grant(WX_STREAM_AVATAR))return 0;
        a->bytes=bytes;a->items=calloc(h->item_count?h->item_count:1,sizeof(WxAvatarItem));a->canvas=malloc(WX_AVATAR_ATLAS_BYTES);a->scratch=malloc(8192);
        a->body=wx_gpu_alloc(WX_AVATAR_MIP_BYTES);a->cape=wx_gpu_alloc(WX_AVATAR_MIP_BYTES);
        if(p->hair_binding)a->hair=wx_gpu_alloc(WX_AVATAR_MIP_BYTES);if(p->extra_binding)a->extra=wx_gpu_alloc(WX_AVATAR_MIP_BYTES);
        if(!a->items||!a->canvas||!a->scratch||!a->body||!a->cape||(p->hair_binding&&!a->hair)||(p->extra_binding&&!a->extra))goto bad;
        avatar_open_next(p,8);
    }
    if(p->phase==8){
        unsigned bytes=h->item_count*sizeof(WxAvatarItem);
        if(p->offset<bytes){int r=avatar_open_read(p,&a->file,p->metadata,h->items_offset,a->items,bytes);if(r<0)goto bad;if(!r)return 0;}
        while(p->checked<h->item_count){
            unsigned i=p->checked;if(!avatar_open_scan(p,sizeof(WxAvatarItem)+i*4))return 0;
            const WxAvatarItem* item=a->items+i;if(!item->display||item->slot>=19)goto bad;
            for(unsigned k=0;k<i;k++)if(a->items[k].display==item->display&&a->items[k].slot==item->slot)goto bad;
            for(unsigned k=0;k<3;k++)if(item->geoset[k]>98)goto bad;
            for(unsigned k=0;k<2;k++)if(item->component[k]&&(item->component[k]!=4096+2*i+k||!has(a,item->component[k])))goto bad;
            for(unsigned k=0;k<8;k++)if(item->overlay[k]&&!span(h,item->overlay[k],regions[k][2]*regions[k][3]*4))goto bad;
            if(item->cape&&(item->slot!=14||!span(h,item->cape,WX_AVATAR_MIP_BYTES)))goto bad;p->checked++;
        }
        p->phase=0;a->profile_ready=1;return 1;
    }return 0;
bad:wx_avatar_close(a);a->failures=1;snprintf(a->error,sizeof a->error,"Invalid/unavailable avatar profile");return -1;
}
