#include "wx_login.h"
#include "wx_input.h"
#include <string.h>
#include <stdio.h>
// All 95 printable ASCII characters; two pages of 48 keys, final cell empty.
char wx_login_key(unsigned page,unsigned key){
    static const char keys[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .:-_@/+?!#()abcdefghijklmnopqrstuvwxyz\"$%&'*,;<=>[\\]^`{|}~";
    unsigned at=page*48+key;return page<2&&key<48&&at<sizeof keys-1?keys[at]:0;
}
void wx_login_init(WxLoginUi* ui,const WxAuthConfig* defaults){
    memset(ui,0,sizeof *ui);if(defaults)ui->config=*defaults;
    ui->config.host[63]=ui->config.username[16]=ui->config.password[16]=0;
    snprintf(ui->server,sizeof ui->server,"%s:%u",ui->config.host,ui->config.port);
    ui->revision=~0u;
}
static char* field(WxLoginUi* ui){return ui->field==0?ui->config.username:ui->field==1?ui->config.password:ui->server;}
static int configuration(WxLoginUi* ui,WxAuthConfig* output){
    if(!ui->config.username[0]||!ui->config.password[0]){snprintf(ui->message,sizeof ui->message,"Enter your account name and password.");return 0;}
    const char* colon=strchr(ui->server,':');
    unsigned port=0;int valid=colon&&colon!=ui->server&&colon-ui->server<64&&colon[1];
    if(valid)for(const char* p=colon+1;*p;p++){if(*p<'0'||*p>'9'||port>6553){valid=0;break;}port=port*10+(unsigned)(*p-'0');}
    unsigned octets=0,value=0,digits=0;
    if(valid)for(const char* p=ui->server;p<=colon;p++){
        if(p==colon||*p=='.'){if(!digits||value>255){valid=0;break;}octets++;value=digits=0;}
        else if(*p>='0'&&*p<='9'){value=value*10+(unsigned)(*p-'0');if(++digits>3){valid=0;break;}}
        else {valid=0;break;}
    }
    if(!valid||octets!=4||!port||port>65535){snprintf(ui->message,sizeof ui->message,"Use a server IPv4 address and port, e.g. 10.0.2.2:3724.");return 0;}
    *output=ui->config;memset(output->host,0,sizeof output->host);memcpy(output->host,ui->server,(size_t)(colon-ui->server));output->port=port;
    ui->config=*output;ui->message[0]=0;return 1;
}
int wx_login_input(WxLoginUi* ui,const WxLoginView* view,const WxRealms* realms,const WxPad* pad,WxLoginCommand* command){
    memset(command,0,sizeof *command);
    if(ui->revision!=view->revision){ui->revision=view->revision;ui->keyboard=0;memset(ui->original,0,sizeof ui->original);}
    if(pad->layer)return 0;unsigned p=pad->pressed;
    if(view->phase==WX_LOGIN_AUTHENTICATING||view->phase==WX_LOGIN_WORLD||view->phase==WX_LOGIN_REALMS){
        if(p&(1u<<WX_B)){command->kind=WX_LOGIN_CANCEL;return 1;}
        if(view->phase!=WX_LOGIN_REALMS)return 0;
        if(ui->selected>=realms->count)ui->selected=0;
        if((p&(1u<<WX_UP))&&ui->selected)ui->selected--;
        if((p&(1u<<WX_DOWN))&&ui->selected+1<realms->count)ui->selected++;
        if((p&(1u<<WX_A))&&ui->selected<realms->count&&wx_realm_available(&realms->items[ui->selected])){command->kind=WX_LOGIN_SELECT;command->index=ui->selected;return 1;}
        return 0;
    }
    if(view->phase!=WX_LOGIN_ACCOUNT&&view->phase!=WX_LOGIN_ERROR)return 0;
    if(ui->keyboard){
        char* text=field(ui);size_t n=strlen(text),capacity=ui->field==2?sizeof ui->server-1:16;
        if(p&(1u<<WX_B)){strcpy(text,ui->original);memset(ui->original,0,sizeof ui->original);ui->keyboard=0;return 0;}
        if(p&(1u<<WX_START)){memset(ui->original,0,sizeof ui->original);ui->keyboard=0;return 0;}
        if(p&(1u<<WX_Y))ui->page^=1;
        if(p&(1u<<WX_LEFT))ui->key=(ui->key+47)%48;
        if(p&(1u<<WX_RIGHT))ui->key=(ui->key+1)%48;
        if(p&(1u<<WX_UP))ui->key=(ui->key+40)%48;
        if(p&(1u<<WX_DOWN))ui->key=(ui->key+8)%48;
        if((p&(1u<<WX_X))&&n)text[n-1]=0;
        else if((p&(1u<<WX_A))&&n<capacity){char value=wx_login_key(ui->page,ui->key);if(value){text[n]=value;text[n+1]=0;}}
        return 0;
    }
    if((p&(1u<<WX_UP))&&ui->field)ui->field--;
    if((p&(1u<<WX_DOWN))&&ui->field<3)ui->field++;
    if((p&(1u<<WX_A))&&ui->field<3){strcpy(ui->original,field(ui));ui->keyboard=1;ui->key=ui->page=0;ui->message[0]=0;return 0;}
    if((p&(1u<<WX_START))||((p&(1u<<WX_A))&&ui->field==3)){
        if(configuration(ui,&command->config)){command->kind=WX_LOGIN_SUBMIT;return 1;}
    }
    return 0;
}
