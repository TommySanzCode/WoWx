#ifndef WX_LOGIN_H
#define WX_LOGIN_H
#include "wx_auth.h"
#include "wx_runtime.h"
#include "wx_ui.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {WX_LOGIN_DISABLED,WX_LOGIN_ACCOUNT,WX_LOGIN_AUTHENTICATING,WX_LOGIN_REALMS,WX_LOGIN_WORLD,WX_LOGIN_ERROR,WX_LOGIN_CANCELLING};
enum {WX_LOGIN_SUBMIT=1,WX_LOGIN_SELECT,WX_LOGIN_CANCEL};
typedef struct WxLoginView {unsigned phase,revision,error,attempts,cancellations,selected,mode;} WxLoginView;
typedef struct WxLoginCommand {unsigned kind,index;WxAuthConfig config;} WxLoginCommand;
typedef struct WxLoginUi {
    WxAuthConfig config;
    char server[72],original[72],message[96];
    unsigned field,keyboard,key,page,selected,revision;
} WxLoginUi;
void wx_network_login_view(WxLoginView* output);
int wx_network_login_command(const WxLoginCommand* command);
void wx_network_login_defaults(WxAuthConfig* output);
void wx_login_init(WxLoginUi* ui,const WxAuthConfig* defaults);
int wx_login_input(WxLoginUi* ui,const WxLoginView* view,const WxRealms* realms,const WxPad* pad,WxLoginCommand* command);
char wx_login_key(unsigned page,unsigned key);
void wx_login_draw(const WxLoginUi* ui,const WxLoginView* view,const WxRealms* realms,WxUi* canvas);
struct WxCharacterLobby;
struct WxCharacterUi;
typedef struct WxLoginReplay {unsigned stage,frame,wait;} WxLoginReplay;
void wx_login_replay(WxLoginReplay* replay,WxLoginUi* ui,const WxLoginView* view,const struct WxCharacterLobby* lobby,struct WxCharacterUi* characters,int world_active,WxPad* pad);
#ifdef __cplusplus
}
#endif
#endif
