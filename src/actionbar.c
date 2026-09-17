#include "wx_icons.h"
#include "wx_actions.h"
#include <math.h>
#include <stdio.h>
unsigned wx_action_cooldown_ui(WxUi* u,const WxCooldownView* cd,float x,float y){
    if(!u||!u->ready||!cd||(!cd->held&&(!cd->remaining||!cd->duration||cd->remaining>cd->duration)))return 0;
    // One small fixed lookup; subsequent frames use only integer threshold tests.
    static uint16_t angles[32*32];static unsigned initialized;
    if(!initialized){
        for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
            float angle=atan2f((float)col-15.5f,15.5f-(float)row);if(angle<0)angle+=6.28318530718f;
            angles[row*32+col]=(uint16_t)(angle*(65536.0f/6.28318530718f));
        }
        initialized=1;
    }
    unsigned before=u->quads,phase=cd->held?0:(unsigned)((uint64_t)(cd->duration-cd->remaining)*65536/cd->duration);
    for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;){
        if(angles[row*32+col]<phase){col++;continue;}
        unsigned start=col++;while(col<32&&angles[row*32+col]>=phase)col++;
        wx_ui_rect(u,x+start,y+row,(float)(col-start),1,0xb8000000);
    }
    if(cd->global&&!cd->held)return u->quads-before; // GCD uses only the short radial sweep
    char text[16];unsigned seconds=(cd->remaining+999)/1000;
    if(cd->held)snprintf(text,sizeof text,"...");
    else if(seconds>=3600)snprintf(text,sizeof text,"%uh",(seconds+3599)/3600);
    else if(seconds>=60)snprintf(text,sizeof text,"%um",(seconds+59)/60);
    else snprintf(text,sizeof text,"%u",seconds);
    wx_ui_center(u,0,x+17,y+9,0xff000000,text);wx_ui_center(u,0,x+16,y+8,WX_UI_WHITE,text);
    return u->quads-before;
}
int wx_actionbar_ui(WxUi* u,WxIcons* icons,const WxGame* game,const WxActionPrompt prompts[8],unsigned layer,unsigned buttons,const WxCooldownView cooldowns[8],const WxActionFeedback* feedback){
    if(!u||!u->ready||!icons||!icons->ready||layer<1||layer>3)return 0;
    const char* keys[]={"A","B","X","Y","Up","Down","Left","Right"};
    const unsigned ids[]={WX_A,WX_B,WX_X,WX_Y,WX_UP,WX_DOWN,WX_LEFT,WX_RIGHT};
    const char* title[]={"Left Trigger","Right Trigger","Both Triggers"};
    wx_ui_panel(u,24,270,592,180);wx_ui_center(u,0,320,281,WX_UI_GOLD,title[layer-1]);
    for(unsigned i=0;i<8;i++){
        float x=40+(i/4)*284,y=305+(i%4)*34;int pressed=(buttons&(1u<<ids[i]))!=0;
        wx_ui_rect(u,x-1,y-1,34,34,pressed?0xffffff00:0xff807050);wx_ui_rect(u,x,y,32,32,0xff151515);
        uint32_t icon=wx_action_icon(game,prompts[i].binding);
        unsigned flags=feedback?feedback[i].flags:0;uint32_t tint=0xffffffff;const char* reason=NULL;
        if(flags&WX_ACTION_RESOURCE){tint=0xff6666ff;reason="Low resource";}
        if(flags&WX_ACTION_FAR)reason="Too far";if(flags&WX_ACTION_NEAR)reason="Too close";
        if(flags&(WX_ACTION_UNLEARNED|WX_ACTION_PASSIVE|WX_ACTION_DEAD|WX_ACTION_FORM|WX_ACTION_NO_ITEM|WX_ACTION_UNSUPPORTED)){
            tint=0xff777777;reason=flags&WX_ACTION_UNLEARNED?"Not learned":flags&WX_ACTION_PASSIVE?"Passive":flags&WX_ACTION_DEAD?"Dead":flags&WX_ACTION_FORM?"Wrong form":flags&WX_ACTION_NO_ITEM?"No item":"Unsupported";
        }
        if(icon&&!wx_icons_image(icons,u,icon,x,y,32,tint))wx_ui_center(u,0,x+16,y+7,WX_UI_GOLD,"...");
        if(icon&&cooldowns)wx_action_cooldown_ui(u,&cooldowns[i],x,y);
        if(feedback&&(flags&WX_ACTION_COUNT_KNOWN)){
            unsigned count=feedback[i].count;
            if(count==1&&(flags&WX_ACTION_CHARGES_KNOWN))count=feedback[i].charges;
            if(count!=1||(flags&WX_ACTION_CHARGES_KNOWN)){char text[12];if(count>99)snprintf(text,sizeof text,"99+");else snprintf(text,sizeof text,"%u",count);
                float tx=x+31-wx_ui_width(u,0,text);wx_ui_text(u,0,tx+1,y+18,0xff000000,text);wx_ui_text(u,0,tx,y+17,WX_UI_WHITE,text);
            }
        }
        wx_ui_text(u,0,x+40,y+7,flags&(WX_ACTION_FAR|WX_ACTION_NEAR)?0xffff4040:pressed?WX_UI_WHITE:WX_UI_GOLD,keys[i]);
        wx_ui_text_fit(u,0,x+88,y+(reason?0:7),180,prompts[i].binding?WX_UI_WHITE:0xff99938a,prompts[i].name);
        if(reason)wx_ui_text(u,0,x+88,y+16,flags&WX_ACTION_RESOURCE?0xff9999ff:0xffdd9988,reason);
    }return 1;
}
