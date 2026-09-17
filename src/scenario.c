#include "wx_scenario.h"
#include "wx_inventory.h"
#include <string.h>
#include <math.h>
void wx_scenario_init(WxScenario* s,const char* path){
    memset(s,0,sizeof *s);FILE* file=fopen(path,"rb");if(!file)return;
    char magic[4];if(fread(magic,1,4,file)==4&&!memcmp(magic,"WXC",3)&&fgetc(file)==EOF){
        if(magic[3]>='1'&&magic[3]<='9')s->enabled=magic[3]-'0';else if(magic[3]=='A')s->enabled=10;else if(magic[3]=='B')s->enabled=11;else if(magic[3]=='C')s->enabled=12;else if(magic[3]=='D')s->enabled=13;else if(magic[3]=='E')s->enabled=14;else if(magic[3]=='F')s->enabled=15;else if(magic[3]=='G')s->enabled=16;else if(magic[3]=='H')s->enabled=17;}fclose(file);
    if(s->enabled==11)wx_journey_load_route(s,"D:\\CAMP.RTE");
    if(!s->enabled)return;
    if(s->enabled==17){
        file=fopen("D:\\SOAK.BIN","rb");unsigned config[2]={0};
        int good=file&&fread(config,1,8,file)==8&&fgetc(file)==EOF&&config[0]==0x31535857&&config[1]>=60&&config[1]<=3600;
        if(file)fclose(file);if(!good){s->stage=9;return;}s->soak_limit_ms=config[1]*1000;
    }
    file=fopen("D:\\TESTCHAR.BIN","rb");if(!file)return;
    char config[17];size_t n=fread(config,1,sizeof config,file);int end=fgetc(file);fclose(file);
    if(n==4&&!memcmp(config,"NONE",4)&&end==EOF)return;
    int good=n==sizeof config&&end==EOF&&!memcmp(config,"WXCN",4)&&memchr(config+4,0,13);
    if(good){unsigned length=(unsigned)strlen(config+4);good=length>=2&&length<=12;
        for(unsigned i=0;i<length;i++)if(!((config[4+i]>='A'&&config[4+i]<='Z')||(config[4+i]>='a'&&config[4+i]<='z')))good=0;
    }
    if(good)memcpy(s->test_name,config+4,13);else s->stage=9;
}
const char* wx_scenario_status(const WxScenario* s){
    if(s->enabled==17)return s->stage==50?"checking menus":wx_journey_status(s);
    if(s->enabled==16)return wx_death_scenario_status(s);
    if(s->enabled==12)return s->stage==8?"passed":s->stage==9?"FAILED":s->stage>=12?"restoring original binding":s->stage>=10?"checking saved assignment":"browsing and assigning spell";
    if(s->enabled==10||s->enabled==11||s->enabled==14||s->enabled==15)return wx_journey_status(s);
    if(s->enabled==9||s->enabled==13){switch(s->stage){
        case 0:return "waiting for world";case 1:return "checking saved character";case 2:return "opening character screen";
        case 3:return "choosing new character";case 4:return "opening name keyboard";case 5:return "typing name";
        case 6:return "reviewing creation";case 7:return "selecting test character";case 10:return "checking new character";
        case 11:return "returning to character screen";case 12:return "selecting original character";
        case 13:return "checking saved progress";case 8:return "passed";default:return "FAILED";}}
    static const char* labels[]={"waiting for world","selecting creature","approaching target","melee combat","opening loot","taking loot / recovering","logging out","reconnecting","passed","FAILED"};
    static const char* boundary[]={"waiting for world","walking across tile boundary","crossed / inspecting terrain","","","","logging out","reconnecting","passed","FAILED"};
    static const char* turnin[]={"waiting for world","inspecting questgiver","opening completed quest","reading reward","claiming reward","checking quest completion","logging out","reconnecting","passed","FAILED"};
    static const char* death[]={"waiting for world","selecting creature","provoking creature","waiting for death","releasing spirit","seeking Spirit Healer","logging out","reconnecting","passed","FAILED"};
    static const char* vendor[]={"waiting for world","selecting merchant","reading stock","buying","choosing item to sell","selling / verifying","logging out","reconnecting","passed","FAILED"};
    static const char* appearance[]={"waiting for world","checking equipped item","storing offhand item","checking removed appearance","equipping offhand item","checking restored appearance","logging out","reconnecting","passed","FAILED"};
    static const char* camera[]={"waiting for world","waiting for assets","turning through four views","looking down","looking up","restoring view","","","passed","FAILED"};
    if(s->enabled==8)return camera[s->stage<10?s->stage:9];
    if(s->enabled==7)return appearance[s->stage<10?s->stage:9];
    if(s->enabled==6)return vendor[s->stage<10?s->stage:9];
    if(s->enabled==3)return boundary[s->stage<10?s->stage:9];
    if(s->enabled==4)return turnin[s->stage<10?s->stage:9];
    if(s->enabled==5)return death[s->stage<10?s->stage:9];
    return labels[s->stage<10?s->stage:9];
}
static void stage(WxScenario* s,unsigned next){s->stage=next;s->stage_frame=s->frame;}
static void pulse(WxRawPad* raw,unsigned button,unsigned age){if(age%12==0)raw->buttons=1u<<button;}
static int16_t camera_axis(float delta,float rate){
    float strength=fminf(fabsf(delta)*30/rate,1);return (int16_t)((delta<0?-1:1)*(.18f+.82f*strength)*32767);
}
static void choose_character(WxRawPad* raw,const WxCharacterLobby* lobby,const WxCharacterUi* ui,uint64_t guid,unsigned age){
    if(lobby->phase!=WX_LOBBY_READY||ui->screen!=WX_CHARACTER_LIST||age%8)return;
    unsigned row=0;while(row<lobby->characters.count&&lobby->characters.items[row].guid!=guid)row++;
    if(row==lobby->characters.count)return;
    raw->buttons=1u<<(ui->selected<row?WX_DOWN:ui->selected>row?WX_UP:WX_A);
}
void wx_scenario_input(WxScenario* s,const WxWorldView* world,const WxEntity* entities,unsigned count,
    const float* p,float yaw,float pitch,uint64_t selected,const WxGame* game,const WxCharacterLobby* lobby,const WxCharacterUi* ui,WxPad* pad){
    if(!s->enabled||s->enabled==12)return;
    WxRawPad raw={0};raw.connected=1;WxControls controls;wx_controls_default(&controls);
    if(!s->stage){if(world->active){s->frame=0;stage(s,1);}else goto input;}
    s->frame++;unsigned age=s->frame-s->stage_frame;
    if(s->enabled==17){wx_soak_scenario_input(s,world,entities,count,p,yaw,selected,game,lobby,ui,&raw);goto input;}
    if(s->enabled==16){wx_death_scenario_input(s,world,entities,count,p,yaw,selected,game,lobby,ui,&raw);goto input;}
    if(s->enabled==10||s->enabled==11||s->enabled==14||s->enabled==15){wx_journey_input(s,world,entities,count,p,yaw,selected,game,lobby,ui,&raw);goto input;}
    if(s->enabled==9||s->enabled==13){
        const char* name=s->test_name[0]?s->test_name:s->enabled==13?"Xboxdawn":"Xboxnight";const WxEntity* self=NULL;
        for(unsigned i=0;i<count;i++)if(entities[i].guid==world->guid)self=&entities[i];
        if(s->stage==1&&age>180&&self){
            s->target=world->guid;s->initial_xp=self->xp;s->initial_level=self->fields[34];memcpy(s->probe_position,p,12);
            WxInventory inv;wx_world_inventory(&inv);s->initial_count=inv.count;s->initial_money=inv.money;
            raw.buttons=1u<<WX_START;stage(s,2);
        }else if(s->stage==2){
            if(age==12)raw.buttons=1u<<WX_Y;
            if(lobby->phase==WX_LOBBY_READY)stage(s,3);
        }else if(s->stage==3&&lobby->phase==WX_LOBBY_READY){
            for(unsigned i=0;i<lobby->characters.count;i++)if(!strcmp(lobby->characters.items[i].name,name))s->created_character=lobby->characters.items[i].guid;
            if(s->created_character)stage(s,7);
            else if(age>30){s->loot_seen=1;raw.buttons=1u<<WX_X;stage(s,4);}
        }else if(s->stage==4&&ui->screen==WX_CHARACTER_CREATE&&age>30){raw.buttons=1u<<WX_A;stage(s,5);}
        else if(s->stage==5&&ui->screen==WX_CHARACTER_KEYBOARD&&age%8==0){
            unsigned n=(unsigned)strlen(ui->draft.name);
            if(n==strlen(name)){raw.buttons=1u<<WX_START;stage(s,6);}
            else if(n<strlen(name)){
                unsigned key=(unsigned)((name[n]>='A'&&name[n]<='Z'?name[n]+32:name[n])-'a');
                if(ui->key==key)raw.buttons=1u<<WX_A;
                else if(ui->key/7<key/7)raw.buttons=1u<<WX_DOWN;
                else if(ui->key/7>key/7)raw.buttons=1u<<WX_UP;
                else raw.buttons=1u<<(ui->key<key?WX_RIGHT:WX_LEFT);
            }else stage(s,9);
        }else if(s->stage==6&&age%8==0){
            if(ui->screen==WX_CHARACTER_CREATE){
                if(s->enabled==13&&ui->row==3&&!ui->draft.gender)raw.buttons=1u<<WX_RIGHT;
                else raw.buttons=1u<<(ui->row<4?WX_DOWN:WX_A);
            }
            else if(ui->screen==WX_CHARACTER_CONFIRM){raw.buttons=1u<<WX_A;stage(s,7);}
        }else if(s->stage==7){
            if(lobby->phase==WX_LOBBY_READY){
                for(unsigned i=0;i<lobby->characters.count;i++)if(!strcmp(lobby->characters.items[i].name,name))s->created_character=lobby->characters.items[i].guid;
                choose_character(&raw,lobby,ui,s->created_character,age);
            }
            if(world->active&&s->created_character&&world->guid==s->created_character)stage(s,10);
        }else if(s->stage==10&&s->enabled==13&&age==1){
            raw.buttons=1u<<WX_RSTICK; // Unobstructed visual inspection during the hold.
        }else if(s->stage==10&&age>(s->enabled==13?900u:180u)&&self){
            if(world->race!=1||world->character_class!=1||world->gender!=(s->enabled==13?1:0)||(s->loot_seen&&(self->fields[34]!=1||self->xp))){stage(s,9);goto input;}
            raw.buttons=1u<<WX_START;stage(s,11);
        }else if(s->stage==11){
            if(age==12)raw.buttons=1u<<WX_Y;
            if(lobby->phase==WX_LOBBY_READY)stage(s,12);
        }else if(s->stage==12){choose_character(&raw,lobby,ui,s->target,age);
            if(world->active&&world->guid==s->target)stage(s,13);
        }else if(s->stage==13&&age>180&&self&&world->equipment_ready_mask==0x7ffff){
            WxInventory inv;wx_world_inventory(&inv);int good=self->xp==s->initial_xp&&self->fields[34]==s->initial_level&&inv.money==s->initial_money&&inv.count==s->initial_count;
            for(unsigned i=0;i<3;i++)if(fabsf(p[i]-s->probe_position[i])>.01f)good=0;stage(s,good?8:9);
        }
        if(lobby->phase==WX_LOBBY_FAILED||(s->stage!=8&&s->stage!=9&&s->frame>30*360))stage(s,9);
        goto input;
    }
    if(s->enabled==8){
        if(s->stage==1&&age>180){s->probe_position[0]=yaw;s->probe_position[1]=pitch;s->detours=s->detour_until=0;stage(s,2);}
        else if(s->stage==2){
            float desired=s->probe_position[0]+(s->detours+1)*1.570796327f;
            float delta=atan2f(sinf(desired-yaw),cosf(desired-yaw));
            if(fabsf(delta)>.002f){raw.axes[2]=camera_axis(delta,2);s->detour_until=0;}
            else if(!s->detour_until)s->detour_until=s->frame+60;
            else if(s->frame>=s->detour_until){s->detour_until=0;if(++s->detours==4)stage(s,3);}
        }else if(s->stage>=3&&s->stage<=5){
            float desired=s->stage==3?-.65f:s->stage==4?.5f:s->probe_position[1],delta=desired-pitch;
            if(fabsf(delta)>.002f){raw.axes[3]=-camera_axis(delta,1.3f);s->detour_until=0;}
            else if(!s->detour_until)s->detour_until=s->frame+90;
            else if(s->frame>=s->detour_until){s->detour_until=0;stage(s,s->stage==5?8:s->stage+1);}
        }
        if(s->stage<8&&s->frame>30*120)stage(s,9);goto input;
    }
    if(s->enabled==7){
        WxInventory inventory;wx_world_inventory(&inventory);
        const WxInventoryItem* item=0;
        for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].guid==s->target)item=&inventory.items[i];
        if(s->stage==1&&age>120&&world->equipment_ready_mask==0x7ffff&&inventory.count){
            if(!world->equipment_entry[16]||!world->equipment_display[16]){stage(s,9);goto input;}
            unsigned occupied=0;
            for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].bag==255){
                if(inventory.items[i].slot>=23)occupied++;
                if(inventory.items[i].slot==16)s->target=inventory.items[i].guid;
            }
            if(!s->target||occupied>=16){stage(s,9);goto input;}
            s->trade_item=world->equipment_entry[16];s->trade_price=world->equipment_display[16];s->trade_bundle=world->equipment_type[16];
            s->initial_money=inventory.money;s->initial_count=inventory.count;s->loot_seen=0;
            raw.buttons=1u<<WX_BLACK;stage(s,2);
        }else if(s->stage==2){
            if(age==12)raw.buttons=1u<<WX_Y;
            if(age>60&&age%12==0){
                unsigned row=0;int found=0;
                for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].bag==255&&inventory.items[i].slot<23){
                    if(inventory.items[i].guid==s->target){found=1;break;}row++;
                }
                if(!found){stage(s,9);goto input;}
                if(s->loot_seen<row){raw.buttons=1u<<WX_DOWN;s->loot_seen++;}
                else {raw.buttons=1u<<WX_X;stage(s,3);}
            }
        }else if(s->stage==3&&age>120&&item){
            if(!world->equipment_entry[16]&&!world->equipment_display[16]&&(world->equipment_ready_mask&(1u<<16))&&
                (item->bag!=255||item->slot>=23)){
                raw.buttons=1u<<WX_Y;s->loot_seen=0;stage(s,4);
            }
        }else if(s->stage==4&&age>60&&age%12==0){
            unsigned row=0;int found=0;
            for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].bag!=255||inventory.items[i].slot>=23){
                if(inventory.items[i].guid==s->target){found=1;break;}row++;
            }
            if(!found){stage(s,9);goto input;}
            if(s->loot_seen<row){raw.buttons=1u<<WX_DOWN;s->loot_seen++;}
            else {raw.buttons=1u<<WX_A;stage(s,5);}
        }else if(s->stage==5&&age>180&&item&&item->bag==255&&item->slot==16&&
            world->equipment_entry[16]==s->trade_item&&world->equipment_display[16]==s->trade_price&&world->equipment_type[16]==s->trade_bundle){
            raw.buttons=1u<<WX_B;stage(s,6);
        }else if(s->stage==6){
            if(age==12)raw.buttons=1u<<WX_START;if(age==24)raw.buttons=1u<<WX_BACK;
            if(age>30*25&&!world->active)stage(s,7);
        }else if(s->stage==7){
            if(age==12)raw.buttons=1u<<WX_BACK;
            if(world->active&&world->revision>=2&&world->equipment_ready_mask==0x7ffff&&inventory.count&&age>180){
                raw.buttons=1u<<WX_START;
                stage(s,world->equipment_entry[16]==s->trade_item&&world->equipment_display[16]==s->trade_price&&
                    world->equipment_type[16]==s->trade_bundle&&inventory.money==s->initial_money&&inventory.count==s->initial_count?8:9);
            }
        }
        if(s->stage<8&&s->frame>30*180)stage(s,9);goto input;
    }
    if(s->enabled==6){
        WxInventory inventory;wx_world_inventory(&inventory);const WxDialog* d=&game->dialog;
        unsigned quantity=0;for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].entry==s->trade_item)quantity+=inventory.items[i].count;
        if(s->stage==1&&age>120){
            if(!s->target)for(unsigned i=0;i<count;i++)if(entities[i].type==3&&entities[i].fields[3]==152){s->target=entities[i].guid;break;}
            if(d->screen==WX_SCREEN_VENDOR)stage(s,2);
            else if(s->target&&selected==s->target)pulse(&raw,WX_X,age);
            else if(s->target)pulse(&raw,WX_Y,age);
        }else if(s->stage==2&&age>180&&d->vendor_count&&inventory.count){
            unsigned row=0;while(row<d->vendor_count&&d->vendor[row].item!=159)row++;
            if(row==d->vendor_count){stage(s,9);goto input;}
            const WxVendorItem* item=&d->vendor[row];
            if(item->price>inventory.money){stage(s,9);goto input;}
            if(s->loot_seen<row){if(age%12==0){raw.buttons=1u<<WX_DOWN;s->loot_seen++;}goto input;}
            if(strcmp(wx_item_name(game,item->item),"Loading item name...")){
                s->trade_item=item->item;s->trade_bundle=item->count;s->initial_money=inventory.money;s->trade_price=item->price;
                s->initial_count=0;for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].entry==s->trade_item)s->initial_count+=inventory.items[i].count;
                raw.buttons=1u<<WX_A;stage(s,3);
            }
        }else if(s->stage==3){
            if(game->bundles_bought&&quantity==s->initial_count+s->trade_bundle){raw.buttons=1u<<WX_Y;s->loot_seen=0;stage(s,4);}
            else if(age==180)raw.buttons=1u<<WX_A;
        }else if(s->stage==4&&age>120){
            unsigned row=0;int found=0;for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].bag!=255||inventory.items[i].slot>=23){
                if(inventory.items[i].entry==s->trade_item){found=1;break;}row++;}
            if(!found){stage(s,9);goto input;}
            if(age%12==0){if(s->loot_seen<row){raw.buttons=1u<<WX_DOWN;s->loot_seen++;}else {raw.buttons=1u<<WX_A;stage(s,5);}}
        }else if(s->stage==5&&age>=180){
            if(game->items_sold&&quantity==s->initial_count+s->trade_bundle-1&&inventory.money>s->initial_money-s->trade_price){
                s->expected_money=inventory.money;
                if(d->screen)pulse(&raw,WX_B,age);else {raw.buttons=1u<<WX_START;stage(s,6);}
            }else if(age==180)raw.buttons=1u<<WX_A;
        }else if(s->stage==6){if(age==12)raw.buttons=1u<<WX_BACK;if(age>30*25&&!world->active)stage(s,7);}
        else if(s->stage==7){if(age==12)raw.buttons=1u<<WX_BACK;
            if(world->active&&world->revision>=2&&inventory.count&&age>180){raw.buttons=1u<<WX_START;
                stage(s,quantity==s->initial_count+s->trade_bundle-1&&inventory.money==s->expected_money?8:9);}}
        if(s->stage<8&&s->frame>30*180)stage(s,9);goto input;
    }
    if(s->enabled==5){
        const WxEntity *self=0,*target=0;for(unsigned i=0;i<count;i++){
            if(entities[i].guid==world->guid)self=&entities[i];if(entities[i].guid==s->target)target=&entities[i];}
        int ghost=self&&(self->fields[190]&16),dead=self&&!self->fields[22];
        if(s->stage==1&&age>120){
            if(self&&!s->initial_level){s->initial_level=self->fields[34];s->initial_xp=self->xp;}
            if(!s->target){float nearest=60*60;for(unsigned i=0;i<count;i++)if(entities[i].type==3&&entities[i].fields[3]==6&&entities[i].fields[22]){
                float dx=entities[i].x-p[0],dy=entities[i].y-p[1],d=dx*dx+dy*dy;if(d<nearest){nearest=d;s->target=entities[i].guid;}}}
            if(s->target&&selected==s->target)stage(s,2);else if(s->target)pulse(&raw,WX_Y,age);
        }else if(s->stage==2){
            if(!target){stage(s,9);goto input;}
            float dx=target->x-p[0],dy=target->y-p[1],distance=sqrtf(dx*dx+dy*dy),difference=remainderf(atan2f(dy,dx)-yaw,6.283185307f);
            if(fabsf(difference)>.05f)raw.axes[2]=(difference<0?-1:1)*18000;
            if(distance>2.5f&&fabsf(difference)<.25f)raw.axes[1]=-26000;
            if(distance<3.4f&&fabsf(difference)<.2f&&!game->attack_target)pulse(&raw,WX_X,age);
            if(game->damage_dealt){raw.buttons=1u<<WX_B;stage(s,3);}
        }else if(s->stage==3){if(dead)stage(s,4);}
        else if(s->stage==4){
            if(ghost){s->target=0;s->loot_seen=0;stage(s,5);}
            else if(age>180)pulse(&raw,WX_A,age);
        }else if(s->stage==5){
            if(self&&!ghost&&!dead){s->loot_seen=4;raw.buttons=1u<<WX_START;stage(s,6);}
            else if(ghost&&world->active){
                if(!s->target){float nearest=100*100;for(unsigned i=0;i<count;i++)if(entities[i].type==3&&(entities[i].fields[147]&0x20)){
                    float dx=entities[i].x-p[0],dy=entities[i].y-p[1],d=dx*dx+dy*dy;if(d<nearest){nearest=d;s->target=entities[i].guid;}}}
                if(s->target&&selected!=s->target)pulse(&raw,WX_Y,age);
                else if(target&&selected==s->target){
                    float dx=target->x-p[0],dy=target->y-p[1],distance=sqrtf(dx*dx+dy*dy),difference=remainderf(atan2f(dy,dx)-yaw,6.283185307f);
                    if(fabsf(difference)>.05f)raw.axes[2]=(difference<0?-1:1)*16000;
                    if(distance>4&&fabsf(difference)<.25f)raw.axes[1]=-20000;
                    if(distance<4.5f){
                        if(!s->loot_seen){raw.buttons=1u<<WX_X;s->loot_seen=1;s->probe_frame=s->frame;}
                        else if(s->frame-s->probe_frame>180)pulse(&raw,WX_A,age);
                    }
                }
            }
        }else if(s->stage==6){if(age==12)raw.buttons=1u<<WX_BACK;if(age>30*25&&!world->active)stage(s,7);}
        else if(s->stage==7){if(age==12)raw.buttons=1u<<WX_BACK;
            if(world->active&&world->revision>=2&&self){raw.buttons=1u<<WX_START;
                stage(s,!ghost&&!dead&&s->loot_seen==4&&self->fields[34]>=s->initial_level?8:9);}}
        if(s->stage<8&&s->frame>30*600)stage(s,9);goto input;
    }
    if(s->enabled==4){
        // Quest 7 must already have its ten kills from normal gameplay. This
        // scenario selects its actual quest-list row and uses the normal UI.
        const WxEntity* self=0;for(unsigned i=0;i<count;i++)if(entities[i].guid==world->guid)self=&entities[i];
        const WxDialog* d=&game->dialog;
        if(s->stage==1){
            if(self&&!s->initial_level){s->initial_xp=self->xp;s->initial_level=self->fields[34];}
            if(age>300){raw.buttons=1u<<WX_X;stage(s,2);}
        }else if(s->stage==2){
            if(d->screen==WX_SCREEN_QUESTS){
                unsigned index=0;while(index<d->quest_count&&d->quests[index].id!=7)index++;
                if(index==d->quest_count){stage(s,9);goto input;}
                if(age>120&&age%12==0){if(s->loot_seen<index){raw.buttons=1u<<WX_DOWN;s->loot_seen++;}else raw.buttons=1u<<WX_A;}
            }else if((d->screen==WX_SCREEN_REQUEST||d->screen==WX_SCREEN_REWARD)&&d->quest==7)stage(s,3);
        }else if(s->stage==3&&age>450){
            if(d->screen==WX_SCREEN_REQUEST&&d->can_complete)pulse(&raw,WX_A,age);
            if(d->screen==WX_SCREEN_REWARD&&d->quest==7)stage(s,4);
        }else if(s->stage==4&&age>450){
            if(game->completed_quest==7)stage(s,5);else if(d->screen==WX_SCREEN_REWARD&&d->quest==7)pulse(&raw,WX_A,age);
        }else if(s->stage==5&&age>180){
            if(d->screen)pulse(&raw,WX_B,age);else {raw.buttons=1u<<WX_START;stage(s,6);}
        }else if(s->stage==6){if(age==12)raw.buttons=1u<<WX_BACK;if(age>30*25&&!world->active)stage(s,7);}
        else if(s->stage==7){
            if(age==12)raw.buttons=1u<<WX_BACK;
            if(world->active&&world->revision>=2&&self){
                unsigned q=0;while(q<20&&self->quests[q*3]!=7)q++;
                if(q==20&&(self->fields[34]>s->initial_level||self->xp>s->initial_xp)){raw.buttons=1u<<WX_START;stage(s,8);}
                else if(age>150)stage(s,9);
            }
        }
        if(s->stage<8&&s->frame>30*160)stage(s,9);goto input;
    }
    if(s->enabled==3){
        // Walk south across the 48/49 terrain boundary, then verify persistence.
        if(s->stage==1){
            if(s->frame==1)raw.buttons=1u<<WX_RSTICK;
            float difference=remainderf(3.14159265f-yaw,6.283185307f);
            if(fabsf(difference)>.04f)raw.axes[2]=(difference<0?-1:1)*16000;
            if(fabsf(difference)<.15f)raw.axes[1]=-32767;
            if(p[0]<-9080){raw.axes[1]=0;stage(s,2);}
            if(s->frame>30*40||!world->active)stage(s,9);
        }else if(s->stage==2&&age>300){raw.buttons=1u<<WX_START;stage(s,6);}
        else if(s->stage==6){if(age==12)raw.buttons=1u<<WX_BACK;if(age>30*25&&!world->active)stage(s,7);}
        else if(s->stage==7){if(age==12)raw.buttons=1u<<WX_BACK;if(world->active&&world->revision>=2){raw.buttons=1u<<WX_START;stage(s,p[0]<-9080?8:9);}}
        if(s->stage<8&&s->frame>30*150)stage(s,9);goto input;
    }
    if(s->stage<8&&s->frame>30*(s->enabled==2?900:240))stage(s,9);
    const WxEntity *self=0,*target=0;
    for(unsigned i=0;i<count;i++){if(entities[i].guid==world->guid)self=&entities[i];if(entities[i].guid==s->target)target=&entities[i];}
    if(s->stage>1&&s->stage<6&&(!world->active||(self&&!self->fields[22])))stage(s,9);
    if(s->stage==1&&s->frame==1)raw.buttons=1u<<WX_RSTICK;
    if(s->stage==1&&age>90){
        if(!s->target){float nearest=s->enabled==2?70*70:40*40;
            if(s->enabled==2&&self){unsigned q=0;while(q<20&&self->quests[q*3]!=7)q++;if(q==20){stage(s,9);goto input;}}
            for(unsigned i=0;i<count;i++)if(entities[i].type==3&&entities[i].fields[3]==(s->enabled==2?6u:299u)&&entities[i].fields[22]){
                float dx=entities[i].x-p[0],dy=entities[i].y-p[1],dz=entities[i].z-p[2],d=dx*dx+dy*dy+dz*dz;
                if(d<nearest){nearest=d;s->target=entities[i].guid;}
            }
            if(self){s->initial_xp=self->xp;s->initial_level=self->fields[34];}s->initial_kills=game->kills;
        }
        if(s->target==selected&&s->target){stage(s,2);s->probe_frame=s->frame;s->detours=s->detour_until=0;memcpy(s->probe_position,p,sizeof s->probe_position);}else if(s->target)pulse(&raw,WX_Y,age);
    }else if(s->stage==2||s->stage==3){
        if(!target){stage(s,9);goto input;}
        float dx=target->x-p[0],dy=target->y-p[1],distance=sqrtf(dx*dx+dy*dy);
        float difference=remainderf(atan2f(dy,dx)-yaw,6.283185307f);
        if(fabsf(difference)>.04f)raw.axes[2]=(difference<0?-1:1)*(int)(32767*fminf(1,.18f+fabsf(difference)*2));
        // Keep the creature visible from the first-person eye height.
        float desired=-atan2f(2.3f,fmaxf(distance,.1f));if(desired<-.85f)desired=-.85f;
        float tilt=desired-pitch;if(fabsf(tilt)>.04f)raw.axes[3]=(tilt<0?1:-1)*15000;
        if(distance>2.5f&&fabsf(difference)<.45f)raw.axes[1]=-26000;
        // Recover from a small obstacle using ordinary strafe input. This is a
        // bounded test-driver detour, not game pathfinding or position injection.
        if(s->stage==2&&distance>3.4f&&s->frame-s->probe_frame>60){
            float mx=p[0]-s->probe_position[0],my=p[1]-s->probe_position[1];
            if(mx*mx+my*my<.25f){s->detour_until=s->frame+50;s->detours++;if(s->detours>8)stage(s,9);}
            memcpy(s->probe_position,p,sizeof s->probe_position);s->probe_frame=s->frame;
        }
        if(s->stage==2&&s->frame<s->detour_until){raw.axes[0]=(s->detours%2?1:-1)*26000;raw.axes[1]=-9000;}
        if(distance<3.4f&&fabsf(difference)<.2f){if(s->stage==2)stage(s,3);if(!game->attack_target)pulse(&raw,WX_X,age);}
        if(s->stage==3&&self&&self->fields[24]>=150&&age%90==0&&game->attack_target){raw.axes[4]=32767;raw.buttons=1u<<WX_B;}
        if(game->kills>s->initial_kills&&!target->fields[22])stage(s,4);
    }else if(s->stage==4){
        if(game->dialog.screen==WX_SCREEN_LOOT){s->loot_seen=1;stage(s,5);}else pulse(&raw,WX_X,age);
    }else if(s->stage==5){
        // Hold the real loot dialog for evidence before taking each item.
        if(age>(s->enabled==2?(game->kills==1?300u:30u):900u)){
            if(game->dialog.screen==WX_SCREEN_LOOT&&(game->dialog.money||game->dialog.loot_count))pulse(&raw,WX_A,age);
            else if(game->dialog.screen)pulse(&raw,WX_B,age);
            else if(self&&(self->fields[34]>s->initial_level||self->xp>s->initial_xp)&&s->loot_seen){
                if(!game->looted_items||(s->enabled==2&&game->kills<10)){
                    if(game->kills>=(s->enabled==2?15u:5u))stage(s,9);
                    else if(self->fields[22]>=self->fields[28]){s->target=0;stage(s,1);}
                }else {raw.buttons=1u<<WX_START;stage(s,6);}
            }
            else stage(s,9);
        }
    }else if(s->stage==6){
        if(age==12)raw.buttons=1u<<WX_BACK;
        if(age>30*25&&!world->active)stage(s,7);
    }else if(s->stage==7){
        if(age==12)raw.buttons=1u<<WX_BACK;
        if(world->active&&world->revision>=2&&self){
            if(self->fields[34]>s->initial_level||self->xp>s->initial_xp){raw.buttons=1u<<WX_START;stage(s,8);}else stage(s,9);
        }
    }
input:wx_input_process(&raw,&controls,&s->previous,pad);
}
