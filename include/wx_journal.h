#ifndef WX_JOURNAL_H
#define WX_JOURNAL_H
#include "wx_entities.h"
#include "wx_inventory.h"
#include "wx_input.h"
#include "wx_world.h"
#define WX_QUEST_CACHE 20
#ifdef __cplusplus
extern "C" {
#endif
typedef struct WxQuestInfo {
    uint32_t id,method,level,zone,type,flags,next,spell,source_item;
    int32_t money;uint32_t maximum_level_money,faction,faction_value;
    uint32_t rewards[4][2],choices[6][2],creatures[4][2],items[4][2];
    uint32_t point_map,point_option;float point_x,point_y;
    char title[128],objectives[1024],details[2048],end_text[256],objective_text[4][128];
} WxQuestInfo;
typedef struct WxQuestCache {WxQuestInfo quests[WX_QUEST_CACHE];unsigned cursor;} WxQuestCache;
typedef struct WxQuestProgress {unsigned present,complete,failed,creatures[4],items[4];} WxQuestProgress;
typedef struct WxJournalUi {unsigned open,selected,detail,story,page;uint32_t ids[20],count;uint64_t player;} WxJournalUi;
// Transactional Vanilla 5875 query parser; the cache has a fixed allocation.
int wx_quest_decode(WxQuestInfo* output,const uint8_t* data,size_t size);
int wx_quest_cache_apply(WxQuestCache* cache,const uint8_t* data,size_t size);
const WxQuestInfo* wx_quest_cached(const WxQuestCache* cache,uint32_t id);
void wx_quest_progress(const WxQuestInfo* quest,const WxEntity* player,const WxInventory* inventory,WxQuestProgress* output);
void wx_journal_sync(WxJournalUi* ui,const WxEntity* player);
void wx_journal_input(WxJournalUi* ui,const WxPad* pad);
int wx_world_quest(uint32_t id,WxQuestInfo* output);
void wx_journal_draw(WxJournalUi* ui,const WxEntity* player,const WxInventory* inventory,const WxWorldView* world,unsigned now);
#ifdef __cplusplus
}
#endif
#endif
