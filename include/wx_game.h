#ifndef WX_GAME_H
#define WX_GAME_H
#include <stdint.h>
#include <stddef.h>
#define WX_ITEM_NAMES 256
#define WX_VENDOR_ITEMS 128
#ifdef __cplusplus
extern "C" {
#endif
typedef struct WxCommand {uint16_t opcode;uint64_t guid;uint32_t value,extra;} WxCommand;
enum WxScreen {WX_SCREEN_NONE,WX_SCREEN_QUESTS,WX_SCREEN_DETAILS,WX_SCREEN_REQUEST,WX_SCREEN_REWARD,WX_SCREEN_LOOT,WX_SCREEN_VENDOR};
typedef struct WxVendorItem {uint32_t slot,item,display,stock,price,durability,count;} WxVendorItem;
typedef struct WxReward {uint32_t item,count,display;} WxReward;
typedef struct WxQuestOption {uint32_t id,icon,level;char title[128];} WxQuestOption;
typedef struct WxGossipOption {uint32_t id;uint8_t coded;char text[256];} WxGossipOption;
typedef struct WxLootItem {uint32_t item,count,display,property;uint8_t slot,kind;} WxLootItem;
// metadata: 0 = name only, 1 = complete Vanilla query, 2 = unavailable item.
typedef struct WxItemName {uint32_t id,display;uint8_t inventory_type,metadata;char name[256];} WxItemName;
typedef struct WxDialog {
    uint64_t guid;
    uint32_t quest,screen,revision,can_complete,money,spell,quest_count,choice_count,reward_count,loot_count;
    char title[128],text[2048],objectives[1024];
    WxQuestOption quests[32];WxReward choices[6],rewards[4];WxLootItem loot[16];
    uint32_t gossip_count;WxGossipOption gossip[15];
    uint32_t vendor_count;WxVendorItem vendor[WX_VENDOR_ITEMS];
} WxDialog;
typedef struct WxGame {
    uint64_t attack_target,last_victim;
    uint32_t name_entry,kills,damage_dealt,damage_received,event_revision;
    char target_name[96],message[96];
    WxDialog dialog;
    uint32_t completed_quest,reward_xp,reward_money;
    uint32_t looted_items,looted_money;
    uint32_t actions[120],spell_count,last_spell,casts_accepted,casts_rejected,spell_hits;
    uint16_t spells[512];
    uint32_t actions_ready,action_revision;
    WxItemName item_names[WX_ITEM_NAMES];uint32_t item_cursor;
    uint32_t corpse_known,corpse_map,corpse_instance_map,corpse_delay_ms,corpse_delay_revision;
    float corpse_position[3];
    uint32_t bundles_bought,items_sold;
} WxGame;
// Returns payload length (including valid zero-length bodies), or -1 on rejection.
int wx_command_encode(const WxCommand* command,uint8_t* output,unsigned capacity);
// -1 = not handled; 0 = malformed; 1 = validated and applied transactionally.
int wx_game_apply(WxGame* game,uint16_t opcode,const uint8_t* data,size_t size,uint64_t player);
int wx_world_command(const WxCommand* command);
void wx_world_game(WxGame* output);
void wx_world_close_dialog(void);
const char* wx_item_name(const WxGame* game,uint32_t id);
const WxItemName* wx_item_info(const WxGame* game,uint32_t id);
int wx_has_spell(const WxGame* game,uint32_t spell);
uint32_t wx_action_binding(const WxGame* game,unsigned slot,unsigned form);
unsigned wx_action_slot(unsigned slot,unsigned form);
#ifdef __cplusplus
}
#endif
#endif
