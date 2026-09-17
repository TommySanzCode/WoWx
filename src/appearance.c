#include "wx_world.h"
#include "wx_game.h"
#include <string.h>

void wx_player_appearance(WxWorldView* view,const WxEntity* e,const WxGame* game){
    if(!view||!e||!game||e->type!=4||!e->guid||e->guid!=view->guid)return;
    WxWorldView next=*view;
    uint32_t identity=e->fields[36],look=e->player_bytes;
    next.race=(uint8_t)identity;next.character_class=(uint8_t)(identity>>8);next.gender=(uint8_t)(identity>>16);
    next.skin=(uint8_t)look;next.face=(uint8_t)(look>>8);next.hair_style=(uint8_t)(look>>16);next.hair_color=(uint8_t)(look>>24);
    next.facial_hair=(uint8_t)e->player_bytes2;
    next.appearance_flags=e->fields[190];next.display_id=e->fields[131];next.native_display_id=e->fields[132];
    next.equipment_ready_mask=0;
    for(unsigned slot=0;slot<19;slot++){
        uint32_t entry=e->visible_items[slot];const WxItemName* item=wx_item_info(game,entry);
        next.equipment_entry[slot]=entry;next.equipment_display[slot]=0;next.equipment_type[slot]=0;
        if(!entry)next.equipment_ready_mask|=1u<<slot;
        else if(item&&item->metadata){
            next.equipment_ready_mask|=1u<<slot;
            if(item->metadata==1){next.equipment_display[slot]=item->display;next.equipment_type[slot]=item->inventory_type;}
        }
    }
    // No renderer invalidation for unrelated movement/health/name updates.
    if(memcmp(&next,view,sizeof next)){next.appearance_revision++;*view=next;}
}
