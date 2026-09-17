// Fixed public test key; this executable never reads real account credentials.
#include "wx_world.h"
#include "wx_journal.h"
#include "wx_game.h"
#include "wx_cooldown.h"
#include "wx_cast.h"
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
extern "C" int wx_random_bytes(uint8_t* out,size_t count){return BCryptGenRandom(nullptr,out,(ULONG)count,BCRYPT_USE_SYSTEM_PREFERRED_RNG)==0;}
int main(int argc,char** argv){
    if(argc!=2&&argc!=3)return 2;unsigned port=strtoul(argv[1],nullptr,10);if(!port||port>65535)return 2;
    WSADATA startup;if(WSAStartup(MAKEWORD(2,2),&startup))return 2;
    WxWorldSession session{};strcpy(session.host,"127.0.0.1");strcpy(session.username,"FIXTURE");session.port=port;
    for(unsigned i=0;i<40;i++)session.key[i]=(uint8_t)(i+1);
    if(argc==3&&!strcmp(argv[2],"--short-key"))session.key[39]=0;
    bool teleport=argc==3&&!strcmp(argv[2],"--teleport");std::atomic<int> movement_checked{0};std::thread producer;
    bool appearance=argc==3&&!strcmp(argv[2],"--appearance");std::atomic<int> appearance_checked{0};
    bool journal=argc==3&&!strcmp(argv[2],"--journal");std::atomic<int> journal_checked{0};
    bool actions=argc==3&&!strcmp(argv[2],"--actions");std::atomic<int> actions_checked{0};
    bool cooldown=argc==3&&!strcmp(argv[2],"--cooldowns");std::atomic<int> cooldown_checked{0};
    bool gcd=argc==3&&!strcmp(argv[2],"--gcd");std::atomic<int> gcd_checked{0};
    bool clock_test=argc==3&&!strcmp(argv[2],"--clock");std::atomic<int> clock_checked{0};
    bool weather_test=argc==3&&!strcmp(argv[2],"--weather");std::atomic<int> weather_checked{0};
    if(weather_test)producer=std::thread([&](){
        uint32_t until=GetTickCount()+8000;unsigned step=0;
        const unsigned types[]={1,1,2,3,0,1,0,3},sounds[]={8533,8535,8538,8558,0,8535,0,8556},instant[]={0,0,1,0,0,1,1,1};
        const float grades[]={.25f,.9f,.65f,.8f,0,1,0,.5f};
        while(GetTickCount()<until&&step<8){
            if(!strcmp(wx_world_status(),"session failed"))return;
            WxWorldView current{};wx_world_view(&current);WxWeather weather{};wx_world_weather(&weather);
            if(current.active&&current.position_revision==(step<6?1:step==6?2:3)&&weather.valid==(step!=6)&&
                weather.type==types[step]&&weather.grade==grades[step]&&weather.sound==sounds[step]&&weather.instant==instant[step]){
                WxMovement move{};move.x=-8900+(float)step;move.y=-160;move.z=82;move.jump_cos=1;move.time_ms=GetTickCount();
                if(!wx_world_submit(&move,current.position_revision)){weather_checked=-1;return;}weather_checked=++step;
            }Sleep(1);
        }
    });
    if(clock_test)producer=std::thread([&](){
        uint32_t until=GetTickCount()+6000;unsigned step=0;
        while(GetTickCount()<until&&step<2){
            if(!strcmp(wx_world_status(),"session failed"))return;
            WxWorldView current{};wx_world_view(&current);WxWorldClock clock{};wx_world_clock(&clock);
            if(current.active&&clock.valid&&clock.minutes==(step?390:1439)&&clock.speed==0){
                WxMovement move{};move.x=-8900+(float)step;move.y=-160;move.z=82;move.jump_cos=1;move.time_ms=GetTickCount();
                if(!wx_world_submit(&move,current.position_revision)){clock_checked=-1;return;}clock_checked=++step;
            }Sleep(1);
        }
    });
    bool casts=argc==3&&!strcmp(argv[2],"--casts");std::atomic<int> casts_checked{0};
    if(casts)producer=std::thread([&](){
        uint32_t until=GetTickCount()+8000;unsigned step=0;
        const unsigned revisions[]={1,2,3,4,5,6,7,9,10,11,12};
        const unsigned phases[]={WX_CAST_PREPARING,WX_CAST_PREPARING,WX_CAST_INTERRUPTED,WX_CAST_PREPARING,WX_CAST_INTERRUPTED,WX_CAST_CHANNEL,WX_CAST_COMPLETE,WX_CAST_COMPLETE,WX_CAST_CHANNEL,WX_CAST_CHANNEL,WX_CAST_COMPLETE};
        while(GetTickCount()<until&&step<11){
            if(!strcmp(wx_world_status(),"session failed"))return;
            WxWorldView current{};wx_world_view(&current);WxCastView cast{};wx_world_cast(&cast);
            if(current.active&&cast.revision==revisions[step]){
                bool good=cast.phase==phases[step];
                if(step==1)good=good&&cast.duration==3500&&cast.delay==500;
                if(step==8)good=good&&cast.infinite&&cast.remaining==UINT32_MAX;
                if(step==9)good=good&&!cast.infinite&&cast.duration==1000&&cast.remaining>500;
                if(!good){fprintf(stderr,"Cast stage %u failed: phase %u, revision %u\n",step,cast.phase,cast.revision);casts_checked=-1;return;}
                if(step==3||step==5){WxCommand cancel{};if(!wx_cast_cancel(&cast,&cancel)||!wx_world_command(&cancel)){casts_checked=-1;return;}}
                else {WxMovement move{};move.x=-8900+(float)step;move.y=-160;move.z=82;move.jump_cos=1;move.time_ms=GetTickCount();if(!wx_world_submit(&move,current.position_revision)){casts_checked=-1;return;}}
                casts_checked=++step;
            }Sleep(1);
        }
    });
    static WxCooldownInfo gcd_info[2]{};static WxCooldownCatalog gcd_catalog{};
    if(gcd){
        for(unsigned i=0;i<2;i++){gcd_info[i].spell=i?133:116;gcd_info[i].gcd_category=133;gcd_info[i].gcd_time=1500;gcd_info[i].family=4;gcd_info[i].family_mask[0]=1;gcd_info[i].damage_class=1;}
        gcd_catalog.rows=gcd_info;gcd_catalog.count=2;gcd_catalog.ready=1;gcd_catalog.version=2;wx_world_cooldown_catalog(&gcd_catalog);
        producer=std::thread([&](){
            uint32_t until=GetTickCount()+5000;unsigned step=0;const uint32_t bindings[8]={133,116};const unsigned packets[6]={1,4,6,7,8,9};
            while(GetTickCount()<until&&step<6){
                if(!strcmp(wx_world_status(),"session failed"))return;
                WxWorldView current{};wx_world_view(&current);WxCooldownView timers[8]{};unsigned metrics[12]{};wx_world_cooldowns(bindings,timers,metrics);
                if(current.active&&metrics[0]==packets[step]){
                    bool good=!metrics[2]&&!metrics[3]&&!metrics[7]&&metrics[10]==500&&metrics[11]==4;
                    if(step==0)good=good&&timers[0].duration==1000&&timers[0].global&&timers[1].global;
                    else if(step<5)good=good&&timers[0].duration==1200&&timers[0].remaining>400&&timers[0].global&&metrics[6]==1&&metrics[9]==1;
                    if(step==3)good=good&&!timers[1].global&&timers[1].duration==7000&&timers[1].remaining>6000;
                    if(step==4)good=good&&timers[1].global;
                    if(step==5)good=good&&!timers[0].remaining&&!timers[1].remaining&&metrics[6]==1;
                    if(!good){fprintf(stderr,"GCD stage %u failed: packets %u, missing %u, overflow %u, ambiguous %u, haste %u, family %u, timer %u/%u global %u\n",step,metrics[0],metrics[2],metrics[3],metrics[7],metrics[10],metrics[11],timers[0].remaining,timers[0].duration,timers[0].global);gcd_checked=-1;return;}
                    WxMovement m{};m.x=-8900+(float)step;m.y=-160;m.z=82;m.jump_cos=1;m.time_ms=GetTickCount();
                    if(!wx_world_submit(&m,current.position_revision)){gcd_checked=-1;return;}gcd_checked=++step;
                }Sleep(1);
            }
        });
    }
    if(cooldown)producer=std::thread([&](){
        uint32_t until=GetTickCount()+5000;unsigned step=0;const uint32_t bindings[8]={78};
        while(GetTickCount()<until&&step<3){
            if(!strcmp(wx_world_status(),"session failed"))return;
            WxWorldView current{};wx_world_view(&current);WxCooldownView timers[8]{};unsigned metrics[12]{};
            wx_world_cooldowns(bindings,timers,metrics);
            if(current.active&&metrics[0]==step+1){
                bool okay=!metrics[2]&&!metrics[3]&&(step==0?timers[0].remaining>4000:step==1?timers[0].remaining>9000:!timers[0].remaining&&!timers[0].held);
                if(!okay){cooldown_checked=-1;return;}
                WxMovement m{};m.x=-8900+(float)step;m.y=-160;m.z=82;m.jump_cos=1;m.time_ms=GetTickCount();
                if(!wx_world_submit(&m,current.position_revision)){cooldown_checked=-1;return;}
                cooldown_checked=++step;
            }Sleep(1);
        }
    });
    if(actions)producer=std::thread([&](){
        uint32_t until=GetTickCount()+5000;unsigned step=0;bool pending=false;
        const uint32_t desired[]={78,0,6603};
        while(GetTickCount()<until&&step<3){WxWorldView current{};wx_world_view(&current);WxGame state{};wx_world_game(&state);
            if(!strcmp(wx_world_status(),"session failed"))return;
            if(current.active&&state.actions_ready&&state.spell_count==2){
                if(!pending){
                    WxCommand bad{0x128,0,9999,23};if(wx_world_command(&bad)){actions_checked=-1;return;}
                    WxCommand edit{0x128,0,desired[step],23};if(!wx_world_command(&edit)){actions_checked=-1;return;}pending=true;
                }else if(state.actions[23]==desired[step]){actions_checked=++step;pending=false;
                    if(step==3){WxMovement m{};m.x=-8900;m.y=-160;m.z=82;m.jump_cos=1;m.time_ms=GetTickCount();
                        if(!wx_world_submit(&m,current.position_revision))actions_checked=-1;return;}}
            }Sleep(1);
        }
    });
    if(journal)producer=std::thread([&](){
        uint32_t until=GetTickCount()+5000;bool sent=false;
        while(GetTickCount()<until){WxWorldView current{};wx_world_view(&current);
            if(!strcmp(wx_world_status(),"session failed"))return;
            if(current.active&&!sent){WxCommand q{0x5c,0,7,0};sent=wx_world_command(&q);}
            WxQuestInfo q{};if(sent&&wx_world_quest(7,&q)){
                if(strcmp(q.title,"Fixture quest")||q.creatures[0][0]!=6||q.creatures[0][1]!=10||q.money!=25){journal_checked=-1;return;}
                WxMovement m{};m.x=-8900;m.y=-160;m.z=82;m.jump_cos=1;m.time_ms=GetTickCount();
                journal_checked=wx_world_submit(&m,current.position_revision)?1:-1;return;
            }Sleep(1);
        }
    });
    if(appearance)producer=std::thread([&](){
        uint32_t until=GetTickCount()+10000;unsigned stage=0;
        while(GetTickCount()<until&&stage<4){WxWorldView current{};wx_world_view(&current);
            bool ready=current.active&&current.equipment_ready_mask==0x7ffff;
            if(stage==0)ready=ready&&current.equipment_entry[16]==2362&&current.equipment_display[16]==18730&&current.equipment_type[16]==14&&current.skin==7&&current.facial_hair==11;
            if(stage==1)ready=ready&&!current.equipment_entry[16]&&!current.equipment_display[16]&&!current.equipment_type[16];
            if(stage==2)ready=ready&&current.equipment_entry[16]==25&&current.equipment_display[16]==1542&&current.equipment_type[16]==21;
            if(stage==3)ready=ready&&!current.skin&&!current.face&&!current.hair_style&&!current.hair_color&&!current.facial_hair&&current.appearance_flags==0x400;
            if(ready){
                WxMovement m{};m.x=-8900+(float)stage;m.y=-160;m.z=82;m.jump_cos=1;m.time_ms=GetTickCount();
                if(!wx_world_submit(&m,current.position_revision)){appearance_checked=-1;return;}
                appearance_checked=++stage;
            }Sleep(1);
        }
    });
    if(teleport)producer=std::thread([&](){
        uint32_t until=GetTickCount()+4000;
        while(GetTickCount()<until){WxWorldView current{};wx_world_view(&current);
            if(current.active&&current.position_revision==2){
                WxMovement stale{};stale.x=-8949.95f;stale.y=-132.493f;stale.z=83.531f;stale.jump_cos=1;
                if(wx_world_submit(&stale,1)){movement_checked=-1;return;}
                WxMovement fresh=stale;fresh.x=current.x;fresh.y=current.y;fresh.z=current.z;fresh.orientation=current.orientation;fresh.time_ms=GetTickCount();
                movement_checked=wx_world_submit(&fresh,current.position_revision)?1:-1;return;
            }Sleep(1);
        }
    });
    int result=argc==3&&(!strcmp(argv[2],"--live")||teleport||appearance||journal||actions||cooldown||gcd||casts||clock_test||weather_test)?wx_world_live(&session):wx_world_probe(&session);
    if(producer.joinable())producer.join();if(teleport&&movement_checked!=1)result=0;
    if(appearance&&appearance_checked!=4)result=0;
    if(journal&&journal_checked!=1)result=0;
    if(actions&&actions_checked!=3)result=0;
    if(cooldown&&cooldown_checked!=3)result=0;
    if(gcd&&gcd_checked!=6)result=0;
    if(casts&&casts_checked!=11)result=0;
    if(clock_test){WxWorldClock clock{};wx_world_clock(&clock);if(clock_checked!=2||clock.valid)result=0;}
    if(weather_test){WxWeather weather{};wx_world_weather(&weather);if(weather_checked!=8||weather.valid||weather.sound||weather.grade)result=0;}
    WxWorldView view{};wx_world_view(&view);
    if(result&&!appearance&&view.revision&&(view.skin!=2||view.face!=3||view.hair_style!=4||view.hair_color!=5||view.facial_hair!=6||view.equipment_display[19]!=0x12345||view.equipment_type[19]!=18))result=0;
    printf("World fixture: %s; position revision %u; %.2f %.2f %.2f\n",wx_world_status(),view.position_revision,view.x,view.y,view.z);return result?0:1;
}
