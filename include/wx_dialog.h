#ifndef WX_DIALOG_H
#define WX_DIALOG_H
#include "wx_game.h"
#include "wx_input.h"
#include "wx_entities.h"
#include "wx_world.h"
void wx_dialog_input(const WxGame* game,const WxPad* pad,const WxEntity* player);
void wx_dialog_draw(const WxGame* game,const WxWorldView* world);
#endif
