#ifndef WX_CAST_H
#define WX_CAST_H
#include "wx_game.h"
#include "wx_ui.h"
#include "wx_spellbook.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {WX_CAST_NONE,WX_CAST_PREPARING,WX_CAST_CHANNEL,WX_CAST_COMPLETE,WX_CAST_INTERRUPTED,WX_CAST_FAILED};
typedef struct WxCastState {uint32_t spell,phase,start,duration,delay,infinite,revision;} WxCastState;
typedef struct WxCastView {uint32_t spell,phase,elapsed,duration,remaining,delay,infinite,revision;} WxCastView;
int wx_cast_apply(WxCastState* state,uint16_t opcode,const uint8_t* data,size_t size,uint64_t player,uint32_t now);
WxCastView wx_cast_query(const WxCastState* state,uint32_t now);
int wx_cast_cancel(const WxCastView* view,WxCommand* command);
void wx_cast_draw(WxUi* ui,const WxCastView* view,const WxSpellBook* book);
void wx_world_cast(WxCastView* view);
#ifdef __cplusplus
}
#endif
#endif
