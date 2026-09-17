// Reuse the verified walking fixture, then exercise bounded UI allocations.
// Only controller samples are injected; the server keeps all saved progress.
#include "wx_scenario.h"
static void stage(WxScenario* s,unsigned next){s->stage=next;s->stage_frame=s->frame;}
void wx_soak_scenario_input(WxScenario* s,const WxWorldView* w,const WxEntity* entities,unsigned count,
    const float* p,float yaw,uint64_t selected,const WxGame* game,const WxCharacterLobby* lobby,const WxCharacterUi* ui,WxRawPad* raw){
    if(s->stage==8||s->stage==9)return;
    if(!s->soak_started&&s->stage==4&&w->active){s->soak_started=1;s->soak_begin_ms=s->now_ms;}
    if(s->stage==50){
        unsigned age=s->frame-s->stage_frame,key=32;
        switch(age){
        case 30:key=WX_BACK;break;case 180:key=WX_A;break;case 210:key=WX_X;break;
        case 240:key=WX_UP;break;case 300:key=WX_DOWN;break;case 360:key=WX_B;break;
        case 480:key=WX_B;break;case 510:key=WX_START;break;case 525:key=WX_WHITE;break;
        case 600:key=WX_B;break;case 630:key=WX_START;break;case 645:key=WX_A;break;
        case 720:key=WX_B;break;case 750:key=WX_START;break;case 810:key=WX_B;break;
        }
        if(key<32)raw->buttons=1u<<key;
        if(age>=390&&age<420)raw->buttons=1u<<WX_BLACK;
        if(age==405)raw->buttons|=1u<<WX_UP;
        if(age>840){
            if((unsigned)(s->now_ms-s->soak_begin_ms)>=s->soak_limit_ms)stage(s,8);
            else {stage(s,4);s->soak_cycle_frame=s->frame;}
        }
    }else{
        // The existing journey's frame limits apply independently to each lap.
        unsigned base=s->soak_cycle_frame;s->frame-=base;s->stage_frame-=base;s->enabled=15;
        wx_journey_input(s,w,entities,count,p,yaw,selected,game,lobby,ui,raw);
        s->enabled=17;s->frame+=base;s->stage_frame+=base;
        if(s->stage==8){s->soak_cycles++;stage(s,50);}
    }
    if(s->soak_started&&(unsigned)(s->now_ms-s->soak_begin_ms)>s->soak_limit_ms+600000)stage(s,9);
}
