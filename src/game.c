#include "wx_game.h"
#include "wx_wire.h"
#include <stdio.h>
static void write_bytes(uint8_t* p,uint64_t value,unsigned count){for(unsigned i=0;i<count;i++)p[i]=(uint8_t)(value>>(i*8));}
int wx_command_encode(const WxCommand* c,uint8_t* out,unsigned capacity){
    if(!c||!out)return -1;unsigned size=0;
    switch(c->opcode){
    case 0x12f:case 0x13b: // Vanilla cancel cast/channel: spell ID only.
        if(c->guid||c->extra||!c->value||c->value>65535||capacity<4)return -1;
        write_bytes(out,c->value,4);return 4;
    case 0x128: // Vanilla: uint8 button, uint32 packed action. Spell/clear here.
        if(c->guid||c->extra>=120||c->value>65535||capacity<5)return -1;
        out[0]=(uint8_t)c->extra;write_bytes(out+1,c->value,4);return 5;
    case 0x5c: // Vanilla quest query: quest ID only.
        if(!c->value||c->value>=0x80000000||capacity<4)return -1;write_bytes(out,c->value,4);return 4;
    case 0x1a2: // vendor GUID, entry, number of bundles, unused byte
        if(!c->guid||!c->value||!c->extra||c->extra>255||capacity<14)return -1;
        write_bytes(out,c->guid,8);write_bytes(out+8,c->value,4);out[12]=(uint8_t)c->extra;out[13]=0;return 14;
    case 0x1a0: // Sell one; value/extra contain the item's complete GUID.
        if(!c->guid||(!c->value&&!c->extra)||capacity<17)return -1;
        write_bytes(out,c->guid,8);write_bytes(out+8,c->value,4);write_bytes(out+12,c->extra,4);out[16]=1;return 17;
    case 0xab:{
        if((c->value!=255&&(c->value<19||c->value>22))||c->extra>38||capacity<14)return -1;
        out[0]=(uint8_t)c->value;out[1]=(uint8_t)c->extra;out[2]=0;write_bytes(out+3,c->guid?2:0,2);if(!c->guid)return 5;
        unsigned at=6;out[5]=0;for(unsigned i=0;i<8;i++)if((c->guid>>(8*i))&255){out[5]|=1u<<i;out[at++]=(uint8_t)(c->guid>>(8*i));}return (int)at;
    }
    case 0x12e:{
        if(!c->value||c->value>65535||capacity<15)return -1;
        write_bytes(out,c->value,4);write_bytes(out+4,c->guid?2:0,2);if(!c->guid)return 6;
        unsigned at=7;out[6]=0;for(unsigned i=0;i<8;i++)if((c->guid>>(8*i))&255){out[6]|=1u<<i;out[at++]=(uint8_t)(c->guid>>(8*i));}return (int)at;
    }
    case 0x13d:size=8;break; // selection (zero clears)
    case 0x141:case 0x184:case 0x17b:case 0x15d:case 0x15f:case 0x1d2:case 0x21c:case 0x19e:if(!c->guid)return -1;size=8;break;
    case 0x15a:if(!capacity)return -1;out[0]=1;return 1; // Vanilla manual release
    case 0x142:case 0x15e:case 0x190:case 0x216:return 0;
    case 0x60:if(!c->guid||!c->value||c->value>=0x80000000)return -1;size=12;break;
    case 0x56:case 0x2c4:if(!c->value||c->value>=0x80000000)return -1;size=12;break;
    case 0x186:case 0x189:case 0x18a:case 0x18c:
        if(!c->guid||!c->value)return -1;size=12;break;
    case 0x17c:if(!c->guid)return -1;size=12;break;
    case 0x18e:if(!c->guid||!c->value||c->extra>=6)return -1;size=16;break;
    case 0x108:if(c->value>255||capacity<1)return -1;out[0]=(uint8_t)c->value;return 1;
    case 0x10a:
        if((c->value!=255&&(c->value<19||c->value>22))||c->extra>38||capacity<2)return -1;
        out[0]=(uint8_t)c->value;out[1]=(uint8_t)c->extra;return 2;
    case 0x10b:
        if(c->value!=255||c->extra>=23||capacity<3)return -1;
        out[0]=255;out[1]=(uint8_t)c->extra;out[2]=255;return 3;
    default:return -1;
    }
    if(capacity<size)return -1;
    if(c->opcode==0x60||c->opcode==0x56||c->opcode==0x2c4){write_bytes(out,c->value,4);write_bytes(out+4,c->guid,8);}
    else {write_bytes(out,c->guid,8);if(size>=12)write_bytes(out+8,c->value,4);if(size==16)write_bytes(out+12,c->extra,4);}
    return size;
}
static void rewards(WxReader* r,WxDialog* d){
    d->choice_count=wx_read(r,4);if(d->choice_count>6){r->ok=0;return;}
    for(unsigned i=0;i<d->choice_count;i++){d->choices[i].item=wx_read(r,4);d->choices[i].count=wx_read(r,4);d->choices[i].display=wx_read(r,4);}
    d->reward_count=wx_read(r,4);if(d->reward_count>4){r->ok=0;return;}
    for(unsigned i=0;i<d->reward_count;i++){d->rewards[i].item=wx_read(r,4);d->rewards[i].count=wx_read(r,4);d->rewards[i].display=wx_read(r,4);}
    d->money=wx_read(r,4);
}
static void emotes(WxReader* r){unsigned count=wx_read(r,4);if(count>4){r->ok=0;return;}for(unsigned i=0;i<count;i++){wx_read(r,4);wx_read(r,4);}}
static void new_dialog(WxDialog* d,unsigned screen){unsigned revision=d->revision+1;memset(d,0,sizeof *d);d->revision=revision;d->screen=screen;}
int wx_has_spell(const WxGame* game,uint32_t spell){for(unsigned i=0;i<game->spell_count;i++)if(game->spells[i]==spell)return 1;return 0;}
unsigned wx_action_slot(unsigned slot,unsigned form){
    if(slot>=120)return 120;
    // Vanilla warrior stance bars occupy server slots 72, 84 and 96.
    // Other classes' bonus bars still need their form mappings.
    if(slot<12&&form>=0x11&&form<=0x13)slot+=72+(form-0x11)*12;
    return slot;
}
uint32_t wx_action_binding(const WxGame* game,unsigned slot,unsigned form){slot=wx_action_slot(slot,form);return slot<120?game->actions[slot]:0;}
const WxItemName* wx_item_info(const WxGame* game,uint32_t id){
    if(game&&id)for(unsigned i=0;i<WX_ITEM_NAMES;i++)if(game->item_names[i].id==id)return &game->item_names[i];
    return 0;
}
static WxItemName* item_slot(WxGame* game,uint32_t id){
    for(unsigned i=0;i<WX_ITEM_NAMES;i++)if(game->item_names[i].id==id)return &game->item_names[i];
    WxItemName* item=&game->item_names[game->item_cursor++%WX_ITEM_NAMES];
    memset(item,0,sizeof *item);item->id=id;return item;
}
const char* wx_item_name(const WxGame* game,uint32_t id){
    const WxItemName* item=wx_item_info(game,id);if(item)return item->name;
    return "Loading item name...";
}
int wx_game_apply(WxGame* game,uint16_t opcode,const uint8_t* data,size_t size,uint64_t player){
    if(!game||(!data&&size)||size>65533)return 0;
    if(opcode==0xa9||opcode==0x1f6||opcode==0xdd||opcode==0xaa)return -1;
    WxGame next=*game;WxReader r={data,size,0,1};
    WxDialog* d=&next.dialog;
    switch(opcode){
    case 0x19f:
        new_dialog(d,WX_SCREEN_VENDOR);d->guid=wx_guid(&r,0);d->vendor_count=wx_read(&r,1);
        if(!d->guid||d->vendor_count>WX_VENDOR_ITEMS)return 0;
        if(!d->vendor_count){wx_read(&r,1);snprintf(next.message,sizeof next.message,"This vendor has no stock");}
        for(unsigned i=0;i<d->vendor_count;i++){
            WxVendorItem* item=&d->vendor[i];item->slot=wx_read(&r,4);item->item=wx_read(&r,4);item->display=wx_read(&r,4);
            item->stock=wx_read(&r,4);item->price=wx_read(&r,4);item->durability=wx_read(&r,4);item->count=wx_read(&r,4);
            if(!item->slot||!item->item||!item->count)return 0;
            for(unsigned j=0;j<i;j++)if(d->vendor[j].slot==item->slot)return 0;
        }break;
    case 0x1a4:{
        uint64_t vendor=wx_guid(&r,0);unsigned slot=wx_read(&r,4),stock=wx_read(&r,4),count=wx_read(&r,4);
        if(!vendor||!slot||!count)return 0;next.bundles_bought+=count;
        if(d->screen==WX_SCREEN_VENDOR&&d->guid==vendor)for(unsigned i=0;i<d->vendor_count;i++)if(d->vendor[i].slot==slot)d->vendor[i].stock=stock;
        snprintf(next.message,sizeof next.message,"Purchase complete: %u bundles",count);break;
    }
    case 0x1a5:{wx_guid(&r,0);wx_read(&r,4);unsigned reason=wx_read(&r,1);
        snprintf(next.message,sizeof next.message,"Purchase declined (%u)",reason);break;}
    case 0x1a1:{wx_guid(&r,0);wx_guid(&r,0);unsigned reason=wx_read(&r,1);
        if(!reason)next.items_sold++;
        snprintf(next.message,sizeof next.message,reason?"Sale declined (%u)":"Sale complete",reason);break;}
    case 0x269:
        next.corpse_delay_ms=wx_read(&r,4);if(next.corpse_delay_ms>3600000)return 0;
        next.corpse_delay_revision++;break;
    case 0x216:
        next.corpse_known=wx_read(&r,1);if(next.corpse_known>1)return 0;
        if(next.corpse_known){next.corpse_map=wx_read(&r,4);
            for(unsigned i=0;i<3;i++){next.corpse_position[i]=wx_real(&r);if(fabsf(next.corpse_position[i])>20000)return 0;}
            next.corpse_instance_map=wx_read(&r,4);if(next.corpse_map>65535||next.corpse_instance_map>65535)return 0;}
        break;
    case 0x129:for(unsigned i=0;i<120;i++)next.actions[i]=wx_read(&r,4);next.actions_ready=1;next.action_revision++;break;
    case 0x12a:{
        if(wx_read(&r,1)!=0)return 0;next.spell_count=wx_read(&r,2);if(next.spell_count>512)return 0;
        for(unsigned i=0;i<next.spell_count;i++){next.spells[i]=(uint16_t)wx_read(&r,2);wx_read(&r,2);if(!next.spells[i])return 0;}
        unsigned cooldowns=wx_read(&r,2);if(cooldowns>512)return 0;
        for(unsigned i=0;i<cooldowns;i++){wx_read(&r,2);wx_read(&r,2);wx_read(&r,2);wx_read(&r,4);wx_read(&r,4);}break;
    }
    case 0x12b:{unsigned spell=wx_read(&r,4);if(!spell||spell>65535)return 0;
        if(!wx_has_spell(&next,spell)){if(next.spell_count==512)return 0;next.spells[next.spell_count++]=(uint16_t)spell;}break;}
    case 0x203:{unsigned spell=wx_read(&r,2);for(unsigned i=0;i<next.spell_count;i++)if(next.spells[i]==spell){next.spells[i]=next.spells[--next.spell_count];break;}break;}
    case 0x130:{
        next.last_spell=wx_read(&r,4);unsigned result=wx_read(&r,1);
        if(result==0){next.casts_accepted++;snprintf(next.message,sizeof next.message,"Spell accepted");}
        else if(result==2){unsigned reason=wx_read(&r,1);if(r.size-r.at!=0&&r.size-r.at!=4&&r.size-r.at!=8)return 0;while(r.ok&&r.at<r.size)wx_read(&r,4);
            next.casts_rejected++;snprintf(next.message,sizeof next.message,"Spell unavailable (%u)",reason);}
        else return 0;break;
    }
    case 0x17d:{
        new_dialog(d,WX_SCREEN_QUESTS);d->guid=wx_guid(&r,0);wx_read(&r,4);
        d->gossip_count=wx_read(&r,4);if(d->gossip_count>15)return 0;
        for(unsigned i=0;i<d->gossip_count;i++){
            d->gossip[i].id=wx_read(&r,4);wx_read(&r,1);d->gossip[i].coded=(uint8_t)wx_read(&r,1);
            wx_string(&r,d->gossip[i].text,sizeof d->gossip[i].text);
        }
        d->quest_count=wx_read(&r,4);if(d->quest_count>32)return 0;
        for(unsigned i=0;i<d->quest_count;i++){
            d->quests[i].id=wx_read(&r,4);d->quests[i].icon=wx_read(&r,4);d->quests[i].level=wx_read(&r,4);wx_string(&r,d->quests[i].title,sizeof d->quests[i].title);
        }break;
    }
    case 0x112:{
        unsigned reason=wx_read(&r,1),level=0;
        if(reason){if(reason==1)level=wx_read(&r,4);wx_guid(&r,0);wx_guid(&r,0);wx_read(&r,1);}
        if(reason==1)snprintf(next.message,sizeof next.message,"Requires level %u",level);
        else snprintf(next.message,sizeof next.message,reason?"Item action rejected (%u)":"Item action complete",reason);
        break;
    }
    case 0x58:{
        unsigned wire_id=wx_read(&r,4),id=wire_id&0x7fffffff;if(!id)return 0;
        WxItemName* item=item_slot(&next,id);
        if(wire_id&0x80000000){
            item->display=0;item->inventory_type=0;item->metadata=2;
            snprintf(item->name,sizeof item->name,"Unavailable item %u",id);break;
        }
        wx_read(&r,4);wx_read(&r,4); // class, subclass
        wx_string(&r,item->name,sizeof item->name);
        for(unsigned i=0;i<3;i++)wx_string(&r,0,256);
        item->display=wx_read(&r,4);
        for(unsigned i=0;i<4;i++)wx_read(&r,4); // quality, flags, buy/sell prices
        unsigned inventory_type=wx_read(&r,4);if(inventory_type>28)return 0;
        item->inventory_type=(uint8_t)inventory_type;
        for(unsigned i=0;i<14+20;i++)wx_read(&r,4); // restrictions/stack/bag, ten stat pairs
        for(unsigned i=0;i<5;i++){wx_real(&r);wx_real(&r);wx_read(&r,4);}
        for(unsigned i=0;i<9;i++)wx_read(&r,4); // seven resistances, delay, ammo
        wx_real(&r); // ranged modifier (5875)
        for(unsigned i=0;i<30+1;i++)wx_read(&r,4); // five spell records, bonding
        wx_string(&r,0,65534); // description; validate but do not retain
        for(unsigned i=0;i<14;i++)wx_read(&r,4); // page through bag family
        item->metadata=1;break;
    }
    case 0x2c5:{
        unsigned id=wx_read(&r,4);if(!id||id>=0x80000000)return 0;
        WxItemName* item=item_slot(&next,id);wx_string(&r,item->name,sizeof item->name);break;
    }
    case 0x185:{
        new_dialog(d,WX_SCREEN_QUESTS);d->guid=wx_guid(&r,0);wx_string(&r,d->text,sizeof d->text);wx_read(&r,4);wx_read(&r,4);
        d->quest_count=wx_read(&r,1);if(d->quest_count>32)return 0;
        for(unsigned i=0;i<d->quest_count;i++){
            d->quests[i].id=wx_read(&r,4);d->quests[i].icon=wx_read(&r,4);d->quests[i].level=wx_read(&r,4);wx_string(&r,d->quests[i].title,sizeof d->quests[i].title);
        }break;
    }
    case 0x188:{
        new_dialog(d,WX_SCREEN_DETAILS);d->guid=wx_guid(&r,0);d->quest=wx_read(&r,4);
        wx_string(&r,d->title,sizeof d->title);wx_string(&r,d->text,sizeof d->text);wx_string(&r,d->objectives,sizeof d->objectives);
        wx_read(&r,4);rewards(&r,d);d->spell=wx_read(&r,4);emotes(&r);break;
    }
    case 0x18b:{
        new_dialog(d,WX_SCREEN_REQUEST);d->guid=wx_guid(&r,0);d->quest=wx_read(&r,4);
        wx_string(&r,d->title,sizeof d->title);wx_string(&r,d->text,sizeof d->text);
        wx_read(&r,4);wx_read(&r,4);wx_read(&r,4);d->money=wx_read(&r,4);
        unsigned count=wx_read(&r,4);if(count>4)return 0;
        for(unsigned i=0;i<count;i++){wx_read(&r,4);wx_read(&r,4);wx_read(&r,4);}
        d->can_complete=1;for(unsigned i=0;i<4;i++)if(!wx_read(&r,4))d->can_complete=0;break;
    }
    case 0x18d:{
        new_dialog(d,WX_SCREEN_REWARD);d->guid=wx_guid(&r,0);d->quest=wx_read(&r,4);
        wx_string(&r,d->title,sizeof d->title);wx_string(&r,d->text,sizeof d->text);
        wx_read(&r,4);emotes(&r);rewards(&r,d);wx_read(&r,4);d->spell=wx_read(&r,4);break;
    }
    case 0x191:{
        next.completed_quest=wx_read(&r,4);wx_read(&r,4);next.reward_xp=wx_read(&r,4);next.reward_money=wx_read(&r,4);
        unsigned count=wx_read(&r,4);if(count>4)return 0;for(unsigned i=0;i<count;i++){wx_read(&r,4);wx_read(&r,4);}
        new_dialog(d,WX_SCREEN_NONE);snprintf(next.message,sizeof next.message,"Quest complete / %u XP earned",next.reward_xp);break;
    }
    case 0x198:wx_read(&r,4);snprintf(next.message,sizeof next.message,"Quest objectives complete");break;
    case 0x199:{
        wx_read(&r,4);wx_read(&r,4);unsigned count=wx_read(&r,4),required=wx_read(&r,4);wx_guid(&r,0);
        snprintf(next.message,sizeof next.message,"Quest progress: %u/%u",count,required);break;
    }
    case 0x18f:case 0x192:case 0x195:{
        if(opcode==0x192)wx_read(&r,4);if(opcode!=0x195)wx_read(&r,4);
        snprintf(next.message,sizeof next.message,"%s",opcode==0x195?"Quest log is full":"Quest request declined");break;
    }
    case 0x17e:new_dialog(d,WX_SCREEN_NONE);break;
    case 0x160:{
        new_dialog(d,WX_SCREEN_LOOT);d->guid=wx_guid(&r,0);unsigned type=wx_read(&r,1);
        if(!type){unsigned reason=wx_read(&r,1);d->screen=WX_SCREEN_NONE;snprintf(next.message,sizeof next.message,"Cannot loot this target (%u)",reason);break;}
        d->money=wx_read(&r,4);d->loot_count=wx_read(&r,1);if(d->loot_count>16)return 0;
        for(unsigned i=0;i<d->loot_count;i++){
            WxLootItem* item=&d->loot[i];item->slot=(uint8_t)wx_read(&r,1);item->item=wx_read(&r,4);item->count=wx_read(&r,4);item->display=wx_read(&r,4);
            wx_read(&r,4);item->property=wx_read(&r,4);item->kind=(uint8_t)wx_read(&r,1);
            for(unsigned j=0;j<i;j++)if(d->loot[j].slot==item->slot)return 0;
        }break;
    }
    case 0x161:{uint64_t guid=wx_guid(&r,0);wx_read(&r,1);if(guid==d->guid)new_dialog(d,WX_SCREEN_NONE);break;}
    case 0x162:{
        unsigned slot=wx_read(&r,1);
        if(d->screen==WX_SCREEN_LOOT){
            for(unsigned i=0;i<d->loot_count;i++)if(d->loot[i].slot==slot){
                next.looted_items+=d->loot[i].count;memmove(&d->loot[i],&d->loot[i+1],(d->loot_count-i-1)*sizeof d->loot[0]);d->loot_count--;break;
            }
        }
        break;
    }
    case 0x165:if(d->screen==WX_SCREEN_LOOT)d->money=0;break;
    case 0x163:{unsigned money=wx_read(&r,4);next.looted_money+=money;snprintf(next.message,sizeof next.message,"Looted %u copper",money);break;}
    case 0x61:{
        unsigned entry=wx_read(&r,4);next.name_entry=entry&0x7fffffff;
        if(entry&0x80000000){next.target_name[0]=0;break;}
        wx_string(&r,next.target_name,sizeof next.target_name);
        for(unsigned i=0;i<4;i++)wx_string(&r,0,256); // three localized names and subtitle
        for(unsigned i=0;i<7;i++)wx_read(&r,4);wx_read(&r,1);wx_read(&r,1);
        break;
    }
    case 0x143:{
        uint64_t attacker=wx_guid(&r,0),victim=wx_guid(&r,0);
        if(attacker==player){next.attack_target=victim;snprintf(next.message,sizeof next.message,"Auto attack active");}break;
    }
    case 0x144:{
        uint64_t attacker=wx_guid(&r,1);wx_guid(&r,1);wx_read(&r,4);
        if(attacker==player){next.attack_target=0;snprintf(next.message,sizeof next.message,"Auto attack stopped");}break;
    }
    case 0x145:case 0x146:case 0x147:case 0x148:case 0x149:{
        const char* labels[]={"Move closer to attack","Face your target","Stand to attack","Target is dead","Cannot attack that target"};
        snprintf(next.message,sizeof next.message,"%s",labels[opcode-0x145]);break;
    }
    case 0x14a:{
        uint32_t flags=wx_read(&r,4);uint64_t attacker=wx_guid(&r,1),victim=wx_guid(&r,1);
        unsigned damage=wx_read(&r,4),count=wx_read(&r,1);if(count>8)return 0;
        for(unsigned i=0;i<count;i++){wx_read(&r,4);wx_real(&r);wx_read(&r,4);wx_read(&r,4);wx_read(&r,4);}
        wx_read(&r,4);wx_read(&r,4);unsigned melee_spell=wx_read(&r,4);wx_read(&r,4);
        // Vanilla HITINFO_DEBUG appends a fixed debug payload; never guess a later layout.
        if(flags&1){wx_read(&r,4);for(unsigned i=0;i<18;i++)wx_real(&r);wx_read(&r,4);}
        if(attacker==player){next.damage_dealt+=damage;next.last_victim=victim;if(melee_spell){next.last_spell=melee_spell;next.spell_hits++;}snprintf(next.message,sizeof next.message,"Hit for %u damage",damage);}
        if(victim==player){next.damage_received+=damage;snprintf(next.message,sizeof next.message,"Took %u damage",damage);}break;
    }
    case 0x1ee: // Auth response is handled by the world session.
        return -1;
    case 0x1f5:{ // SMSG_PARTYKILLLOG
        uint64_t killer=wx_guid(&r,0),victim=wx_guid(&r,0);
        if(killer==player){next.kills++;next.last_victim=victim;next.attack_target=0;snprintf(next.message,sizeof next.message,"Defeated target / X to loot");}break;
    }
    default:return -1;
    }
    if(!r.ok||r.at!=r.size)return 0;next.event_revision++;*game=next;return 1;
}
