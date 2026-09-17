#include "wx_journal.h"
#include "wx_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,length;static uint8_t packet[8192];
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"Journal check failed at line %u\n",__LINE__);exit(1);}}while(0)
static void word(uint32_t x){for(unsigned i=0;i<4;i++)packet[length++]=(uint8_t)(x>>(i*8));}
static void string(const char* x){unsigned n=(unsigned)strlen(x)+1;memcpy(packet+length,x,n);length+=n;}
static void fixture(void){
    length=0;word(7);word(2);word(2);word(12);word(0);word(0);word(0);word(0);word(0);word(15);word(25);word(60);word(0);word(0);word(0);
    for(unsigned i=0;i<4;i++){word(i==0?117:0);word(i==0?2:0);}
    for(unsigned i=0;i<12;i++)word(0);word(0);word(0);word(0);word(0);
    string("Kobold Camp Cleanup");string("Defeat 10 Kobold Vermin.");string("Return to Marshal McBride, $N.");string("");
    word(6);word(10);word(123);word(4);
    for(unsigned i=0;i<12;i++)word(0);
    string("Kobold Vermin defeated");string("");string("");string("");
}
int main(void){
    static WxQuestInfo q,before;static WxQuestCache cache;uint8_t wire[4];WxCommand request={0x5c,0,7,0};
    CHECK(sizeof cache<96*1024);CHECK(wx_command_encode(&request,wire,4)==4&&wire[0]==7&&wire[1]==0);
    CHECK(wx_command_encode(&request,wire,3)==-1);request.value=0;CHECK(wx_command_encode(&request,wire,4)==-1);
    request.value=0x80000007;CHECK(wx_command_encode(&request,wire,4)==-1);
    fixture();CHECK(wx_quest_decode(&q,packet,length)&&q.id==7&&q.money==25&&q.next==15&&q.creatures[0][0]==6&&q.creatures[0][1]==10);
    CHECK(!strcmp(q.title,"Kobold Camp Cleanup")&&!strcmp(q.objectives,"Defeat 10 Kobold Vermin.")&&q.rewards[0][0]==117&&q.items[0][1]==4);
    before=q;for(unsigned n=0;n<length;n++)CHECK(!wx_quest_decode(&q,packet,n)&&!memcmp(&q,&before,sizeof q));
    packet[length]=0;CHECK(!wx_quest_decode(&q,packet,length+1)&&!memcmp(&q,&before,sizeof q));
    packet[4]=3;CHECK(!wx_quest_decode(&q,packet,length));fixture();
    packet[146]=0xc0;packet[147]=0x7f;CHECK(!wx_quest_decode(&q,packet,length));fixture();
    packet[length-1]='x';CHECK(!wx_quest_decode(&q,packet,length));fixture();
    CHECK(wx_quest_cache_apply(&cache,packet,length));CHECK(wx_quest_cached(&cache,7)&&!wx_quest_cached(&cache,0));
    for(unsigned id=100;id<121;id++){memcpy(packet,&id,4);CHECK(wx_quest_cache_apply(&cache,packet,length));}
    CHECK(!wx_quest_cached(&cache,7)&&wx_quest_cached(&cache,120));unsigned cursor=cache.cursor;
    CHECK(wx_quest_cache_apply(&cache,packet,length)&&cache.cursor==cursor);
    fixture();CHECK(wx_quest_decode(&q,packet,length));
    WxEntity player={0};player.guid=2;player.quests[0]=7;player.quests[1]=3|(5u<<6)|(7u<<12)|(9u<<18)|(1u<<24);
    WxInventory inv={0};inv.count=2;inv.items[0].entry=inv.items[1].entry=123;inv.items[0].count=2;inv.items[1].count=1;
    WxQuestProgress progress;wx_quest_progress(&q,&player,&inv,&progress);
    CHECK(progress.present&&progress.complete&&!progress.failed&&progress.creatures[0]==3&&progress.creatures[1]==5&&progress.creatures[2]==7&&progress.creatures[3]==9&&progress.items[0]==3);
    player.quests[1]=1u<<25;wx_quest_progress(&q,&player,&inv,&progress);CHECK(progress.failed&&!progress.complete);
    player.quests[0]=8;wx_quest_progress(&q,&player,&inv,&progress);CHECK(!progress.present&&!progress.items[0]);
    WxJournalUi ui={0};wx_journal_sync(&ui,&player);ui.open=1;CHECK(ui.count==1&&ui.ids[0]==8&&ui.player==2);
    WxPad pad={0};pad.pressed=1u<<WX_A;wx_journal_input(&ui,&pad);CHECK(ui.detail);
    pad.pressed=1u<<WX_RIGHT;wx_journal_input(&ui,&pad);CHECK(ui.page==1);
    pad.pressed=1u<<WX_X;wx_journal_input(&ui,&pad);CHECK(ui.story&&!ui.page);
    pad.pressed=1u<<WX_B;wx_journal_input(&ui,&pad);CHECK(ui.open&&!ui.detail);wx_journal_input(&ui,&pad);CHECK(!ui.open);
    for(unsigned i=0;i<20;i++)player.quests[i*3]=i+1;wx_journal_sync(&ui,&player);ui.open=1;
    pad.pressed=1u<<WX_DOWN;for(unsigned i=0;i<25;i++)wx_journal_input(&ui,&pad);CHECK(ui.selected==19&&ui.count==20);
    ui.detail=1;ui.page=3;player.quests[19*3]=0;wx_journal_sync(&ui,&player);CHECK(ui.selected==18&&!ui.detail&&!ui.page);
    player.guid=3;wx_journal_sync(&ui,&player);CHECK(!ui.open&&ui.selected==0&&ui.player==3);
    printf("Quest query, journal and objective checks: %u passed; cache %u bytes\n",checks,(unsigned)sizeof cache);return 0;
}
