#ifndef WX_INVENTORY_UI_H
#define WX_INVENTORY_UI_H
#include "wx_inventory.h"
#include "wx_game.h"
#include "wx_input.h"
void wx_inventory_input(const WxInventory* inventory,const WxPad* pad);
void wx_inventory_draw(const WxInventory* inventory,const WxGame* game);
void wx_vendor_input(const WxInventory* inventory,const WxGame* game,const WxPad* pad);
void wx_vendor_draw(const WxInventory* inventory,const WxGame* game);
#endif
