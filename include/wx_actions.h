#ifndef WX_ACTIONS_H
#define WX_ACTIONS_H
#include "wx_spellbook.h"
#include "wx_inventory.h"
#include "wx_cooldown.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {
    WX_ACTION_UNLEARNED=1,WX_ACTION_PASSIVE=2,WX_ACTION_DEAD=4,WX_ACTION_FORM=8,
    WX_ACTION_RESOURCE=16,WX_ACTION_FAR=32,WX_ACTION_NEAR=64,WX_ACTION_NO_ITEM=128,
    WX_ACTION_UNSUPPORTED=256,WX_ACTION_UNKNOWN=512,WX_ACTION_COST_KNOWN=1024,
    WX_ACTION_RANGE_KNOWN=2048,WX_ACTION_COUNT_KNOWN=4096,WX_ACTION_CHARGES_KNOWN=8192
};
typedef struct WxActionModifiers {uint32_t known,charge_slot;int32_t cost[2],range[2];} WxActionModifiers;
typedef struct WxActionFeedback {uint32_t flags,cost,current,distance_milli,count,charges;} WxActionFeedback;
void wx_action_modifiers(const WxCooldowns* state,const WxCooldownCatalog* catalog,uint32_t binding,WxActionModifiers* output);
void wx_world_action_modifiers(const uint32_t bindings[8],WxActionModifiers output[8]);
WxActionFeedback wx_action_feedback(uint32_t binding,const WxSpellBook* book,const WxGame* game,
    const WxInventory* inventory,const WxEntity* self,const WxEntity* target,const float position[3],const WxActionModifiers* modifiers);
#ifdef __cplusplus
}
#endif
#endif
