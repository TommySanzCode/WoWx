#ifndef WX_ICONS_H
#define WX_ICONS_H
#include "wx_ui.h"
#include "wx_spellbook.h"
#include "wx_cooldown.h"
#define WX_ICON_SIZE 32u
#define WX_ICON_SLOTS 64u
#define WX_ICON_REQUESTS 32u
#define WX_ICON_INDEX_LIMIT 131072u
#define WX_ICON_IMAGE_LIMIT 16384u
#define WX_ICON_ITEM 0x80000000u
typedef struct WxIconIndex {uint32_t key,image;} WxIconIndex;
typedef struct WxIconSlot {uint32_t image,used;} WxIconSlot;
typedef struct WxIcons {
    FILE* file;WxIconIndex* index;uint32_t* pixels;
    WxIconSlot slots[WX_ICON_SLOTS];
    unsigned count,images,offset,bytes,ready,loads,failures,serial,pending,drawn;
} WxIcons;
int wx_icons_open(WxIcons* icons,const char* path);
void wx_icons_close(WxIcons* icons);
/* Call after the previous GPU frame drains. At most one 4 KiB icon read/upload. */
void wx_icons_update(WxIcons* icons,const uint32_t* keys,unsigned count);
int wx_icons_slot(const WxIcons* icons,uint32_t key);
int wx_icons_image(WxIcons* icons,WxUi* ui,uint32_t key,float x,float y,float size,uint32_t color);
uint32_t wx_action_icon(const WxGame* game,uint32_t binding);
struct WxActionFeedback;
int wx_actionbar_ui(WxUi* ui,WxIcons* icons,const WxGame* game,const WxActionPrompt prompts[8],unsigned layer,unsigned buttons,const WxCooldownView cooldowns[8],const struct WxActionFeedback* feedback);
unsigned wx_action_cooldown_ui(WxUi* ui,const WxCooldownView* cooldown,float x,float y);
#endif
