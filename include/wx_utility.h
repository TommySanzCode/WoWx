#ifndef WX_UTILITY_H
#define WX_UTILITY_H
#include "wx_input.h"
enum {WX_UTILITY_NONE,WX_UTILITY_BAGS,WX_UTILITY_SPELLS,WX_UTILITY_QUESTS,WX_UTILITY_SETTINGS};
typedef struct WxUtility {unsigned pending,open,selection,started,latched,action,revision;} WxUtility;
// Black tap toggles bags; hold 350ms for a radial. The caller supplies availability.
// Captured input is consumed, including the opening/closing frame.
unsigned wx_utility_input(WxUtility* ui,WxPad* pad,unsigned now,int allowed);
void wx_utility_draw(const WxUtility* ui);
#endif
