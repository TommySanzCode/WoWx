#include "wx_game.h"
#include "wx_text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks,length;static uint8_t packet[4096];
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Gameplay packet check failed at line %u\n",__LINE__);exit(1);}}while(0)
static void bytes(uint64_t n,unsigned count){for(unsigned i=0;i<count;i++)packet[length++]=(uint8_t)(n>>(8*i));}
static void word(uint32_t n){bytes(n,4);}
static void real(float n){uint32_t v;memcpy(&v,&n,4);word(v);}
static void string(const char* s){size_t n=strlen(s)+1;memcpy(packet+length,s,n);length+=(unsigned)n;}
static void item_response(void){
    length=0;word(2362);word(4);word(6);string("Worn Wooden Shield");string("");string("");string("");
    word(18730);word(0);word(0);word(5);word(1);word(14);
    // Exact 5875 tail: 14 restrictions/stack/bag, 20 stat words, 15 damage
    // words, seven resistances, delay/ammo/range, 30 spell words and bonding.
    for(unsigned i=0;i<14+20+15+7+3+30+1;i++)word(0);
    string("A fixture description");for(unsigned i=0;i<14;i++)word(0);
}
static void item_tests(void){
    static WxGame game,before;WxCommand c={0x56,0,2362,0};uint8_t out[12];
    CHECK(wx_command_encode(&c,out,sizeof out)==12&&out[0]==0x3a&&out[1]==9&&out[4]==0);
    CHECK(wx_command_encode(&c,out,11)==-1);c.value=0x80000001;CHECK(wx_command_encode(&c,out,12)==-1);
    item_response();CHECK(wx_game_apply(&game,0x58,packet,length,1)==1);
    const WxItemName* item=wx_item_info(&game,2362);
    CHECK(item&&item->metadata==1&&item->display==18730&&item->inventory_type==14&&!strcmp(item->name,"Worn Wooden Shield"));
    CHECK(!wx_item_info(&game,0)&&!wx_item_info(0,2362));
    before=game;for(unsigned n=0;n<length;n++)CHECK(wx_game_apply(&game,0x58,packet,n,1)==0&&!memcmp(&game,&before,sizeof game));
    packet[length]=0;CHECK(wx_game_apply(&game,0x58,packet,length+1,1)==0&&!memcmp(&game,&before,sizeof game));
    unsigned inventory_offset=12+sizeof("Worn Wooden Shield")+3+20;
    packet[inventory_offset]=29;CHECK(wx_game_apply(&game,0x58,packet,length,1)==0&&!memcmp(&game,&before,sizeof game));item_response();
    unsigned damage_offset=inventory_offset+4+(14+20)*4;
    packet[damage_offset+2]=0xc0;packet[damage_offset+3]=0x7f;
    CHECK(wx_game_apply(&game,0x58,packet,length,1)==0&&!memcmp(&game,&before,sizeof game));item_response();
    // Negative query result is complete only at four bytes and clears stale metadata.
    length=0;word(0x8000093a);CHECK(wx_game_apply(&game,0x58,packet,length,1)==1);
    item=wx_item_info(&game,2362);CHECK(item&&item->metadata==2&&!item->display&&!item->inventory_type);
    before=game;
    packet[length]=0;CHECK(wx_game_apply(&game,0x58,packet,length+1,1)==0&&!memcmp(&game,&before,sizeof game));
    // A name-only response must not discard an already resolved display.
    item_response();CHECK(wx_game_apply(&game,0x58,packet,length,1)==1);
    length=0;word(2362);string("Localized shield");CHECK(wx_game_apply(&game,0x2c5,packet,length,1)==1);
    CHECK(wx_item_info(&game,2362)->display==18730&&wx_item_info(&game,2362)->metadata==1);
    // Reusing a bounded cache slot must never leak the evicted item's display.
    for(unsigned i=0;i<WX_ITEM_NAMES;i++){
        length=0;word(10000+i);string("Name only");CHECK(wx_game_apply(&game,0x2c5,packet,length,1)==1);
    }
    CHECK(!wx_item_info(&game,2362));item=wx_item_info(&game,10000+WX_ITEM_NAMES-1);
    CHECK(item&&!item->metadata&&!item->display&&!item->inventory_type);
}
static void dialog_tests(void){
    WxGame game={0},before;
    length=0;bytes(7,8);string("Greetings");word(0);word(0);bytes(1,1);word(783);word(6);word(1);string("A Threat Within");
    CHECK(wx_game_apply(&game,0x185,packet,length,1)==1&&game.dialog.quest_count==1&&game.dialog.quests[0].id==783);
    length=0;bytes(7,8);word(783);string("A Threat Within");string("Speak to Marshal McBride.$BWelcome, $N.");string("Find Marshal McBride.");word(1);
    word(0);word(0);word(0);word(0);word(4);for(unsigned i=0;i<8;i++)word(0);
    CHECK(wx_game_apply(&game,0x188,packet,length,1)==1&&game.dialog.screen==WX_SCREEN_DETAILS&&game.dialog.quest==783);
    before=game;for(unsigned n=0;n<length;n++)CHECK(wx_game_apply(&game,0x188,packet,n,1)==0&&!memcmp(&game,&before,sizeof game));
    packet[length]=0;CHECK(wx_game_apply(&game,0x188,packet,length+1,1)==0);
    length=0;bytes(7,8);word(783);string("A Threat Within");string("You made it.");for(unsigned i=0;i<5;i++)word(0);
    word(2);word(3);word(4);word(8);
    CHECK(wx_game_apply(&game,0x18b,packet,length,1)==1&&game.dialog.can_complete);
    packet[length-16]=0;CHECK(wx_game_apply(&game,0x18b,packet,length,1)==1&&!game.dialog.can_complete);
    length=0;bytes(7,8);word(783);string("A Threat Within");string("Thank you.");word(1);word(0);
    word(1);word(25);word(1);word(100);word(0);word(10);word(0);word(0);
    CHECK(wx_game_apply(&game,0x18d,packet,length,1)==1&&game.dialog.choice_count==1&&game.dialog.money==10);
    length=0;word(25);string("Worn Shortsword");CHECK(wx_game_apply(&game,0x2c5,packet,length,1)==1&&!strcmp(wx_item_name(&game,25),"Worn Shortsword"));
    length=0;bytes(7,8);bytes(1,1);word(11);bytes(2,1);
    for(unsigned i=0;i<2;i++){bytes(i,1);word(25+i);word(1);word(100);word(0);word(0);bytes(0,1);}
    CHECK(wx_game_apply(&game,0x160,packet,length,1)==1&&game.dialog.loot_count==2&&game.dialog.money==11);
    before=game;for(unsigned n=0;n<length;n++)CHECK(wx_game_apply(&game,0x160,packet,n,1)==0&&!memcmp(&game,&before,sizeof game));
    packet[36]=0;CHECK(wx_game_apply(&game,0x160,packet,length,1)==0);
    packet[0]=0;CHECK(wx_game_apply(&game,0x162,packet,1,1)==1&&game.dialog.loot_count==1&&game.dialog.loot[0].slot==1);
    CHECK(wx_game_apply(&game,0x165,0,0,1)==1&&!game.dialog.money);
    length=0;word(783);word(0);word(40);word(10);word(0);
    CHECK(wx_game_apply(&game,0x191,packet,length,1)==1&&game.completed_quest==783&&game.reward_xp==40&&!game.dialog.screen);
    WxCommand c={0x189,7,783,0};uint8_t out[16];CHECK(wx_command_encode(&c,out,16)==12&&out[8]==15&&out[9]==3);
    c.opcode=0x18e;c.extra=6;CHECK(wx_command_encode(&c,out,16)==-1);c.extra=5;CHECK(wx_command_encode(&c,out,16)==16&&out[12]==5);
}
static void death_packets(void){
    WxGame game={0},before;WxCommand c={0x15a,0,0,0};uint8_t out[16];
    CHECK(wx_command_encode(&c,out,16)==1&&out[0]==1);CHECK(wx_command_encode(&c,out,0)==-1);
    c.opcode=0x216;CHECK(wx_command_encode(&c,out,16)==0);
    c.opcode=0x1d2;CHECK(wx_command_encode(&c,out,16)==-1);c.guid=123;CHECK(wx_command_encode(&c,out,16)==8&&out[0]==123);
    c.opcode=0x21c;CHECK(wx_command_encode(&c,out,16)==8);c.opcode=0x17b;CHECK(wx_command_encode(&c,out,16)==8);
    length=0;word(30000);CHECK(wx_game_apply(&game,0x269,packet,length,1)==1&&game.corpse_delay_ms==30000&&game.corpse_delay_revision==1);
    length=0;bytes(1,1);word(0);real(-8800);real(-170);real(82);word(0);
    CHECK(wx_game_apply(&game,0x216,packet,length,1)==1&&game.corpse_known&&game.corpse_position[0]==-8800);
    before=game;for(unsigned i=0;i<length;i++)CHECK(wx_game_apply(&game,0x216,packet,i,1)==0&&!memcmp(&before,&game,sizeof game));
    packet[length]=0;CHECK(wx_game_apply(&game,0x216,packet,length+1,1)==0);
    length=0;bytes(1,1);word(0);real(NAN);real(0);real(0);word(0);CHECK(wx_game_apply(&game,0x216,packet,length,1)==0);
    packet[0]=2;CHECK(wx_game_apply(&game,0x216,packet,1,1)==0);packet[0]=0;
    CHECK(wx_game_apply(&game,0x216,packet,1,1)==1&&!game.corpse_known);length=0;
}
static void vendor_tests(void){
    static WxGame game,before;uint8_t encoded[32];
    WxCommand c={0x19e,7,0,0};CHECK(wx_command_encode(&c,encoded,8)==8&&encoded[0]==7);
    c=(WxCommand){0x1a2,7,159,1};CHECK(wx_command_encode(&c,encoded,14)==14&&encoded[8]==159&&encoded[12]==1&&encoded[13]==0);
    CHECK(wx_command_encode(&c,encoded,13)==-1);c.extra=0;CHECK(wx_command_encode(&c,encoded,32)==-1);
    c.extra=256;CHECK(wx_command_encode(&c,encoded,32)==-1);
    c=(WxCommand){0x1a0,7,0x12345678,0x40000000};CHECK(wx_command_encode(&c,encoded,17)==17&&encoded[8]==0x78&&encoded[15]==0x40&&encoded[16]==1);
    CHECK(wx_command_encode(&c,encoded,16)==-1);c.value=c.extra=0;CHECK(wx_command_encode(&c,encoded,32)==-1);
    length=0;bytes(7,8);bytes(2,1);
    for(unsigned i=0;i<2;i++){word(i+1);word(159+i);word(100);word(i?1:UINT32_MAX);word(25);word(0);word(5);}
    CHECK(wx_game_apply(&game,0x19f,packet,length,1)==1&&game.dialog.screen==WX_SCREEN_VENDOR&&game.dialog.vendor_count==2);
    CHECK(game.dialog.vendor[0].item==159&&game.dialog.vendor[0].count==5&&game.dialog.vendor[1].stock==1);
    before=game;for(unsigned i=0;i<length;i++)CHECK(!wx_game_apply(&game,0x19f,packet,i,1)&&!memcmp(&before,&game,sizeof game));
    packet[length]=0;CHECK(!wx_game_apply(&game,0x19f,packet,length+1,1));
    packet[37]=1;CHECK(!wx_game_apply(&game,0x19f,packet,length,1));packet[37]=2; // duplicate slot
    packet[33]=0;CHECK(!wx_game_apply(&game,0x19f,packet,length,1));packet[33]=5; // zero bundle
    packet[8]=129;CHECK(!wx_game_apply(&game,0x19f,packet,length,1));
    length=0;bytes(7,8);bytes(128,1);for(unsigned i=0;i<128;i++){word(i+1);word(159+i);word(100);word(UINT32_MAX);word(25);word(0);word(5);}
    CHECK(wx_game_apply(&game,0x19f,packet,length,1)==1&&game.dialog.vendor_count==128);
    length=0;bytes(7,8);word(1);word(0);word(1);
    CHECK(wx_game_apply(&game,0x1a4,packet,length,1)==1&&game.bundles_bought==1&&game.dialog.vendor[0].stock==0);
    before=game;for(unsigned i=0;i<length;i++)CHECK(!wx_game_apply(&game,0x1a4,packet,i,1)&&!memcmp(&game,&before,sizeof game));
    length=0;bytes(7,8);bytes(99,8);bytes(0,1);CHECK(wx_game_apply(&game,0x1a1,packet,length,1)==1&&game.items_sold==1);
    packet[16]=1;CHECK(wx_game_apply(&game,0x1a1,packet,length,1)==1&&game.items_sold==1);
    length=0;bytes(7,8);word(159);bytes(2,1);CHECK(wx_game_apply(&game,0x1a5,packet,length,1)==1&&strstr(game.message,"declined"));
    length=0;bytes(7,8);bytes(0,1);bytes(0,1);CHECK(wx_game_apply(&game,0x19f,packet,length,1)==1&&!game.dialog.vendor_count);
}
int main(void){
    death_packets();
    WxCommand c={0x141,0x1122334455667788ull,0,0};uint8_t out[32];
    CHECK(wx_command_encode(&c,out,sizeof out)==8&&out[0]==0x88&&out[7]==0x11);
    CHECK(wx_command_encode(&c,out,7)==-1);c.guid=0;CHECK(wx_command_encode(&c,out,32)==-1);
    c.opcode=0x13d;CHECK(wx_command_encode(&c,out,32)==8);c.opcode=0x142;CHECK(wx_command_encode(&c,out,32)==0);
    c.opcode=0xffff;CHECK(wx_command_encode(&c,out,32)==-1);
    c=(WxCommand){0x60,7,299,0};CHECK(wx_command_encode(&c,out,32)==12&&out[0]==43&&out[1]==1&&out[4]==7);
    WxGame game={0},before;bytes(1,8);bytes(7,8);CHECK(wx_game_apply(&game,0x143,packet,length,1)==1&&game.attack_target==7);
    before=game;CHECK(wx_game_apply(&game,0x143,packet,length-1,1)==0&&!memcmp(&game,&before,sizeof game));
    length=0;bytes(1,1);bytes(1,1);bytes(1,1);bytes(7,1);word(0);
    CHECK(wx_game_apply(&game,0x144,packet,length,1)==1&&!game.attack_target);
    length=0;word(299);memcpy(packet+length,"Young Wolf",11);length+=11;for(unsigned i=0;i<4;i++)bytes(0,1);
    for(unsigned i=0;i<7;i++)word(0);bytes(0,2);
    CHECK(wx_game_apply(&game,0x61,packet,length,1)==1&&game.name_entry==299&&!strcmp(game.target_name,"Young Wolf"));
    before=game;for(unsigned n=0;n<length;n++)CHECK(wx_game_apply(&game,0x61,packet,n,1)==0&&!memcmp(&game,&before,sizeof game));
    length=0;word(2);bytes(1,1);bytes(1,1);bytes(1,1);bytes(7,1);word(5);bytes(1,1);
    word(0);real(5);word(5);word(0);word(0);for(unsigned i=0;i<4;i++)word(0);
    CHECK(wx_game_apply(&game,0x14a,packet,length,1)==1&&game.damage_dealt==5&&game.last_victim==7);
    before=game;for(unsigned n=0;n<length;n++)CHECK(wx_game_apply(&game,0x14a,packet,n,1)==0&&!memcmp(&game,&before,sizeof game));
    length=0;bytes(1,8);bytes(7,8);CHECK(wx_game_apply(&game,0x1f5,packet,length,1)==1&&game.kills==1);
    CHECK(wx_game_apply(&game,0x145,0,0,1)==1&&!strcmp(game.message,"Move closer to attack"));
    CHECK(wx_game_apply(&game,0xffff,0,0,1)==-1);
    c=(WxCommand){0x10a,0,255,23};CHECK(wx_command_encode(&c,out,32)==2&&out[0]==255&&out[1]==23);
    c.value=24;CHECK(wx_command_encode(&c,out,32)==-1);
    length=0;bytes(1,1);word(10);bytes(1,8);bytes(2,8);bytes(0,1);
    CHECK(wx_game_apply(&game,0x112,packet,length,1)==1&&!strcmp(game.message,"Requires level 10"));
    before=game;CHECK(wx_game_apply(&game,0x112,packet,length-1,1)==0&&!memcmp(&before,&game,sizeof game));
    length=0;bytes(7,8);word(42);word(1);word(0);bytes(1,1);bytes(0,1);string("Browse wares");word(1);word(783);word(4);word(1);string("A Threat Within");
    CHECK(wx_game_apply(&game,0x17d,packet,length,1)==1&&game.dialog.quest_count==1&&game.dialog.gossip_count==1&&game.dialog.quests[0].id==783);
    before=game;for(unsigned n=0;n<length;n++)CHECK(wx_game_apply(&game,0x17d,packet,n,1)==0&&!memcmp(&before,&game,sizeof game));
    c=(WxCommand){0x12e,7,78,0};CHECK(wx_command_encode(&c,out,32)==8&&out[0]==78&&out[4]==2&&out[6]==1&&out[7]==7);
    c.guid=0;CHECK(wx_command_encode(&c,out,32)==6&&out[4]==0);c.value=65536;CHECK(wx_command_encode(&c,out,32)==-1);
    length=0;bytes(0,1);bytes(2,2);bytes(78,2);bytes(0,2);bytes(6603,2);bytes(0,2);bytes(0,2);
    CHECK(wx_game_apply(&game,0x12a,packet,length,1)==1&&wx_has_spell(&game,78)&&!wx_has_spell(&game,2457));
    before=game;for(unsigned n=0;n<length;n++)CHECK(wx_game_apply(&game,0x12a,packet,n,1)==0&&!memcmp(&game,&before,sizeof game));
    length=0;word(78);bytes(0,1);CHECK(wx_game_apply(&game,0x130,packet,length,1)==1&&game.casts_accepted==1);
    length=0;word(78);bytes(2,1);bytes(5,1);CHECK(wx_game_apply(&game,0x130,packet,length,1)==1&&game.casts_rejected==1);
    length=0;for(unsigned i=0;i<120;i++)word(i==0?6603:i==1?78:0);
    CHECK(wx_game_apply(&game,0x129,packet,length,1)==1&&game.actions[1]==78);
    game.actions[72]=6603;game.actions[73]=78;
    CHECK(wx_action_binding(&game,0,17)==6603&&wx_action_binding(&game,1,17)==78&&wx_action_binding(&game,120,17)==0);
    c=(WxCommand){0xab,0,255,23};CHECK(wx_command_encode(&c,out,32)==5&&out[0]==255&&out[1]==23&&out[2]==0&&out[3]==0);
    char rows[4][49];WxTextIdentity identity={"Xboxer",1,1,0};
    CHECK(wx_text_wrap("Hello $N, $R $C.$B$Ghis:hers;",rows,4,&identity)==2&&!strcmp(rows[0],"Hello Xboxer, human warrior.")&&!strcmp(rows[1],"his"));
    identity.gender=1;CHECK(wx_text_wrap("$Ghis:hers;",rows,4,&identity)==1&&!strcmp(rows[0],"hers"));
    CHECK(wx_text_wrap("There is a long sentence with several words that should wrap.",rows,4,&identity)==2&&!strcmp(rows[0],"There is a long sentence with several words that")&&!strcmp(rows[1],"should wrap."));
    struct {char row[1][49];char guard[4];} bounded;memcpy(bounded.guard,"safe",4);
    CHECK(wx_text_wrap("This unbroken string exceeds the capacity of one tiny text page by many characters",bounded.row,1,&identity)==1&&bounded.row[0][48]==0&&!memcmp(bounded.guard,"safe",4));
    dialog_tests();vendor_tests();item_tests();printf("Gameplay command/combat/quest/loot packet checks: %u passed\n",checks);return 0;
}
