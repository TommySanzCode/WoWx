// Native development acceptance replay. All actions are ordinary pad samples.
// These Northshire waypoints are a test fixture; engine loading remains generic.
#include "wx_scenario.h"
#include "wx_inventory.h"
#include <math.h>
#include <string.h>
static const float to_willem[][3]={{-8949.95f,-132.493f,83.5309f},{-8934,-140,83.227203f}};
static const float to_mcbride[][3]={
    {-8934,-140,83.227203f},{-8923.5f,-142.5f,81.250992f},{-8920,-141,80.830734f},
    {-8918.5f,-139.5f,81.257523f},{-8914.5f,-140.5f,81.926895f},
    {-8912.5f,-143,82.208252f},{-8905,-160,81.938919f}};
static const float from_mcbride[][3]={
    {-8905,-160,81.938919f},{-8912.5f,-143,82.208252f},{-8914.5f,-140.5f,81.926895f},
    {-8918.5f,-139.5f,81.257523f},{-8920,-141,80.830734f},{-8923.5f,-142.5f,81.250992f},
    {-8934,-140,83.227203f}};
// Populated from the host collision route checker, then verified in xemu.
static const float to_camp[][3]={
    {-8934,-140,83.227203f},{-8910.5f,-133.5f,80.512527f},
    {-8876,-146.5f,80.036224f},{-8803,-177,81.812775f}};
void wx_journey_load_route(WxScenario* s,const char* path){
    FILE* f=fopen(path,"rb");if(!f)return;char magic[4];unsigned n=0;
    int good=fread(magic,1,4,f)==4;
    if(good&&!memcmp(magic,"NONE",4)&&fgetc(f)==EOF){fclose(f);return;}
    good=good&&!memcmp(magic,"WXRP",4)&&fread(&n,4,1,f)==1&&n>=2&&n<=128;
    if(good)good=fread(s->return_points,12,n,f)==n&&fgetc(f)==EOF;
    fclose(f);
    for(unsigned i=0;good&&i<n;i++)for(unsigned k=0;k<3;k++)if(!isfinite(s->return_points[i][k])||fabsf(s->return_points[i][k])>20000)good=0;
    if(good)s->return_count=n;else s->stage=9;
}
static void stage(WxScenario* s,unsigned next){s->stage=next;s->stage_frame=s->frame;s->trade_bundle=0;s->trade_price=0;}
static unsigned quest(const WxEntity* self,unsigned id){if(self)for(unsigned q=0;q<20;q++)if(self->quests[q*3]==id)return q+1;return 0;}
static void pulse(WxRawPad* pad,unsigned key,unsigned age){if(age%15==0)pad->buttons=1u<<key;}
static unsigned nearest_point(const float (*path)[3],unsigned count,const float* p){
    unsigned best=0;float distance=1e30f;for(unsigned i=0;i<count;i++){float dx=p[0]-path[i][0],dy=p[1]-path[i][1],d=dx*dx+dy*dy;
        if(d<distance){distance=d;best=i;}}return best;
}
static int walk_to(WxScenario* s,const float (*path)[3],unsigned count,const float* p,float yaw,WxRawPad* raw,float tolerance){
    if(s->detours>=count)return 1;const float* goal=path[s->detours];float dx=goal[0]-p[0],dy=goal[1]-p[1],distance=sqrtf(dx*dx+dy*dy);
    if(distance<tolerance){if(fabsf(goal[2]-p[2])>1){stage(s,9);return 0;}s->detours++;s->probe_frame=s->frame;memcpy(s->probe_position,p,12);return s->detours==count;}
    float delta=atan2f(sinf(atan2f(dy,dx)-yaw),cosf(atan2f(dy,dx)-yaw));
    if(fabsf(delta)>.004f)raw->axes[2]=(int16_t)((delta<0?-1:1)*(.18f+.82f*fminf(1,fabsf(delta)*15))*32767);
    if(fabsf(delta)<(tolerance<.01f?.004f:.05f))raw->axes[1]=(int16_t)(-(.18f+.82f*fminf(.7f,distance*2))*32767);
    if(s->frame-s->probe_frame>180){float mx=p[0]-s->probe_position[0],my=p[1]-s->probe_position[1];
        if(mx*mx+my*my<.0025f){stage(s,9);return 0;}s->probe_frame=s->frame;memcpy(s->probe_position,p,12);}
    return 0;
}
static int walk(WxScenario* s,const float (*path)[3],unsigned count,const float* p,float yaw,WxRawPad* raw){return walk_to(s,path,count,p,yaw,raw,.055f);}
static void choose_quest(WxScenario* s,const WxGame* game,unsigned id,unsigned age,WxRawPad* raw){
    const WxDialog* d=&game->dialog;if(s->trade_bundle!=d->revision){s->trade_bundle=d->revision;s->trade_price=0;}
    if(d->screen==WX_SCREEN_QUESTS){unsigned row=0;while(row<d->quest_count&&d->quests[row].id!=id)row++;
        if(row==d->quest_count){stage(s,9);return;}
        if(age%15==0){if(s->trade_price<row){raw->buttons=1u<<WX_DOWN;s->trade_price++;}else raw->buttons=1u<<WX_A;}
    }else if(d->quest==id&&(d->screen==WX_SCREEN_DETAILS||d->screen==WX_SCREEN_REWARD||
        (d->screen==WX_SCREEN_REQUEST&&d->can_complete)))pulse(raw,WX_A,age);
}
static void interact(WxScenario* s,unsigned entry,const WxEntity* entities,unsigned count,uint64_t selected,unsigned age,WxRawPad* raw){
    for(unsigned i=0;i<count;i++)if(entities[i].type==3&&entities[i].fields[3]==entry){s->target=entities[i].guid;break;}
    if(!s->target)return;pulse(raw,selected==s->target?WX_X:WX_Y,age);
}
static unsigned kobolds(const WxEntity* self){unsigned q=quest(self,7);return q?(self->quests[(q-1)*3+1]&63):0;}
static int blocked_target(const WxScenario* s,uint64_t guid){for(unsigned i=0;i<8;i++)if(s->blocked_targets[i]==guid)return 1;return 0;}
static void route_start(WxScenario* s,unsigned next,const float* p){s->detours=0;s->probe_frame=s->frame;memcpy(s->probe_position,p,12);stage(s,next);}
static void camp_input(WxScenario* s,const WxWorldView* world,const WxEntity* entities,unsigned count,
    const WxEntity* self,const float* p,float yaw,uint64_t selected,const WxGame* game,WxRawPad* raw){
    unsigned age=s->frame-s->stage_frame;const WxEntity* target=0;
    if(s->enabled==14&&s->stage>=22&&s->stage<=26&&!wx_trail_record(&s->trail,p,0)){stage(s,9);return;}
    for(unsigned i=0;i<count;i++)if(entities[i].guid==s->target)target=&entities[i];
    if(s->stage==4&&age>120&&self){
        if(!quest(self,7)){stage(s,9);return;}
        s->initial_xp=self->xp;s->initial_level=self->fields[34];s->initial_kills=game->kills;
        s->initial_objectives=kobolds(self);
        raw->buttons=1u<<WX_START;stage(s,40);
    }else if(s->stage==40){
        if(age==12)raw->buttons=1u<<WX_A;
        if(age==90){raw->buttons=1u<<WX_A;stage(s,41);}
    }else if(s->stage==41&&age>600){raw->buttons=1u<<WX_X;stage(s,42);}
    else if(s->stage==42&&age>600){raw->buttons=1u<<WX_B;stage(s,43);}
    else if(s->stage==43){
        if(age==30)raw->buttons=1u<<WX_B;if(age<60)return;
        if(p[0]>-8860){s->target=0;stage(s,22);}
        else if(p[0]<-8920&&fabsf(p[1]+140)<15){route_start(s,21,p);s->detours=nearest_point(to_camp,sizeof to_camp/sizeof *to_camp,p);}
        else {route_start(s,20,p);s->detours=nearest_point(from_mcbride,7,p);}
    }else if(s->stage==20){if(walk(s,from_mcbride,7,p,yaw,raw))route_start(s,21,p);}
    else if(s->stage==21){if(walk(s,to_camp,sizeof to_camp/sizeof *to_camp,p,yaw,raw)){
        if(s->enabled==15)route_start(s,28,p);
        else {s->target=0;wx_trail_reset(&s->trail,p);stage(s,22);}
    }}
    else if(s->stage==22&&age>60&&self){
        if(kobolds(self)>=10){
            if(s->enabled==14&&!wx_trail_record(&s->trail,p,1)){stage(s,9);return;}
            route_start(s,27,p);return;
        }
        if(!s->target){float nearest=55*55;
            for(unsigned i=0;i<count;i++)if(entities[i].type==3&&entities[i].fields[3]==6&&entities[i].fields[22]&&!blocked_target(s,entities[i].guid)){
                float dx=entities[i].x-p[0],dy=entities[i].y-p[1],dz=entities[i].z-p[2],d=dx*dx+dy*dy+dz*dz;
                if(d<nearest){nearest=d;s->target=entities[i].guid;}}
        }
        if(s->target==selected&&s->target){route_start(s,23,p);s->initial_kills=game->kills;s->detour_until=0;}
        else if(s->target)pulse(raw,WX_Y,age);
    }else if(s->stage==23||s->stage==24){
        if(!target){stage(s,9);return;}
        float dx=target->x-p[0],dy=target->y-p[1],distance=sqrtf(dx*dx+dy*dy);
        float delta=atan2f(sinf(atan2f(dy,dx)-yaw),cosf(atan2f(dy,dx)-yaw));
        if(fabsf(delta)>.02f)raw->axes[2]=(int16_t)((delta<0?-1:1)*(.18f+.82f*fminf(1,fabsf(delta)*8))*32767);
        if(distance>2.6f&&fabsf(delta)<.25f)raw->axes[1]=-24000;
        if(s->stage==23&&distance>3.4f&&s->frame-s->probe_frame>90){
            float mx=p[0]-s->probe_position[0],my=p[1]-s->probe_position[1];
            if(mx*mx+my*my<.25f){s->detour_until=s->frame+45;if(++s->detours>2){
                s->blocked_targets[s->blocked_cursor++%8]=s->target;s->target=0;stage(s,22);return;}}
            memcpy(s->probe_position,p,12);s->probe_frame=s->frame;
        }
        if(s->stage==23&&s->frame<s->detour_until){raw->axes[0]=(s->detours%2?1:-1)*24000;raw->axes[1]=-9000;}
        if(distance<3.4f&&fabsf(delta)<.2f){if(s->stage==23)stage(s,24);if(!game->attack_target)pulse(raw,WX_X,age);}
        if(s->stage==24&&self&&self->fields[24]>=150&&age%90==0&&game->attack_target){raw->axes[4]=32767;raw->buttons=1u<<WX_B;}
        if(game->kills>s->initial_kills&&!target->fields[22]){raw->axes[1]=0;stage(s,25);}
    }else if(s->stage==25){
        if(game->dialog.screen==WX_SCREEN_LOOT){s->loot_seen=1;stage(s,26);}else pulse(raw,WX_X,age);
    }else if(s->stage==26&&age>60){
        if(game->dialog.screen==WX_SCREEN_LOOT&&(game->dialog.money||game->dialog.loot_count))pulse(raw,WX_A,age);
        else if(game->dialog.screen)pulse(raw,WX_B,age);
        else if(self&&self->fields[22]>=self->fields[28]){s->target=0;stage(s,22);}
    }else if(s->stage==27){
        // Rejoin the validated return route from the last normally fought mob.
        if(s->enabled==14){
            if(s->trail.failed||!s->trail.count){stage(s,9);return;}
            if(walk_to(s,s->trail.points+s->trail.count-1,1,p,yaw,raw,.004f)){
                s->detours=0;if(!--s->trail.count)route_start(s,28,p);
            }
        }else if(s->return_count){
            if(!s->detours){float dx=p[0]-s->return_points[0][0],dy=p[1]-s->return_points[0][1];
                if(dx*dx+dy*dy>1){stage(s,9);return;}}
            if(walk(s,s->return_points,s->return_count,p,yaw,raw))route_start(s,28,p);
        }else if(walk(s,to_camp+sizeof to_camp/sizeof *to_camp-1,1,p,yaw,raw))route_start(s,28,p);
    }else if(s->stage==28){
        float reverse[sizeof to_camp/sizeof *to_camp][3];unsigned n=sizeof to_camp/sizeof *to_camp;
        for(unsigned i=0;i<n;i++)memcpy(reverse[i],to_camp[n-i-1],12);
        if(walk(s,reverse,n,p,yaw,raw))route_start(s,29,p);
    }else if(s->stage==29){if(walk(s,to_mcbride,7,p,yaw,raw)){
        if(s->enabled==15)route_start(s,32,p);else {s->target=0;stage(s,30);}
    }}
    else if(s->stage==32){if(walk(s,s->return_points,1,p,yaw,raw))stage(s,33);}
    else if(s->stage==33){
        float delta=atan2f(sinf(s->return_points[1][0]-yaw),cosf(s->return_points[1][0]-yaw));
        if(fabsf(delta)>.004f)raw->axes[2]=(int16_t)((delta<0?-1:1)*(.18f+.82f*fminf(1,fabsf(delta)*15))*32767);
        else if(age>180&&self){
            WxInventory inv;wx_world_inventory(&inv);
            if(self->xp!=s->initial_xp||self->fields[34]!=s->initial_level||inv.count!=s->initial_count||inv.money!=s->initial_money){stage(s,9);return;}
            memcpy(s->probe_position,p,12);raw->buttons=1u<<WX_START;stage(s,6);
        }
    }
    else if(s->stage==30){
        if(game->completed_quest==7){if(game->dialog.screen)pulse(raw,WX_B,age);else stage(s,31);}
        else if(game->dialog.screen)choose_quest(s,game,7,age,raw);else interact(s,197,entities,count,selected,age,raw);
    }else if(s->stage==31&&age>120&&self){
        if(quest(self,7)||game->completed_quest!=7||(s->initial_objectives<10&&(!s->loot_seen||!game->looted_items))){stage(s,9);return;}
        WxInventory inv;wx_world_inventory(&inv);s->initial_xp=self->xp;s->initial_level=self->fields[34];
        s->initial_count=inv.count;s->initial_money=inv.money;memcpy(s->probe_position,p,12);raw->buttons=1u<<WX_START;stage(s,6);
    }
    if(!world->active&&s->stage>=20)stage(s,9);
}
const char* wx_journey_status(const WxScenario* s){switch(s->stage){
    case 0:return "waiting for world";case 1:return "opening character menu";case 2:return "loading roster";
    case 3:return "selecting fresh character";case 4:return "checking quest state";
    case 10:return "walking to Deputy Willem";case 11:return "accepting A Threat Within";
    case 12:return "walking into the Abbey";case 13:return "turning in A Threat Within";
    case 14:return "accepting Kobold Camp Cleanup";case 15:return "checking rewards";
    case 20:return "leaving the Abbey";case 21:return "walking to Kobold camp";
    case 22:return "selecting a Kobold";case 23:return "approaching Kobold";case 24:return "fighting Kobold";
    case 25:return "opening loot";case 26:return "looting and recovering";
    case 27:return "walking back from combat";case 28:return "returning to the Abbey";
    case 29:return "entering the Abbey";case 30:return "turning in Kobold Camp Cleanup";case 31:return "checking quest rewards";
    case 32:return "returning to saved position";case 33:return "restoring saved view";
    case 40:return "opening quest journal";case 41:return "reading quest objectives";
    case 42:return "reading quest description";case 43:return "closing quest journal";
    case 6:return "logging out";case 7:return "checking saved quest progress";case 8:return "passed";default:return "FAILED";}}
void wx_journey_input(WxScenario* s,const WxWorldView* world,const WxEntity* entities,unsigned count,
    const float* p,float yaw,uint64_t selected,const WxGame* game,const WxCharacterLobby* lobby,const WxCharacterUi* ui,WxRawPad* raw){
    unsigned age=s->frame-s->stage_frame;const WxEntity* self=0;for(unsigned i=0;i<count;i++)if(entities[i].guid==world->guid)self=&entities[i];
    WxInventory inventory;wx_world_inventory(&inventory);
    if(s->stage==1&&age==1)raw->buttons=1u<<WX_RSTICK;
    if(s->stage==1&&age>90){raw->buttons=1u<<WX_START;stage(s,2);}
    else if(s->stage==2){if(age==12)raw->buttons=1u<<WX_Y;if(lobby->phase==WX_LOBBY_READY)stage(s,3);}
    else if(s->stage==3){
        if(lobby->phase==WX_LOBBY_READY&&age%15==0){unsigned row=0;const char* name=s->test_name[0]?s->test_name:s->enabled==14?"Xboxdawn":"Xboxnight";
            while(row<lobby->characters.count&&strcmp(lobby->characters.items[row].name,name))row++;
            if(row==lobby->characters.count){stage(s,9);return;}s->created_character=lobby->characters.items[row].guid;
            raw->buttons=1u<<(ui->selected<row?WX_DOWN:ui->selected>row?WX_UP:WX_A);}
        if(world->active&&world->guid==s->created_character)stage(s,4);
    }else if(s->enabled==15&&s->stage==4&&age>120&&self&&inventory.count){
        float dx=p[0]+8905,dy=p[1]+160;
        if(dx*dx+dy*dy>1||fabsf(p[2]-81.938919f)>1||quest(self,783)||quest(self,7)){stage(s,9);return;}
        s->initial_xp=self->xp;s->initial_level=self->fields[34];s->initial_count=inventory.count;s->initial_money=inventory.money;
        memcpy(s->return_points[0],p,12);s->return_points[1][0]=yaw;route_start(s,20,p);
    }else if((s->enabled==11&&s->stage==4)||((s->enabled==11||s->enabled==14||s->enabled==15)&&s->stage>=20)){
        camp_input(s,world,entities,count,self,p,yaw,selected,game,raw);
    }else if(s->stage==4&&age>120&&self&&inventory.count){
        if(s->enabled==14&&(self->xp||self->fields[34]!=1||quest(self,7)||quest(self,783))){stage(s,9);return;}
        s->initial_xp=self->xp;s->initial_level=self->fields[34];s->target=0;
        if(quest(self,7))stage(s,15);
        else if(quest(self,783)||p[0]>-8910){s->detours=nearest_point(to_mcbride,7,p);stage(s,12);}
        else {s->detours=nearest_point(to_willem,2,p);stage(s,10);}
        s->probe_frame=s->frame;memcpy(s->probe_position,p,12);
    }else if(s->stage==10){if(walk(s,to_willem,2,p,yaw,raw)){s->target=0;stage(s,11);}}
    else if(s->stage==11){
        if(quest(self,783)){if(game->dialog.screen)pulse(raw,WX_B,age);else {s->detours=0;s->probe_frame=s->frame;memcpy(s->probe_position,p,12);s->target=0;stage(s,12);}}
        else if(game->dialog.screen)choose_quest(s,game,783,age,raw);else interact(s,823,entities,count,selected,age,raw);
    }else if(s->stage==12){if(walk(s,to_mcbride,7,p,yaw,raw)){s->target=0;stage(s,13);}}
    else if(s->stage==13){
        if(game->completed_quest==783||(!quest(self,783)&&self&&self->xp>=40)){if(game->dialog.screen)pulse(raw,WX_B,age);else {s->target=0;stage(s,14);}}
        else if(game->dialog.screen)choose_quest(s,game,783,age,raw);else interact(s,197,entities,count,selected,age,raw);
    }else if(s->stage==14){
        if(quest(self,7)){if(game->dialog.screen)pulse(raw,WX_B,age);else stage(s,15);}
        else if(game->dialog.screen)choose_quest(s,game,7,age,raw);else interact(s,197,entities,count,selected,age,raw);
    }else if(s->stage==15&&age>120&&self){
        if(!quest(self,7)||quest(self,783)||self->xp<40){stage(s,9);return;}
        if(s->enabled==14){
            s->initial_objectives=kobolds(self);s->initial_kills=game->kills;
            raw->buttons=1u<<WX_START;stage(s,40);return;
        }
        s->initial_xp=self->xp;s->initial_level=self->fields[34];s->initial_count=inventory.count;s->initial_money=inventory.money;
        memcpy(s->probe_position,p,12);raw->buttons=1u<<WX_START;stage(s,6);
    }else if(s->stage==6){if(age==12)raw->buttons=1u<<WX_BACK;if(age>30*25&&!world->active)stage(s,7);}
    else if(s->stage==7){if(age==12)raw->buttons=1u<<WX_BACK;
        if(age>180&&world->active&&self&&world->equipment_ready_mask==0x7ffff){
            int good=world->guid==s->created_character&&((s->enabled==11||s->enabled==14||s->enabled==15)?!quest(self,7):quest(self,7))&&!quest(self,783)&&self->xp==s->initial_xp&&self->fields[34]==s->initial_level&&
                inventory.count==s->initial_count&&inventory.money==s->initial_money;
            for(unsigned i=0;i<3;i++)if(fabsf(p[i]-s->probe_position[i])>.01f)good=0;raw->buttons=1u<<WX_START;stage(s,good?8:9);
        }
    }
    if(lobby->phase==WX_LOBBY_FAILED||(s->stage>4&&s->stage!=6&&s->stage!=7&&self&&!self->fields[22])||
        (s->stage!=8&&s->stage!=9&&s->frame>30*((s->enabled==11||s->enabled==14)?1800:600)))stage(s,9);
}
