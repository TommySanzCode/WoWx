#include "wx_icons.h"
#include "wx_runtime.h"
#include <stdlib.h>
#include <string.h>
#define ATLAS_BYTES (256u*256u*4u)
static unsigned interleave(unsigned x,unsigned y){unsigned n=0;for(unsigned i=0;i<3;i++)n|=((x>>i)&1)<<(2*i)|((y>>i)&1)<<(2*i+1);return n;}
void wx_icons_close(WxIcons* c){
    if(!c)return;if(c->file)fclose(c->file);free(c->index);wx_gpu_free(c->pixels);
    c->file=NULL;c->index=NULL;c->pixels=NULL;c->count=c->images=c->ready=c->bytes=c->pending=0;
}
int wx_icons_open(WxIcons* c,const char* path){
    if(!c||!path)return 0;memset(c,0,sizeof *c);c->file=fopen(path,"rb");uint32_t h[6];if(!c->file)goto bad;
    if(fread(h,1,sizeof h,c->file)!=sizeof h||h[0]!=0x43495857||h[1]!=1||!h[2]||h[2]>WX_ICON_INDEX_LIMIT||
       !h[3]||h[3]>WX_ICON_IMAGE_LIMIT||h[4]!=24+h[2]*8||h[5]!=h[4]+h[3]*4096)goto bad;
    if(fseek(c->file,0,SEEK_END)||ftell(c->file)!=(long)h[5]||fseek(c->file,24,SEEK_SET))goto bad;
    c->bytes=ATLAS_BYTES+h[2]*8;if(wx_free_memory()<8u*1024u*1024u+c->bytes+65536)goto bad;
    c->index=malloc(h[2]*8);c->pixels=wx_gpu_alloc(ATLAS_BYTES);if(!c->index||!c->pixels)goto bad;
    if(fread(c->index,8,h[2],c->file)!=h[2])goto bad;
    for(unsigned i=0;i<h[2];i++){
        uint32_t key=c->index[i].key;if(c->index[i].image>=h[3]||(i&&key<=c->index[i-1].key))goto bad;
        if(key&WX_ICON_ITEM){if(!(key&~WX_ICON_ITEM)||(key&~WX_ICON_ITEM)>0xffffff)goto bad;}
        else if(key>65535)goto bad;
    }
    if(c->index[0].key||c->index[0].image)goto bad; // original question-mark fallback
    memset(c->pixels,0,ATLAS_BYTES);for(unsigned i=0;i<WX_ICON_SLOTS;i++)c->slots[i].image=UINT32_MAX;
    c->count=h[2];c->images=h[3];c->offset=h[4];c->ready=1;return 1;
bad:wx_icons_close(c);c->failures++;return 0;
}
static unsigned lookup(const WxIcons* c,uint32_t key){
    unsigned lo=0,hi=c->count;while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(c->index[mid].key<key)lo=mid+1;else hi=mid;}
    return lo<c->count&&c->index[lo].key==key?c->index[lo].image:0;
}
int wx_icons_slot(const WxIcons* c,uint32_t key){
    if(!c||!c->ready||!key)return -1;unsigned image=lookup(c,key);
    for(unsigned i=0;i<WX_ICON_SLOTS;i++)if(c->slots[i].image==image)return (int)i;return -1;
}
void wx_icons_update(WxIcons* c,const uint32_t* keys,unsigned count){
    if(!c||!c->ready||count>WX_ICON_REQUESTS||(!keys&&count))return;
    c->drawn=0;
    if(!++c->serial){c->serial=1;for(unsigned i=0;i<WX_ICON_SLOTS;i++)c->slots[i].used=0;}
    unsigned wanted[WX_ICON_REQUESTS],needed=0;c->pending=0;
    for(unsigned i=0;i<count;i++)if(keys[i]){
        unsigned image=lookup(c,keys[i]),j=0;while(j<needed&&wanted[j]!=image)j++;if(j==needed)wanted[needed++]=image;
        for(j=0;j<WX_ICON_SLOTS;j++)if(c->slots[j].image==image)c->slots[j].used=c->serial;
    }
    unsigned next=UINT32_MAX;
    for(unsigned i=0;i<needed;i++){unsigned j=0;while(j<WX_ICON_SLOTS&&c->slots[j].image!=wanted[i])j++;
        if(j==WX_ICON_SLOTS){c->pending++;if(next==UINT32_MAX)next=wanted[i];}}
    if(next==UINT32_MAX)return;
    unsigned slot=WX_ICON_SLOTS,age=0;
    for(unsigned i=0;i<WX_ICON_SLOTS;i++)if(c->slots[i].used!=c->serial){unsigned old=c->serial-c->slots[i].used;
        if(slot==WX_ICON_SLOTS||old>age||c->slots[i].image==UINT32_MAX){slot=i;age=old;if(c->slots[i].image==UINT32_MAX)break;}}
    if(slot==WX_ICON_SLOTS){c->failures++;return;}
    uint32_t pixels[1024];
    if(fseek(c->file,c->offset+next*4096,SEEK_SET)||fread(pixels,1,sizeof pixels,c->file)!=sizeof pixels){c->failures++;return;}
    memcpy(c->pixels+interleave(slot%8,slot/8)*1024,pixels,sizeof pixels);
    c->slots[slot]=(WxIconSlot){next,c->serial};c->loads++;c->pending--;
}
int wx_icons_image(WxIcons* c,WxUi* ui,uint32_t key,float x,float y,float size,uint32_t color){
    if(!ui||!ui->ready)return 0;
    int slot=wx_icons_slot(c,key);if(slot<0)return 0;
    float tx=(slot%8)*32+.5f,ty=(slot/8)*32+.5f;
    unsigned before=ui->quads;wx_ui_texture(ui,c->pixels,256,x,y,size,size,tx/256,ty/256,(tx+31)/256,(ty+31)/256,color);
    if(ui->quads==before)return 0;c->drawn++;return 1;
}
uint32_t wx_action_icon(const WxGame* g,uint32_t binding){
    unsigned id=binding&0xffffff,type=binding>>24;if(!id)return 0;
    if(type==0)return id<=65535?id:0;
    if(type==0x80&&g){const WxItemName* item=wx_item_info(g,id);if(item&&item->metadata==1&&item->display&&item->display<=0xffffff)return WX_ICON_ITEM|item->display;}
    return 0;
}
