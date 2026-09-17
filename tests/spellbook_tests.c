#include "wx_spellbook.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,available=64u*1024u*1024u;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return available;}
static void fixture(unsigned mode){
    uint32_t h[]={0x31535857,2,3,sizeof(WxSpellInfo)};uint16_t ids[]={78,81,6603};WxSpellInfo records[3]={{0,"Heroic Strike","Rank 1"},{WX_SPELL_PASSIVE,"Dodge","Passive"},{0,"Attack",""}};
    for(unsigned i=0;i<3;i++)records[i].metadata=1;
    if(mode==1)ids[2]=81;if(mode==2)h[2]=65536;if(mode==3)memset(records[0].name,'x',80);
    if(mode==5)records[0].attributes=WX_SPELL_HIDDEN;
    if(mode==6)records[0].attributes=WX_SPELL_RECIPE;
    if(mode==7)records[0].metadata=0;if(mode==8)records[0].school=7;
    if(mode==9)records[0].min_range=NAN;if(mode==10){records[0].min_range=10;records[0].max_range=5;}
    if(mode==11)records[0].max_range=INFINITY;
    if(mode==12){h[1]=1;h[3]=108;}
    FILE* f=fopen("spellbook-fixture.wxs","wb");CHECK(f);fwrite(h,1,sizeof h,f);fwrite(ids,1,sizeof ids,f);if(mode==12){for(unsigned i=0;i<3;i++)fwrite(&records[i],1,108,f);}else fwrite(records,1,sizeof records-(mode==4),f);fclose(f);
}
static WxPad key(unsigned button){WxPad p={0};p.pressed=1u<<button;p.slot=p.action=-1;return p;}
int main(int argc,char** argv){
    static WxSpellBook book;static WxGame game,old;WxSpellUi ui={0};WxCommand command;uint8_t bindings[24];for(unsigned i=0;i<24;i++)bindings[i]=(uint8_t)i;
    if(argc==2){CHECK(wx_spellbook_open(&book,argv[1]));game.spell_count=3;game.spells[0]=78;game.spells[1]=81;game.spells[2]=6603;
        for(unsigned i=0;i<3;i++)wx_spellbook_sync(&book,&game);
        CHECK(book.loaded==3&&!book.failures);CHECK(!strcmp(book.known[0].info.name,"Heroic Strike"));CHECK(!strcmp(book.known[0].info.rank,"Rank 1"));CHECK(book.known[1].info.attributes&WX_SPELL_PASSIVE);CHECK(!strcmp(book.known[2].info.name,"Attack"));
        for(unsigned first=0;first<book.catalog_count;first+=512){
            game.spell_count=book.catalog_count-first<512?book.catalog_count-first:512;
            for(unsigned i=0;i<game.spell_count;i++)game.spells[i]=book.ids[first+i];
            for(unsigned i=0;i<game.spell_count;i++)wx_spellbook_sync(&book,&game);
            for(unsigned i=0;i<game.spell_count;i++)CHECK(book.known[i].state==1&&book.known[i].id==game.spells[i]);
        }
        CHECK(!book.failures);printf("Actual Vanilla catalog: all %u rows verified, index %u bytes, known cache %u bytes\n",book.catalog_count,book.catalog_count*2,(unsigned)sizeof book.known);wx_spellbook_close(&book);return 0;}
    fixture(0);CHECK(wx_spellbook_open(&book,"spellbook-fixture.wxs"));game.spell_count=3;game.spells[0]=78;game.spells[1]=81;game.spells[2]=6603;
    wx_spellbook_sync(&book,&game);CHECK(book.loaded==1&&book.known[0].state==1&&book.known[1].state==0);
    wx_spellbook_sync(&book,&game);wx_spellbook_sync(&book,&game);CHECK(book.loaded==3&&!book.failures);
    CHECK(book.row_count==3&&wx_spellbook_selected(&book,0)->id==6603&&wx_spellbook_selected(&book,1)->id==78&&wx_spellbook_selected(&book,2)->id==81);
    CHECK(wx_spellbook_find(&book,81)->info.attributes&WX_SPELL_PASSIVE);CHECK(!wx_spellbook_find(&book,999));
    old=game;WxActionPrompt prompt;
    CHECK(!wx_action_prompt(&book,&game,bindings,0,0,&prompt)&&!strcmp(prompt.name,"Loading actions"));
    CHECK(!wx_action_prompt(&book,&game,bindings,0,24,&prompt)&&prompt.server_slot==120&&!strcmp(prompt.name,"Unavailable"));
    CHECK(!wx_action_prompt(&book,NULL,bindings,0,0,&prompt));CHECK(!wx_action_prompt(&book,&game,NULL,0,0,&prompt));
    CHECK(!wx_action_prompt(&book,&game,bindings,0,0,NULL));
    game.actions_ready=1;game.actions[0]=6603;game.actions[72]=78;game.actions[84]=0;game.actions[96]=81;
    CHECK(wx_action_prompt(&book,&game,bindings,0,0,&prompt)&&prompt.binding==6603&&!strcmp(prompt.name,"Attack"));
    CHECK(wx_action_prompt(&book,&game,bindings,0x11,0,&prompt)&&prompt.server_slot==72&&prompt.binding==78&&!strcmp(prompt.name,"Heroic Strike"));
    CHECK(wx_action_prompt(&book,&game,bindings,0x12,0,&prompt)&&prompt.server_slot==84&&!strcmp(prompt.name,"Empty"));
    CHECK(wx_action_prompt(&book,&game,bindings,0x13,0,&prompt)&&prompt.server_slot==96&&!strcmp(prompt.name,"Dodge"));
    for(unsigned i=0;i<120;i++)game.actions[i]=1000+i;
    for(unsigned i=0;i<24;i++){bindings[i]=(uint8_t)((i*17+3)%120);CHECK(wx_action_prompt(&book,&game,bindings,0,i,&prompt));
        CHECK(prompt.server_slot==bindings[i]&&prompt.binding==1000+bindings[i]&&!strcmp(prompt.name,"Unlearned spell"));}
    for(unsigned i=0;i<24;i++)bindings[i]=(uint8_t)i;
    game.actions[0]=0x80000000|6948;game.item_names[0].id=6948;strcpy(game.item_names[0].name,"Hearthstone");
    CHECK(wx_action_prompt(&book,&game,bindings,0,0,&prompt)&&!strcmp(prompt.name,"Hearthstone"));
    game.actions[0]=0x40000001;CHECK(wx_action_prompt(&book,&game,bindings,0,0,&prompt)&&!strcmp(prompt.name,"Unsupported action"));
    game.actions[0]=78;book.known[0].state=0;CHECK(wx_action_prompt(&book,&game,bindings,0,0,&prompt)&&!strcmp(prompt.name,"Loading spell name"));
    book.known[0].state=2;CHECK(wx_action_prompt(&book,&game,bindings,0,0,&prompt)&&!strcmp(prompt.name,"Spell name missing"));
    book.known[0].state=1;strcpy(book.known[0].info.name,"Heroic\nStrike");
    CHECK(wx_action_prompt(&book,&game,bindings,0,0,&prompt)&&!strcmp(prompt.name,"Heroic Strike"));strcpy(book.known[0].info.name,"Heroic Strike");
    bindings[0]=255;CHECK(!wx_action_prompt(&book,&game,bindings,0,0,&prompt)&&prompt.server_slot==120);bindings[0]=0;game=old;
    ui.open=1;ui.selected=1;WxPad p=key(WX_A);CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command)&&ui.screen==1);
    CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command)&&ui.screen==1); // actions not yet received
    uint8_t wire[480]={0};CHECK(wx_game_apply(&game,0x129,wire,480,1)==1&&game.actions_ready&&game.action_revision==1);
    old=game;for(unsigned n=0;n<480;n++){CHECK(!wx_game_apply(&game,0x129,wire,n,1));CHECK(!memcmp(&old,&game,sizeof game));}
    p.slot=23;p.layer=3;CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command)&&ui.slot==23&&ui.screen==1);
    p=key(WX_A);CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command)&&ui.screen==2);
    CHECK(wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command)&&command.opcode==0x128&&command.value==78&&command.extra==23);
    CHECK(wx_command_encode(&command,wire,5)==5&&!memcmp(wire,(uint8_t[]){23,78,0,0,0},5));
    CHECK(wx_command_encode(&command,wire,4)==-1);command.extra=120;CHECK(wx_command_encode(&command,wire,5)==-1);
    command.extra=23;command.value=65536;CHECK(wx_command_encode(&command,wire,5)==-1);command.value=0;
    CHECK(wx_command_encode(&command,wire,5)==5&&!memcmp(wire,(uint8_t[]){23,0,0,0,0},5));command.guid=1;CHECK(wx_command_encode(&command,wire,5)==-1);
    // Reviewing or cancelling never queues an edit. Changes to form, binding,
    // learned spell or current action invalidate a pending confirmation.
    for(unsigned change=0;change<4;change++){
        ui=(WxSpellUi){0};ui.open=1;ui.selected=1;game.actions[0]=0;bindings[0]=0;
        CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command));CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command)&&ui.screen==2);
        if(change==1)bindings[0]=5;if(change==2)game.actions[0]=6603;if(change==3)book.known[0].id=6603;
        CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,change==0?0x11:0,&p,&command)&&ui.screen==1);book.known[0].id=78;
    }
    bindings[0]=0;game.actions[0]=0;ui=(WxSpellUi){0};ui.open=1;ui.selected=2;
    CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command)&&ui.screen==0); // passive
    ui.selected=1;CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command));
    p=key(WX_X);CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0x11,&p,&command)&&ui.screen==2&&ui.confirmed_slot==72&&ui.clear);
    p=key(WX_A);CHECK(wx_spell_ui_input(&ui,&book,&game,bindings,0x11,&p,&command)&&command.value==0&&command.extra==72);
    CHECK(wx_action_slot(0,0x12)==84&&wx_action_slot(11,0x13)==107&&wx_action_slot(12,0x11)==12&&wx_action_slot(120,0)==120);
    p=key(WX_B);CHECK(!wx_spell_ui_input(&ui,&book,&game,bindings,0,&p,&command)&&!ui.open);
    game.spell_count=1;game.spells[0]=999;wx_spellbook_sync(&book,&game);CHECK(book.count==1&&book.loaded==1&&book.known[0].state==2&&book.failures==1);
    wx_spellbook_close(&book);CHECK(!book.file&&!book.ids);
    for(unsigned mode=1;mode<=12;mode++){
        fixture(mode);int ok=wx_spellbook_open(&book,"spellbook-fixture.wxs");CHECK(ok==(mode==3||mode>=5));
        if(ok){game.spells[0]=78;wx_spellbook_sync(&book,&game);
            if(mode==3||(mode>=7&&mode<=11))CHECK(book.known[0].state==2&&book.failures==1);
            else if(mode==12)CHECK(book.version==1&&book.record_bytes==108&&book.known[0].state==1&&!book.known[0].info.metadata&&!strcmp(book.known[0].info.name,"Heroic Strike"));
            else CHECK(book.known[0].state==1&&!book.failures&&!book.row_count&&!wx_spellbook_selected(&book,0));}
        wx_spellbook_close(&book);
    }
    fixture(0);available=8u*1024u*1024u;CHECK(!wx_spellbook_open(&book,"spellbook-fixture.wxs")&&!book.ids&&!book.file);
    remove("spellbook-fixture.wxs");printf("Spell catalog, action protocol and controller checks: %u passed\n",checks);return 0;
}
