#include "wx_lobby.h"
#include "wx_input.h"
#include "wx_replay.h"
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"PREVIEW REPLAY FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char** argv){
    CHECK(argc==4);WxReplay replay;CHECK(wx_replay_open(&replay,argv[1])&&replay.header.count>256);
    FILE* plan=fopen(argv[3],"w");CHECK(plan);static WxLooks looks;
    fprintf(plan,"{\"version\":1,\"records\":%u,\"frames\":%u,\"samples\":[\n",replay.header.count,replay.header.frames);
    WxControls controls;wx_controls_default(&controls);WxCharacterUi ui={0};WxCharacterLobby lobby={0};
    unsigned previous=0,menu=0,entered=0,frames=0,seen[9][12][2]={0},changed[9][2][5]={0},samples=0,randomized[9][2]={0};
    lobby.characters.count=5;for(unsigned i=0;i<5;i++){lobby.characters.items[i].guid=i+1;lobby.characters.items[i].race=1;lobby.characters.items[i].character_class=1;}
    while(replay.active){
        WxRawPad raw={0};WxPad pad;wx_replay_apply(&replay,&raw);wx_input_process(&raw,&controls,&previous,&pad);frames++;
        if(lobby.phase==WX_LOBBY_CLOSED){
            if(pad.pressed&(1u<<WX_START))menu^=1;
            if(menu&&(pad.pressed&(1u<<WX_Y))){lobby.phase=WX_LOBBY_READY;menu=0;wx_character_ui_sync(&ui,&lobby);}
            continue;
        }
        WxLobbyCommand command;wx_character_ui_sync(&ui,&lobby);
        if(!looks.file||looks.header.race!=ui.draft.race||looks.header.sex!=ui.draft.gender){
            wx_looks_close(&looks);char path[512];snprintf(path,sizeof path,"%s/L%02X%02X.WXL",argv[2],ui.draft.race,ui.draft.gender);
            CHECK(wx_looks_open(&looks,path));
        }
        ui.looks=&looks;
        if(wx_character_ui_input(&ui,&lobby,&pad,&command)){
            CHECK(command.kind==WX_LOBBY_SELECT&&command.guid==1);entered++;lobby.phase=WX_LOBBY_CLOSED;
        }
        if(ui.screen==WX_CHARACTER_CREATE){
            CHECK(wx_character_class_allowed(ui.draft.race,ui.draft.character_class)&&ui.draft.gender<2);
            seen[ui.draft.race][ui.draft.character_class][ui.draft.gender]=1;
        }
        if(ui.screen==WX_CHARACTER_APPEARANCE){
            uint32_t v[]={ui.draft.race,ui.draft.gender,ui.draft.skin,ui.draft.face,ui.draft.hair_style,ui.draft.hair_color,ui.draft.facial_hair};
            CHECK(wx_looks_valid(&looks,v));
            if(pad.pressed&(1u<<WX_RIGHT)){
                CHECK(v[2+ui.appearance_row]!=0&&!changed[v[0]][v[1]][ui.appearance_row]);
                changed[v[0]][v[1]][ui.appearance_row]=1;
                fprintf(plan,"%s{\"identity\":%u,\"row\":%u,\"look\":%u,\"facial\":%u,\"frame\":%u}",
                    samples++?",\n":"",v[0]|(v[1]<<8),ui.appearance_row,v[2]|(v[3]<<8)|(v[4]<<16)|(v[5]<<24),v[6],replay.frame);
            }
            if(pad.pressed&(1u<<WX_X)){
                CHECK(ui.randomizations==(v[0]-1)*2+v[1]+1);
                randomized[v[0]][v[1]]=1;
            }
        }
        CHECK(frames<=108001);
    }
    unsigned count=0;for(unsigned r=1;r<=8;r++)for(unsigned c=1;c<=11;c++)for(unsigned sex=0;sex<2;sex++)count+=seen[r][c][sex];
    CHECK(count==80&&samples==80&&entered==1&&!replay.error);
    for(unsigned r=1;r<=8;r++)for(unsigned sex=0;sex<2;sex++){CHECK(randomized[r][sex]);for(unsigned field=0;field<5;field++)CHECK(changed[r][sex][field]);}
    fprintf(plan,"\n],\"scope\":\"Expected controller-selected looks; host UI only, no native rendering claim\"}\n");CHECK(!fclose(plan));wx_looks_close(&looks);
    printf("Combined input trace: %u records / %u frames; all 80 drafts, 80 customization changes, 16 randomizations, keyboard cancel, original selection, no creation/deletion commands. Host UI replay only.\n",replay.header.count,replay.header.frames);
    return 0;
}
