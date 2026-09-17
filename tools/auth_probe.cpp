#include "wx_auth.h"
#include "wx_world.h"
#include "wx_game.h"
#include "wx_inventory.h"
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <cstdlib>
static uint8_t fixture_entropy[19];static bool fixture_rng=false;
extern "C" int wx_random_bytes(uint8_t* out,size_t count){
    if(fixture_rng){if(count!=19)return 0;memcpy(out,fixture_entropy,19);return 1;}
    return BCryptGenRandom(nullptr,out,(ULONG)count,BCRYPT_USE_SYSTEM_PREFERRED_RNG)==0;
}
int main(int argc,char** argv){
    if(argc<2||argc>3){fprintf(stderr,"Usage: wowx_auth_probe <local config binary> [--world|--move|--commands|--quest]\n");return 2;}
    FILE* file=fopen(argv[1],"rb");WxAuthConfig config{};
    if(!file||fread(&config,1,sizeof config,file)!=sizeof config){fprintf(stderr,"Missing auth config\n");return 2;}
    fclose(file);strcpy(config.host,"127.0.0.1");
    // Deterministic public protocol fixtures only; never part of the Xbox RNG.
    if(argc==3&&!strncmp(argv[2],"--fixture-a=",12)){
        if(strcmp(config.username,"FIXTURE")||strcmp(config.password,"DISPOSABLE")||strlen(argv[2]+12)!=38)return 2;
        for(unsigned i=0;i<19;i++){char byte[3]={argv[2][12+i*2],argv[2][13+i*2],0};char* end=nullptr;
            fixture_entropy[i]=(uint8_t)strtoul(byte,&end,16);if(*end)return 2;}
        fixture_rng=true;
    }
    std::thread cancellation;
    if(argc==3&&!strcmp(argv[2],"--cancel-auth"))cancellation=std::thread([](){Sleep(200);wx_auth_cancel();});
    int result=wx_auth_run(&config);
    if(cancellation.joinable())cancellation.join();
    memset(config.password,0,sizeof config.password);
    printf("Vanilla auth probe: %s\n",wx_auth_status());
    if(result){
        static WxRealms realms;
        if(!wx_auth_realms(&realms))return 1;
        unsigned available=0;
        for(unsigned i=0;i<realms.count;i++){
            WxWorldSession selected{},before{};memset(&selected,0xa5,sizeof selected);before=selected;
            int good=wx_auth_select(i,&selected);
            if(good!=wx_realm_available(&realms.items[i]))return 1;
            if(good){if(strcmp(selected.host,realms.items[i].host)||selected.port!=realms.items[i].port)return 1;available++;}
            else if(memcmp(&before,&selected,sizeof selected))return 1;
            memset(selected.key,0,sizeof selected.key);memset(before.key,0,sizeof before.key);
        }
        WxWorldSession invalid{};if(wx_auth_select(realms.count,&invalid)||wx_auth_select(0,nullptr))return 1;
        printf("Realm list: %u entries / %u available; selection verified\n",realms.count,available);
    }
    if(result&&argc==3&&(!strcmp(argv[2],"--world")||!strcmp(argv[2],"--move")||!strcmp(argv[2],"--commands")||!strcmp(argv[2],"--quest"))){
        WxWorldSession session{};result=wx_auth_session(&session);strcpy(session.host,"127.0.0.1");
        bool quest=argc==3&&!strcmp(argv[2],"--quest");
        bool commands=argc==3&&(!strcmp(argv[2],"--commands")||quest);bool scenario_passed=!commands;
        if(result&&(!strcmp(argv[2],"--move")||commands)){
            std::thread input([&](){WxWorldView view{};uint32_t start=GetTickCount();
                while(GetTickCount()-start<15000){wx_world_view(&view);if(view.active)break;Sleep(10);}
                if(!view.active)return;
                printf("World entry: %.4f %.4f %.4f\n",view.x,view.y,view.z);
                for(unsigned i=0;i<=20;i++){WxMovement m{};m.x=view.x+(commands?0:i*.025f);m.y=view.y;m.z=view.z;m.orientation=0;m.time_ms=GetTickCount();
                    m.flags=i<20?WX_MOVE_FORWARD:0;m.jump_cos=1;wx_world_submit(&m,view.position_revision);Sleep(100);}
                Sleep(250);WxEntity entities[128];unsigned count=wx_world_entities(entities,128);
                printf("Entity records: %u\n",count);
                for(unsigned i=0;i<count;i++){float scale;memcpy(&scale,&entities[i].fields[4],4);
                    printf("entity type=%u entry=%u display=%u npc=%u faction=%u health=%u scale=%.3f pos=%.3f,%.3f,%.3f\n",
                    entities[i].type,entities[i].fields[3],entities[i].fields[131],entities[i].fields[147],entities[i].fields[35],entities[i].fields[22],scale,entities[i].x,entities[i].y,entities[i].z);}
                WxInventory inventory;wx_world_inventory(&inventory);WxGame names;wx_world_game(&names);
                printf("Known spells: %u; first actions: %u %u %u\n",names.spell_count,names.actions[0],names.actions[1],names.actions[2]);
                printf("Inventory: %u items, %u copper\n",inventory.count,inventory.money);
                for(unsigned i=0;i<inventory.count;i++){const auto& item=inventory.items[i];printf("inventory bag=%u slot=%u entry=%u count=%u name=%s\n",item.bag,item.slot,item.entry,item.count,wx_item_name(&names,item.entry));}
                if(commands){
                    WxEntity* target=nullptr;float nearest=1e20f;
                    for(unsigned i=0;i<count;i++)if(entities[i].type==3&&(entities[i].fields[147]&2)){
                        float dx=entities[i].x-view.x,dy=entities[i].y-view.y,dz=entities[i].z-view.z,d=dx*dx+dy*dy+dz*dz;
                        if(d<nearest){target=&entities[i];nearest=d;}
                    }
                    if(target){
                        WxCommand select{0x13d,target->guid,0,0},query{0x60,target->guid,target->fields[3],0},attack{0x141,target->guid,0,0},stop{0x142,0,0,0};
                        bool sent=wx_world_command(&select)&&wx_world_command(&query)&&wx_world_command(&attack);
                        Sleep(2000);WxGame state;wx_world_game(&state);
                        printf("Command response: name=%s entry=%u active_attack=%llu message=%s\n",state.target_name,state.name_entry,(unsigned long long)state.attack_target,state.message);
                        scenario_passed=sent&&state.name_entry==target->fields[3]&&state.target_name[0]&&!state.attack_target&&state.event_revision>=2;
                        wx_world_command(&stop);
                        if(quest){
                            WxCommand hello{0x184,target->guid,0,0};wx_world_command(&hello);Sleep(2000);wx_world_game(&state);
                            printf("Quest response: screen=%u quest=%u choices=%u title=%s\n",state.dialog.screen,state.dialog.quest,state.dialog.quest_count,state.dialog.title);
                            for(unsigned i=0;i<state.dialog.quest_count;i++)printf("quest option %u: %u %s\n",i,state.dialog.quests[i].id,state.dialog.quests[i].title);
                            scenario_passed=scenario_passed&&state.dialog.screen!=0;
                            WxCommand cancel{0x190,0,0,0};wx_world_command(&cancel);
                        }
                    }
                }
                wx_world_logout();
            });
            result=wx_world_live(&session);input.join();if(!scenario_passed)result=0;
        }else if(result)result=wx_world_probe(&session);
        printf("World scenario: %s\n",wx_world_status());
    }
    return result?0:1;
}
