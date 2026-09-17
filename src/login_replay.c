// Explicit private development fixture; no credentials enter logs/telemetry.
#include "wx_login.h"
#include "wx_lobby.h"
#include "wx_input.h"
#include <string.h>
static void next(WxLoginReplay* r){r->stage++;r->wait=0;}
static void press(WxPad* pad,unsigned button){pad->buttons=pad->pressed=1u<<button;}
static void defaults(WxLoginUi* ui){WxAuthConfig config;wx_network_login_defaults(&config);wx_login_init(ui,&config);volatile char* p=config.password;for(unsigned i=0;i<sizeof config.password;i++)p[i]=0;}
void wx_login_replay(WxLoginReplay* r,WxLoginUi* ui,const WxLoginView* v,const WxCharacterLobby* lobby,WxCharacterUi* characters,int online,WxPad* pad){
    if(v->mode!=2)return;
    if(r->stage==16)return; // A completed login fixture may hand off to the preview input trace.
    memset(pad,0,sizeof *pad);pad->connected=1;pad->action=pad->slot=-1;
    if(r->stage==99)return;
    if(++r->frame>12000){r->stage=99;return;}r->wait++;
    switch(r->stage){
        case 0:if(v->phase==WX_LOGIN_ACCOUNT&&wx_network_ready())next(r);break;
        case 1:if(r->wait>=600){press(pad,WX_A);next(r);}break;
        case 2:if(ui->keyboard&&r->wait>=360){press(pad,WX_B);next(r);}break;
        case 3:if(!ui->keyboard&&r->wait>=60){ui->config.password[0]=ui->config.password[0]=='A'?'B':'A';press(pad,WX_START);next(r);}break;
        case 4:if(v->phase==WX_LOGIN_ERROR&&wx_auth_state()==WX_AUTH_REJECTED)next(r);break;
        case 5:if(r->wait>=300){defaults(ui);press(pad,WX_START);next(r);}break;
        case 6:if(v->phase==WX_LOGIN_REALMS)next(r);break;
        case 7:if(r->wait>=600){press(pad,WX_B);next(r);}break;
        case 8:if(v->phase==WX_LOGIN_ACCOUNT)next(r);break;
        case 9:if(r->wait>=120){defaults(ui);press(pad,WX_START);next(r);}break;
        case 10:if(v->phase==WX_LOGIN_REALMS)next(r);break;
        case 11:if(r->wait>=180){press(pad,WX_A);next(r);}break;
        case 12:if(lobby->phase==WX_LOBBY_READY)next(r);break;
        case 13:if(r->wait>=600)next(r);break;
        case 14:
            if(lobby->phase==WX_LOBBY_READY){unsigned selected=0;
                while(selected<lobby->characters.count&&strcmp(lobby->characters.items[selected].name,"Xboxer"))selected++;
                if(selected==lobby->characters.count){r->stage=99;break;}
                if(characters->selected<selected)press(pad,WX_DOWN);else if(characters->selected>selected)press(pad,WX_UP);
                else {press(pad,WX_A);next(r);}
            }break;
        case 15:if(online&&r->wait>=600)next(r);break;
        default:r->stage=99;break;
    }
}
