// Development acceptance: only ordinary controller samples, no packet shortcuts.
#include "wx_scenario.h"
#include "wx_inventory.h"
#include <math.h>
#include <string.h>
static void step(WxScenario* s,unsigned stage){s->stage=stage;s->stage_frame=s->frame;}
static void control(WxRawPad* raw,unsigned slot){
    const unsigned buttons[]={WX_A,WX_B,WX_X,WX_Y,WX_UP,WX_DOWN,WX_LEFT,WX_RIGHT};unsigned layer=1+slot/8;
    raw->axes[4]=layer&1?32767:0;raw->axes[5]=layer&2?32767:0;raw->buttons=1u<<buttons[slot%8];
}
void wx_spell_scenario_input(WxScenario* s,const WxWorldView* w,const WxEntity* self,const float* p,const WxGame* g,const WxSpellBook* b,const WxSpellUi* u,const uint8_t bindings[24],unsigned form,WxPad* pad){
    if(s->enabled!=12)return;WxRawPad raw={0};raw.connected=1;WxControls controls;wx_controls_default(&controls);
    if(!s->stage){if(w->active){s->frame=0;step(s,1);}else goto input;}
    s->frame++;unsigned age=s->frame-s->stage_frame;
    if(s->stage==1&&age==1)raw.buttons=1u<<WX_RSTICK;
    if(s->stage!=8&&s->stage!=9&&s->frame>30*300)step(s,9);
    if(s->stage==1&&age>120&&self&&g->actions_ready&&b->count&&b->loaded==b->count){
        const WxKnownSpell* spell=wx_spellbook_find(b,78);if(!spell||spell->state!=1){step(s,9);goto input;}
        unsigned candidate=24,found=0;while(candidate){candidate--;unsigned slot=wx_action_slot(bindings[candidate],form);if(slot<120&&!g->actions[slot]){s->trade_price=slot;s->trade_bundle=candidate;found=1;break;}}
        if(!found){step(s,9);goto input;}
        s->initial_kills=w->revision;s->initial_xp=self->xp;s->initial_level=self->fields[34];memcpy(s->probe_position,p,12);
        WxInventory inv;wx_world_inventory(&inv);s->initial_count=inv.count;s->initial_money=inv.money;
        raw.buttons=1u<<WX_START;step(s,2);
    }else if(s->stage==2){if(age==12)raw.buttons=1u<<WX_WHITE;if(u->open&&age>300)step(s,3);}
    else if(s->stage==3||s->stage==13){
        unsigned row=0;while(row<b->row_count&&b->known[b->rows[row]].id!=78)row++;
        if(row==b->row_count){step(s,9);goto input;}
        if(age>60&&age%12==0){raw.buttons=1u<<(u->selected<row?WX_DOWN:u->selected>row?WX_UP:WX_A);if(u->selected==row)step(s,s->stage==3?4:14);}
    }else if(s->stage==4||s->stage==14){
        if(age==12)control(&raw,s->trade_bundle);
        if(age>300&&u->screen==1&&u->slot==s->trade_bundle){raw.buttons=1u<<(s->stage==4?WX_A:WX_X);step(s,s->stage==4?5:15);}
    }else if(s->stage==5||s->stage==15){
        if(age>300&&u->screen==2){raw.buttons=1u<<WX_A;step(s,s->stage==5?6:16);}
    }else if(s->stage==6||s->stage==16){
        if(age>30&&g->actions[s->trade_price]==(s->stage==6?78u:0u)){raw.buttons=1u<<WX_B;step(s,s->stage==6?10:17);}
    }else if(s->stage==10||s->stage==17){
        // Show the live action panel after each edit, before authoritative reconnect.
        if(age>=30&&age<330){unsigned layer=1+s->trade_bundle/8;raw.axes[4]=layer&1?32767:0;raw.axes[5]=layer&2?32767:0;}
        if(age==360)raw.buttons=1u<<WX_START;if(age==372)raw.buttons=1u<<WX_BACK;
        if(age>360+30*25&&!w->active)step(s,s->stage==10?11:18);
    }else if(s->stage==11||s->stage==18){
        if(age==12)raw.buttons=1u<<WX_BACK;
        if(age>120&&w->active&&w->revision>=s->initial_kills+(s->stage==11?1u:2u)&&g->actions_ready&&self){
            unsigned expected=s->stage==11?78:0;if(g->actions[s->trade_price]!=expected){step(s,9);goto input;}
            WxInventory inv;wx_world_inventory(&inv);
            if(self->xp!=s->initial_xp||self->fields[34]!=s->initial_level||inv.money!=s->initial_money||inv.count!=s->initial_count){step(s,9);goto input;}
            for(unsigned i=0;i<3;i++)if(fabsf(p[i]-s->probe_position[i])>.01f){step(s,9);goto input;}
            raw.buttons=1u<<WX_START;step(s,s->stage==11?12:8);
        }
    }else if(s->stage==12){if(age==12)raw.buttons=1u<<WX_START;if(age==24)raw.buttons=1u<<WX_WHITE;if(u->open&&age>90)step(s,13);}
input:wx_input_process(&raw,&controls,&s->previous,pad);
}
