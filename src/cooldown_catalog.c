#include "wx_cooldown.h"
#include "wx_runtime.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int wx_cooldown_catalog_open(WxCooldownCatalog* c,const char* path){
    if(!c||!path)return 0;
    memset(c,0,sizeof *c);FILE* f=fopen(path,"rb");if(!f){c->failures=1;return 0;}
    uint32_t h[4];
    if(fread(h,1,16,f)!=16||h[0]!=0x44435857||(h[1]!=1&&h[1]!=2)||!h[2]||h[2]>65535||h[3]!=(h[1]==1?20:sizeof(WxCooldownInfo)))goto bad;
    if(fseek(f,0,SEEK_END)||ftell(f)!=(long)(16+h[2]*h[3]))goto bad;
    c->bytes=h[2]*sizeof(WxCooldownInfo);
    if(wx_free_memory()<8u*1024u*1024u+c->bytes+65536u||(c->rows=calloc(h[2],sizeof(WxCooldownInfo)))==NULL)goto bad;
    if(fseek(f,16,SEEK_SET))goto bad;
    if(h[1]==2){if(fread(c->rows,1,c->bytes,f)!=c->bytes)goto bad;}
    else for(unsigned i=0;i<h[2];i++)if(fread(c->rows+i,1,20,f)!=20)goto bad;
    for(unsigned i=0;i<h[2];i++){
        const WxCooldownInfo* r=&c->rows[i];
        if(!r->spell||r->spell>65535||(i&&r->spell<=c->rows[i-1].spell)||r->category>65535||r->recovery>0x7fffffffu||r->category_recovery>0x7fffffffu||
           r->gcd_category>65535||r->gcd_time>0x7fffffffu||r->family>65535||r->damage_class>3)goto bad;
    }
    fclose(f);c->count=h[2];c->version=h[1];c->ready=1;return 1;
bad:fclose(f);wx_cooldown_catalog_close(c);c->failures=1;return 0;
}
void wx_cooldown_catalog_close(WxCooldownCatalog* c){if(c){free(c->rows);memset(c,0,sizeof *c);}}
