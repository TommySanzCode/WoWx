#include "wx_outfit.h"
#include <stdio.h>
#include <string.h>
_Static_assert(sizeof(WxOutfit)==100&&sizeof(WxOutfitHeader)==16,"Outfit file ABI");
int wx_outfit_slot(unsigned type){
    static const int8_t slots[]={-1,0,1,2,3,4,5,6,7,8,9,10,12,15,16,17,14,15,-1,18,4,15,16,16,-1,17,17,-1,17};
    return type<sizeof slots?slots[type]:-1;
}
int wx_outfit_valid(const WxOutfit* o){
    unsigned race=o->key&255,cl=(o->key>>8)&255,sex=o->key>>16;
    if(!race||race>8||!cl||cl>11||sex>1||o->type[19])return 0;
    for(unsigned i=0;i<19;i++)if(o->display[i]?wx_outfit_slot(o->type[i])!=(int)i:o->type[i]!=0)return 0;
    return 1;
}
int wx_outfits_open(WxOutfits* out,const char* path){
    memset(out,0,sizeof *out);FILE* f=fopen(path,"rb");if(!f)return 0;WxOutfitHeader h;
    int ok=fread(&h,1,sizeof h,f)==sizeof h&&!memcmp(h.magic,"WXOF",4)&&h.version==1&&h.count&&h.count<=WX_OUTFIT_LIMIT&&h.entry_size==sizeof(WxOutfit);
    if(ok)ok=fread(out->rows,sizeof(WxOutfit),h.count,f)==h.count&&fgetc(f)==EOF;
    fclose(f);if(ok)for(unsigned i=0;i<h.count;i++){
        if(!wx_outfit_valid(out->rows+i)){ok=0;break;}
        for(unsigned j=0;j<i;j++)if(out->rows[i].key==out->rows[j].key)ok=0;
    }
    if(!ok){memset(out,0,sizeof *out);return 0;}out->count=h.count;out->ready=1;return 1;
}
const WxOutfit* wx_outfit_find(const WxOutfits* outfits,unsigned race,unsigned cl,unsigned gender){
    if(!outfits->ready||race<1||race>8||!cl||cl>11||gender>1)return NULL;
    unsigned key=race|(cl<<8)|(gender<<16);for(unsigned i=0;i<outfits->count;i++)if(outfits->rows[i].key==key)return outfits->rows+i;return NULL;
}
