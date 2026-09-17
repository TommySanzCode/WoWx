#include "wx_lobby.h"
#include <pbkit/pbkit.h>
#include "wx_font.h"
static void legacy_draw(const WxCharacterUi* ui,const WxCharacterLobby* lobby){
    if(lobby->phase==WX_LOBBY_CLOSED)return;
    pb_fill(24,30,592,420,0xff17212a);wx_font_clear();wx_font_printat(1,3,"CHARACTERS / Local realm");
    if(lobby->phase==WX_LOBBY_LOADING){wx_font_printat(5,3,"Loading your characters...");return;}
    if(lobby->phase==WX_LOBBY_FAILED){wx_font_printat(5,3,"Character session disconnected");
        if(lobby->result_kind==WX_LOBBY_DELETE&&lobby->result==WX_CHAR_DELETE_UNCONFIRMED){wx_font_printat(7,3,"Deletion not confirmed.");wx_font_printat(8,3,"Reconnect to check your character list.");}
        wx_font_printat(14,3,"B: close and return to connection menu");return;}
    if(lobby->phase==WX_LOBBY_PENDING){wx_font_printat(5,3,lobby->pending_kind==WX_LOBBY_DELETE?"Deleting character...":"Waiting for the realm...");return;}
    if(ui->screen==WX_CHARACTER_LIST){
        unsigned first=ui->selected/5*5;
        for(unsigned i=first;i<lobby->characters.count&&i<first+5;i++){
            const WxCharacter* c=&lobby->characters.items[i];unsigned line=3+(i-first)*2;
            wx_font_printat(line,3,"%s %.24s  Level %u",ui->selected==i?">":" ",c->name,c->level);
            wx_font_printat(line+1,5,"%s %s",wx_race_name(c->race),wx_class_name(c->character_class));
        }
        if(!lobby->characters.count)wx_font_printat(4,3,"No characters yet. X: create one");
        wx_font_printat(14,3,"A: enter  X: new  Y: refresh  B: back");
        wx_font_printat(13,3,"White: delete selected character");
    }else if(ui->screen==WX_CHARACTER_CREATE){
        wx_font_printat(3,3,"NEW CHARACTER");
        wx_font_printat(5,3,"%s Name: %s",ui->row==0?">":" ",ui->draft.name[0]?ui->draft.name:"Choose a name");
        wx_font_printat(6,3,"%s Race: %s",ui->row==1?">":" ",wx_race_name(ui->draft.race));
        wx_font_printat(7,3,"%s Class: %s",ui->row==2?">":" ",wx_class_name(ui->draft.character_class));
        wx_font_printat(8,3,"%s Sex: %s",ui->row==3?">":" ",ui->draft.gender?"Female":"Male");
        wx_font_printat(10,3,"%s Create character",ui->row==4?">":" ");
        wx_font_printat(12,3,"Y: customize appearance");
        wx_font_printat(14,3,"D-pad: choose  A: select  B: back");
    }else if(ui->screen==WX_CHARACTER_APPEARANCE){
        const char* labels[]={"Skin","Face","Hair style","Hair color","Facial feature"};
        wx_font_printat(3,3,"CUSTOMIZE APPEARANCE");
        for(unsigned i=0;i<5;i++){unsigned selected,n=wx_character_appearance_choices(ui,i+2,&selected);
            wx_font_printat(5+i,3,"%s %s: %u / %u",i==ui->appearance_row?">":" ",labels[i],n?selected+1:0,n);}
        wx_font_printat(11,3,"X: randomize appearance");
        wx_font_printat(12,3,"Right stick: rotate / zoom");wx_font_printat(14,3,"D-pad: choose/change  A or B: done");
    }else if(ui->screen==WX_CHARACTER_KEYBOARD||ui->screen==WX_CHARACTER_DELETE_KEYBOARD){
        unsigned deleting=ui->screen==WX_CHARACTER_DELETE_KEYBOARD;
        wx_font_printat(3,3,"%s: %s_",deleting?"TYPE DELETE":"NAME",deleting?ui->delete_text:ui->draft.name);
        for(unsigned i=0;i<28;i++){
            unsigned row=6+i/7,col=4+(i%7)*7;
            if(i<26)wx_font_printat(row,col,"%s%c",ui->key==i?">":" ",'A'+i);
            else wx_font_printat(row,col,"%s%s",ui->key==i?">":" ",i==26?"DEL":"DONE");
        }
        wx_font_printat(12,3,deleting?"Return to the confirmation after typing":"2-12 letters; the realm checks availability");
        wx_font_printat(14,3,"A: key  X: erase  Start: done  B: cancel");
    }else if(ui->screen==WX_CHARACTER_DELETE){
        wx_font_printat(4,3,"Delete %.24s?",ui->delete_name);
        wx_font_printat(6,3,"This character and its items will be lost.");
        wx_font_printat(8,3,"Type DELETE to confirm. This cannot be undone.");
        wx_font_printat(14,3,wx_character_delete_ready(ui,lobby)?"A: DELETE CHARACTER  B: cancel":"A: type DELETE  B: cancel");
    }else {
        wx_font_printat(4,3,"Create %s?",ui->draft.name);
        wx_font_printat(6,3,"%s %s / %s",wx_race_name(ui->draft.race),wx_class_name(ui->draft.character_class),ui->draft.gender?"Female":"Male");
        wx_font_printat(14,3,"A: create  B: review choices");
    }
    wx_font_printat(15,3,"%.51s",ui->message);
}
void wx_character_ui_draw(const WxCharacterUi* ui,const WxCharacterLobby* lobby,WxUi* c){
    if(lobby->phase==WX_LOBBY_CLOSED)return;
    if(!c->ready){legacy_draw(ui,lobby);return;}
    wx_font_clear();
    wx_ui_image(c,WX_UI_LOGO,18,8,200,100,0xffffffff);
    if(lobby->phase!=WX_LOBBY_READY){
        wx_ui_panel(c,70,170,500,155);
        wx_ui_center(c,1,320,220,WX_UI_WHITE,lobby->phase==WX_LOBBY_LOADING?"Retrieving character list...":
            lobby->phase==WX_LOBBY_FAILED?"Disconnected from server":
            lobby->pending_kind==WX_LOBBY_DELETE?"Deleting character...":"Waiting for the realm...");
        if(lobby->phase==WX_LOBBY_FAILED&&lobby->result_kind==WX_LOBBY_DELETE&&lobby->result==WX_CHAR_DELETE_UNCONFIRMED){
            wx_ui_center(c,0,320,260,0xffffc080,"Deletion not confirmed.");
            wx_ui_center(c,0,320,282,WX_UI_WHITE,"Reconnect to check your character list.");}
        if(lobby->phase==WX_LOBBY_FAILED)wx_ui_center(c,0,320,448,WX_UI_GOLD,"B: Back");return;
    }
    if(ui->screen==WX_CHARACTER_LIST){
        unsigned first=ui->selected/5*5;
        wx_ui_panel(c,418,30,207,328);
        wx_ui_center(c,2,522,49,WX_UI_GOLD,"Characters");
        for(unsigned i=first;i<lobby->characters.count&&i<first+5;i++){
            const WxCharacter* ch=&lobby->characters.items[i];float y=98+(i-first)*48;
            if(ui->selected==i){wx_ui_rect(c,431,y-3,180,44,0xff6e5322);wx_ui_rect(c,433,y-1,176,40,0xff30281d);}
            wx_ui_textf(c,1,440,y,ui->selected==i?WX_UI_WHITE:WX_UI_GOLD,"%.12s",ch->name);
            wx_ui_textf(c,0,440,y+23,0xffbfb9a8,"%u %s %s",ch->level,wx_race_name(ch->race),wx_class_name(ch->character_class));
        }
        if(!lobby->characters.count)wx_ui_center(c,0,522,170,WX_UI_WHITE,"No characters");
        wx_ui_button(c,430,372,180,"Create New Character",0,lobby->characters.count<10);
        wx_ui_button(c,430,410,180,"White: Delete",0,lobby->characters.count>0);
        wx_ui_button(c,124,410,182,"Enter World",1,lobby->characters.count>0);
        wx_ui_center(c,0,215,445,WX_UI_GOLD,"Right stick: Rotate / Zoom");
        wx_ui_center(c,0,320,464,WX_UI_GOLD,"D-pad: Select   A: Enter   X: Create   Y: Refresh   B: Back");
    }else if(ui->screen==WX_CHARACTER_CREATE){
        wx_ui_panel(c,18,114,213,310);
        wx_ui_center(c,1,124,138,WX_UI_GOLD,"Create Character");
        const char* labels[]={"Name","Race","Class","Sex"};
        const char* values[]={ui->draft.name[0]?ui->draft.name:"Choose a name",wx_race_name(ui->draft.race),wx_class_name(ui->draft.character_class),ui->draft.gender?"Female":"Male"};
        for(unsigned i=0;i<4;i++){float y=184+i*48;if(ui->row==i)wx_ui_rect(c,30,y-4,187,43,0xff44351e);
            wx_ui_text(c,0,40,y,WX_UI_GOLD,labels[i]);wx_ui_text(c,1,40,y+17,WX_UI_WHITE,values[i]);}
        wx_ui_button(c,37,380,175,"Create Character",ui->row==4,1);
        wx_ui_button(c,313,401,209,"Y: Customize",0,1);
        wx_ui_center(c,0,420,440,WX_UI_GOLD,"Right stick: Rotate / Zoom");
        wx_ui_center(c,0,320,464,WX_UI_GOLD,"D-pad: Choose / Change   A: Select   B: Back");
    }else if(ui->screen==WX_CHARACTER_APPEARANCE){
        const char* labels[]={"Skin","Face","Hair style","Hair color","Facial feature"};
        wx_ui_panel(c,18,114,213,310);wx_ui_center(c,1,124,136,WX_UI_GOLD,"Appearance");
        for(unsigned i=0;i<5;i++){float y=179+i*43;unsigned selected,n=wx_character_appearance_choices(ui,i+2,&selected);
            if(ui->appearance_row==i)wx_ui_rect(c,30,y-4,187,40,0xff44351e);
            wx_ui_text(c,0,40,y,WX_UI_GOLD,labels[i]);
            if(n)wx_ui_textf(c,0,40,y+18,WX_UI_WHITE,"<  %u / %u  >",selected+1,n);
            else wx_ui_text(c,0,40,y+18,0xffbfb9a8,"Loading options...");
        }
        wx_ui_center(c,0,420,440,WX_UI_GOLD,"Right stick: Rotate / Zoom");
        wx_ui_button(c,313,401,209,"X: Randomize",0,ui->looks&&ui->looks->file&&ui->looks->header.race==ui->draft.race&&ui->looks->header.sex==ui->draft.gender);
        wx_ui_center(c,0,320,464,WX_UI_GOLD,"D-pad: Choose / Change   A or B: Done");
    }else if(ui->screen==WX_CHARACTER_KEYBOARD||ui->screen==WX_CHARACTER_DELETE_KEYBOARD){
        unsigned deleting=ui->screen==WX_CHARACTER_DELETE_KEYBOARD;
        wx_ui_panel(c,20,118,600,315);
        wx_ui_center(c,1,320,144,WX_UI_GOLD,deleting?"Type DELETE to confirm":"Character Name");
        wx_ui_textf(c,1,80,179,WX_UI_WHITE,"%s_",deleting?ui->delete_text:ui->draft.name);
        for(unsigned i=0;i<28;i++){float x=78+(i%7)*69,y=222+(i/7)*38;char label[5]={0};
            if(i<26){label[0]=(char)('A'+i);}else {const char* action=i==26?"DEL":"DONE";for(unsigned j=0;action[j];j++)label[j]=action[j];}
            wx_ui_button(c,x,y,65,label,ui->key==i,1);
        }
        wx_ui_center(c,0,320,397,0xffbfb9a8,deleting?"Done returns to the deletion confirmation":"2-12 letters");
        wx_ui_center(c,0,320,448,WX_UI_GOLD,"A: Key   X: Erase   Start: Done   B: Cancel");
    }else if(ui->screen==WX_CHARACTER_DELETE){
        int ready=wx_character_delete_ready(ui,lobby);
        wx_ui_panel(c,62,132,516,290);
        wx_ui_center(c,1,320,161,WX_UI_GOLD,"Delete this character?");
        wx_ui_textf(c,2,132,207,WX_UI_WHITE,"%.24s",ui->delete_name);
        wx_ui_center(c,0,320,260,0xffffc080,"This character and its items will be lost.");
        wx_ui_center(c,0,320,283,WX_UI_WHITE,"Type DELETE to confirm. This cannot be undone.");
        wx_ui_button(c,215,347,210,ready?"Delete Character":"Type DELETE",1,1);
        wx_ui_center(c,0,320,442,WX_UI_GOLD,ready?"A: Delete Character   B: Cancel":"A: Open Keyboard   B: Cancel");
    }else{
        wx_ui_panel(c,85,150,470,260);
        wx_ui_center(c,1,320,185,WX_UI_GOLD,"Create this character?");
        wx_ui_center(c,2,320,232,WX_UI_WHITE,ui->draft.name);
        wx_ui_textf(c,0,180,286,WX_UI_GOLD,"%s %s / %s",wx_race_name(ui->draft.race),wx_class_name(ui->draft.character_class),ui->draft.gender?"Female":"Male");
        wx_ui_button(c,225,348,190,"Create Character",1,1);
        wx_ui_center(c,0,320,448,WX_UI_GOLD,"A: Create   B: Review Choices");
    }
    if(ui->message[0])wx_ui_textf(c,0,40,452,0xffffc080,"%.78s",ui->message);
}
