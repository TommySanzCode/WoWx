#include "wx_inventory.h"
int wx_inventory_sale_completed(const WxInventory* inventory,uint64_t item,unsigned before_count,unsigned before_money){
    if(!inventory||!item||!before_count||inventory->money<=before_money)return 0;
    unsigned remaining=0;for(unsigned i=0;i<inventory->count;i++)if(inventory->items[i].guid==item)remaining=inventory->items[i].count;
    return remaining==before_count-1;
}
#include <string.h>
static const WxEntity* entity(const WxEntities* s,uint64_t guid){
    if(guid)for(unsigned i=0;i<WX_MAX_ENTITIES;i++)if(s->items[i].guid==guid)return &s->items[i];return 0;
}
static uint64_t guid_at(const uint32_t* fields,unsigned at){return fields[at]|((uint64_t)fields[at+1]<<32);}
static void add(WxInventory* out,const WxEntity* item,unsigned bag,unsigned slot){
    if(!item||(item->type!=1&&item->type!=2)||out->count==WX_INVENTORY_SLOTS)return;
    WxInventoryItem* v=&out->items[out->count++];v->guid=item->guid;v->entry=item->fields[3];v->count=item->fields[14];
    v->durability=item->fields[46];v->maximum_durability=item->fields[47];v->bag=(uint8_t)bag;v->slot=(uint8_t)slot;
    for(unsigned i=0;i<5;i++)memcpy(&v->charges[i],&item->fields[16+i],4);
}
void wx_inventory_snapshot(const WxEntities* s,uint64_t player,WxInventory* out){
    memset(out,0,sizeof *out);const WxEntity* self=entity(s,player);if(!self||self->type!=4)return;out->money=self->money;
    for(unsigned slot=0;slot<39;slot++){
        const WxEntity* item=entity(s,guid_at(self->inventory,slot*2));add(out,item,255,slot);
        if(slot>=19&&slot<23&&item&&item->type==2){
            unsigned slots=item->fields[48];if(slots>36)slots=36;
            for(unsigned j=0;j<slots;j++)add(out,entity(s,guid_at(item->fields,50+j*2)),slot,j);
        }
    }
}
