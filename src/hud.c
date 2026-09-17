// Vanilla 5875 unit fields; no class-based guesses about the active resource.
#include "wx_hud.h"
#include <string.h>
#include <stdio.h>
float wx_hud_fraction(uint32_t value,uint32_t maximum){return !maximum?0:value>=maximum?1:(float)((double)value/maximum);}
uint32_t wx_hud_power_color(unsigned type){
    static const uint32_t colors[]={0xff0000ff,0xffff0000,0xffff8040,0xffffff00,0xff00ffff};
    return type<5?colors[type]:0xff808080;
}
const char* wx_hud_power_name(unsigned type){static const char* names[]={"Mana","Rage","Focus","Energy","Happiness"};return type<5?names[type]:"";}
unsigned wx_hud_power_display(unsigned type,uint32_t value){return type==1?value/10:value;}
void wx_hud_unit(WxHudUnit* u,const WxEntity* e,const char* name){
    memset(u,0,sizeof *u);if(!e||!e->guid||(e->type!=3&&e->type!=4))return;
    u->present=1;u->guid=e->guid;u->health=e->fields[22];u->max_health=e->fields[28];u->level=e->fields[34];
    u->power_type=e->fields[36]>>24;u->character_class=(e->fields[36]>>8)&255;
    if(u->power_type<5){u->power=e->fields[23+u->power_type];u->max_power=e->fields[29+u->power_type];u->power_known=u->max_power!=0;}
    u->ghost=e->type==4&&(e->fields[190]&16)!=0;u->dead=u->max_health&&!u->health;
    snprintf(u->name,sizeof u->name,"%.95s",name&&*name?name:"Unknown");
}
static void bar(WxUi* ui,float x,float y,float w,float h,uint32_t value,uint32_t maximum,uint32_t color){
    float fraction=wx_hud_fraction(value,maximum);wx_ui_rect(ui,x,y,w,h,0xc0000000);
    if(fraction>0)wx_ui_image_region(ui,WX_UI_STATUS_BAR,x,y,w*fraction,h,0,0,fraction,1,color);
}
static void centered(WxUi* ui,float x,float y,uint32_t color,const char* text){
    wx_ui_center(ui,0,x+1,y+1,0xff000000,text);wx_ui_center(ui,0,x,y,color,text);
}
static void unit(WxUi* ui,const WxHudUnit* u,float x,float y,int player){
    if(!u->present)return;
    float bx=x+(player?106:7),center=bx+59.5f;
    wx_ui_rect(ui,bx,y+22,119,42,0x90000000);
    bar(ui,bx,y+41,119,12,u->ghost?0:u->health,u->max_health,0xff00ff00);
    bar(ui,bx,y+52,119,12,u->dead||u->ghost?0:u->power,u->power_known?u->max_power:0,wx_hud_power_color(u->power_type));
    // Same cropped source texture and horizontal reversal as Vanilla FrameXML.
    wx_ui_image_region(ui,WX_UI_UNIT_FRAME,x,y,232,100,player?1:0,0,player?0:1,1,0xffffffff);
    wx_ui_text_fit(ui,0,bx+3,y+22,113,WX_UI_GOLD,u->name);
    char text[48];
    if(u->ghost)snprintf(text,sizeof text,"Ghost");else if(u->dead)snprintf(text,sizeof text,"Dead");
    else if(u->max_health)snprintf(text,sizeof text,"%u / %u",u->health,u->max_health);else snprintf(text,sizeof text,"--");
    if(wx_ui_width(ui,0,text)>115)snprintf(text,sizeof text,"%u%%",(unsigned)(wx_hud_fraction(u->health,u->max_health)*100));
    centered(ui,center,y+37,WX_UI_WHITE,text);
    if(!u->dead&&!u->ghost&&u->power_known){
        snprintf(text,sizeof text,"%u / %u",wx_hud_power_display(u->power_type,u->power),wx_hud_power_display(u->power_type,u->max_power));
        if(wx_ui_width(ui,0,text)>115)snprintf(text,sizeof text,"%u%%",(unsigned)(wx_hud_fraction(u->power,u->max_power)*100));
        centered(ui,center,y+49,WX_UI_WHITE,text);
    }
    if(!player&&u->level==UINT32_MAX)wx_ui_image(ui,WX_UI_UNIT_SKULL,x+169,y+55,20,20,0xffffffff);
    else {if(u->level&&u->level<=255)snprintf(text,sizeof text,"%u",u->level);else snprintf(text,sizeof text,"??");
        centered(ui,x+(player?53:179),y+58,WX_UI_GOLD,text);}
    // Resource label is below the frame for controller readability at 640x480.
    if(u->power_known)centered(ui,center,y+78,WX_UI_GOLD,wx_hud_power_name(u->power_type));
}
int wx_hud_draw(WxHud* h,WxUi* ui){
    h->drawn=h->quads=0;if(!ui||!ui->ready||ui->sprite_count<WX_UI_SPRITES||!h->player.present)return 0;
    unsigned before=ui->quads;unit(ui,&h->player,8,20,1);unit(ui,&h->target,272,20,0);
    if(h->next_xp&&h->player.level<60){
        wx_ui_rect(ui,32,126,208,14,0xff756548);bar(ui,33,127,206,12,h->xp,h->next_xp,0xff9445cb);
        char text[48];snprintf(text,sizeof text,"XP %u / %u",h->xp,h->next_xp);
        if(wx_ui_width(ui,0,text)>202)snprintf(text,sizeof text,"XP %u%%",(unsigned)(wx_hud_fraction(h->xp,h->next_xp)*100));
        centered(ui,136,123,WX_UI_WHITE,text);
    }
    h->quads=ui->quads-before;h->drawn=1;return 1;
}
void wx_hud_metrics(const WxHud* h,unsigned out[17]){
    const WxHudUnit* p=&h->player;const WxHudUnit* t=&h->target;
    unsigned values[]={h->drawn,h->quads,p->health,p->max_health,p->power_type,p->power,p->max_power,
        (unsigned)t->guid,(unsigned)(t->guid>>32),t->health,t->max_health,t->power_type,t->power,t->max_power,h->xp,h->next_xp,t->present};
    memcpy(out,values,sizeof values);
}
