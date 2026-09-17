/* Production pack code with a counted/failable CPU allocator. No GPU timing. */
#include "wx_runtime.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
static unsigned checks,objects,allocations;static size_t used,peak;
static size_t limit=48u*1024u*1024u;static int fail_after=-1;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Index FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef union Block {max_align_t alignment;struct {size_t bytes;unsigned magic;} value;} Block;
static void* counted_malloc(size_t bytes){
    if(fail_after==0)return NULL;if(fail_after>0)fail_after--;
    if(bytes>limit||used>limit-bytes||sizeof(Block)>limit-used-bytes)return NULL;
    Block* b=malloc(sizeof *b+bytes);if(!b)return NULL;b->value.bytes=bytes;b->value.magic=0x5749584d;
    used+=sizeof *b+bytes;if(used>peak)peak=used;objects++;allocations++;return b+1;
}
static void counted_free(void* p){if(p){Block* b=(Block*)p-1;CHECK(b->value.magic==0x5749584d);
    used-=sizeof *b+b->value.bytes;objects--;b->value.magic=0;free(b);}}
static void* counted_realloc(void* p,size_t bytes){
    if(!p)return counted_malloc(bytes);if(!bytes){counted_free(p);return NULL;}
    Block* b=(Block*)p-1;void* out=counted_malloc(bytes);if(!out)return NULL;
    memcpy(out,p,bytes<b->value.bytes?bytes:b->value.bytes);counted_free(p);return out;
}
unsigned wx_free_memory(void){return (unsigned)(limit-used);}
void* wx_gpu_alloc(unsigned n){return counted_malloc(n);}
void wx_gpu_free(void* p){counted_free(p);}
#define malloc counted_malloc
#define free counted_free
#define realloc counted_realloc
#include "../src/pack.c"
#undef malloc
#undef free
#undef realloc

#define ENTRIES 4096u
static unsigned fixture(const char* path,unsigned first){
    WxPackHeader h={{'W','X','P','1'},8,ENTRIES,sizeof(WxEntry),{0},32+ENTRIES*64+106};
    FILE* f=fopen(path,"wb");CHECK(f);CHECK(fwrite(&h,1,sizeof h,f)==sizeof h);
    WxEntry e={0};e.kind=WX_KIND_TERRAIN;e.vertex_count=e.index_count=3;e.width=e.height=1;e.radius=2;
    e.vertex_offset=32+ENTRIES*64;e.index_offset=e.vertex_offset+96;e.texture_offset=e.index_offset+6;
    for(unsigned i=0;i<ENTRIES;i++){e.id=first+i;CHECK(fwrite(&e,1,sizeof e,f)==sizeof e);}
    WxVertex v[3]={0};v[1].p[0]=1;v[2].p[1]=1;uint16_t indices[3]={0,1,2};uint32_t pixel=0xffffffff;
    CHECK(fwrite(v,1,sizeof v,f)==sizeof v);CHECK(fwrite(indices,1,sizeof indices,f)==sizeof indices);
    CHECK(fwrite(&pixel,1,4,f)==4);CHECK(!fclose(f));return h.file_size;
}
static int tick(WxScene* s,WxPackPending* p){
    wx_stream_frame_begin(1);int result=wx_pack_attach_pump(s,p);
    CHECK(wx_stream_index_copy_bytes()<=WX_STREAM_FRAME_COPY_BYTES);
    CHECK(p->read_bytes<=32768&&p->checked_frame<=512);
    CHECK(wx_pack_pending_bytes(p)==(p->entries?p->header.count*64:0)+(p->joined?p->join_total:0));
    wx_stream_frame_end();return result;
}
static void reach(WxScene* s,WxPackPending* p,unsigned phase){
    unsigned frames=0;while(p->phase!=phase&&frames++<150)CHECK(tick(s,p)==0);
    CHECK(p->phase==phase&&frames<150);
}
static void finish(WxScene* s,WxPackPending* p){
    int result=0;unsigned frames=0;while(!result&&frames++<150)result=tick(s,p);
    CHECK(result>0&&frames<150&&!p->phase&&!p->entries&&!p->joined&&!p->file&&!wx_pack_pending_bytes(p));
}
static void first(WxScene* s,unsigned bytes){
    wx_scene_init(s);WxPackPending p={0};CHECK(wx_pack_attach_begin(&p,"index-a.wxp",bytes));
    WxEntry* transferred=p.entries;unsigned before=allocations;finish(s,&p);
    CHECK(s->entries==transferred&&allocations==before&&s->header.count==ENTRIES);
}
static void contents(const WxScene* s,unsigned group,unsigned first_id){
    for(unsigned i=0;i<ENTRIES;i++)CHECK(s->entries[group*ENTRIES+i].id==first_id+i);
}
static void publication(unsigned bytes){
    WxScene s;first(&s,bytes);WxEntry* old=s.entries;unsigned revision=s.index_revision;
    WxPackPending p={0};CHECK(wx_pack_attach_begin(&p,"index-b.wxp",bytes));reach(&s,&p,4);
    CHECK(s.entries==old&&s.header.count==ENTRIES&&s.index_revision==revision);
    CHECK(p.join_total==2*ENTRIES*64&&wx_pack_pending_bytes(&p)==3*ENTRIES*64);
    wx_stream_frame_begin(1);CHECK(!wx_pack_attach_pump(&s,&p));unsigned at=p.join_progress;
    CHECK(at==WX_STREAM_FRAME_COPY_BYTES&&p.phase==4);
    for(unsigned i=0;i<4;i++)CHECK(!wx_pack_attach_pump(&s,&p)&&p.join_progress==at);
    CHECK(s.entries==old&&s.header.count==ENTRIES);wx_stream_frame_end();
    wx_stream_frame_begin(1);CHECK(!wx_pack_attach_pump(&s,&p)&&p.phase==5);
    CHECK(!wx_pack_attach_pump(&s,&p)&&s.entries==old);wx_stream_frame_end();
    CHECK(tick(&s,&p)==2&&s.entries!=old&&s.header.count==2*ENTRIES&&s.index_revision!=revision);
    contents(&s,0,10000);contents(&s,1,20000);wx_pack_close(&s);wx_pack_attach_cancel(&p);CHECK(!used&&!objects);
}
static void cancellation(unsigned bytes){
    for(unsigned phase=3;phase<=5;phase++){
        WxScene s;first(&s,bytes);WxEntry* old=s.entries;size_t before=used;
        WxPackPending p={0};CHECK(wx_pack_attach_begin(&p,"index-b.wxp",bytes));reach(&s,&p,phase);
        if(phase==4)CHECK(!tick(&s,&p)&&p.join_progress>0&&p.join_progress<p.join_total);
        wx_pack_attach_cancel(&p);CHECK(used==before&&s.entries==old&&s.header.count==ENTRIES);
        CHECK(!p.phase&&!p.joined&&!p.entries&&!p.file&&!wx_pack_pending_bytes(&p));contents(&s,0,10000);
        wx_pack_close(&s);CHECK(!used&&!objects);
    }
}
static void mutation(unsigned bytes){
    for(unsigned mode=0;mode<3;mode++){
        WxScene s;first(&s,bytes);WxPackPending second={0};CHECK(wx_pack_attach_begin(&second,"index-b.wxp",bytes));finish(&s,&second);
        WxPackPending p={0};CHECK(wx_pack_attach_begin(&p,"index-c.wxp",bytes));reach(&s,&p,4);CHECK(!tick(&s,&p)&&p.join_progress>0);
        if(mode==0)wx_pack_detach(&s,0);
        else{wx_pack_close(&s);wx_scene_init(&s);if(mode==2){WxPackPending replacement={0};CHECK(wx_pack_attach_begin(&replacement,"index-b.wxp",bytes));finish(&s,&replacement);}}
        CHECK(!tick(&s,&p)&&p.phase==3&&!p.joined&&!p.join_total&&!p.join_progress&&p.join_restarts==1);
        finish(&s,&p);CHECK(s.header.count==(mode==1?1:2)*ENTRIES);
        if(mode!=1)contents(&s,0,20000);contents(&s,mode==1?0:1,30000);
        wx_pack_close(&s);wx_pack_attach_cancel(&p);CHECK(!used&&!objects);
    }
}
static void failures(unsigned bytes){
    for(unsigned mode=0;mode<2;mode++){
        WxScene s;first(&s,bytes);WxEntry* old=s.entries;size_t before=used;
        WxPackPending p={0};CHECK(wx_pack_attach_begin(&p,"index-b.wxp",bytes));reach(&s,&p,3);
        if(mode==0)fail_after=0;else limit=used+8u*1024u*1024u+2*ENTRIES*64; // Missing safety margin.
        CHECK(tick(&s,&p)==-1);fail_after=-1;limit=48u*1024u*1024u;
        CHECK(s.entries==old&&s.header.count==ENTRIES&&used==before&&!p.entries&&!p.joined&&!p.file);
        contents(&s,0,10000);wx_pack_close(&s);CHECK(!used&&!objects);
    }
}
int main(void){
    unsigned bytes=fixture("index-a.wxp",10000);CHECK(fixture("index-b.wxp",20000)==bytes);CHECK(fixture("index-c.wxp",30000)==bytes);
    publication(bytes);cancellation(bytes);mutation(bytes);failures(bytes);
    remove("index-a.wxp");remove("index-b.wxp");remove("index-c.wxp");
    printf("Staged index publication: %u checks; CPU/GPU allocator peak %zu bytes; no leaks\n",checks,peak);return 0;
}
