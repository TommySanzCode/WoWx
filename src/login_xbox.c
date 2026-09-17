#include "wx_login.h"
#include "wx_font.h"
#include <stdio.h>
#include <string.h>
static const char* error(const WxLoginView* v){
    if(v->error==1)return "The network could not be initialized.";
    if(v->error==3)return "Disconnected from the realm. Please log in again.";
    switch(wx_auth_state()){
        case WX_AUTH_REJECTED:return "The account name or password was not accepted.";
        case WX_AUTH_NETWORK_ERROR:return "Unable to connect. Check the server address.";
        case WX_AUTH_PROTOCOL_ERROR:return "The server sent an invalid login response.";
        case WX_AUTH_CRYPTO_ERROR:return "The login proof could not be verified.";
        default:return "Unable to log in. Please try again.";
    }
}
static void displayed(const WxLoginUi* ui,unsigned field,char* output,size_t size){
    if(field==1){unsigned n=(unsigned)strlen(ui->config.password);if(n>=size)n=(unsigned)size-1;memset(output,'*',n);output[n]=0;}
    else snprintf(output,size,"%s",field==0?ui->config.username:ui->server);
}
void wx_login_draw(const WxLoginUi* ui,const WxLoginView* view,const WxRealms* realms,WxUi* c){
    wx_font_clear();
    if(!c->ready){wx_font_printat(2,3,"WORLD OF WARCRAFT / LOGIN");wx_font_printat(5,3,"Interface assets unavailable");wx_font_printat(7,3,"%s",wx_auth_status());return;}
    wx_ui_image(c,WX_UI_LOGO,22,8,256,128,0xffffffff);
    wx_ui_text(c,0,25,454,0xff9f9988,"Version 1.12.1 (5875)");
    if(view->phase==WX_LOGIN_CANCELLING||view->phase==WX_LOGIN_AUTHENTICATING||view->phase==WX_LOGIN_WORLD){
        wx_ui_panel(c,120,194,400,126);
        const char* status=view->phase==WX_LOGIN_CANCELLING?"Disconnecting...":view->phase==WX_LOGIN_WORLD?"Connecting to game server...":
            wx_auth_state()==WX_AUTH_PROOF?"Authenticating...":wx_auth_state()==WX_AUTH_REALMS?"Retrieving realm list...":"Connecting...";
        wx_ui_center(c,1,320,215,WX_UI_GOLD,status);
        if(view->phase!=WX_LOGIN_CANCELLING){wx_ui_button(c,230,257,180,"Cancel",1,1);wx_ui_center(c,0,320,427,WX_UI_GOLD,"B: Cancel");}return;
    }
    if(view->phase==WX_LOGIN_REALMS){
        wx_ui_center(c,2,446,47,WX_UI_GOLD,"Realm Selection");wx_ui_panel(c,28,136,584,273);
        wx_ui_text(c,0,44,150,WX_UI_GOLD,"Realm Name");wx_ui_text(c,0,342,150,WX_UI_GOLD,"Type");wx_ui_text(c,0,410,150,WX_UI_GOLD,"Characters");wx_ui_text(c,0,508,150,WX_UI_GOLD,"Population");
        unsigned first=ui->selected/8*8;
        for(unsigned i=first;i<realms->count&&i<first+8;i++){
            const WxRealm* r=&realms->items[i];float y=179+(i-first)*27;uint32_t color=wx_realm_available(r)?WX_UI_WHITE:0xff888888;
            if(i==ui->selected)wx_ui_rect(c,40,y-2,558,25,0xff44351e);
            wx_ui_textf(c,0,47,y,color,"%.34s",r->name);wx_ui_text(c,0,344,y,color,wx_realm_type(r->type));
            wx_ui_textf(c,0,447,y,color,"%u",r->characters);wx_ui_text(c,0,514,y,color,wx_realm_population(r));
        }
        if(!realms->count)wx_ui_center(c,1,320,240,WX_UI_WHITE,"No realms are currently available.");
        wx_ui_center(c,0,320,427,WX_UI_GOLD,"D-pad: Select Realm   A: Connect   B: Back");return;
    }
    if(ui->keyboard){
        const char* label=ui->field==0?"Account Name":ui->field==1?"Password":"Server Address";
        wx_ui_center(c,2,450,50,WX_UI_GOLD,label);wx_ui_panel(c,35,135,570,278);
        char value[80];displayed(ui,ui->field,value,sizeof value);wx_ui_textf(c,1,55,147,WX_UI_WHITE,"%.48s_",value);
        for(unsigned i=0;i<48;i++){
            char letter=wx_login_key(ui->page,i),text[4]={letter,0};if(letter==' ')strcpy(text,"SP");
            wx_ui_button(c,54+(i%8)*67,188+(i/8)*35,62,text,ui->key==i,letter!=0);
        }
        wx_ui_center(c,0,320,427,WX_UI_GOLD,"A: Key   X: Erase   Y: Page   Start: Done   B: Cancel");return;
    }
    wx_ui_center(c,2,450,50,WX_UI_GOLD,"Account Login");
    const char* labels[]={"Account Name","Password","Server Address"};
    for(unsigned i=0;i<3;i++){
        float y=145+i*62;char text[80];displayed(ui,i,text,sizeof text);
        wx_ui_center(c,0,320,y,WX_UI_GOLD,labels[i]);
        if(ui->field==i)wx_ui_rect(c,179,y+19,282,38,0xff6e5322);
        wx_ui_panel(c,182,y+22,276,32);wx_ui_textf(c,0,192,y+28,WX_UI_WHITE,"%.34s",text);
    }
    wx_ui_button(c,230,338,180,"Login",ui->field==3,1);
    if(view->phase==WX_LOGIN_ERROR)wx_ui_center(c,0,320,385,0xffff8880,error(view));
    if(ui->message[0])wx_ui_center(c,0,320,405,0xffffc080,ui->message);
    wx_ui_center(c,0,320,427,WX_UI_GOLD,"D-pad: Choose   A: Edit / Select   Start: Login");
}
