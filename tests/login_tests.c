#include "wx_login.h"
#include "wx_input.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
static unsigned checks,masked;
#define CHECK(v) do{checks++;if(!(v)){fprintf(stderr,"Login failure %u: %s\n",__LINE__,#v);exit(1);}}while(0)
static WxLoginUi ui;static WxLoginView view;static WxRealms realms;static WxLoginCommand command;
static int press(unsigned b){WxPad p={0};p.pressed=1u<<b;return wx_login_input(&ui,&view,&realms,&p,&command);}
static void observe(const char* s){CHECK(!strstr(s,"FixtureSecret"));if(strstr(s,"*************"))masked++;}
unsigned wx_auth_state(void){return WX_AUTH_REJECTED;}
const char* wx_auth_status(void){return "fixture";}
void wx_font_clear(void){}
void wx_font_printat(int row,int col,const char* format,...){(void)row;(void)col;char text[512];va_list a;va_start(a,format);vsnprintf(text,sizeof text,format,a);va_end(a);observe(text);}
void wx_ui_rect(WxUi* c,float x,float y,float w,float h,uint32_t color){(void)c;(void)x;(void)y;(void)w;(void)h;(void)color;}
void wx_ui_image(WxUi* c,unsigned sprite,float x,float y,float w,float h,uint32_t color){(void)sprite;wx_ui_rect(c,x,y,w,h,color);}
void wx_ui_panel(WxUi* c,float x,float y,float w,float h){wx_ui_rect(c,x,y,w,h,0);}
void wx_ui_text(WxUi* c,unsigned f,float x,float y,uint32_t color,const char* text){(void)c;(void)f;(void)x;(void)y;(void)color;observe(text);}
void wx_ui_center(WxUi* c,unsigned f,float x,float y,uint32_t color,const char* text){wx_ui_text(c,f,x,y,color,text);}
void wx_ui_textf(WxUi* c,unsigned f,float x,float y,uint32_t color,const char* format,...){char text[512];va_list a;va_start(a,format);vsnprintf(text,sizeof text,format,a);va_end(a);wx_ui_text(c,f,x,y,color,text);}
void wx_ui_button(WxUi* c,float x,float y,float w,const char* label,int focus,int enabled){(void)w;(void)focus;(void)enabled;wx_ui_text(c,0,x,y,0,label);}
int main(void){
    unsigned seen[128]={0};for(unsigned p=0;p<2;p++)for(unsigned k=0;k<48;k++){unsigned c=(unsigned char)wx_login_key(p,k);if(c){CHECK(c>=32&&c<127);seen[c]++;}}
    for(unsigned c=32;c<127;c++)CHECK(seen[c]==1);CHECK(!wx_login_key(2,0)&&!wx_login_key(0,48));
    WxAuthConfig defaults={"10.0.2.2","FIXTURE","",3724};view.phase=WX_LOGIN_ACCOUNT;view.mode=1;
    wx_login_init(&ui,&defaults);CHECK(!strcmp(ui.server,"10.0.2.2:3724"));CHECK(!press(WX_START)&&strstr(ui.message,"password"));
    press(WX_DOWN);CHECK(ui.field==1);press(WX_A);CHECK(ui.keyboard);
    for(unsigned i=0;i<17;i++)press(WX_A);CHECK(strlen(ui.config.password)==16);
    press(WX_X);CHECK(strlen(ui.config.password)==15);press(WX_B);CHECK(!ui.config.password[0]&&!ui.keyboard&&!ui.original[0]);
    press(WX_A);press(WX_LEFT);CHECK(ui.key==47);press(WX_RIGHT);CHECK(ui.key==0);press(WX_UP);CHECK(ui.key==40);press(WX_DOWN);CHECK(ui.key==0);
    press(WX_Y);press(WX_A);CHECK(ui.config.password[0]==wx_login_key(1,0));press(WX_START);CHECK(!ui.keyboard&&!ui.original[0]);
    CHECK(press(WX_START)&&command.kind==WX_LOGIN_SUBMIT&&command.config.port==3724&&!strcmp(command.config.host,"10.0.2.2"));
    const char* invalid[]={"","host:3724","1.2.3:3724","1.2.3.256:3724","1.2.3.4:0","1.2.3.4:65536","1.2.3.4:-1","1.2.3.4:7a","1.2.3.4","1..2.3.4:1","0000.2.3.4:1"};
    for(unsigned i=0;i<sizeof invalid/sizeof *invalid;i++){strcpy(ui.server,invalid[i]);CHECK(!press(WX_START)&&!command.kind);}
    strcpy(ui.server,"127.0.0.1:65535");CHECK(press(WX_START)&&command.config.port==65535);
    WxPad modified={0};modified.layer=1;modified.pressed=1u<<WX_START;CHECK(!wx_login_input(&ui,&view,&realms,&modified,&command));
    view.phase=WX_LOGIN_AUTHENTICATING;view.revision++;CHECK(!press(WX_A));CHECK(press(WX_B)&&command.kind==WX_LOGIN_CANCEL);
    view.phase=WX_LOGIN_CANCELLING;view.revision++;CHECK(!press(WX_A)&&!press(WX_B));
    view.phase=WX_LOGIN_REALMS;view.revision++;CHECK(!press(WX_A));
    realms.count=255;for(unsigned i=0;i<realms.count;i++){realms.items[i].port=8086;strcpy(realms.items[i].host,"127.0.0.1");}
    realms.items[0].flags=2;CHECK(!press(WX_A));press(WX_DOWN);CHECK(press(WX_A)&&command.kind==WX_LOGIN_SELECT&&command.index==1);
    for(unsigned i=0;i<300;i++)press(WX_DOWN);CHECK(ui.selected==254);press(WX_DOWN);CHECK(ui.selected==254);
    realms.count=1;press(WX_UP);CHECK(ui.selected==0);CHECK(press(WX_B)&&command.kind==WX_LOGIN_CANCEL);
    wx_login_init(&ui,&defaults);strcpy(ui.config.password,"FixtureSecret");view.phase=WX_LOGIN_ACCOUNT;view.revision++;
    WxUi canvas={0};canvas.ready=1;wx_login_draw(&ui,&view,&realms,&canvas);CHECK(masked==1);
    press(WX_DOWN);press(WX_A);CHECK(ui.keyboard);wx_login_draw(&ui,&view,&realms,&canvas);CHECK(masked==2);
    view.phase=WX_LOGIN_AUTHENTICATING;view.revision++;press(WX_A);CHECK(!ui.keyboard&&!ui.original[0]);
    for(unsigned phase=WX_LOGIN_AUTHENTICATING;phase<=WX_LOGIN_CANCELLING;phase++){view.phase=phase;wx_login_draw(&ui,&view,&realms,&canvas);}
    printf("Controller login: %u checks passed; passwords masked\n",checks);return 0;
}
