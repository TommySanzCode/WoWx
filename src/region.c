#include "wx_region.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define TILE (1600.0f/3.0f)
static float distance_to_cell(const WxRegionCell* c,const float* p){
    if(c->reserved==WX_REGION_GLOBAL_WMO)return 0;
    float lo_x=(31-(int)c->y)*TILE,hi_x=(32-(int)c->y)*TILE;
    float lo_y=(31-(int)c->x)*TILE,hi_y=(32-(int)c->x)*TILE;
    float x=fmaxf(lo_x-p[0],fmaxf(0,p[0]-hi_x)),y=fmaxf(lo_y-p[1],fmaxf(0,p[1]-hi_y));return x*x+y*y;
}
int wx_region_cell_at(uint32_t map,const float* p,const WxRegionCell* c){
    if(!p||!c||!isfinite(p[0])||!isfinite(p[1])||fabsf(p[0])>18000||fabsf(p[1])>18000)return 0;
    if(c->reserved==WX_REGION_GLOBAL_WMO)return c->map==map;
    int x=(int)floorf(32-p[1]/TILE),y=(int)floorf(32-p[0]/TILE);
    return c->map==map&&x==c->x&&y==c->y;
}
int wx_region_open(WxRegion* r,const char* path){
    memset(r,0,sizeof *r);r->selected=r->pending_cell=-1;for(unsigned i=0;i<WX_REGION_FILES;i++)r->active[i]=r->wanted[i]=-1;
    if(!path||strlen(path)>=sizeof r->directory)return 0;
    FILE* file=fopen(path,"rb");if(!file)return 0;
    uint32_t header[4];int ok=fread(header,1,16,file)==16&&header[0]==0x31495857&&(header[1]==1||header[1]==2)&&header[2]>0&&header[2]<=8192&&header[3]==32;
    if(ok){r->count=header[2];r->cells=malloc(r->count*sizeof *r->cells);ok=r->cells&&fread(r->cells,sizeof *r->cells,r->count,file)==r->count&&fgetc(file)==EOF;}
    fclose(file);
    for(unsigned i=0;ok&&i<r->count;i++){
        WxRegionCell* c=&r->cells[i];char expected[16];
        if(c->reserved==WX_REGION_GLOBAL_WMO)snprintf(expected,sizeof expected,"G%03u.WXP",c->map);
        else snprintf(expected,sizeof expected,"M%03u%02u%02u.WXP",c->map,c->x,c->y);
        int kind=c->reserved==WX_REGION_TERRAIN||(header[1]==2&&c->reserved==WX_REGION_GLOBAL_WMO&&!c->x&&!c->y);
        ok=c->map<=999&&c->x<64&&c->y<64&&kind&&c->bytes>32&&c->bytes<1024u*1024u*1024u&&memchr(c->file,0,sizeof c->file)&&!strcmp(c->file,expected);
        if(i){const WxRegionCell* p=&r->cells[i-1];ok=ok&&(c->map>p->map||(c->map==p->map&&!c->reserved&&!p->reserved&&(c->x>p->x||(c->x==p->x&&c->y>p->y))));}
    }
    if(!ok){wx_region_close(r);snprintf(r->error,sizeof r->error,"Invalid world region index");return 0;}
    strcpy(r->directory,path);char* end=strrchr(r->directory,'/'),*back=strrchr(r->directory,'\\');if(back&&(!end||back>end))end=back;
    if(end)end[1]=0;else r->directory[0]=0;return 1;
}
void wx_region_close(WxRegion* r){wx_pack_attach_cancel(&r->pending);free(r->cells);r->cells=NULL;r->count=0;}
int wx_region_update(WxRegion* r,WxScene* s,uint32_t map,const float* p){
    r->pending.read_bytes=r->pending.read_ops=r->pending.checked_frame=0;
    if(!r->cells||!p||!isfinite(p[0])||!isfinite(p[1])||!isfinite(p[2])||fabsf(p[0])>18000||fabsf(p[1])>18000)return 0;
    float dx=p[0]-r->position[0],dy=p[1]-r->position[1];
    if(!r->position_ready||r->map!=map||dx*dx+dy*dy>=4){
        float scores[WX_REGION_FILES];for(unsigned i=0;i<WX_REGION_FILES;i++){scores[i]=1e20f;r->wanted[i]=-1;}
        r->selected=-1;
        for(unsigned i=0;i<r->count;i++)if(r->cells[i].map==map){
            if(wx_region_cell_at(map,p,&r->cells[i]))r->selected=(int)i;
            float distance=distance_to_cell(&r->cells[i],p);int retained=0;
            for(unsigned k=0;k<WX_REGION_FILES;k++)if(r->active[k]==(int)i)retained=1;
            if(distance>(retained?240*240:220*220))continue;
            for(unsigned j=0;j<WX_REGION_FILES;j++)if(distance<scores[j]){
                for(unsigned k=WX_REGION_FILES-1;k>j;k--){scores[k]=scores[k-1];r->wanted[k]=r->wanted[k-1];}scores[j]=distance;r->wanted[j]=(int)i;break;}
        }
        memcpy(r->position,p,sizeof r->position);r->map=map;r->position_ready=1;
    }
    if(r->pending.phase){unsigned i=0;while(i<WX_REGION_FILES&&r->wanted[i]!=r->pending_cell)i++;
        if(i==WX_REGION_FILES){wx_pack_attach_cancel(&r->pending);r->pending_cell=-1;}}
    for(unsigned slot=0;slot<WX_REGION_FILES;slot++)if(r->active[slot]>=0){
        unsigned j=0;while(j<WX_REGION_FILES&&r->wanted[j]!=r->active[slot])j++;
        if(j==WX_REGION_FILES){wx_pack_detach(s,slot);r->active[slot]=-1;r->loaded--;r->transitions++;}
    }
    if(!r->pending.phase&&wx_stream_index_ready())for(unsigned j=0;j<WX_REGION_FILES;j++)if(r->wanted[j]>=0){
        int wanted=r->wanted[j];unsigned slot=0;while(slot<WX_REGION_FILES&&r->active[slot]!=wanted)slot++;
        if(slot<WX_REGION_FILES)continue;
        char path[256];snprintf(path,sizeof path,"%s%s",r->directory,r->cells[wanted].file);
        if(!wx_pack_attach_begin(&r->pending,path,r->cells[wanted].bytes)){
            if(r->pending.deferred)break;
            r->failures++;snprintf(r->error,sizeof r->error,"Unable to prepare %s",r->cells[wanted].file);return 0;}
        r->pending_cell=wanted;break;
    }
    if(r->pending.phase){
        int attached=wx_pack_attach_pump(s,&r->pending);
        if(attached<0){r->failures++;snprintf(r->error,sizeof r->error,"Invalid world pack index");return 0;}
        if(attached){r->active[attached-1]=r->pending_cell;r->pending_cell=-1;r->loaded++;r->transitions++;}
    }
    if(r->selected<0){snprintf(r->error,sizeof r->error,"Current world region is not installed");return 0;}
    for(unsigned i=0;i<WX_REGION_FILES;i++)if(r->active[i]==r->selected){r->error[0]=0;return 1;}return 0;
}
