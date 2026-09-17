#ifndef WX_INVENTORY_H
#define WX_INVENTORY_H
#include "wx_entities.h"
#define WX_INVENTORY_SLOTS 183
#ifdef __cplusplus
extern "C" {
#endif
typedef struct WxInventoryItem {uint64_t guid;uint32_t entry,count,durability,maximum_durability;uint8_t bag,slot;int32_t charges[5];} WxInventoryItem;
typedef struct WxInventory {uint32_t count,money;WxInventoryItem items[WX_INVENTORY_SLOTS];} WxInventory;
void wx_inventory_snapshot(const WxEntities* entities,uint64_t player,WxInventory* output);
void wx_world_inventory(WxInventory* output);
int wx_inventory_sale_completed(const WxInventory* inventory,uint64_t item,unsigned before_count,unsigned before_money);
#ifdef __cplusplus
}
#endif
#endif
