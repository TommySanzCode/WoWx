#include "wx_actions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks;static WxSpellBook book;static WxGame game;static WxEntity self,target;static WxInventory inventory;
static WxCooldowns cooldowns;static WxCooldownInfo info;static WxCooldownCatalog catalog;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Action FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return 48u*1024u*1024u;}
static void f(WxEntity* e,unsigned index,float value){memcpy(&e->fields[index],&value,4);}
static WxActionFeedback query(uint32_t binding){WxActionModifiers modifiers;wx_action_modifiers(&cooldowns,&catalog,binding,&modifiers);float position[3]={0,0,0};return wx_action_feedback(binding,&book,&game,&inventory,&self,&target,position,&modifiers);}
int main(int argc,char** argv){
    game.spell_count=1;game.spells[0]=133;book.count=1;book.known[0].id=133;book.known[0].state=1;
    WxSpellInfo* s=&book.known[0].info;*s=(WxSpellInfo){.metadata=1,.school=2,.cost=30,.range_index=4,.max_range=30,.target_a={6}};
    self.guid=1;self.type=4;self.fields[22]=100;self.fields[23]=100;self.fields[34]=10;self.fields[162]=100;self.fields[163]=100;f(&self,130,1.5f);
    target.guid=2;target.type=3;target.positioned=1;target.x=20;f(&target,130,1.5f);
    info=(WxCooldownInfo){.spell=133,.family=3,.family_mask={1,0}};catalog=(WxCooldownCatalog){&info,1,sizeof info,1,0,2};wx_cooldown_reset(&cooldowns);wx_cooldown_unit(&cooldowns,8,1,0);
    WxActionFeedback v=query(133);CHECK(v.cost==30&&v.current==100&&(v.flags&WX_ACTION_COST_KNOWN)&&(v.flags&WX_ACTION_RANGE_KNOWN)&&!(v.flags&(WX_ACTION_RESOURCE|WX_ACTION_FAR|WX_ACTION_UNKNOWN)));
    self.fields[23]=29;CHECK(query(133).flags&WX_ACTION_RESOURCE);self.fields[23]=30;CHECK(!(query(133).flags&WX_ACTION_RESOURCE));
    cooldowns.modifiers[1][14][0]=-50;CHECK(query(133).cost==15);cooldowns.modifiers[0][14][0]=-10;CHECK(query(133).cost==10);
    self.fields[175]=10;f(&self,182,.5f);CHECK(query(133).cost==22);self.fields[175]=0;f(&self,182,0);wx_cooldown_reset(&cooldowns);wx_cooldown_unit(&cooldowns,8,1,0);
    s->cost_percent=10;CHECK(query(133).cost==40);s->cost_percent=0;s->cost_per_level=1;CHECK((query(133).flags&WX_ACTION_UNKNOWN)&&!(query(133).flags&WX_ACTION_COST_KNOWN));s->cost_per_level=0;
    s->attributes_ex=2;CHECK(query(133).cost==30&&!(query(133).flags&WX_ACTION_RESOURCE));s->attributes_ex=0;
    s->power_type=1;s->cost=150;self.fields[24]=149;CHECK(query(133).flags&WX_ACTION_RESOURCE);self.fields[24]=150;CHECK(!(query(133).flags&WX_ACTION_RESOURCE));
    s->power_type=UINT32_MAX;s->cost=100;CHECK(query(133).flags&WX_ACTION_RESOURCE);s->cost=99;CHECK(!(query(133).flags&WX_ACTION_RESOURCE));s->power_type=0;s->cost=30;
    target.x=37;CHECK(query(133).flags&WX_ACTION_FAR);cooldowns.modifiers[1][5][0]=100;CHECK(!(query(133).flags&WX_ACTION_FAR));cooldowns.modifiers[1][5][0]=0;
    target.x=4;s->min_range=5;CHECK(query(133).flags&WX_ACTION_NEAR);s->min_range=0;
    target.positioned=0;CHECK((query(133).flags&WX_ACTION_UNKNOWN)&&!(query(133).flags&WX_ACTION_RANGE_KNOWN));target.positioned=1;
    s->range_index=1;CHECK(!(query(133).flags&WX_ACTION_RANGE_KNOWN));s->range_index=2;target.z=100;target.x=3;CHECK(!(query(133).flags&WX_ACTION_FAR));target.x=30;CHECK(query(133).flags&WX_ACTION_FAR);s->attributes=0x400;CHECK(!(query(133).flags&WX_ACTION_RANGE_KNOWN));s->attributes=0;target.z=0;
    s->stance_allow=1;CHECK(query(133).flags&WX_ACTION_FORM);self.fields[138]=1<<16;CHECK(!(query(133).flags&WX_ACTION_FORM));s->stance_deny=1;CHECK(query(133).flags&WX_ACTION_FORM);s->stance_allow=s->stance_deny=0;
    self.fields[22]=0;CHECK(query(133).flags&WX_ACTION_DEAD);s->attributes=0x800000;CHECK(!(query(133).flags&WX_ACTION_DEAD));s->attributes=WX_SPELL_PASSIVE;CHECK(query(133).flags&WX_ACTION_PASSIVE);self.fields[22]=100;s->attributes=0;
    CHECK(query(134).flags==WX_ACTION_UNLEARNED);CHECK(query(0x01000001).flags==WX_ACTION_UNSUPPORTED);CHECK(!query(0).flags);
    info.family_mask[0]=3;cooldowns.modifiers[0][14][0]=10;cooldowns.modifiers[0][14][1]=10;CHECK((query(133).flags&WX_ACTION_UNKNOWN)&&!(query(133).flags&WX_ACTION_COST_KNOWN));
    CHECK(query(0x80001b24).flags&WX_ACTION_NO_ITEM);inventory.count=2;inventory.items[0]=(WxInventoryItem){.guid=8,.entry=6948,.count=3};inventory.items[1]=(WxInventoryItem){.guid=9,.entry=6948,.count=2};CHECK(query(0x80001b24).count==5);
    inventory.count=1;inventory.items[0].count=1;inventory.items[0].charges[2]=-4;cooldowns.items[0].item=6948;cooldowns.items[0].spells[2]=(WxItemSpellCooldown){.spell=8690,.trigger=0,.charges=-5};v=query(0x80001b24);CHECK((v.flags&WX_ACTION_CHARGES_KNOWN)&&v.charges==4&&v.count==1);
    inventory.items[0].charges[2]=0;CHECK(query(0x80001b24).charges==0);inventory.items[0].charges[2]=INT32_MIN;CHECK(query(0x80001b24).charges==0x80000000u);
    static WxEntities entities;entities.items[0].guid=1;entities.items[0].type=4;entities.items[0].inventory[23*2]=2;entities.items[1].guid=2;entities.items[1].type=1;entities.items[1].fields[3]=6948;entities.items[1].fields[14]=1;entities.items[1].fields[18]=(uint32_t)-3;
    wx_inventory_snapshot(&entities,1,&inventory);CHECK(inventory.count==1&&inventory.items[0].charges[2]==-3);
    if(argc==2){memset(&book,0,sizeof book);CHECK(wx_spellbook_open(&book,argv[1])&&book.version==2);game.spells[0]=133;wx_spellbook_sync(&book,&game);s=&book.known[0].info;CHECK(s->metadata&&s->cost==30&&s->power_type==0&&s->school==2&&s->range_index==35&&s->max_range==35&&s->target_a[0]==6);
        wx_spellbook_request(&book,8690);wx_spellbook_sync(&book,&game);CHECK(wx_spellbook_find(&book,8690)->state==1&&!strcmp(wx_spellbook_find(&book,8690)->info.name,"Hearthstone"));wx_spellbook_close(&book);
    }
    printf("Action resources, signed modifiers, range hints, forms, item counts/charges and metadata: %u checks pass\n",checks);return 0;
}
