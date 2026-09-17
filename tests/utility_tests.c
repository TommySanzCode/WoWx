#include "wx_utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Utility FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static WxPad pad(unsigned buttons,unsigned pressed){WxPad p={0};p.connected=1;p.buttons=buttons;p.pressed=pressed;p.action=p.slot=-1;return p;}
static void hold(WxUtility* u,unsigned now){WxPad p=pad(1u<<WX_BLACK,1u<<WX_BLACK);CHECK(!wx_utility_input(u,&p,now,1)&&u->pending&&!u->open);p=pad(1u<<WX_BLACK,0);CHECK(!wx_utility_input(u,&p,now+350,1)&&u->open);}
int main(void){
    WxUtility u={0};WxPad p=pad(1u<<WX_BLACK,1u<<WX_BLACK);p.move_y=1;p.look_x=1;p.action=10;
    CHECK(!wx_utility_input(&u,&p,100,1)&&u.pending&&!p.pressed&&p.action==-1&&!p.move_y&&!p.look_x);
    p=pad(0,0);CHECK(wx_utility_input(&u,&p,200,1)==WX_UTILITY_BAGS&&!u.open&&u.revision==1);
    p=pad(0,1u<<WX_BLACK);CHECK(wx_utility_input(&u,&p,300,1)==WX_UTILITY_BAGS&&u.revision==2); // latched fast tap
    const unsigned keys[]={WX_UP,WX_RIGHT,WX_DOWN,WX_LEFT};
    for(unsigned i=0;i<4;i++){
        hold(&u,1000);p=pad(1u<<WX_BLACK,1u<<keys[i]);CHECK(!wx_utility_input(&u,&p,1400,1)&&u.selection==i+1);
        p=pad(0,0);CHECK(wx_utility_input(&u,&p,1450,1)==i+1&&!u.open&&!u.pending);
        hold(&u,2000);p=pad(1u<<WX_BLACK,0);p.move_x=i==1?1:i==3?-1:0;p.move_y=i==0?1:i==2?-1:0;
        CHECK(!wx_utility_input(&u,&p,2400,1)&&u.selection==i+1&&!p.move_x&&!p.move_y);
        p=pad(1u<<WX_BLACK,1u<<WX_A);p.layer=3;p.action=16;
        CHECK(wx_utility_input(&u,&p,2500,1)==i+1&&u.latched&&p.action==-1&&!p.layer&&!p.pressed);
        p=pad(1u<<WX_BLACK,1u<<WX_X);CHECK(!wx_utility_input(&u,&p,2600,0)&&!p.pressed);
        p=pad(0,0);CHECK(!wx_utility_input(&u,&p,2700,0)&&!u.latched);
    }
    hold(&u,100);p=pad(0,0);CHECK(!wx_utility_input(&u,&p,500,1)&&!u.open); // neutral release cancels
    hold(&u,100);p=pad(1u<<WX_BLACK,1u<<WX_B);CHECK(!wx_utility_input(&u,&p,500,1)&&!u.open&&u.latched);
    p=pad(0,0);wx_utility_input(&u,&p,550,1);
    hold(&u,100);p=pad(1u<<WX_BLACK,0);CHECK(!wx_utility_input(&u,&p,500,0)&&!u.open&&u.latched);
    p=pad(0,0);wx_utility_input(&u,&p,550,0);
    hold(&u,100);p=pad(0,0);p.connected=0;CHECK(!wx_utility_input(&u,&p,500,1)&&!u.open&&!u.latched);
    p=pad(1u<<WX_BLACK,1u<<WX_BLACK);p.layer=1;CHECK(!wx_utility_input(&u,&p,100,1)&&!u.pending&&p.pressed);
    p=pad(1u<<WX_BLACK,1u<<WX_BLACK);CHECK(!wx_utility_input(&u,&p,100,0)&&!u.pending);
    hold(&u,0xffffff00u);p=pad(1u<<WX_BLACK,1u<<WX_START);CHECK(wx_utility_input(&u,&p,200,1)==WX_UTILITY_SETTINGS&&!p.pressed);
    printf("Utility input, cancellation and capture checks: %u passed\n",checks);return 0;
}
