#ifndef WX_REGION_H
#define WX_REGION_H
#include "wx_runtime.h"
#define WX_REGION_TERRAIN 0u
#define WX_REGION_GLOBAL_WMO 1u
typedef struct WxRegionCell {
    uint32_t map;
    uint16_t x,y;
    char file[16];
    uint32_t bytes,reserved; /* WXI1 v2: region kind; v1 requires zero. */
} WxRegionCell;
typedef struct WxRegion {
    WxRegionCell* cells;
    unsigned count,loaded,transitions,failures;
    int active[WX_REGION_FILES],wanted[WX_REGION_FILES];
    int selected,position_ready;
    uint32_t map;
    float position[3];
    WxPackPending pending;
    int pending_cell;
    char directory[240],error[128];
} WxRegion;
int wx_region_open(WxRegion* region,const char* path);
void wx_region_close(WxRegion* region);
// Call after GPU work completes, before geometry streaming. Opens at most one
// bounded index stage per call; committed packs share the scene cache budget.
int wx_region_update(WxRegion* region,WxScene* scene,uint32_t map,const float* position);
int wx_region_cell_at(uint32_t map,const float* position,const WxRegionCell* cell);
#endif
