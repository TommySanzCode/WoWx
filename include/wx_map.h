#ifndef WX_MAP_H
#define WX_MAP_H
#include "wx_input.h"
#define WX_MAP_LIMIT 128u
#define WX_MAP_PIXELS (512u*512u)
#define WX_MAP_BYTES (WX_MAP_PIXELS*4u)
#define WX_MAP_MAGIC 0x314d5857u
#define WX_MAP_INDEX_MAGIC 0x31495857u
typedef struct WxMapEntry {uint32_t id,map,area;float left,right,top,bottom;char name[48];} WxMapEntry;
typedef struct WxMapPatch {uint32_t bits[4],x,y,width,height,offset;} WxMapPatch;
typedef struct WxMap {
    WxMapEntry entries[WX_MAP_LIMIT];unsigned count,open,latched,selected,ready,loads,failures,bytes,revision,overlays;
    unsigned loaded_id;uint32_t explored[64];uint32_t* pixels;void* vertices;
    float zoom,u,v,player_u,player_v;unsigned marker;
    float body_u,body_v;unsigned body;
    char root[192],error[80];
} WxMap;
int wx_map_open(WxMap* map,const char* root);
int wx_map_project(const WxMapEntry* e,unsigned world,float x,float y,float* u,float* v);
int wx_map_find(const WxMap* map,unsigned world,float x,float y,int continent);
int wx_map_input(WxMap* map,WxPad* pad,int allowed,unsigned world,float x,float y,float dt);
void wx_map_update(WxMap* map,const uint32_t* explored);
unsigned wx_map_morton(unsigned x,unsigned y);
int wx_map_revealed(const WxMapPatch* patch,const uint32_t* explored);
void wx_map_draw(WxMap* map);
void wx_map_body(WxMap* map,int known,unsigned world,float x,float y);
#endif
