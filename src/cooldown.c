#include "wx_actions.h"
#include "wx_cooldown.h"
#include "wx_wire.h"
#include "wx_spell_wire.h"

static uint32_t remaining(uint32_t start,uint32_t duration,uint32_t now){
    uint32_t elapsed=now-start;return elapsed<duration?duration-elapsed:0;
}
static int active(const WxCooldownEntry* e,uint32_t now){
    return e->held||remaining(e->start,e->duration,now)||remaining(e->start,e->category_duration,now)||remaining(e->lock_start,e->lock_duration,now);
}
static WxCooldownEntry* entry(WxCooldowns* s,uint32_t spell,uint32_t now){
    WxCooldownEntry* vacant=NULL;
    for(unsigned i=0;i<WX_COOLDOWN_LIMIT;i++){
        if(s->rows[i].spell==spell)return &s->rows[i];
        if(!vacant&&(!s->rows[i].spell||!active(&s->rows[i],now)))vacant=&s->rows[i];
    }
    if(vacant){memset(vacant,0,sizeof *vacant);vacant->spell=spell;}
    return vacant;
}
static const WxItemCooldown* item_info(const WxCooldowns* s,uint32_t item){
    for(unsigned i=0;i<WX_COOLDOWN_ITEMS;i++)if(s->items[i].item==item)return &s->items[i];
    return NULL;
}
static const WxItemSpellCooldown* item_spell(const WxCooldowns* s,uint32_t item,uint32_t spell){
    const WxItemCooldown* info=item_info(s,item);
    if(info)for(unsigned i=0;i<5;i++)if(info->spells[i].spell==spell)return &info->spells[i];
    return NULL;
}
void wx_cooldown_reset(WxCooldowns* s){if(s){memset(s,0,sizeof *s);s->cast_speed=1;}}
void wx_cooldown_unit(WxCooldowns* s,unsigned cls,float cast_speed,float ranged_ms){
    static const unsigned families[12]={0,4,10,9,8,6,0,11,3,5,0,7};
    if(!s)return;
    s->player_family=cls<12?families[cls]:0;
    s->cast_speed=isfinite(cast_speed)&&cast_speed>0&&cast_speed<=100?cast_speed:1;
    s->ranged_ms=isfinite(ranged_ms)&&ranged_ms>=0&&ranged_ms<=600000?ranged_ms:0;
}
int wx_cooldown_modifiers(const WxCooldowns* s,const WxCooldownInfo* info,unsigned op,int32_t out[2]){
    if(!out)return 0;out[0]=out[1]=0;
    if(!s||!info||op>=WX_SPELLMOD_OPS)return 0;
    if(!s->player_family||info->family!=s->player_family||(info->attributes_ex3&0x20000000u))return 1;
    int32_t values[2]={0};unsigned counts[2]={0};
    for(unsigned type=0;type<2;type++)for(unsigned bit=0;bit<64;bit++){
        if((info->family_mask[bit/32]&(1u<<(bit%32)))&&s->modifiers[type][op][bit]){
            values[type]=s->modifiers[type][op][bit];counts[type]++;
        }
    }
    // Each wire bit is a total, without the contributing aura masks. Multiple
    // matching nonzero totals cannot safely be summed or deduplicated: either
    // choice can double-count an overlapping aura. Keep this gap measurable.
    if(counts[0]>1||counts[1]>1)return 0;
    out[0]=values[0];out[1]=values[1];return 1;
}
static uint32_t modified(WxCooldowns* s,const WxCooldownInfo* info,unsigned op,uint32_t base){
    int32_t values[2];if(!wx_cooldown_modifiers(s,info,op,values)){s->modifier_ambiguous++;return base;}
    int64_t value=(int64_t)base+values[0];if(value<0)value=0;if(value>INT32_MAX)value=INT32_MAX;
    if(base&&values[1])value=value*(100LL+values[1])/100;
    return value<=0?0:value>INT32_MAX?INT32_MAX:(uint32_t)value;
}
static WxGlobalCooldown* global_timer(WxCooldowns* s,uint32_t category,uint32_t duration,uint32_t spell,uint32_t now){
    if(!duration)return NULL;
    WxGlobalCooldown* slot=NULL;
    for(unsigned i=0;i<WX_GCD_LIMIT;i++){
        WxGlobalCooldown* g=&s->global[i];
        if(g->duration&&g->category==category){
            if(remaining(g->start,g->duration,now))return g;
            slot=g;break;
        }
        if(!slot&&!remaining(g->start,g->duration,now))slot=g;
    }
    if(!slot){s->overflow++;return NULL;}
    *slot=(WxGlobalCooldown){category,now,duration,spell};s->gcd_starts++;return slot;
}
static WxGlobalCooldown* start_global(WxCooldowns* s,const WxCooldownCatalog* c,uint32_t spell,uint32_t now){
    const WxCooldownInfo* info=wx_cooldown_info(c,spell);if(!info){s->missing++;return NULL;}
    uint32_t duration=modified(s,info,21,info->gcd_time);
    if(info->gcd_category==133&&duration==1500&&info->damage_class!=2&&info->damage_class!=3&&!(info->attributes&0x12)){
        float adjusted=1500*(s->cast_speed>0?s->cast_speed:1);
        duration=adjusted<1000?1000:adjusted>1500?1500:(uint32_t)adjusted;
    }
    return global_timer(s,info->gcd_category,duration,spell,now);
}
static void cancel_preparing(WxCooldowns* s,uint32_t spell){
    if(s->pending_spell!=spell)return;
    for(unsigned i=0;i<WX_GCD_LIMIT;i++){
        WxGlobalCooldown* g=&s->global[i];
        if(g->spell==spell&&g->start==s->pending_start&&g->duration){g->duration=0;s->gcd_cancels++;}
    }
    s->pending_spell=0;
}
static void begin(WxCooldowns* s,const WxCooldownCatalog* c,uint32_t spell,uint32_t item,uint32_t now,int event){
    const WxCooldownInfo* info=wx_cooldown_info(c,spell);
    if(!info){s->missing++;return;}
    uint32_t category=info->category,duration=info->recovery,cat=info->category_recovery;
    if(item){
        const WxItemSpellCooldown* is=item_spell(s,item,spell);
        if(!is){s->missing++;return;}
        if(is->category)category=is->category;
        if(is->recovery!=UINT32_MAX)duration=is->recovery;
        if(is->category_recovery!=UINT32_MAX)cat=is->category_recovery;
    }
    int held=!event&&(info->attributes&WX_COOLDOWN_ON_EVENT)!=0;
    if(!held&&(info->attributes&2)&&!(info->attributes_ex2&0x20000)){
        if(s->ranged_ms>0){uint64_t sum=(uint64_t)duration+(uint32_t)s->ranged_ms;duration=sum>INT32_MAX?INT32_MAX:(uint32_t)sum;}
        else s->missing++;
    }
    if(!duration&&!cat&&!held){
        if(event)for(unsigned i=0;i<WX_COOLDOWN_LIMIT;i++)if(s->rows[i].spell==spell){s->rows[i].duration=s->rows[i].category_duration=s->rows[i].held=0;}
        return;
    }
    if(!held){
        if(duration)duration=modified(s,info,11,duration);
        else if(category&&cat)cat=modified(s,info,11,cat);
    }
    WxCooldownEntry* e=entry(s,spell,now);if(!e){s->overflow++;return;}
    e->item=item;e->category=category;e->start=now;e->duration=duration;e->category_duration=cat;e->held=held;
    if(!held&&duration&&category==info->category&&(info->category_flags&2))global_timer(s,133,duration,spell,now);
}
static int read_item(WxReader* r,WxItemCooldown* out){
    uint32_t id=wx_read(r,4);out->item=id&0x7fffffff;
    if(!out->item)return 0;
    if(id&0x80000000)return r->ok&&r->at==r->size;
    wx_read(r,4);wx_read(r,4);
    for(unsigned i=0;i<4;i++)wx_string(r,NULL,256);
    for(unsigned i=0;i<5;i++)wx_read(r,4);
    if(wx_read(r,4)>28)return 0;
    for(unsigned i=0;i<34;i++)wx_read(r,4);
    for(unsigned i=0;i<5;i++){wx_real(r);wx_real(r);wx_read(r,4);}
    for(unsigned i=0;i<9;i++)wx_read(r,4);
    wx_real(r);
    for(unsigned i=0;i<5;i++){
        WxItemSpellCooldown* a=&out->spells[i];a->spell=wx_read(r,4);a->trigger=wx_read(r,4);uint32_t charges=wx_read(r,4);memcpy(&a->charges,&charges,4);
        a->recovery=wx_read(r,4);a->category=wx_read(r,4);a->category_recovery=wx_read(r,4);
        if(a->spell>65535||a->category>65535||(a->recovery>0x7fffffffu&&a->recovery!=UINT32_MAX)||(a->category_recovery>0x7fffffffu&&a->category_recovery!=UINT32_MAX))return 0;
    }
    wx_read(r,4);wx_string(r,NULL,65534);
    for(unsigned i=0;i<14;i++)wx_read(r,4);
    return r->ok&&r->at==r->size;
}
int wx_cooldown_apply(WxCooldowns* s,const WxCooldownCatalog* c,const WxInventory* inventory,
    uint16_t opcode,const uint8_t* data,size_t size,uint64_t player,uint32_t now){
    if(opcode!=0x12a&&opcode!=0x134&&opcode!=0x135&&opcode!=0x1de&&opcode!=0x132&&opcode!=0x131&&opcode!=0x2a6&&opcode!=0x266&&opcode!=0x267&&opcode!=0x58)return -1;
    if(!s||(!data&&size)||size>65533)return 0;
    WxReader r={data,size,0,1};
    if(opcode==0x266||opcode==0x267){
        unsigned bit=wx_read(&r,1),op=wx_read(&r,1);uint32_t bits=wx_read(&r,4);int32_t value;memcpy(&value,&bits,4);
        if(bit>=64||op>=WX_SPELLMOD_OPS||!r.ok||r.at!=size)return 0;
        s->modifiers[opcode==0x267][op][bit]=value;s->modifier_updates++;
    }else if(opcode==0x2a6){
        uint64_t caster=wx_guid(&r,0);uint32_t spell=wx_read(&r,4);
        if(!spell||spell>65535||!r.ok||r.at!=size)return 0;
        if(caster!=player)return 1;
        cancel_preparing(s,spell);
    }else if(opcode==0x58){
        WxItemCooldown item={0};if(!read_item(&r,&item))return 0;
        unsigned at=WX_COOLDOWN_ITEMS;
        for(unsigned i=0;i<WX_COOLDOWN_ITEMS;i++)if(s->items[i].item==item.item){at=i;break;}
        if(at==WX_COOLDOWN_ITEMS)at=s->item_cursor++%WX_COOLDOWN_ITEMS;
        s->items[at]=item;
    }else if(opcode==0x135||opcode==0x1de){
        uint32_t spell=wx_read(&r,4);uint64_t guid=wx_guid(&r,0);
        if(!spell||spell>65535||!r.ok||r.at!=size)return 0;
        if(guid!=player)return 1;
        uint32_t item=0;
        for(unsigned i=0;i<WX_COOLDOWN_LIMIT;i++)if(s->rows[i].spell==spell){
            item=s->rows[i].item;
            if(opcode==0x1de)memset(&s->rows[i],0,sizeof s->rows[i]);
        }
        if(opcode==0x135)begin(s,c,spell,item,now,1);
    }else if(opcode==0x132||opcode==0x131){
        WxSpellEvent event;if(!wx_spell_event(opcode,data,size,&event))return 0;
        uint64_t caster=event.caster,unit=event.unit;uint32_t spell=event.spell;
        if(unit!=player)return 1;
        if(opcode==0x131){
            WxGlobalCooldown* g=start_global(s,c,spell,now);s->pending_spell=spell;s->pending_start=g?g->start:now;
            s->packets++;s->revision++;return 1;
        }
        if(s->pending_spell==spell)s->pending_spell=0; // completion does not restart or later cancel its GCD
        uint32_t item=0;
        if(caster!=unit){
            if(inventory&&inventory->count<=WX_INVENTORY_SLOTS)for(unsigned i=0;i<inventory->count;i++)if(inventory->items[i].guid==caster){item=inventory->items[i].entry;break;}
            if(!item){s->missing++;s->packets++;s->revision++;return 1;}
        }
        begin(s,c,spell,item,now,0);
    }else{
        struct Incoming {uint32_t spell,item,category,duration,cat;} incoming[WX_COOLDOWN_LIMIT];
        unsigned count=0;
        uint64_t guid=player;
        if(opcode==0x12a){
            if(wx_read(&r,1))return 0;
            unsigned spells=wx_read(&r,2);if(spells>512)return 0;
            for(unsigned i=0;i<spells;i++){if(!wx_read(&r,2))return 0;wx_read(&r,2);}
            count=wx_read(&r,2);if(count>WX_COOLDOWN_LIMIT)return 0;
        }else{
            guid=wx_guid(&r,0);if(!r.ok||(size-r.at)%8)return 0;
            count=(unsigned)(size-r.at)/8;if(count>WX_COOLDOWN_LIMIT)return 0;
        }
        for(unsigned i=0;i<count;i++){
            struct Incoming* p=&incoming[i];memset(p,0,sizeof *p);p->spell=wx_read(&r,opcode==0x12a?2:4);
            if(opcode==0x12a){p->item=wx_read(&r,2);p->category=wx_read(&r,2);}
            p->duration=wx_read(&r,4);if(opcode==0x12a)p->cat=wx_read(&r,4);
            if(!p->spell||p->spell>65535||p->duration>0x7fffffffu)return 0;
            for(unsigned j=0;j<i;j++)if(incoming[j].spell==p->spell)return 0;
        }
        if(!r.ok||r.at!=size)return 0;
        if(guid!=player)return 1;
        if(opcode==0x12a){memset(s->rows,0,sizeof s->rows);memset(s->global,0,sizeof s->global);s->pending_spell=0;}
        else{
            unsigned free_slots=0,needed=0;
            for(unsigned j=0;j<WX_COOLDOWN_LIMIT;j++)if(!s->rows[j].spell||!active(&s->rows[j],now))free_slots++;
            for(unsigned i=0;i<count;i++){
                unsigned found=0;for(unsigned j=0;j<WX_COOLDOWN_LIMIT;j++)if(s->rows[j].spell==incoming[i].spell&&active(&s->rows[j],now)){found=1;break;}
                if(!found&&incoming[i].duration)needed++;
            }
            if(needed>free_slots){s->overflow++;s->packets++;s->revision++;return 1;}
        }
        for(unsigned i=0;i<count;i++){
            struct Incoming* p=&incoming[i];
            if(opcode==0x134&&!p->duration){
                // Vanilla's zero-duration spell cooldown is a GCD-only hint
                // (for example an in-combat weapon switch), not a lockout clear.
                start_global(s,c,p->spell,now);
                continue;
            }
            WxCooldownEntry* e=entry(s,p->spell,now);
            if(!e){s->overflow++;continue;}
            if(opcode==0x12a){e->item=p->item;e->category=p->category;e->start=now;e->duration=p->duration;e->category_duration=p->cat&0x7fffffffu;e->held=p->cat>>31;}
            else{e->lock_start=now;e->lock_duration=p->duration;}
        }
    }
    s->packets++;s->revision++;return 1;
}
static void consider(WxCooldownView* out,uint32_t start,uint32_t duration,uint32_t now,unsigned held){
    uint32_t ms=remaining(start,duration,now);
    if(held){out->held=1;out->remaining=out->duration=out->global=0;}
    else if(!out->held&&ms>out->remaining){out->remaining=ms;out->duration=duration;out->global=0;}
}
WxCooldownView wx_cooldown_query(const WxCooldowns* s,const WxCooldownCatalog* c,uint32_t binding,uint32_t now){
    WxCooldownView out={0};uint32_t id=binding&0xffffff,type=binding>>24;
    if(!s||!id||(type!=0&&type!=0x80))return out;
    const WxCooldownInfo* info=type==0?wx_cooldown_info(c,id):NULL;
    const WxItemCooldown* item=type==0x80?item_info(s,id):NULL;
    for(unsigned i=0;i<WX_COOLDOWN_LIMIT;i++){
        const WxCooldownEntry* e=&s->rows[i];if(!e->spell)continue;
        unsigned direct=type==0?e->spell==id:e->item==id;
        unsigned category=info&&e->category&&info->category==e->category;
        if(item)for(unsigned j=0;j<5;j++){
            const WxItemSpellCooldown* is=&item->spells[j];if(!is->spell||is->trigger!=0)continue;
            const WxCooldownInfo* si=wx_cooldown_info(c,is->spell);
            unsigned cat=is->category?is->category:si?si->category:0;
            if(cat&&cat==e->category)category=1;
            if(is->spell==e->spell)direct=1;
        }
        if(direct){consider(&out,e->start,e->duration,now,e->held);consider(&out,e->lock_start,e->lock_duration,now,0);}
        if(direct||category)consider(&out,e->start,e->category_duration,now,e->held);
    }
    for(unsigned i=0;i<WX_GCD_LIMIT;i++){
        const WxGlobalCooldown* g=&s->global[i];unsigned match=info&&(info->gcd_category||info->gcd_time)&&info->gcd_category==g->category;
        if(item)for(unsigned j=0;j<5;j++)if(item->spells[j].spell&&item->spells[j].trigger==0){
            const WxCooldownInfo* si=wx_cooldown_info(c,item->spells[j].spell);
            if(si&&(si->gcd_category||si->gcd_time)&&si->gcd_category==g->category)match=1;
        }
        uint32_t ms=remaining(g->start,g->duration,now);
        if(match&&!out.held&&ms>out.remaining){out.remaining=ms;out.duration=g->duration;out.global=1;}
    }
    return out;
}

const WxCooldownInfo* wx_cooldown_info(const WxCooldownCatalog* c,uint32_t spell){
    if(!c||!c->ready)return NULL;
    unsigned lo=0,hi=c->count;while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(c->rows[mid].spell<spell)lo=mid+1;else hi=mid;}
    return lo<c->count&&c->rows[lo].spell==spell?&c->rows[lo]:NULL;
}

void wx_action_modifiers(const WxCooldowns* s,const WxCooldownCatalog* c,uint32_t binding,WxActionModifiers* out){
    memset(out,0,sizeof *out);if(!s)return;unsigned id=binding&0xffffff,type=binding>>24;
    if(type==0){const WxCooldownInfo* info=wx_cooldown_info(c,id);
        if(wx_cooldown_modifiers(s,info,14,out->cost))out->known|=1;
        if(wx_cooldown_modifiers(s,info,5,out->range))out->known|=2;
    }else if(type==0x80)for(unsigned i=0;i<WX_COOLDOWN_ITEMS;i++)if(s->items[i].item==id){
        out->known=4;
        for(unsigned j=0;j<5;j++)if(s->items[i].spells[j].spell&&s->items[i].spells[j].trigger==0&&s->items[i].spells[j].charges){out->charge_slot=j+1;break;}
        break;
    }
}
