#include "wx_map.h"
#include <string.h>
#include <math.h>
#include <limits.h>
#define MAP_VERTEX_BYTES 96u
static void consume(WxPad* p){p->pressed=p->buttons=p->layer=0;p->action=p->slot=-1;p->move_x=p->move_y=p->look_x=p->look_y=0;}
static int neutral(const WxPad* p){return !p->buttons&&!p->layer&&fabsf(p->move_x)<.05f&&fabsf(p->move_y)<.05f&&fabsf(p->look_x)<.05f&&fabsf(p->look_y)<.05f;}
static float clamp(float x,float a,float b){return fmaxf(a,fminf(b,x));}
static int entry_valid(const WxMapEntry* e){
    if(!e->id||!memchr(e->name,0,sizeof e->name)||!e->name[0])return 0;
    for(unsigned i=0;e->name[i];i++)if((unsigned char)e->name[i]<32||(unsigned char)e->name[i]>126)return 0;
    return isfinite(e->left)&&isfinite(e->right)&&isfinite(e->top)&&isfinite(e->bottom)&&
        e->left>e->right&&e->top>e->bottom&&fabsf(e->left)<100000&&fabsf(e->right)<100000&&fabsf(e->top)<100000&&fabsf(e->bottom)<100000;
}
int wx_map_open(WxMap* m,const char* root){
    memset(m,0,sizeof *m);m->loaded_id=UINT_MAX;m->zoom=1;m->u=m->v=.5f;
    if(strlen(root)>=sizeof m->root-16)return 0;strcpy(m->root,root);
    char path[224];snprintf(path,sizeof path,"%sMAPS.WMI",root);FILE* f=fopen(path,"rb");uint32_t h[4];
    int ok=f&&fread(h,1,sizeof h,f)==sizeof h&&h[0]==WX_MAP_INDEX_MAGIC&&h[1]==1&&h[2]&&h[2]<=WX_MAP_LIMIT&&h[3]==sizeof(WxMapEntry);
    if(ok){m->count=h[2];ok=fread(m->entries,sizeof(WxMapEntry),m->count,f)==m->count&&fgetc(f)==EOF;}
    if(f)fclose(f);
    for(unsigned i=0;ok&&i<m->count;i++){ok=entry_valid(&m->entries[i]);for(unsigned j=0;j<i;j++)if(m->entries[i].id==m->entries[j].id)ok=0;}
    if(!ok){m->count=0;m->failures++;strcpy(m->error,"Map catalog unavailable");}return ok;
}
int wx_map_project(const WxMapEntry* e,unsigned world,float x,float y,float* u,float* v){
    if(world!=e->map||!isfinite(x)||!isfinite(y)||!entry_valid(e))return 0;
    *u=(e->left-y)/(e->left-e->right);*v=(e->top-x)/(e->top-e->bottom);
    return *u>=0&&*u<=1&&*v>=0&&*v<=1;
}
int wx_map_find(const WxMap* m,unsigned world,float x,float y,int continent){
    int found=-1;float best=1e30f;
    for(unsigned i=0;i<m->count;i++){const WxMapEntry* e=&m->entries[i];float u,v;
        if(continent&&e->area)continue;if(!wx_map_project(e,world,x,y,&u,&v))continue;
        float size=(e->left-e->right)*(e->top-e->bottom);if(size<best){found=(int)i;best=size;}
    }return found;
}
static void center(WxMap* m,unsigned world,float x,float y){float u=.5f,v=.5f;
    if(m->count)m->marker=wx_map_project(&m->entries[m->selected],world,x,y,&m->player_u,&m->player_v);
    if(m->marker){u=m->player_u;v=m->player_v;}float edge=.5f/m->zoom;m->u=clamp(u,edge,1-edge);m->v=clamp(v,edge,1-edge);}
int wx_map_input(WxMap* m,WxPad* p,int allowed,unsigned world,float x,float y,float dt){
    if(m->latched){if(neutral(p)||!p->connected)m->latched=0;consume(p);return 1;}
    if(!allowed||!p->connected){if(m->open){m->open=0;m->latched=p->connected&&!neutral(p);consume(p);return 1;}return 0;}
    if(!m->open){
        if(p->layer||!(p->pressed&(1u<<WX_BACK)))return 0;
        m->open=1;m->zoom=1;int which=wx_map_find(m,world,x,y,0);m->selected=which<0?0:(unsigned)which;center(m,world,x,y);m->revision++;
        consume(p);return 1;
    }
    if(p->pressed&((1u<<WX_BACK)|(1u<<WX_B)|(1u<<WX_START))){m->open=0;m->latched=!neutral(p);consume(p);return 1;}
    if(m->count){
        if(p->pressed&((1u<<WX_LEFT)|(1u<<WX_RIGHT))){int direction=(p->pressed&(1u<<WX_RIGHT))?1:-1;
            for(unsigned step=1;step<=m->count;step++){unsigned n=(m->selected+m->count+direction*(int)step)%m->count;
                if(m->entries[n].map==world){m->selected=n;m->zoom=1;center(m,world,x,y);m->revision++;break;}}
        }
        if(p->pressed&((1u<<WX_UP)|(1u<<WX_DOWN))){int n=wx_map_find(m,world,x,y,(p->pressed&(1u<<WX_UP))!=0);
            if(n>=0){m->selected=(unsigned)n;m->zoom=1;center(m,world,x,y);m->revision++;}}
        if(p->pressed&(1u<<WX_A)){m->zoom=m->zoom==1?2:1;m->revision++;}
        if(p->pressed&(1u<<WX_X)){center(m,world,x,y);m->revision++;}
        if(!isfinite(dt)||dt<0)dt=0;if(dt>.05f)dt=.05f;
        if(isfinite(p->move_x)&&isfinite(p->move_y)){m->u+=p->move_x*dt*.6f/m->zoom;m->v-=p->move_y*dt*.6f/m->zoom;}
        float edge=.5f/m->zoom;m->u=clamp(m->u,edge,1-edge);m->v=clamp(m->v,edge,1-edge);
        m->marker=wx_map_project(&m->entries[m->selected],world,x,y,&m->player_u,&m->player_v);
    }consume(p);return 1;
}
unsigned wx_map_morton(unsigned x,unsigned y){unsigned a=0;for(unsigned b=0;b<9;b++)a|=((x>>b)&1)<<(b*2)|((y>>b)&1)<<(b*2+1);return a;}
void wx_map_body(WxMap* m,int known,unsigned world,float x,float y){m->body=known&&m->open&&m->count&&wx_map_project(&m->entries[m->selected],world,x,y,&m->body_u,&m->body_v);}
int wx_map_revealed(const WxMapPatch* p,const uint32_t* explored){if(!explored)return 0;for(unsigned i=0;i<4;i++)if(p->bits[i]<2048&&(explored[p->bits[i]/32]&(1u<<(p->bits[i]%32))))return 1;return 0;}
static void release(WxMap* m){wx_gpu_free(m->pixels);wx_gpu_free(m->vertices);m->pixels=NULL;m->vertices=NULL;m->bytes=m->ready=0;}
void wx_map_update(WxMap* m,const uint32_t* explored){
    if(!m->open){release(m);m->loaded_id=UINT_MAX;return;}
    if(!m->count)return;uint32_t empty[64]={0};if(!explored)explored=empty;
    unsigned id=m->entries[m->selected].id;
    if(m->loaded_id==id&&!memcmp(m->explored,explored,sizeof m->explored))return;
    m->loaded_id=id;memcpy(m->explored,explored,sizeof m->explored);m->ready=m->overlays=0;
    if(!m->pixels){
        if(wx_free_memory()<8u*1024*1024+WX_MAP_BYTES+MAP_VERTEX_BYTES+65536)goto fail;
        m->pixels=wx_gpu_alloc(WX_MAP_BYTES);m->vertices=wx_gpu_alloc(MAP_VERTEX_BYTES);
        if(!m->pixels||!m->vertices){release(m);goto fail;}m->bytes=WX_MAP_BYTES+MAP_VERTEX_BYTES;
    }
    char path[224];snprintf(path,sizeof path,"%sZ%07u.WMP",m->root,id);FILE* f=fopen(path,"rb");
    if(!f)goto fail;
    uint32_t h[4]={0},row[512];WxMapPatch patches[WX_MAP_LIMIT];
    int good=fread(h,1,sizeof h,f)==sizeof h&&h[0]==WX_MAP_MAGIC&&h[1]==id&&h[2]<=WX_MAP_LIMIT&&h[3]<16u*1024*1024;
    if(good)good=fseek(f,0,SEEK_END)==0&&ftell(f)==(long)h[3]&&fseek(f,16,SEEK_SET)==0;
    if(good)good=fread(patches,sizeof(WxMapPatch),h[2],f)==h[2];
    uint32_t end=16+h[2]*sizeof(WxMapPatch)+WX_MAP_BYTES;
    for(unsigned i=0;good&&i<h[2];i++){WxMapPatch* p=&patches[i];
        good=p->width&&p->height&&p->width<=501&&p->height<=334&&p->x<501&&p->y<334&&p->width<=501-p->x&&p->height<=334-p->y&&p->offset==end;
        for(unsigned k=0;k<4;k++)if(p->bits[k]>=2048&&p->bits[k]!=UINT_MAX)good=0;
        if(good)end+=p->width*p->height*4;
    }if(end!=h[3])good=0;
    for(unsigned y=0;good&&y<512;y++){good=fread(row,4,512,f)==512;if(good)for(unsigned x=0;x<512;x++)m->pixels[wx_map_morton(x,y)]=row[x];}
    for(unsigned i=0;good&&i<h[2];i++){WxMapPatch* p=&patches[i];if(!wx_map_revealed(p,explored))continue;
        good=fseek(f,p->offset,SEEK_SET)==0;
        for(unsigned y=0;good&&y<p->height;y++){good=fread(row,4,p->width,f)==p->width;
            if(good)for(unsigned x=0;x<p->width;x++){uint32_t src=row[x],a=src>>24,*dst=&m->pixels[wx_map_morton(x+p->x,y+p->y)],out=0xff000000;
                for(unsigned c=0;c<24;c+=8)out|=(((((src>>c)&255)*a+((*dst>>c)&255)*(255-a)+127)/255)<<c);*dst=out;}}
        m->overlays++;
    }fclose(f);if(!good)goto fail;m->ready=1;m->loads++;m->error[0]=0;return;
fail:m->failures++;strcpy(m->error,"Map image unavailable");
}
