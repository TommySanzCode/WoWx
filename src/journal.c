// Wire layout: pinned vMaNGOS QuestQueryResponse::AppendBodyTo, build 5875.
#include "wx_journal.h"
#include "wx_wire.h"
int wx_quest_decode(WxQuestInfo* out,const uint8_t* data,size_t size){
    if(!out||!data||size>65533)return 0;WxQuestInfo q={0};WxReader r={data,size,0,1};
    q.id=wx_read(&r,4);q.method=wx_read(&r,4);q.level=wx_read(&r,4);q.zone=wx_read(&r,4);q.type=wx_read(&r,4);
    q.faction=wx_read(&r,4);q.faction_value=wx_read(&r,4);wx_read(&r,4);wx_read(&r,4);
    q.next=wx_read(&r,4);q.money=(int32_t)wx_read(&r,4);q.maximum_level_money=wx_read(&r,4);
    q.spell=wx_read(&r,4);q.source_item=wx_read(&r,4);q.flags=wx_read(&r,4);
    if(!q.id||q.id>=0x80000000||q.method>2)return 0;
    for(unsigned i=0;i<4;i++)for(unsigned k=0;k<2;k++)q.rewards[i][k]=wx_read(&r,4);
    for(unsigned i=0;i<6;i++)for(unsigned k=0;k<2;k++)q.choices[i][k]=wx_read(&r,4);
    q.point_map=wx_read(&r,4);q.point_x=wx_real(&r);q.point_y=wx_real(&r);q.point_option=wx_read(&r,4);
    wx_string(&r,q.title,sizeof q.title);wx_string(&r,q.objectives,sizeof q.objectives);
    wx_string(&r,q.details,sizeof q.details);wx_string(&r,q.end_text,sizeof q.end_text);
    for(unsigned i=0;i<4;i++){
        q.creatures[i][0]=wx_read(&r,4);q.creatures[i][1]=wx_read(&r,4);
        q.items[i][0]=wx_read(&r,4);q.items[i][1]=wx_read(&r,4);
    }
    for(unsigned i=0;i<4;i++)wx_string(&r,q.objective_text[i],sizeof q.objective_text[i]);
    if(!r.ok||r.at!=r.size)return 0;*out=q;return 1;
}
const WxQuestInfo* wx_quest_cached(const WxQuestCache* cache,uint32_t id){
    if(cache&&id)for(unsigned i=0;i<WX_QUEST_CACHE;i++)if(cache->quests[i].id==id)return &cache->quests[i];return 0;
}
int wx_quest_cache_apply(WxQuestCache* cache,const uint8_t* data,size_t size){
    if(!cache)return 0;WxQuestInfo q;if(!wx_quest_decode(&q,data,size))return 0;
    const WxQuestInfo* old=wx_quest_cached(cache,q.id);unsigned slot=old?(unsigned)(old-cache->quests):cache->cursor++%WX_QUEST_CACHE;
    cache->quests[slot]=q;return 1;
}
void wx_quest_progress(const WxQuestInfo* q,const WxEntity* player,const WxInventory* inv,WxQuestProgress* out){
    if(!out)return;memset(out,0,sizeof *out);if(!q||!player)return;
    for(unsigned i=0;i<20;i++)if(player->quests[i*3]==q->id){uint32_t packed=player->quests[i*3+1];
        out->present=1;out->complete=(packed>>24)&1;out->failed=(packed>>25)&1;
        for(unsigned k=0;k<4;k++)out->creatures[k]=(packed>>(k*6))&63;break;}
    if(!out->present||!inv)return;
    for(unsigned k=0;k<4;k++)if(q->items[k][0])for(unsigned i=0;i<inv->count&&i<WX_INVENTORY_SLOTS;i++)if(inv->items[i].entry==q->items[k][0]){
        uint32_t n=inv->items[i].count;out->items[k]=UINT32_MAX-out->items[k]<n?UINT32_MAX:out->items[k]+n;
    }
}
void wx_journal_sync(WxJournalUi* ui,const WxEntity* player){
    if(!ui)return;unsigned old=ui->selected<ui->count?ui->ids[ui->selected]:0;ui->count=0;
    if(!player){ui->open=0;return;}
    if(ui->player!=player->guid){memset(ui,0,sizeof *ui);ui->player=player->guid;old=0;}
    for(unsigned i=0;i<20;i++)if(player->quests[i*3])ui->ids[ui->count++]=player->quests[i*3];
    if(old){unsigned row=0;while(row<ui->count&&ui->ids[row]!=old)row++;
        if(row<ui->count)ui->selected=row;else ui->detail=ui->page=ui->story=0;}
    if(ui->selected>=ui->count)ui->selected=ui->count?ui->count-1:0;
}
void wx_journal_input(WxJournalUi* ui,const WxPad* pad){
    if(!ui||!pad||!ui->open||pad->layer)return;
    if(pad->pressed&(1u<<WX_B)){if(ui->detail)ui->detail=ui->page=ui->story=0;else ui->open=0;return;}
    if(!ui->detail){
        if((pad->pressed&(1u<<WX_UP))&&ui->selected)ui->selected--;
        if((pad->pressed&(1u<<WX_DOWN))&&ui->selected+1<ui->count)ui->selected++;
        if((pad->pressed&(1u<<WX_A))&&ui->count)ui->detail=1;
    }else{
        if((pad->pressed&(1u<<WX_LEFT))&&ui->page)ui->page--;
        if((pad->pressed&(1u<<WX_RIGHT))&&ui->page<127)ui->page++;
        if(pad->pressed&(1u<<WX_X)){ui->story=!ui->story;ui->page=0;}
    }
}
