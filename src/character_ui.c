#include "wx_lobby.h"
#include "wx_input.h"
#include <string.h>
#include <stdio.h>
static void draft_look(const WxCharacterDraft* d,uint32_t v[7]){
    v[0]=d->race;v[1]=d->gender;v[2]=d->skin;v[3]=d->face;v[4]=d->hair_style;v[5]=d->hair_color;v[6]=d->facial_hair;
}
static void reset_look(WxCharacterDraft* d){d->skin=d->face=d->hair_style=d->hair_color=d->facial_hair=0;}
unsigned wx_character_appearance_choices(const WxCharacterUi* ui,unsigned field,unsigned* selected){
    uint32_t v[7];uint8_t ids[256];draft_look(&ui->draft,v);*selected=0;
    unsigned n=wx_looks_choices(ui->looks,v,field,ids);for(unsigned i=0;i<n;i++)if(ids[i]==v[field])*selected=i;return n;
}
static int same_name(const char* a,const char* b){for(unsigned i=0;i<13;i++){
    unsigned x=(unsigned char)a[i],y=(unsigned char)b[i];if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;
    if(x!=y)return 0;if(!x)return 1;}return 0;}
static void cancel_delete(WxCharacterUi* ui){
    ui->screen=WX_CHARACTER_LIST;ui->delete_guid=0;ui->delete_name[0]=ui->delete_text[0]=0;
}
int wx_character_delete_ready(const WxCharacterUi* ui,const WxCharacterLobby* lobby){
    if(!ui||!lobby||!ui->open||lobby->phase!=WX_LOBBY_READY||ui->screen!=WX_CHARACTER_DELETE)return 0;
    const WxCharacter* c=wx_character_find(&lobby->characters,ui->delete_guid);
    if(!c||!memchr(ui->delete_name,0,sizeof ui->delete_name)||strcmp(c->name,ui->delete_name))return 0;
    static const char word[]="DELETE";
    for(unsigned i=0;i<6;i++){unsigned ch=(unsigned char)ui->delete_text[i];if(ch>='a'&&ch<='z')ch-=32;if(ch!=(unsigned)word[i])return 0;}
    return ui->delete_text[6]==0;
}
void wx_character_ui_sync(WxCharacterUi* ui,const WxCharacterLobby* lobby){
    if(lobby->phase==WX_LOBBY_CLOSED){ui->open=0;return;}
    if(!ui->open){memset(ui,0,sizeof *ui);ui->open=1;ui->draft.race=ui->draft.character_class=1;
        ui->result_revision=lobby->result_revision-(lobby->result_kind!=0);
        for(unsigned i=0;i<lobby->characters.count;i++)if(lobby->characters.items[i].guid==lobby->preferred)ui->selected=i;
    }
    if(ui->selected>=lobby->characters.count)ui->selected=0;
    if((ui->screen==WX_CHARACTER_DELETE||ui->screen==WX_CHARACTER_DELETE_KEYBOARD)&&lobby->phase==WX_LOBBY_READY){
        const WxCharacter* target=wx_character_find(&lobby->characters,ui->delete_guid);
        if(!target||strcmp(target->name,ui->delete_name)){cancel_delete(ui);snprintf(ui->message,sizeof ui->message,"Character list changed; select the character again");}
        else ui->selected=(unsigned)(target-lobby->characters.items);
    }
    if(ui->result_revision!=lobby->result_revision){
        if(lobby->result_kind==WX_LOBBY_DELETE||lobby->result_kind==WX_LOBBY_SELECT){
            ui->result_revision=lobby->result_revision;cancel_delete(ui);
            snprintf(ui->message,sizeof ui->message,"%s",lobby->result_kind==WX_LOBBY_DELETE?wx_character_delete_result(lobby->result):wx_character_login_result(lobby->result));return;
        }
        ui->result_revision=lobby->result_revision;snprintf(ui->message,sizeof ui->message,"%s",wx_character_create_result(lobby->result));
        if(lobby->result==0x2e){ui->screen=WX_CHARACTER_LIST;
            for(unsigned i=0;i<lobby->characters.count;i++)if(same_name(ui->draft.name,lobby->characters.items[i].name))ui->selected=i;
        }else if(ui->screen==WX_CHARACTER_CONFIRM)ui->screen=WX_CHARACTER_CREATE;
    }
}
int wx_character_ui_input(WxCharacterUi* ui,const WxCharacterLobby* lobby,const WxPad* pad,WxLobbyCommand* command){
    memset(command,0,sizeof *command);if(pad->layer||!ui->open)return 0;
    unsigned p=pad->pressed;
    if(lobby->phase==WX_LOBBY_FAILED){if(p&(1u<<WX_B)){command->kind=WX_LOBBY_CANCEL;return 1;}return 0;}
    if(lobby->phase!=WX_LOBBY_READY)return 0;
    if(ui->screen==WX_CHARACTER_LIST){
        if(p&(1u<<WX_UP)){if(ui->selected)ui->selected--;}
        if(p&(1u<<WX_DOWN)){if(ui->selected+1<lobby->characters.count)ui->selected++;}
        if((p&(1u<<WX_WHITE))&&ui->selected<lobby->characters.count){
            const WxCharacter* c=lobby->characters.items+ui->selected;ui->delete_guid=c->guid;
            memcpy(ui->delete_name,c->name,sizeof ui->delete_name);ui->delete_text[0]=0;
            ui->screen=WX_CHARACTER_DELETE;ui->message[0]=0;return 0;
        }
        if((p&(1u<<WX_A))&&lobby->characters.count){command->kind=WX_LOBBY_SELECT;command->guid=lobby->characters.items[ui->selected].guid;return 1;}
        if(p&(1u<<WX_X)){
            if(lobby->characters.count==10)snprintf(ui->message,sizeof ui->message,"Ten characters already exist on this realm");
            else {memset(&ui->draft,0,sizeof ui->draft);ui->draft.race=ui->draft.character_class=1;ui->row=0;ui->screen=WX_CHARACTER_CREATE;ui->message[0]=0;}
        }
        if(p&(1u<<WX_Y)){command->kind=WX_LOBBY_REFRESH;return 1;}
        if(p&(1u<<WX_B)){command->kind=WX_LOBBY_CANCEL;return 1;}
    }else if(ui->screen==WX_CHARACTER_CREATE){
        if(p&(1u<<WX_B)){ui->screen=WX_CHARACTER_LIST;ui->message[0]=0;return 0;}
        if(p&(1u<<WX_Y)){ui->screen=WX_CHARACTER_APPEARANCE;ui->appearance_row=0;ui->message[0]=0;return 0;}
        if((p&(1u<<WX_UP))&&ui->row)ui->row--;
        if((p&(1u<<WX_DOWN))&&ui->row<4)ui->row++;
        int direction=p&(1u<<WX_LEFT)?-1:p&(1u<<WX_RIGHT)?1:0;
        if(direction&&ui->row==1){ui->draft.race=(uint8_t)((ui->draft.race-1+direction+8)%8+1);reset_look(&ui->draft);
            if(!wx_character_class_allowed(ui->draft.race,ui->draft.character_class))ui->draft.character_class=1;}
        if(direction&&ui->row==2){unsigned c=ui->draft.character_class;do{c=(c-1+11+direction)%11+1;}while(!wx_character_class_allowed(ui->draft.race,c));ui->draft.character_class=(uint8_t)c;}
        if(direction&&ui->row==3){ui->draft.gender^=1;reset_look(&ui->draft);}
        if(p&(1u<<WX_A)){
            if(ui->row==0){memcpy(ui->original_name,ui->draft.name,13);ui->key=0;ui->screen=WX_CHARACTER_KEYBOARD;}
            else if(ui->row==4){uint8_t data[22];if(wx_character_create_encode(&ui->draft,data,sizeof data)){ui->screen=WX_CHARACTER_CONFIRM;ui->message[0]=0;}
                else snprintf(ui->message,sizeof ui->message,"Use 2 to 12 letters for the name");}
        }
    }else if(ui->screen==WX_CHARACTER_APPEARANCE){
        if(p&((1u<<WX_B)|(1u<<WX_A))){ui->screen=WX_CHARACTER_CREATE;return 0;}
        if((p&(1u<<WX_UP))&&ui->appearance_row)ui->appearance_row--;
        if((p&(1u<<WX_DOWN))&&ui->appearance_row<4)ui->appearance_row++;
        int direction=p&(1u<<WX_LEFT)?-1:p&(1u<<WX_RIGHT)?1:0;
        int randomize=(p&(1u<<WX_X))!=0;
        if(direction||randomize){uint32_t v[7];draft_look(&ui->draft,v);
            if(randomize?wx_looks_randomize(ui->looks,v,&ui->random_state):wx_looks_step(ui->looks,v,2+ui->appearance_row,direction)){
                ui->draft.skin=(uint8_t)v[2];ui->draft.face=(uint8_t)v[3];ui->draft.hair_style=(uint8_t)v[4];ui->draft.hair_color=(uint8_t)v[5];ui->draft.facial_hair=(uint8_t)v[6];ui->message[0]=0;
                if(randomize)ui->randomizations++;
            }else snprintf(ui->message,sizeof ui->message,"Appearance options are not loaded");
        }
    }else if(ui->screen==WX_CHARACTER_DELETE){
        if(p&(1u<<WX_B)){cancel_delete(ui);ui->message[0]=0;return 0;}
        if(p&(1u<<WX_A)){
            if(wx_character_delete_ready(ui,lobby)){command->kind=WX_LOBBY_DELETE;command->guid=ui->delete_guid;return 1;}
            const WxCharacter* c=wx_character_find(&lobby->characters,ui->delete_guid);
            if(!c||strcmp(c->name,ui->delete_name)){cancel_delete(ui);snprintf(ui->message,sizeof ui->message,"Character list changed; select the character again");return 0;}
            ui->screen=WX_CHARACTER_DELETE_KEYBOARD;ui->delete_text[0]=0;ui->key=0;
        }
    }else if(ui->screen==WX_CHARACTER_KEYBOARD||ui->screen==WX_CHARACTER_DELETE_KEYBOARD){
        unsigned deleting=ui->screen==WX_CHARACTER_DELETE_KEYBOARD;
        char* text=deleting?ui->delete_text:ui->draft.name;unsigned limit=deleting?6:12;
        unsigned parent=deleting?WX_CHARACTER_DELETE:WX_CHARACTER_CREATE;
        if(p&(1u<<WX_B)){if(deleting)text[0]=0;else memcpy(text,ui->original_name,13);ui->screen=parent;return 0;}
        if(p&(1u<<WX_START)){ui->screen=parent;return 0;}
        if(p&(1u<<WX_LEFT))ui->key=(ui->key+27)%28;
        if(p&(1u<<WX_RIGHT))ui->key=(ui->key+1)%28;
        if(p&(1u<<WX_UP))ui->key=(ui->key+21)%28;
        if(p&(1u<<WX_DOWN))ui->key=(ui->key+7)%28;
        size_t n=strlen(text);
        if((p&(1u<<WX_X))||((p&(1u<<WX_A))&&ui->key==26)){if(n)text[n-1]=0;}
        else if(p&(1u<<WX_A)){
            if(ui->key==27)ui->screen=parent;
            else if(n<limit){text[n]=(char)((!deleting&&n?'a':'A')+ui->key);text[n+1]=0;}
        }
    }else if(ui->screen==WX_CHARACTER_CONFIRM){
        if(p&(1u<<WX_B))ui->screen=WX_CHARACTER_CREATE;
        else if(p&(1u<<WX_A)){command->kind=WX_LOBBY_CREATE;command->draft=ui->draft;return 1;}
    }
    return 0;
}
