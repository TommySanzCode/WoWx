/* Included by avatar.c. One unpublished texture set; no per-frame allocation. */
static int compose_key(const WxAvatarCompose* j,const WxAvatar* a,const WxWorldView* w){
    return !memcmp(j->look,a->header.look,sizeof j->look)&&!memcmp(j->equipment,w->equipment_display,sizeof j->equipment)&&j->flags==w->appearance_flags;
}
static int compose_layer(WxAvatarCompose* j,unsigned source,unsigned offset,unsigned bytes,unsigned region){
    if(j->layer_count==80)return 0;
    j->layers[j->layer_count++]=(WxAvatarLayer){offset,bytes,region,source};return 1;
}
static int compose_look_layer(WxAvatar* a,const WxLookRow* row,unsigned layer,unsigned region){
    return !row||!row->offset[layer]||compose_layer(&a->compose,1,row->offset[layer],wx_looks_bytes(row->region[layer]),region);
}
static int compose_plan(WxAvatar* a,const WxWorldView* world){
    WxAvatarCompose* j=&a->compose;
    if(j->phase&&j->phase!=8)j->cancelled++;
    j->phase=1;j->count=j->missing=j->layer_count=j->layer=j->offset=j->hash_at=j->copy_at=0;
    j->hash=2166136261u;j->mip_size=128;j->mip_base=j->mip_row=0;
    memcpy(j->look,a->header.look,sizeof j->look);memcpy(j->equipment,world->equipment_display,sizeof j->equipment);j->flags=world->appearance_flags;
    const WxAvatarItem* worn[19]={0};
    for(unsigned slot=0;slot<19;slot++)if(world->equipment_display[slot]){
        for(unsigned k=0;k<a->header.item_count;k++)if(a->items[k].display==world->equipment_display[slot]&&a->items[k].slot==slot){worn[slot]=a->items+k;break;}
        if(!worn[slot])j->missing++;
    }else if(world->equipment_entry[slot]&&!(world->equipment_ready_mask&(1u<<slot)))j->missing++;
    if(world->appearance_flags&0x400)worn[0]=NULL;if(world->appearance_flags&0x800)worn[14]=NULL;
    const WxLookRow *skin=NULL,*hair=NULL;
    if(a->header.version==1){if(!compose_layer(j,0,a->header.base_offset,WX_AVATAR_ATLAS_BYTES,10))return 0;}
    else{
        const uint32_t* v=j->look;skin=wx_looks_find(&a->looks,0,0,v[2]);hair=wx_looks_find(&a->looks,3,v[4],v[5]);
        if(!skin||!hair||!compose_look_layer(a,skin,0,10))return 0;
        const WxLookRow* layers[]={wx_looks_find(&a->looks,1,v[3],v[2]),wx_looks_find(&a->looks,4,0,v[2]),wx_looks_find(&a->looks,2,v[6],v[5]),hair};
        for(unsigned i=0;i<4;i++)if(layers[i])for(unsigned k=i==3?1:0;k<3;k++)
            if(!compose_look_layer(a,layers[i],k,layers[i]->region[k]))return 0;
    }
    static const unsigned order[]={3,6,4,18,5,8,9,7};
    for(unsigned i=0;i<8;i++)if(worn[order[i]])for(unsigned r=0;r<8;r++){
        unsigned offset=worn[order[i]]->overlay[r];if(offset&&!compose_layer(j,0,offset,regions[r][2]*regions[r][3]*4,r))return 0;
    }
    // Auxiliary mip chains are copied/read after the composed body's mips exist.
    if(!compose_layer(j,worn[14]&&worn[14]->cape?0:2,worn[14]?worn[14]->cape:0,WX_AVATAR_MIP_BYTES,11))return 0;
    if(a->hair&&!compose_layer(j,hair&&hair->offset[0]?1:3,hair?hair->offset[0]:0,WX_AVATAR_MIP_BYTES,12))return 0;
    if(a->extra&&!compose_layer(j,skin&&skin->offset[1]?1:3,skin?skin->offset[1]:0,WX_AVATAR_MIP_BYTES,13))return 0;
    unsigned robe=group(worn[4],2),sleeve=group(worn[4],0),shirt=group(worn[3],0);if(shirt>sleeve)sleeve=shirt;
    unsigned choices[]={a->header.scalp,100+a->header.facial[0],200+a->header.facial[1],300+a->header.facial[2],
        401+group(worn[9],0),robe?0:501+group(worn[7],0),702,801+sleeve,robe?0:902,
        worn[18]&&!robe?1201+group(worn[18],0):0,1301+(robe?robe:group(worn[6],0)),worn[14]?1502:1501,
        j->look[0]==4?1701:0,2002,2001};
    if(!add(j,0))return 0;
    for(unsigned i=0;i<sizeof choices/sizeof *choices;i++){
        if(i>=1&&i<=3&&!a->header.facial[i-1])continue;
        unsigned id=choices[i];if(!id)continue;id=resolve(a,id);if(!id)continue;
        unsigned g=id/100,v=id%100,column=g<=3?g:g==7?4:99;
        if(worn[0]&&column<5&&v<32&&(worn[0]->hide[column]&(1u<<v)))continue;if(!add(j,id))return 0;
    }
    for(unsigned i=0;i<19;i++)if(worn[i])for(unsigned k=0;k<2;k++)if(worn[i]->component[k]&&!add(j,worn[i]->component[k]))return 0;
    // Every profile validates a complete idle sequence. Hold this request fixed
    // while appearances change; live locomotion resumes after atomic publication.
    for(unsigned i=0;i<j->count;i++)j->ids[i]=j->families[i]<<8;
    return 1;
}
static int compose_buffers(WxAvatar* a){
    WxAvatarCompose* j=&a->compose;if(j->body)return 1;
    unsigned bytes=(2+!!a->hair+!!a->extra)*WX_AVATAR_MIP_BYTES;
    if(wx_free_memory()<8u*1024u*1024u+bytes+65536)return 0;
    if(!wx_stream_allocation_grant(WX_STREAM_AVATAR))return 0;
    j->body=wx_gpu_alloc(WX_AVATAR_MIP_BYTES);j->cape=wx_gpu_alloc(WX_AVATAR_MIP_BYTES);
    if(a->hair)j->hair=wx_gpu_alloc(WX_AVATAR_MIP_BYTES);if(a->extra)j->extra=wx_gpu_alloc(WX_AVATAR_MIP_BYTES);
    if(!j->body||!j->cape||(a->hair&&!j->hair)||(a->extra&&!j->extra)){
        wx_gpu_free(j->body);wx_gpu_free(j->cape);wx_gpu_free(j->hair);wx_gpu_free(j->extra);j->body=j->cape=j->hair=j->extra=NULL;return -1;
    }a->bytes+=bytes;return 1;
}
static int compose_read(WxAvatar* a,const WxAvatarLayer* l,uint8_t* destination){
    WxAvatarCompose* j=&a->compose;unsigned remain=l->bytes-j->offset;
    unsigned wanted=remain<32768?remain:32768;
    if(wanted>65536-j->read_bytes)wanted=65536-j->read_bytes;
    if(j->read_ops==8)return 0;
    unsigned n=wx_stream_read_grant(WX_STREAM_AVATAR,wanted);if(!n)return 0;
    FILE* file=l->source?a->looks.file:a->file;unsigned offset=l->offset+j->offset;
    unsigned first=l->source?a->looks.header.data_offset:a->header.items_offset+a->header.item_count*sizeof(WxAvatarItem);
    unsigned size=l->source?a->looks.header.file_size:a->header.file_size;
    j->read_bytes+=n;j->read_ops++;
    if(!file||offset<l->offset||offset<first||offset>size||n>size-offset||fseek(file,offset,SEEK_SET)||fread(destination+j->offset,1,n,file)!=n)return -1;
    j->offset+=n;return 1;
}
static unsigned morton(unsigned x,unsigned y,unsigned size){
    unsigned result=0;for(unsigned bit=0;(1u<<bit)<size;bit++)result|=((x>>bit)&1u)<<(bit*2)|((y>>bit)&1u)<<(bit*2+1);return result;
}
static int compose_pump(WxAvatar* a){
    WxAvatarCompose* j=&a->compose;
    for(unsigned step=0;step<256;step++){
        if(j->phase==1){int r=compose_buffers(a);if(r<=0)return r;j->phase=2;}
        if(j->phase==2){
            const WxAvatarLayer* l=j->layers+j->layer;
            if(j->layer==j->layer_count||l->region>=11){j->phase=3;continue;}
            if(j->offset<l->bytes){int r=compose_read(a,l,l->region==10?a->canvas:a->scratch);if(r<=0)return r;continue;}
            if(l->region<10){unsigned pixels=l->bytes/4;if(pixels>WX_AVATAR_COMPOSE_PIXELS-j->work_pixels)return 0;
                if(!wx_avatar_blend(a->canvas,a->scratch,l->region))return -1;j->work_pixels+=pixels;}
            j->offset=0;j->layer++;continue;
        }
        if(j->phase==3){
            unsigned n=WX_AVATAR_ATLAS_BYTES-j->hash_at,room=(WX_AVATAR_COMPOSE_PIXELS-j->work_pixels)*4;if(n>room)n=room;if(!n)return 0;
            for(unsigned i=0;i<n;i++)j->hash=(j->hash^a->canvas[j->hash_at++])*16777619u;j->work_pixels+=n/4;
            if(j->hash_at==WX_AVATAR_ATLAS_BYTES)j->phase=4;continue;
        }
        if(j->phase==4){
            unsigned size=j->mip_size;if(size>WX_AVATAR_COMPOSE_PIXELS-j->work_pixels)return 0;
            unsigned y=j->mip_row++;for(unsigned x=0;x<size;x++){
                const uint8_t* s=a->canvas+(y*size+x)*4;
                j->body[j->mip_base+morton(x,y,size)]=((uint32_t)s[3]<<24)|((uint32_t)s[0]<<16)|((uint32_t)s[1]<<8)|s[2];
            }j->work_pixels+=size;
            if(j->mip_row==size){j->mip_base+=size*size;j->mip_row=0;j->phase=size==1?6:5;}continue;
        }
        if(j->phase==5){
            unsigned size=j->mip_size,next=size/2;if(next*4>WX_AVATAR_COMPOSE_PIXELS-j->work_pixels)return 0;
            unsigned y=j->mip_row++;for(unsigned x=0;x<next;x++)for(unsigned c=0;c<4;c++){
                unsigned sum=0;for(unsigned iy=0;iy<2;iy++)for(unsigned ix=0;ix<2;ix++)sum+=a->canvas[((2*y+iy)*size+2*x+ix)*4+c];
                a->canvas[(y*next+x)*4+c]=(uint8_t)((sum+2)/4);
            }j->work_pixels+=next*4;
            if(j->mip_row==next){j->mip_size=next;j->mip_row=0;j->phase=4;}continue;
        }
        if(j->phase==6){
            if(j->layer==j->layer_count){j->phase=7;return 1;}
            const WxAvatarLayer* l=j->layers+j->layer;uint8_t* to=(uint8_t*)(l->region==11?j->cape:l->region==12?j->hair:j->extra);
            if(l->source<2){if(j->offset<l->bytes){int r=compose_read(a,l,to);if(r<=0)return r;continue;}}
            else{unsigned n=l->bytes-j->offset,room=(WX_AVATAR_COMPOSE_PIXELS-j->work_pixels)*4;if(n>room)n=room;if(!n)return 0;
                if(l->source==2)memset(to+j->offset,255,n);else memcpy(to+j->offset,(uint8_t*)j->body+j->offset,n);
                j->offset+=n;j->work_pixels+=n/4;if(j->offset<l->bytes)continue;
            }j->offset=0;j->layer++;continue;
        }
        return j->phase==7?1:0;
    }return 0;
}
static void compose_publish(WxAvatar* a){
    WxAvatarCompose* j=&a->compose;uint32_t* swap;
    swap=a->body;a->body=j->body;j->body=swap;swap=a->cape;a->cape=j->cape;j->cape=swap;
    swap=a->hair;a->hair=j->hair;j->hair=swap;swap=a->extra;a->extra=j->extra;j->extra=swap;
    memcpy(a->families,j->families,sizeof a->families);memcpy(a->ids,j->ids,sizeof a->ids);a->count=j->count;a->missing=j->missing;
    memcpy(a->equipment,j->equipment,sizeof a->equipment);memcpy(a->published_look,j->look,sizeof a->published_look);a->flags=j->flags;
    a->atlas_hash=j->hash;a->appearance_ready=1;a->complete_clip=0;a->revision++;j->commits++;j->phase=0;
}
