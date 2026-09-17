// Northshire acceptance fixture: ordinary controller input only. No teleport,
// health change, direct packet send, or saved-character reset is performed here.
#include "wx_scenario.h"
#include "wx_inventory.h"
#include <math.h>
#include <string.h>
static const float outward[][2]={{-8949.95f,-132.493f},{-8934,-140},{-8910.5f,-133.5f},{-8876,-146.5f},{-8803,-177}};
static const float ghost_route[][2]={{-8947.2256f,-179.5460f},{-8933.9248f,-140.3460f},{-8934,-140},{-8910.5f,-133.5f},{-8876,-146.5f},{-8803,-177}};
static void next(WxScenario* s,unsigned stage,const float* p){s->stage=stage;s->stage_frame=s->frame;s->detours=0;s->probe_frame=s->frame;memcpy(s->probe_position,p,12);}
static void pulse(WxRawPad* r,unsigned button,unsigned age){if(age%15==0)r->buttons=1u<<button;}
static int steer(WxScenario* s,float x,float y,const float* p,float yaw,WxRawPad* r,float stop){
    float dx=x-p[0],dy=y-p[1],distance=sqrtf(dx*dx+dy*dy);if(distance<stop)return 1;
    float delta=atan2f(sinf(atan2f(dy,dx)-yaw),cosf(atan2f(dy,dx)-yaw));
    if(fabsf(delta)>.004f)r->axes[2]=(int16_t)((delta<0?-1:1)*(.18f+.82f*fminf(1,fabsf(delta)*15))*32767);
    if(fabsf(delta)<.05f)r->axes[1]=(int16_t)(-(.18f+.82f*fminf(.7f,distance*2))*32767);
    if(s->frame-s->probe_frame>180){float mx=p[0]-s->probe_position[0],my=p[1]-s->probe_position[1];
        if(mx*mx+my*my<.0025f){next(s,9,p);return 0;}s->probe_frame=s->frame;memcpy(s->probe_position,p,12);}
    return 0;
}
static int walk(WxScenario* s,const float (*route)[2],unsigned n,const float* p,float yaw,WxRawPad* r){
    if(s->detours>=n)return 1;if(steer(s,route[s->detours][0],route[s->detours][1],p,yaw,r,.055f)){
        s->detours++;s->probe_frame=s->frame;memcpy(s->probe_position,p,12);
    }return s->detours==n;
}
const char* wx_death_scenario_status(const WxScenario* s){switch(s->stage){
    case 0:return "waiting for world";case 1:return "opening character menu";case 2:return "loading roster";case 3:return "selecting test character";
    case 4:return "checking initial state";case 10:return "walking to camp";case 11:return "selecting kobold";case 12:return "provoking kobold";
    case 13:return "waiting for death";case 14:return "releasing spirit";case 15:return "arriving at graveyard";case 16:return "reading corpse map";
    case 17:return "logging out as ghost";case 18:return "reconnecting as ghost";case 19:return "checking saved corpse";
    case 20:return "walking back to body";case 21:return "reclaiming body";case 22:return "checking resurrection";
    case 6:return "logging out alive";case 7:return "checking saved recovery";case 8:return "passed";default:return "FAILED";}}
void wx_death_scenario_input(WxScenario* s,const WxWorldView* w,const WxEntity* entities,unsigned count,const float* p,float yaw,
    uint64_t selected,const WxGame* g,const WxCharacterLobby* lobby,const WxCharacterUi* ui,WxRawPad* raw){
    unsigned age=s->frame-s->stage_frame;const WxEntity* self=NULL,*target=NULL;
    for(unsigned i=0;i<count;i++){if(entities[i].guid==w->guid)self=entities+i;if(entities[i].guid==s->target)target=entities+i;}
    int ghost=self&&(self->fields[190]&16),dead=self&&!self->fields[22];WxInventory inv;wx_world_inventory(&inv);
    if(s->stage==1&&age==1)raw->buttons=1u<<WX_RSTICK;
    if(s->stage==1&&age>90){raw->buttons=1u<<WX_START;next(s,2,p);}
    else if(s->stage==2){if(age==12)raw->buttons=1u<<WX_Y;if(lobby->phase==WX_LOBBY_READY)next(s,3,p);}
    else if(s->stage==3){
        if(lobby->phase==WX_LOBBY_READY&&age%15==0){unsigned row=0;const char* name=s->test_name[0]?s->test_name:"Xboxspirit";
            while(row<lobby->characters.count&&strcmp(lobby->characters.items[row].name,name))row++;
            if(row==lobby->characters.count){next(s,9,p);return;}s->created_character=lobby->characters.items[row].guid;
            raw->buttons=1u<<(ui->selected<row?WX_DOWN:ui->selected>row?WX_UP:WX_A);}
        if(w->active&&w->guid==s->created_character)next(s,4,p);
    }else if(s->stage==4&&age>120&&self&&inv.count){
        s->initial_level=self->fields[34];s->initial_xp=self->xp;s->initial_count=inv.count;s->initial_money=inv.money;
        if(ghost){if(g->corpse_known){memcpy(s->return_points[0],g->corpse_position,12);next(s,19,p);}return;}
        if(dead){next(s,14,p);return;}
        if(self->fields[34]!=1||self->xp||fabsf(p[0]+8949.95f)>2||fabsf(p[1]+132.493f)>2){next(s,9,p);return;}
        next(s,10,p);
    }else if(s->stage==10){if(walk(s,outward,sizeof outward/sizeof *outward,p,yaw,raw))next(s,11,p);}
    else if(s->stage==11&&age>60&&self){
        if(!s->target){float nearest=55*55;for(unsigned i=0;i<count;i++)if(entities[i].type==3&&entities[i].fields[3]==6&&entities[i].fields[22]){
            float dx=entities[i].x-p[0],dy=entities[i].y-p[1],d=dx*dx+dy*dy;if(d<nearest){nearest=d;s->target=entities[i].guid;}}}
        if(s->target&&selected==s->target)next(s,12,p);else if(s->target)pulse(raw,WX_Y,age);
    }else if(s->stage==12){
        if(!target){next(s,9,p);return;}
        if(steer(s,target->x,target->y,p,yaw,raw,2.6f)&&!g->attack_target)pulse(raw,WX_X,age);
        if(g->damage_dealt&&self){raw->buttons=1u<<WX_B;s->initial_xp=self->xp;s->initial_level=self->fields[34];next(s,13,p);}
    }else if(s->stage==13&&dead){memcpy(s->return_points[0],p,12);s->initial_objectives=w->position_revision;next(s,14,p);}
    else if(s->stage==14){if(ghost)next(s,15,p);else if(age>90)pulse(raw,WX_A,age);}
    else if(s->stage==15&&ghost&&w->active){
        float dx=p[0]-s->return_points[0][0],dy=p[1]-s->return_points[0][1];
        if(w->position_revision!=s->initial_objectives&&dx*dx+dy*dy>60*60&&g->corpse_known){raw->buttons=1u<<WX_BACK;next(s,16,p);}
    }else if(s->stage==16&&age>300){raw->buttons=1u<<WX_BACK;next(s,17,p);}
    else if(s->stage==17){if(age==15)raw->buttons=1u<<WX_START;if(age==30)raw->buttons=1u<<WX_BACK;
        if(age>30*25&&!w->active)next(s,18,p);}
    else if(s->stage==18){if(age==12)raw->buttons=1u<<WX_BACK;
        if(w->active&&self&&age>90){if(!ghost){next(s,9,p);return;}raw->buttons=1u<<WX_START;next(s,19,p);}}
    else if(s->stage==19&&age>120&&g->corpse_known&&ghost){
        if(w->map||fabsf(p[0]+8935.3f)>30||fabsf(p[1]+188.65f)>30){next(s,9,p);return;}next(s,20,p);
    }else if(s->stage==20){if(walk(s,ghost_route,sizeof ghost_route/sizeof *ghost_route,p,yaw,raw))next(s,21,p);}
    else if(s->stage==21){
        if(self&&!ghost&&!dead){s->loot_seen=1;next(s,22,p);}
        else if(ghost&&g->corpse_known&&age>90){
            if(steer(s,g->corpse_position[0],g->corpse_position[1],p,yaw,raw,30))pulse(raw,WX_A,age);
        }
    }else if(s->stage==22&&age>180&&self){
        if(ghost||dead||g->kills||self->xp!=s->initial_xp||self->fields[34]!=s->initial_level||inv.count!=s->initial_count||inv.money!=s->initial_money){next(s,9,p);return;}
        raw->buttons=1u<<WX_START;next(s,6,p);
    }else if(s->stage==6){if(age==12)raw->buttons=1u<<WX_BACK;if(age>30*25&&!w->active){memcpy(s->return_points[1],p,12);next(s,7,p);}}
    else if(s->stage==7){if(age==12)raw->buttons=1u<<WX_BACK;
        if(w->active&&self&&age>180&&w->equipment_ready_mask==0x7ffff){
            int good=!ghost&&!dead&&s->loot_seen&&self->xp==s->initial_xp&&self->fields[34]==s->initial_level&&inv.count==s->initial_count&&inv.money==s->initial_money;
            for(unsigned i=0;i<3;i++)if(fabsf(p[i]-s->return_points[1][i])>.01f)good=0;raw->buttons=1u<<WX_START;next(s,good?8:9,p);
        }
    }
    if(lobby->phase==WX_LOBBY_FAILED||(s->stage!=8&&s->stage!=9&&s->frame>30*900))next(s,9,p);
}
