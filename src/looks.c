#include "wx_looks.h"
#include <string.h>
unsigned wx_looks_bytes(unsigned region){
    static const unsigned sizes[]={8192,8192,4096,8192,4096,8192,8192,4096,8192,4096,65536,87380};
    return region<12?sizes[region]:0;
}
void wx_looks_close(WxLooks* l){if(l->file)fclose(l->file);memset(l,0,sizeof *l);}
const WxLookRow* wx_looks_find(const WxLooks* l,unsigned kind,unsigned variation,unsigned color){
    if(!l||!l->file)return NULL;
    for(unsigned i=0;i<l->header.count;i++){const WxLookRow* r=l->rows+i;
        if(r->kind==kind&&r->variation==variation&&r->color==color)return r;}return NULL;
}
int wx_looks_header_valid(const WxLookHeader* h){
    return !memcmp(h->magic,"WXLK",4)&&h->version==1&&h->count&&h->count<=WX_LOOK_ROWS&&h->row_size==sizeof(WxLookRow)&&
        h->race>=1&&h->race<=8&&h->sex<=1&&h->data_offset==sizeof *h+h->count*sizeof(WxLookRow)&&h->file_size>=h->data_offset&&h->file_size<=64u*1024u*1024u;
}
int wx_looks_row_valid(const WxLooks* l,unsigned i){
    const WxLookHeader* h=&l->header;if(i>=h->count||i>=WX_LOOK_ROWS)return 0;const WxLookRow* r=l->rows+i;
        if(r->kind>6||r->reserved||((r->kind==0||r->kind==4)&&r->variation)||
           (r->kind>=5&&r->color))return 0;
        for(unsigned j=0;j<i;j++)if(l->rows[j].kind==r->kind&&l->rows[j].variation==r->variation&&l->rows[j].color==r->color)return 0;
        for(unsigned k=0;k<3;k++){
            unsigned bytes=wx_looks_bytes(r->region[k]);
            if(r->geoset[k]>99||(r->kind<5&&r->geoset[k])||(r->kind==5&&k&&r->geoset[k]))return 0;
            if(!r->offset[k]){if(r->region[k])return 0;continue;}
            if(r->kind>=5||!bytes||r->offset[k]<h->data_offset||r->offset[k]>h->file_size||bytes>h->file_size-r->offset[k])return 0;
            if(r->kind==0){if(k>1||r->region[k]!=(k?WX_LOOK_MIPS:WX_LOOK_BASE))return 0;}
            else if(r->kind==3&&k==0){if(r->region[k]!=WX_LOOK_MIPS)return 0;}
            else if(r->region[k]>9)return 0;
        }
        if(r->kind==0&&!r->offset[0])return 0;
    return 1;
}
int wx_looks_open(WxLooks* l,const char* path){
    memset(l,0,sizeof *l);l->file=fopen(path,"rb");if(!l->file)return 0;
    WxLookHeader* h=&l->header;
    if(fread(h,1,sizeof *h,l->file)!=sizeof *h||!wx_looks_header_valid(h)||fseek(l->file,0,SEEK_END)||ftell(l->file)!=(long)h->file_size||
       fseek(l->file,sizeof *h,SEEK_SET)||fread(l->rows,sizeof(WxLookRow),h->count,l->file)!=h->count)goto bad;
    for(unsigned i=0;i<h->count;i++)if(!wx_looks_row_valid(l,i))goto bad;
    return 1;
bad:wx_looks_close(l);return 0;
}
static int identity(const WxLooks* l,const uint32_t v[7]){
    if(!l||!l->file||!v||v[0]!=l->header.race||v[1]!=l->header.sex)return 0;
    for(unsigned i=2;i<7;i++)if(v[i]>255)return 0;return 1;
}
static int facial_required(const uint32_t v[7]){return v[0]!=6&&(!v[1]||v[0]==4||v[0]==5);}
int wx_looks_valid(const WxLooks* l,const uint32_t v[7]){
    return identity(l,v)&&wx_looks_find(l,0,0,v[2])&&wx_looks_find(l,1,v[3],v[2])&&
        wx_looks_find(l,3,v[4],v[5])&&wx_looks_find(l,6,v[6],0)&&
        (!facial_required(v)||wx_looks_find(l,2,v[6],v[5]));
}
unsigned wx_looks_choices(const WxLooks* l,const uint32_t v[7],unsigned field,uint8_t ids[256]){
    if(!ids||!identity(l,v)||field<2||field>6)return 0;
    uint8_t seen[256]={0};
    for(unsigned i=0;i<l->header.count;i++){const WxLookRow* r=l->rows+i;unsigned id=256;
        if(field==2&&r->kind==0)id=r->color;
        if(field==3&&r->kind==1&&r->color==v[2])id=r->variation;
        if(field==4&&r->kind==3)id=r->variation;
        if(field==5&&r->kind==3&&r->variation==v[4])id=r->color;
        if(field==6&&r->kind==6&&(!facial_required(v)||wx_looks_find(l,2,r->variation,v[5])))id=r->variation;
        if(id<256)seen[id]=1;
    }
    unsigned n=0;for(unsigned i=0;i<256;i++)if(seen[i])ids[n++]=(uint8_t)i;return n;
}
int wx_looks_step(const WxLooks* l,uint32_t v[7],unsigned field,int direction){
    if(!v||!direction||!wx_looks_valid(l,v))return 0;
    uint8_t ids[256];unsigned n=wx_looks_choices(l,v,field,ids),at=0;if(!n)return 0;
    while(at<n&&ids[at]!=v[field])at++;
    uint32_t next[7];memcpy(next,v,sizeof next);next[field]=ids[(at+n+(direction<0?-1:1))%n];
    // Skin changes face availability; style changes colors; color changes facial details.
    for(unsigned i=0;i<3;i++){unsigned dependent=i==0?3:i==1?5:6;
        n=wx_looks_choices(l,next,dependent,ids);if(!n)return 0;at=0;
        while(at<n&&ids[at]!=next[dependent])at++;if(at==n)next[dependent]=ids[0];
    }
    if(!wx_looks_valid(l,next))return 0;memcpy(v,next,sizeof next);return 1;
}
static uint32_t random_next(uint32_t* state){
    uint32_t x=*state?*state:0x6d2b79f5u;x^=x<<13;x^=x>>17;x^=x<<5;return *state=x;
}
int wx_looks_randomize(const WxLooks* l,uint32_t v[7],uint32_t* seed){
    if(!seed||!identity(l,v))return 0;
    uint32_t next[7],state=*seed;memcpy(next,v,sizeof next);
    uint8_t ids[256],colors[256]={0};unsigned count=0,n;
    // Choose a skin with at least one face. Sparse IDs are never range sampled.
    for(unsigned i=0;i<l->header.count;i++)if(l->rows[i].kind==0){
        uint32_t trial[7];memcpy(trial,next,sizeof trial);trial[2]=l->rows[i].color;
        if(wx_looks_choices(l,trial,3,ids)&&random_next(&state)%++count==0)next[2]=trial[2];
    }
    if(!count)return 0;
    n=wx_looks_choices(l,next,3,ids);next[3]=ids[random_next(&state)%n];
    // Precompute colors with facial choices, then sample valid style/color pairs.
    // This avoids random retries and the unavailable/unreachable section keys.
    if(!facial_required(next)){
        if(!wx_looks_choices(l,next,6,ids))return 0;memset(colors,1,sizeof colors);
    }else for(unsigned i=0;i<l->header.count;i++){
        const WxLookRow* r=l->rows+i;if(r->kind==2&&wx_looks_find(l,6,r->variation,0))colors[r->color]=1;
    }
    count=0;
    for(unsigned i=0;i<l->header.count;i++){
        const WxLookRow* r=l->rows+i;
        if(r->kind==3&&colors[r->color]&&random_next(&state)%++count==0){next[4]=r->variation;next[5]=r->color;}
    }
    if(!count)return 0;
    n=wx_looks_choices(l,next,6,ids);if(!n)return 0;next[6]=ids[random_next(&state)%n];
    if(!wx_looks_valid(l,next))return 0;
    // Make the button visibly change the look whenever the catalog permits it.
    if(!memcmp(next,v,sizeof next))for(unsigned field=2;field<7;field++){
        uint32_t trial[7];memcpy(trial,next,sizeof trial);
        if(wx_looks_step(l,trial,field,1)&&memcmp(trial,next,sizeof trial)){memcpy(next,trial,sizeof next);break;}
    }
    memcpy(v,next,sizeof next);*seed=state;return 1;
}
int wx_looks_read(WxLooks* l,const WxLookRow* row,unsigned layer,void* destination,unsigned capacity){
    if(!l||!l->file||!row||!destination||layer>=3||!row->offset[layer])return 0;
    unsigned bytes=wx_looks_bytes(row->region[layer]),offset=row->offset[layer];
    return bytes&&bytes<=capacity&&offset>=l->header.data_offset&&offset<=l->header.file_size&&bytes<=l->header.file_size-offset&&
        !fseek(l->file,offset,SEEK_SET)&&fread(destination,1,bytes,l->file)==bytes;
}
