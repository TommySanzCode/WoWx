#include "wx_actions.h"
#include <string.h>
#include <math.h>
#include <limits.h>
static uint32_t clamp(double value){return value<0?0:value>INT32_MAX?INT32_MAX:(uint32_t)value;}
static float field_float(const WxEntity* e,unsigned index){float value;memcpy(&value,&e->fields[index],4);return value;}
static int32_t field_int(const WxEntity* e,unsigned index){int32_t value;memcpy(&value,&e->fields[index],4);return value;}
static int cost(const WxSpellInfo* info,const WxEntity* self,const WxActionModifiers* mod,uint32_t* amount,uint32_t* power){
    if(info->power_type>4&&info->power_type!=UINT32_MAX)return 0;
    *power=info->power_type==UINT32_MAX?self->fields[22]:self->fields[23+info->power_type];
    if(info->attributes_ex&2){*amount=*power;return 1;}
    // Per-skill scaling needs the private skill-line state, not an assumed player level.
    if(info->cost_per_level||!mod||(mod->known&1)==0)return 0;
    double value=info->cost;
    if(info->cost_percent){unsigned base=info->power_type==UINT32_MAX?self->fields[163]:info->power_type==0?self->fields[162]:self->fields[29+info->power_type];
        value+=(uint64_t)info->cost_percent*base/100;
    }
    value+=field_int(self,173+info->school);double base=value;
    value+=mod->cost[0];if(base!=0)value*=1.0+mod->cost[1]/100.0;
    if(info->attributes&0x80000){
        if(!self->fields[34])return 0;double divisor=1.117*info->spell_level/self->fields[34]-.1327;
        if(divisor<=0)return 0;value/=divisor;
    }
    float multiplier=field_float(self,180+info->school);if(!isfinite(multiplier)||multiplier< -1||multiplier>100)return 0;
    value*=1.0+multiplier;*amount=clamp(value);return 1;
}
static int unit_target(const WxSpellInfo* s){
    for(unsigned i=0;i<3;i++){unsigned a=s->target_a[i],b=s->target_b[i];
        if(a==6||a==21||a==25||a==35||a==37||a==45||b==6||b==21||b==25||b==35||b==37||b==45)return 1;
    }return 0;
}
WxActionFeedback wx_action_feedback(uint32_t binding,const WxSpellBook* b,const WxGame* g,const WxInventory* inventory,
    const WxEntity* self,const WxEntity* target,const float position[3],const WxActionModifiers* mod){
    WxActionFeedback out={0};unsigned id=binding&0xffffff,type=binding>>24;if(!id)return out;
    if(type==0x80){
        if(!inventory){out.flags=WX_ACTION_UNKNOWN;return out;}out.flags=WX_ACTION_COUNT_KNOWN;
        const WxInventoryItem* first=NULL;
        for(unsigned i=0;i<inventory->count&&i<WX_INVENTORY_SLOTS;i++)if(inventory->items[i].entry==id){
            if(!first)first=&inventory->items[i];uint64_t sum=(uint64_t)out.count+inventory->items[i].count;out.count=sum>UINT32_MAX?UINT32_MAX:(uint32_t)sum;
        }
        if(!first||!out.count)out.flags|=WX_ACTION_NO_ITEM;
        else if(mod&&(mod->known&4)&&mod->charge_slot&&mod->charge_slot<=5){
            int64_t charges=first->charges[mod->charge_slot-1];out.charges=(uint32_t)(charges<0?-charges:charges);out.flags|=WX_ACTION_CHARGES_KNOWN;
        }
        return out;
    }
    if(type){out.flags=WX_ACTION_UNSUPPORTED;return out;}
    if(!g||!wx_has_spell(g,id)){out.flags=WX_ACTION_UNLEARNED;return out;}
    const WxKnownSpell* spell=wx_spellbook_find(b,id);
    if(!spell||spell->state!=1){out.flags=WX_ACTION_UNKNOWN;return out;}const WxSpellInfo* info=&spell->info;
    if(info->attributes&WX_SPELL_PASSIVE)out.flags|=WX_ACTION_PASSIVE;
    if(!self||!info->metadata||info->school>6){out.flags|=WX_ACTION_UNKNOWN;return out;}
    if(!self->fields[22]&&!(info->attributes&0x800000))out.flags|=WX_ACTION_DEAD;
    unsigned form=(self->fields[138]>>16)&255;
    if(form>32)out.flags|=WX_ACTION_UNKNOWN;
    else {unsigned mask=form?1u<<(form-1):0;
        if((mask&info->stance_deny)||(info->stance_allow&&!(mask&info->stance_allow)&&!(info->attributes_ex2&0x80000)))out.flags|=WX_ACTION_FORM;
    }
    if(cost(info,self,mod,&out.cost,&out.current)){
        out.flags|=WX_ACTION_COST_KNOWN;
        if(out.cost>out.current||(info->power_type==UINT32_MAX&&out.cost==out.current))out.flags|=WX_ACTION_RESOURCE;
    }else out.flags|=WX_ACTION_UNKNOWN;
    if(info->range_index==1||info->range_index==13)return out;
    if(!unit_target(info))return out;
    if(!target||!target->positioned||!position||!mod||(mod->known&2)==0){out.flags|=WX_ACTION_UNKNOWN;return out;}
    if(target->guid==self->guid)return out;
    float self_reach=field_float(self,130),target_reach=field_float(target,130);
    float dx=position[0]-target->x,dy=position[1]-target->y,dz=position[2]-target->z;
    if(!isfinite(self_reach)||!isfinite(target_reach)||self_reach<0||target_reach<0||self_reach>100||target_reach>100||!isfinite(dx)||!isfinite(dy)||!isfinite(dz)){out.flags|=WX_ACTION_UNKNOWN;return out;}
    double distance=sqrt((double)dx*dx+(double)dy*dy+(double)dz*dz),minimum=info->min_range,maximum;
    if(info->range_index==2){
        if(info->attributes&0x404)return out; // next-swing spells queue before entering melee range
        distance=sqrt((double)dx*dx+(double)dy*dy);minimum=0;
        double modified=(5.0+mod->range[0])*(1.0+mod->range[1]/100.0);
        maximum=fmax(5.0,self_reach+target_reach+1.333333373+1+(modified-5));
    }else{
        distance=fmax(0,distance-self_reach-target_reach);
        maximum=(info->max_range+mod->range[0])*(1.0+mod->range[1]/100.0)+1.25;
    }
    // Movement speed/flags are not fully published yet. Include the maximum
    // Vanilla leeway so this advisory never falsely rejects that allowance.
    maximum+=2.66;out.flags|=WX_ACTION_RANGE_KNOWN;out.distance_milli=clamp(distance*1000);
    if(distance>fmax(0,maximum))out.flags|=WX_ACTION_FAR;
    if(minimum&&distance<minimum)out.flags|=WX_ACTION_NEAR;
    return out;
}
