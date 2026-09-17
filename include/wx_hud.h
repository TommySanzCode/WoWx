#ifndef WX_HUD_H
#define WX_HUD_H
#include "wx_entities.h"
#include "wx_ui.h"
typedef struct WxHudUnit {
    uint64_t guid;
    uint32_t health,max_health,power,max_power,level,power_type,character_class;
    unsigned present,power_known,dead,ghost;
    char name[96];
} WxHudUnit;
typedef struct WxHud {
    WxHudUnit player,target;
    uint32_t xp,next_xp;
    unsigned drawn,quads;
} WxHud;
void wx_hud_unit(WxHudUnit* unit,const WxEntity* entity,const char* name);
float wx_hud_fraction(uint32_t value,uint32_t maximum);
uint32_t wx_hud_power_color(unsigned type);
const char* wx_hud_power_name(unsigned type);
unsigned wx_hud_power_display(unsigned type,uint32_t value);
int wx_hud_draw(WxHud* hud,WxUi* ui);
void wx_hud_metrics(const WxHud* hud,unsigned output[17]);
#endif
