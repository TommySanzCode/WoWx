/* Source-owned environment residency is independent of drawable cache slots.
   Publication is atomic. The world lane pays for reads, scans and allocation. */
static void environment_release(WxScene* s,WxPackSource* source){
    free(source->environment.data);s->bytes-=source->environment.bytes;
    memset(&source->environment,0,sizeof source->environment);
}
static int environment_scan(WxScene* s,unsigned bytes){
    if(bytes>WX_STREAM_SCAN_BYTES-s->streaming.scan_bytes||!wx_stream_scan_grant(WX_STREAM_WORLD,bytes))return 0;
    s->streaming.scan_bytes+=bytes;return 1;
}
static int environment_pump_source(WxScene* s,WxPackSource* source){
    WxEnvironmentCache* c=&source->environment;WxEnvironmentFooter* f=&c->footer;
    if(source->version<9||c->phase==5)return 1;if(c->phase==6)return -1;
    if(!c->phase){
        if(source->bytes<sizeof *f)goto bad;
        // Fixed headers must never be partially consumed across frames.
        if(stream_budget_read_room(WX_STREAM_WORLD,sizeof *f)<sizeof *f||s->streaming.read_ops==WX_STREAM_READ_OPS)return 0;
        if(wx_stream_read_grant(WX_STREAM_WORLD,sizeof *f)!=sizeof *f)return 0;
        s->streaming.read_bytes+=sizeof *f;s->streaming.read_ops++;
        if(fseek(source->file,source->bytes-sizeof *f,SEEK_SET)||fread(f,1,sizeof *f,source->file)!=sizeof *f||
           !wx_environment_footer_valid(f,source->bytes,32+source->count*sizeof(WxEntry)))goto bad;
        c->phase=1;c->next=f->count*sizeof(WxEnvironmentRecord);
    }
    if(c->phase==1){
        // Geometry must not address metadata bytes. v9 is a world-pack format;
        // character/atlas packs continue to use v8 with their existing offsets.
        while(c->checked<source->count){
            if(!environment_scan(s,sizeof(WxEntry)))return 0;
            const WxEntry* e=s->entries+source->first+c->checked;
            if((e->flags&WX_ANIMATED)||!range(e->vertex_offset,e->vertex_count,sizeof(WxVertex),f->offset)||
               !range(e->index_offset,e->index_count,2,f->offset)||!range(e->texture_offset,wx_texture_bytes(e),1,f->offset))goto bad;
            c->checked++;
        }c->phase=2;
    }
    if(c->phase==2){
        if(f->bytes){
            if(f->bytes>s->budget_bytes||s->bytes>s->budget_bytes-f->bytes||wx_free_memory()<HEADROOM+f->bytes+65536)goto bad;
            if(!wx_stream_allocation_grant(WX_STREAM_WORLD))return 0;
            c->data=malloc(f->bytes);if(!c->data)goto bad;c->bytes=f->bytes;s->bytes+=f->bytes;
            if(s->bytes>s->peak_bytes)s->peak_bytes=s->bytes;
        }c->phase=3;
    }
    if(c->phase==3){
        if(c->offset<f->bytes){
            unsigned n=f->bytes-c->offset;if(n>16384)n=16384;
            if(n>WX_STREAM_READ_BYTES-s->streaming.read_bytes)n=WX_STREAM_READ_BYTES-s->streaming.read_bytes;
            if(s->streaming.read_ops==WX_STREAM_READ_OPS||!n)return 0;
            n=wx_stream_read_grant(WX_STREAM_WORLD,n);if(!n)return 0;
            s->streaming.read_bytes+=n;s->streaming.read_ops++;
            if(fseek(source->file,f->offset+c->offset,SEEK_SET)||fread((uint8_t*)c->data+c->offset,1,n,source->file)!=n)goto bad;
            c->offset+=n;if(c->offset<f->bytes)return 0;
        }c->phase=4;
    }
    if(c->phase==4){
        while(c->record<f->count){
            const WxEnvironmentRecord* r=(const WxEnvironmentRecord*)c->data+c->record;
            if(!c->heights){
                if(!environment_scan(s,sizeof *r))return 0;
                if(!wx_environment_record_valid(r,f->bytes,c->next))goto bad;
                c->heights=1;
            }
            if(r->kind==WX_ENV_LIQUID){
                unsigned n=(r->x_tiles+1)*(r->y_tiles+1);const float* heights=(const float*)((const uint8_t*)c->data+r->heights_offset);
                while(c->heights<=n){
                    unsigned at=c->heights-1,count=n-at;if(count>256)count=256;
                    if(!environment_scan(s,count*4))return 0;
                    for(unsigned k=0;k<count;k++)if(!isfinite(heights[at+k])||fabsf(heights[at+k])>100000)goto bad;
                    c->heights+=count;
                }
                unsigned tiles=r->x_tiles*r->y_tiles,padded=(tiles+3)&~3u;
                if(!environment_scan(s,4))return 0;
                const uint8_t* flags=(const uint8_t*)c->data+r->flags_offset;
                for(unsigned k=tiles;k<padded;k++)if(flags[k])goto bad;
                c->next=r->flags_offset+padded;
            }
            c->record++;c->heights=0;
        }
        if(c->next!=f->bytes)goto bad;c->phase=5;
    }return 1;
bad:environment_release(s,source);c->phase=6;s->failures++;
    snprintf(s->error,sizeof s->error,"Invalid/unavailable WMO environment metadata");return -1;
}
int wx_environment_ready(const WxScene* s){
    for(unsigned i=0;i<WX_REGION_FILES;i++)if(s->sources[i].file&&s->sources[i].version>=9&&s->sources[i].environment.phase!=5)return 0;
    return 1;
}
static int environment_pump(WxScene* s){
    for(unsigned i=0;i<WX_REGION_FILES;i++)if(s->sources[i].file){
        int result=environment_pump_source(s,s->sources+i);if(result!=1)return result;
    }return 1;
}
void wx_environment_sample(const WxScene* s,const float* position,WxEnvironmentSample* out){
    if(!out)return;memset(out,0,sizeof *out);out->source=UINT32_MAX;
    if(!s||!position)return;for(unsigned k=0;k<3;k++)if(!isfinite(position[k]))return;
    unsigned files=0;out->known=1;
    for(unsigned i=0;i<WX_REGION_FILES;i++)if(s->sources[i].file){files++;
        if(s->sources[i].version<9||s->sources[i].environment.phase!=5)out->known=0;}
    if(!files)out->known=0;if(!out->known)return;
    float smallest=INFINITY,water_depth=INFINITY;unsigned liquid_group=0,liquid_source=UINT32_MAX;
    for(unsigned i=0;i<WX_REGION_FILES;i++){
        const WxEnvironmentCache* c=&s->sources[i].environment;
        for(unsigned j=0;j<c->footer.count;j++){
            const WxEnvironmentRecord* r=(const WxEnvironmentRecord*)c->data+j;float p[3];out->records++;
            for(unsigned k=0;k<3;k++)p[k]=r->inverse[k*4]*position[0]+r->inverse[k*4+1]*position[1]+r->inverse[k*4+2]*position[2]+r->inverse[k*4+3];
            if(p[0]<r->lo[0]||p[0]>r->hi[0]||p[1]<r->lo[1]||p[1]>r->hi[1]||p[2]<r->lo[2]||p[2]>r->hi[2])continue;
            if(r->kind==WX_ENV_GROUP&&(r->flags&0x2000)){
                float volume=(r->hi[0]-r->lo[0])*(r->hi[1]-r->lo[1])*(r->hi[2]-r->lo[2]);
                if(volume<smallest){smallest=volume;out->environment=1;out->group=r->group;out->source=i;}
            }else if(r->kind==WX_ENV_LIQUID){
                float x=(p[0]-r->corner[0])/r->tile_size,y=(p[1]-r->corner[1])/r->tile_size;
                if(x<0||y<0||x>=r->x_tiles||y>=r->y_tiles)continue;
                unsigned ix=(unsigned)x,iy=(unsigned)y,w=r->x_tiles+1;
                const uint8_t* flags=(const uint8_t*)c->data+r->flags_offset;
                if((flags[iy*r->x_tiles+ix]&15)==15)continue;
                const float* h=(const float*)((const uint8_t*)c->data+r->heights_offset)+iy*w+ix;
                x-=ix;y-=iy;float surface=x>=y?h[0]*(1-x)+h[1]*(x-y)+h[w+1]*y:h[0]*(1-y)+h[w]*(y-x)+h[w+1]*x;
                float depth=surface-p[2];
                // Retain the resolved liquid ID for diagnostics/gameplay. Only
                // known water/ocean uses the water palette; magma/slime stay
                // explicitly unsupported rather than acquiring blue water fog.
                if(depth>0&&depth<water_depth){water_depth=depth;out->liquid_type=r->liquid_type;out->depth=depth;
                    liquid_group=r->group;liquid_source=i;}
            }
        }
    }
    if(out->depth>0&&(out->liquid_type==1||out->liquid_type==2)){out->environment=2;out->group=liquid_group;out->source=liquid_source;}
}
