#include "wx_journal.h"
#include "wx_game.h"
#include "wx_text.h"
#include <pbkit/pbkit.h>
#include "wx_font.h"
#include <stdio.h>
#include <string.h>
static WxQuestInfo info;
static char lines[128][49];
static uint32_t requested_id,requested_at,last_query_at;
static uint64_t requested_player;
static int read_quest(uint32_t id,const WxWorldView* world,unsigned now){
    if(wx_world_quest(id,&info)){if(requested_id==id)requested_id=0;return 1;}
    if(requested_player!=world->guid){requested_id=requested_at=last_query_at=0;requested_player=world->guid;}
    // One outstanding request, one-second retry, at most four queries/second.
    if((!requested_id||now-requested_at>=1000)&&now-last_query_at>=250){
        WxCommand query={0x5c,0,id,0};if(wx_world_command(&query)){requested_id=id;requested_at=last_query_at=now;}
    }return 0;
}
void wx_journal_draw(WxJournalUi* ui,const WxEntity* player,const WxInventory* inv,const WxWorldView* world,unsigned now){
    if(!ui->open)return;pb_fill(24,30,592,420,0xff17212a);wx_font_clear();
    if(!ui->detail)wx_font_printat(1,3,"QUEST JOURNAL / %u of 20",ui->count);
    if(!ui->count){wx_font_printat(5,3,"Your quest log is empty.");wx_font_printat(14,3,"B: close");return;}
    unsigned id=ui->ids[ui->selected];
    if(!ui->detail){
        // Fill the visible page lazily; keep each network request bounded.
        unsigned first=ui->selected/8*8;
        for(unsigned i=first;i<ui->count&&i<first+8;i++){
            if(read_quest(ui->ids[i],world,now)){
                WxQuestProgress progress;wx_quest_progress(&info,player,inv,&progress);
                wx_font_printat(3+i-first,3,"%s %.42s%s",i==ui->selected?">":" ",info.title,progress.complete?" *":progress.failed?" !":"");
            }else wx_font_printat(3+i-first,3,"%s Quest %u / loading",i==ui->selected?">":" ",ui->ids[i]);
        }
        wx_font_printat(12,3,"* Complete   ! Failed");wx_font_printat(14,3,"D-pad: choose  A: read  B: close");return;
    }
    if(!read_quest(id,world,now)){wx_font_printat(5,3,"Loading quest %u...",id);wx_font_printat(14,3,"B: quest list");return;}
    WxQuestProgress progress;wx_quest_progress(&info,player,inv,&progress);
    wx_font_printat(1,3,"%.50s",info.title);
    wx_font_printat(2,3,"Level %u / %s",info.level,progress.failed?"Failed":progress.complete?"Complete - return for reward":"In progress");
    WxTextIdentity identity={world->name,world->race,world->character_class,world->gender};unsigned rows=0;
    if(ui->story)rows=wx_text_wrap(info.details,lines,128,&identity);
    else{
        rows=wx_text_wrap(info.objectives,lines,128,&identity);
        for(unsigned i=0;i<4&&rows+3<128;i++){
            if(info.objective_text[i][0])rows+=wx_text_wrap(info.objective_text[i],lines+rows,128-rows,&identity);
            if(info.creatures[i][0]&&rows<128)snprintf(lines[rows++],49,"%s %u: %u/%u",info.creatures[i][0]&0x80000000?"Object":"Target",i+1,progress.creatures[i],info.creatures[i][1]);
            if(info.items[i][0]&&rows<128)snprintf(lines[rows++],49,"Item objective %u: %u/%u",i+1,progress.items[i],info.items[i][1]);
        }
        if(info.end_text[0]&&rows<128)rows+=wx_text_wrap(info.end_text,lines+rows,128-rows,&identity);
        if(info.money<0&&rows<128)snprintf(lines[rows++],49,"Money: %u/%u copper",inv->money,(unsigned)(-(int64_t)info.money));
    }
    unsigned pages=(rows+7)/8;if(!pages)pages=1;if(ui->page>=pages)ui->page=pages-1;
    for(unsigned i=0;i<8&&ui->page*8+i<rows;i++)wx_font_printat(4+i,3,"%s",lines[ui->page*8+i]);
    wx_font_printat(12,3,"%s / page %u of %u",ui->story?"Description":"Objectives",ui->page+1,pages);
    wx_font_printat(14,3,"Left/right: page  X: %s",ui->story?"objectives":"description");wx_font_printat(15,3,"B: quest list");
}
