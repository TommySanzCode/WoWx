#ifndef WX_PREVIEW_H
#define WX_PREVIEW_H
#include "wx_avatar.h"
#include "wx_backdrop.h"
#include "wx_lobby.h"
#include "wx_outfit.h"
typedef struct WxPreview {
    WxAvatar avatar;WxAvatarSelection selection;WxBackdrop scene;WxWorldView subject;
    float position[3],angle,rotation,zoom;unsigned active,race,background_missing,changes;
    WxOutfits outfits;unsigned outfit_attempted,outfit_ready;
} WxPreview;
/* Display-only snapshot. It is never submitted to the world connection. */
int wx_preview_subject(const WxCharacterUi* ui,const WxCharacterLobby* lobby,WxWorldView* out);
void wx_preview_input(WxPreview* p,const WxPad* pad,float dt);
/* Call after the preceding GPU submission has drained. */
void wx_preview_update(WxPreview* p,const WxCharacterUi* ui,const WxCharacterLobby* lobby,const char* directory,unsigned time);
void wx_preview_close(WxPreview* p);
#endif
