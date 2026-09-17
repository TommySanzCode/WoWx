#ifndef WX_PORTRAIT_H
#define WX_PORTRAIT_H
#include <stdint.h>
#define WX_PORTRAIT_LIMIT 512u
#define WX_PORTRAIT_PLAYER 0x80000000u
/* Model coordinates, shared by every animation/appearance of the same model. */
typedef struct WxPortraitEntry {
    uint32_t key,source; /* source: 1 authored camera, 2 head bone, 3 bounds */
    float camera[3],target[3],fov,near_clip,far_clip;
} WxPortraitEntry;
typedef struct WxPortraitCatalog {
    WxPortraitEntry entries[WX_PORTRAIT_LIMIT];
    unsigned count,ready,failures;
} WxPortraitCatalog;
typedef struct WxPortraitView {
    float camera[4],right[4],up[4],forward[4],focal,near_clip,far_clip;
} WxPortraitView;
typedef struct WxPortraitSpan {unsigned x,y,width,height;} WxPortraitSpan;
#ifdef __cplusplus
extern "C" {
#endif
int wx_portrait_valid(const WxPortraitEntry* entry);
int wx_portrait_open(WxPortraitCatalog* catalog,const char* path);
const WxPortraitEntry* wx_portrait_find(const WxPortraitCatalog* catalog,uint32_t key);
int wx_portrait_view(const WxPortraitEntry* entry,WxPortraitView* view);
/* Exact pixel-centre circle, grouped into <=64 non-overlapping clear rectangles. */
unsigned wx_portrait_spans(WxPortraitSpan spans[64],unsigned x,unsigned y);
#ifdef __cplusplus
}
#endif
#endif
