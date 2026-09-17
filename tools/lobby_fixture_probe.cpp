// Public fixture credentials/key only. No account config file is read here.
#include "wx_world.h"
#include "wx_lobby.h"
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <thread>
extern "C" int wx_random_bytes(uint8_t* out,size_t count){return BCryptGenRandom(nullptr,out,(ULONG)count,BCRYPT_USE_SYSTEM_PREFERRED_RNG)==0;}
int main(int argc,char** argv){
    if(argc!=3)return 2;unsigned port=strtoul(argv[1],nullptr,10);if(!port||port>65535)return 2;
    WSADATA startup;if(WSAStartup(MAKEWORD(2,2),&startup))return 2;
    WxWorldSession session{};strcpy(session.host,"127.0.0.1");strcpy(session.username,"FIXTURE");session.port=port;
    for(unsigned i=0;i<40;i++)session.key[i]=(uint8_t)(i+1);
    std::atomic<int> done{0},checks{0};bool create=strstr(argv[2],"create")!=nullptr,retry=!strcmp(argv[2],"create_retry");
    bool deleting=!strncmp(argv[2],"delete_",7),recover=!strncmp(argv[2],"login_recover_",14);
    const uint64_t delete_guid=0x123456789abcdef0ULL;
    wx_world_characters_open();
    std::thread input([&](){unsigned state=0,until=GetTickCount()+45000;
        while(!done&&GetTickCount()<until){WxCharacterLobby lobby{};wx_world_characters(&lobby);
            if(lobby.phase==WX_LOBBY_READY){WxLobbyCommand command{};
                if(state==0){
                    command.kind=WX_LOBBY_SELECT;command.guid=9999;if(wx_world_character_command(&command)){checks=-1;return;}checks++;
                    command.kind=WX_LOBBY_DELETE;if(wx_world_character_command(&command)){checks=-1;return;}checks++;
                    command={};command.kind=WX_LOBBY_CREATE;strcpy(command.draft.name,"x");command.draft.race=command.draft.character_class=1;
                    if(wx_world_character_command(&command)){checks=-1;return;}checks++;
                    command={};
                    if(!strcmp(argv[2],"cancel"))command.kind=WX_LOBBY_CANCEL;
                    else if(!strcmp(argv[2],"refresh_bad"))command.kind=WX_LOBBY_REFRESH;
                    else if(create){command.kind=WX_LOBBY_CREATE;strcpy(command.draft.name,"Newhero");command.draft.race=command.draft.character_class=1;}
                    else if(deleting){command.kind=WX_LOBBY_DELETE;command.guid=delete_guid;}
                    else if(recover){command.kind=WX_LOBBY_SELECT;command.guid=1;}
                    else{command.kind=WX_LOBBY_SELECT;command.guid=2;}
                    if(!strcmp(argv[2],"keepalive")){for(unsigned i=0;i<310&&!done;i++)Sleep(100);if(done)return;}
                    if(!wx_world_character_command(&command)){checks=-1;return;}checks++;state=1;
                    if(wx_world_character_command(&command)){checks=-1;return;}checks++;
                }else if(deleting&&state==1&&lobby.result_revision){
                    unsigned wanted=!strcmp(argv[2],"delete_refused")?0x3a:!strcmp(argv[2],"delete_transfer")?0x3b:0x39;
                    bool present=wx_character_find(&lobby.characters,delete_guid)!=nullptr;
                    if(lobby.result_kind!=WX_LOBBY_DELETE||lobby.result!=wanted||present!=(wanted!=0x39)||lobby.pending_kind){checks=-1;return;}
                    command.kind=WX_LOBBY_CANCEL;if(!wx_world_character_command(&command)){checks=-1;return;}checks++;state=2;
                }else if(recover&&state==1&&lobby.result_revision){
                    unsigned wanted=strtoul(argv[2]+14,nullptr,16);
                    if(lobby.result_kind!=WX_LOBBY_SELECT||lobby.result!=wanted||lobby.preferred!=1||lobby.pending_kind){checks=-1;return;}
                    command.kind=WX_LOBBY_SELECT;command.guid=2;if(!wx_world_character_command(&command)){checks=-1;return;}checks++;state=2;
                }else if(create&&lobby.result_revision){
                    if(retry&&state==1){if(lobby.result!=0x31){checks=-1;return;}
                        command.kind=WX_LOBBY_CREATE;strcpy(command.draft.name,"Goodname");command.draft.race=command.draft.character_class=1;
                        if(!wx_world_character_command(&command)){checks=-1;return;}checks++;state=2;
                    }else if(lobby.result==0x2e){
                        if(lobby.characters.count!=1||lobby.characters.items[0].guid!=2){checks=-1;return;}
                        command.kind=WX_LOBBY_SELECT;command.guid=2;if(!wx_world_character_command(&command)){checks=-1;return;}checks++;state=3;
                    }
                }
            }Sleep(1);
        }
    });
    int result=wx_world_live(&session);done=1;input.join();WxWorldView view{};wx_world_view(&view);WxCharacterLobby lobby{};wx_world_characters(&lobby);
    if(checks<0)result=0;
    if(result&&strcmp(argv[2],"cancel")&&!deleting&&(view.guid!=2||lobby.phase!=WX_LOBBY_CLOSED))result=0;
    if(deleting&&!result&&(lobby.result_kind!=WX_LOBBY_DELETE||lobby.result!=WX_CHAR_DELETE_UNCONFIRMED)){checks=-1;}
    printf("Character fixture: result=%d checks=%d phase=%u guid=%llu roster=%u code=%u\n",result,checks.load(),lobby.phase,(unsigned long long)view.guid,lobby.characters.count,lobby.result);
    return checks<0?2:result?0:1;
}
