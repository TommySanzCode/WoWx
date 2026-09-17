#include "wx_lobby.h"
#include "wx_input.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Character UI failed line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static WxCharacterUi ui;static WxCharacterLobby lobby;static WxLobbyCommand command;
static int press(unsigned button){WxPad p={0};p.pressed=1u<<button;return wx_character_ui_input(&ui,&lobby,&p,&command);}
static void appearance(void){
    static WxLooks options;memset(&ui,0,sizeof ui);memset(&lobby,0,sizeof lobby);lobby.phase=WX_LOBBY_READY;
    wx_character_ui_sync(&ui,&lobby);press(WX_X);press(WX_Y);CHECK(ui.screen==WX_CHARACTER_APPEARANCE);
    CHECK(!press(WX_RIGHT)&&strstr(ui.message,"not loaded"));
    CHECK(!press(WX_X)&&!ui.randomizations&&strstr(ui.message,"not loaded"));
    options.file=tmpfile();CHECK(options.file);options.header.race=1;options.header.count=10;
    unsigned keys[][3]={{0,0,0},{0,0,2},{1,0,0},{1,3,2},{3,0,0},{3,1,0},{5,0,0},{5,1,0},{6,0,0},{2,0,0}};
    for(unsigned i=0;i<10;i++){options.rows[i].kind=keys[i][0];options.rows[i].variation=keys[i][1];options.rows[i].color=keys[i][2];}
    ui.looks=&options;unsigned selected;CHECK(wx_character_appearance_choices(&ui,2,&selected)==2&&selected==0);
    CHECK(!press(WX_RIGHT)&&ui.draft.skin==2&&ui.draft.face==3&&!ui.message[0]);
    press(WX_DOWN);press(WX_DOWN);press(WX_RIGHT);CHECK(ui.draft.hair_style==1);
    CHECK(!press(WX_A)&&ui.screen==WX_CHARACTER_CREATE);strcpy(ui.draft.name,"Testhero");ui.row=4;press(WX_A);
    CHECK(press(WX_A)&&command.kind==WX_LOBBY_CREATE&&command.draft.skin==2&&command.draft.face==3&&command.draft.hair_style==1);
    uint8_t wire[22];CHECK(wx_character_create_encode(&command.draft,wire,sizeof wire)==18&&wire[12]==2&&wire[13]==3&&wire[14]==1);
    press(WX_B);ui.row=3;press(WX_RIGHT);CHECK(ui.draft.gender==1&&!ui.draft.skin&&!ui.draft.face&&!ui.draft.hair_style);
    press(WX_Y);CHECK(!press(WX_RIGHT)&&strstr(ui.message,"not loaded"));press(WX_B);CHECK(ui.screen==WX_CHARACTER_CREATE);
    ui.draft.gender=0;press(WX_Y);strcpy(ui.draft.name,"Keepname");ui.draft.character_class=2;ui.random_state=42;
    for(unsigned i=0;i<32;i++){
        CHECK(!press(WX_X)&&!command.kind&&ui.screen==WX_CHARACTER_APPEARANCE&&ui.randomizations==i+1);
        uint32_t v[]={ui.draft.race,ui.draft.gender,ui.draft.skin,ui.draft.face,ui.draft.hair_style,ui.draft.hair_color,ui.draft.facial_hair};
        CHECK(wx_looks_valid(&options,v)&&ui.draft.character_class==2&&!strcmp(ui.draft.name,"Keepname"));
    }
    WxPad both={0};both.pressed=(1u<<WX_A)|(1u<<WX_X);CHECK(!wx_character_ui_input(&ui,&lobby,&both,&command)&&ui.randomizations==32&&ui.screen==WX_CHARACTER_CREATE);
    wx_looks_close(&options);
}
static void deletion(void){
    memset(&ui,0,sizeof ui);memset(&lobby,0,sizeof lobby);
    lobby.phase=WX_LOBBY_READY;lobby.characters.count=2;
    lobby.characters.items[0].guid=0x123456789abcdef0ULL;strcpy(lobby.characters.items[0].name,"First");
    lobby.characters.items[1].guid=2;strcpy(lobby.characters.items[1].name,"Second");
    wx_character_ui_sync(&ui,&lobby);
    WxPad both={0};both.pressed=(1u<<WX_WHITE)|(1u<<WX_A);
    CHECK(!wx_character_ui_input(&ui,&lobby,&both,&command)&&ui.screen==WX_CHARACTER_DELETE&&!command.kind);
    CHECK(ui.delete_guid==0x123456789abcdef0ULL&&!strcmp(ui.delete_name,"First")&&!wx_character_delete_ready(&ui,&lobby));
    CHECK(!press(WX_A)&&ui.screen==WX_CHARACTER_DELETE_KEYBOARD);
    // The real keyboard must complete without submitting a request.
    for(unsigned i=0;i<6;i++){ui.key="DELETE"[i]-'A';CHECK(!press(WX_A)&&!command.kind);}
    CHECK(!strcmp(ui.delete_text,"DELETE"));ui.key=0;press(WX_A);CHECK(strlen(ui.delete_text)==6);
    CHECK(!press(WX_START)&&ui.screen==WX_CHARACTER_DELETE&&wx_character_delete_ready(&ui,&lobby));
    WxCharacter swap=lobby.characters.items[0];lobby.characters.items[0]=lobby.characters.items[1];lobby.characters.items[1]=swap;
    wx_character_ui_sync(&ui,&lobby);CHECK(ui.selected==1&&wx_character_delete_ready(&ui,&lobby));
    CHECK(press(WX_A)&&command.kind==WX_LOBBY_DELETE&&command.guid==0x123456789abcdef0ULL);
    both.layer=1;CHECK(!wx_character_ui_input(&ui,&lobby,&both,&command));
    lobby.phase=WX_LOBBY_PENDING;CHECK(!press(WX_A)&&!press(WX_B)&&!wx_character_delete_ready(&ui,&lobby));
    lobby.phase=WX_LOBBY_READY;lobby.result_kind=WX_LOBBY_DELETE;lobby.result=WX_CHAR_DELETE_FAILED;lobby.result_revision++;
    wx_character_ui_sync(&ui,&lobby);CHECK(ui.screen==WX_CHARACTER_LIST&&!ui.delete_guid&&strstr(ui.message,"Server refused"));
    press(WX_WHITE);strcpy(ui.delete_text,"DELETX");CHECK(!wx_character_delete_ready(&ui,&lobby));
    strcpy(ui.delete_text,"delete");CHECK(wx_character_delete_ready(&ui,&lobby));
    strcpy(lobby.characters.items[1].name,"Renamed");CHECK(!wx_character_delete_ready(&ui,&lobby));
    wx_character_ui_sync(&ui,&lobby);CHECK(ui.screen==WX_CHARACTER_LIST&&!ui.delete_guid&&strstr(ui.message,"list changed"));
    press(WX_WHITE);press(WX_A);ui.key=3;press(WX_A);CHECK(ui.delete_text[0]=='D');
    press(WX_B);CHECK(ui.screen==WX_CHARACTER_DELETE&&!ui.delete_text[0]);press(WX_B);CHECK(ui.screen==WX_CHARACTER_LIST);
    press(WX_WHITE);press(WX_A);lobby.characters.count=1;wx_character_ui_sync(&ui,&lobby);
    CHECK(ui.screen==WX_CHARACTER_LIST&&!ui.delete_guid&&ui.selected==0);
    for(unsigned result=WX_CHAR_DELETE_SUCCESS;result<=WX_CHAR_DELETE_TRANSFER;result++){
        lobby.result=result;lobby.result_revision++;wx_character_ui_sync(&ui,&lobby);
        CHECK(!strcmp(ui.message,wx_character_delete_result(result))&&ui.screen==WX_CHARACTER_LIST);
    }
    lobby.phase=WX_LOBBY_FAILED;lobby.result=WX_CHAR_DELETE_UNCONFIRMED;lobby.result_revision++;wx_character_ui_sync(&ui,&lobby);
    CHECK(strstr(ui.message,"not confirmed")&&!press(WX_A)&&press(WX_B)&&command.kind==WX_LOBBY_CANCEL);
    for(unsigned result=0x3e;result<=0x44;result++){
        ui.open=0;lobby.phase=WX_LOBBY_LOADING;lobby.result_kind=WX_LOBBY_SELECT;lobby.result=result;lobby.result_revision++;
        wx_character_ui_sync(&ui,&lobby);CHECK(ui.screen==WX_CHARACTER_LIST&&!strcmp(ui.message,wx_character_login_result(result)));
    }
    lobby.phase=WX_LOBBY_READY;lobby.characters.count=0;CHECK(!press(WX_WHITE)&&ui.screen==WX_CHARACTER_LIST);
}
int main(void){
    static const unsigned classes[8][7]={{1,2,4,5,8,9,0},{1,3,4,7,9,0},{1,2,3,4,5,0},{1,3,4,5,11,0},
        {1,4,5,8,9,0},{1,3,7,11,0},{1,4,8,9,0},{1,3,4,5,7,8,0}};
    for(unsigned race=0;race<=9;race++)for(unsigned c=0;c<=12;c++){
        int expected=0;if(race>=1&&race<=8)for(unsigned i=0;i<7;i++)if(classes[race-1][i]==c&&c)expected=1;
        CHECK(wx_character_class_allowed(race,c)==expected);
    }
    WxCharacterDraft draft={"Ab",1,1,0,2,3,4,5,6};uint8_t wire[32];
    const uint8_t golden[]={'A','b',0,1,1,0,2,3,4,5,6,0};
    CHECK(wx_character_create_encode(&draft,wire,sizeof wire)==sizeof golden&&!memcmp(wire,golden,sizeof golden));
    for(unsigned cap=0;cap<sizeof golden;cap++){memset(wire,0xcc,sizeof wire);CHECK(!wx_character_create_encode(&draft,wire,cap)&&wire[0]==0xcc);}
    strcpy(draft.name,"A");CHECK(!wx_character_create_encode(&draft,wire,sizeof wire));
    strcpy(draft.name,"Ab2");CHECK(!wx_character_create_encode(&draft,wire,sizeof wire));
    memset(draft.name,'a',sizeof draft.name);CHECK(!wx_character_create_encode(&draft,wire,sizeof wire));
    strcpy(draft.name,"Abcdefghijkl");CHECK(wx_character_create_encode(&draft,wire,22)==22);
    draft.character_class=7;CHECK(!wx_character_create_encode(&draft,wire,sizeof wire));draft.character_class=1;
    draft.gender=2;CHECK(!wx_character_create_encode(&draft,wire,sizeof wire));
    lobby.phase=WX_LOBBY_READY;lobby.characters.count=2;lobby.characters.items[0].guid=1;lobby.characters.items[1].guid=2;lobby.preferred=2;
    wx_character_ui_sync(&ui,&lobby);CHECK(ui.open&&ui.selected==1);CHECK(press(WX_A)&&command.kind==WX_LOBBY_SELECT&&command.guid==2);
    press(WX_UP);CHECK(ui.selected==0);press(WX_UP);CHECK(ui.selected==0);press(WX_DOWN);press(WX_DOWN);CHECK(ui.selected==1);
    CHECK(press(WX_Y)&&command.kind==WX_LOBBY_REFRESH);CHECK(press(WX_B)&&command.kind==WX_LOBBY_CANCEL);
    press(WX_X);CHECK(ui.screen==WX_CHARACTER_CREATE&&ui.row==0&&ui.draft.race==1&&ui.draft.character_class==1);
    press(WX_A);CHECK(ui.screen==WX_CHARACTER_KEYBOARD);press(WX_A);CHECK(!strcmp(ui.draft.name,"A"));
    press(WX_RIGHT);press(WX_A);CHECK(!strcmp(ui.draft.name,"Ab"));press(WX_X);CHECK(!strcmp(ui.draft.name,"A"));
    press(WX_B);CHECK(ui.screen==WX_CHARACTER_CREATE&&!ui.draft.name[0]);
    press(WX_A);press(WX_LEFT);CHECK(ui.key==27);press(WX_UP);CHECK(ui.key==20);press(WX_DOWN);CHECK(ui.key==27);
    press(WX_RIGHT);CHECK(ui.key==0);for(unsigned i=0;i<13;i++)press(WX_A);CHECK(strlen(ui.draft.name)==12);
    press(WX_START);CHECK(ui.screen==WX_CHARACTER_CREATE);
    for(unsigned i=0;i<4;i++)press(WX_DOWN);press(WX_A);CHECK(ui.screen==WX_CHARACTER_CONFIRM);
    CHECK(press(WX_A)&&command.kind==WX_LOBBY_CREATE&&strlen(command.draft.name)==12);
    lobby.phase=WX_LOBBY_PENDING;CHECK(!press(WX_A));CHECK(!press(WX_B));lobby.phase=WX_LOBBY_READY;lobby.result=0x31;lobby.result_revision++;
    wx_character_ui_sync(&ui,&lobby);CHECK(ui.screen==WX_CHARACTER_CREATE&&strstr(ui.message,"already in use"));
    ui.row=1;ui.draft.race=1;ui.draft.character_class=2;press(WX_LEFT);CHECK(ui.draft.race==8&&ui.draft.character_class==1);
    ui.row=2;press(WX_LEFT);CHECK(ui.draft.character_class==8);press(WX_RIGHT);CHECK(ui.draft.character_class==1);
    ui.row=3;press(WX_LEFT);CHECK(ui.draft.gender==1);press(WX_RIGHT);CHECK(ui.draft.gender==0);
    // Every class chooser stays in the allowed set in both directions.
    for(unsigned race=1;race<=8;race++){ui.draft.race=race;ui.draft.character_class=1;ui.row=2;
        for(unsigned i=0;i<22;i++){press(i<11?WX_RIGHT:WX_LEFT);CHECK(wx_character_class_allowed(race,ui.draft.character_class));}}
    strcpy(ui.draft.name,"Newhero");strcpy(lobby.characters.items[1].name,"Newhero");lobby.result=0x2e;lobby.result_revision++;
    wx_character_ui_sync(&ui,&lobby);CHECK(ui.screen==WX_CHARACTER_LIST&&ui.selected==1);
    lobby.characters.count=10;press(WX_X);CHECK(ui.screen==WX_CHARACTER_LIST&&strstr(ui.message,"Ten characters"));
    lobby.phase=WX_LOBBY_FAILED;CHECK(press(WX_B)&&command.kind==WX_LOBBY_CANCEL);
    lobby.phase=WX_LOBBY_CLOSED;wx_character_ui_sync(&ui,&lobby);CHECK(!ui.open&&!press(WX_A));
    deletion();appearance();printf("Character packets, deletion and controller UI: %u checks passed\n",checks);return 0;
}
