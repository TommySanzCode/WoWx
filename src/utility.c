#include "wx_utility.h"
#include <math.h>
static void consume(WxPad* p){p->pressed=p->buttons=p->layer=0;p->action=p->slot=-1;p->move_x=p->move_y=p->look_x=p->look_y=0;}
static unsigned finish(WxUtility* u,unsigned action,int held){u->pending=u->open=0;u->latched=held;u->action=action;if(action)u->revision++;return action;}
unsigned wx_utility_input(WxUtility* u,WxPad* p,unsigned now,int allowed){
    unsigned result=0,held=(p->buttons&(1u<<WX_BLACK))!=0;
    if(!p->connected){u->pending=u->open=u->latched=0;return 0;}
    if(u->latched){u->latched=held;consume(p);return 0;}
    if(!allowed){if(u->pending||u->open){finish(u,0,held);consume(p);}return 0;}
    if(!u->pending&&!u->open){
        if(p->layer||!(p->pressed&(1u<<WX_BLACK)))return 0;
        u->pending=1;u->selection=0;u->started=now;
    }
    if(p->pressed&(1u<<WX_B)){finish(u,0,held);consume(p);return 0;}
    if(p->pressed&(1u<<WX_START)){result=finish(u,WX_UTILITY_SETTINGS,held);consume(p);return result;}
    if(u->pending&&(unsigned)(now-u->started)>=350){u->pending=0;u->open=1;}
    if(u->open){
        float x=p->move_x,y=p->move_y;
        if(isfinite(x)&&isfinite(y)&&x*x+y*y>.20f){
            u->selection=fabsf(y)>=fabsf(x)?(y>0?WX_UTILITY_BAGS:WX_UTILITY_QUESTS):(x>0?WX_UTILITY_SPELLS:WX_UTILITY_SETTINGS);
        }
        if(!p->layer){
            if(p->pressed&(1u<<WX_UP))u->selection=WX_UTILITY_BAGS;
            if(p->pressed&(1u<<WX_RIGHT))u->selection=WX_UTILITY_SPELLS;
            if(p->pressed&(1u<<WX_DOWN))u->selection=WX_UTILITY_QUESTS;
            if(p->pressed&(1u<<WX_LEFT))u->selection=WX_UTILITY_SETTINGS;
        }
        if(!held||(p->pressed&(1u<<WX_A)))result=finish(u,u->selection,held);
    }else if(!held)result=finish(u,WX_UTILITY_BAGS,0);
    consume(p);return result;
}
