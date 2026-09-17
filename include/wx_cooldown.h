#ifndef WX_COOLDOWN_H
#define WX_COOLDOWN_H
#include <stdint.h>
#include <stddef.h>
#include "wx_inventory.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WX_COOLDOWN_LIMIT 512u
#define WX_COOLDOWN_ITEMS 256u
#define WX_COOLDOWN_ON_EVENT 0x02000000u
#define WX_GCD_LIMIT 16u
#define WX_SPELLMOD_OPS 29u
typedef struct WxCooldownInfo {
    uint32_t spell,category,recovery,category_recovery,attributes;
    uint32_t gcd_category,gcd_time,family,family_mask[2],attributes_ex2,attributes_ex3,damage_class,category_flags;
} WxCooldownInfo;
typedef struct WxCooldownCatalog {WxCooldownInfo* rows;unsigned count,bytes,ready,failures,version;} WxCooldownCatalog;
typedef struct WxCooldownEntry {uint32_t spell,item,category,start,duration,category_duration,lock_start,lock_duration,held;} WxCooldownEntry;
typedef struct WxItemSpellCooldown {uint32_t spell,trigger,recovery,category,category_recovery;int32_t charges;} WxItemSpellCooldown;
typedef struct WxItemCooldown {uint32_t item;WxItemSpellCooldown spells[5];} WxItemCooldown;
typedef struct WxGlobalCooldown {uint32_t category,start,duration,spell;} WxGlobalCooldown;
typedef struct WxCooldowns {
    WxCooldownEntry rows[WX_COOLDOWN_LIMIT];WxItemCooldown items[WX_COOLDOWN_ITEMS];
    unsigned item_cursor,revision,packets,missing,overflow;
    int32_t modifiers[2][WX_SPELLMOD_OPS][64]; // server SET values, not additive deltas
    WxGlobalCooldown global[WX_GCD_LIMIT];
    uint32_t player_family,pending_spell,pending_start,gcd_starts,gcd_cancels,modifier_updates,modifier_ambiguous;
    float cast_speed,ranged_ms;
} WxCooldowns;
typedef struct WxCooldownView {uint32_t remaining,duration,held,global;} WxCooldownView;
int wx_cooldown_catalog_open(WxCooldownCatalog* catalog,const char* path);
void wx_cooldown_catalog_close(WxCooldownCatalog* catalog);
const WxCooldownInfo* wx_cooldown_info(const WxCooldownCatalog* catalog,uint32_t spell);
void wx_cooldown_reset(WxCooldowns* state);
void wx_cooldown_unit(WxCooldowns* state,unsigned character_class,float cast_speed,float ranged_ms);
int wx_cooldown_modifiers(const WxCooldowns* state,const WxCooldownInfo* info,unsigned operation,int32_t output[2]);
/* Strict Vanilla 5875 layouts. 1 handled, 0 malformed, -1 unrelated.
   Caller owns synchronization. No allocations, no large transactional stack copy. */
int wx_cooldown_apply(WxCooldowns* state,const WxCooldownCatalog* catalog,
    const WxInventory* inventory,uint16_t opcode,const uint8_t* data,size_t size,uint64_t player,uint32_t now);
WxCooldownView wx_cooldown_query(const WxCooldowns* state,const WxCooldownCatalog* catalog,uint32_t binding,uint32_t now);
/* Catalog is immutable and must outlive the world worker; install before start. */
void wx_world_cooldown_catalog(const WxCooldownCatalog* catalog);
void wx_world_cooldowns(const uint32_t bindings[8],WxCooldownView views[8],unsigned metrics[12]);
#ifdef __cplusplus
}
#endif
#endif
