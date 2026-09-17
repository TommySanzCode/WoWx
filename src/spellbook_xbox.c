#include "wx_spellbook.h"
#include <pbkit/pbkit.h>
#include "wx_font.h"
static const char* keys[8]={"A","B","X","Y","UP","DOWN","LEFT","RIGHT"};
static const char* layers[3]={"LT","RT","LT RT"};
void wx_actionbar_draw(const WxActionPrompt prompts[8],unsigned layer,unsigned buttons){
    if(!layer||layer>3)return;
    const unsigned button_ids[]={WX_A,WX_B,WX_X,WX_Y,WX_UP,WX_DOWN,WX_LEFT,WX_RIGHT};
    pb_fill(24,292,592,134,0xff17212a);
    wx_font_printat(11,3,"%s ACTIONS",layers[layer-1]);wx_font_printat(11,25,"Start then White: assign");
    for(unsigned i=0;i<8;i++){
        unsigned row=i%4,col=i/4;
        if(buttons&(1u<<button_ids[i]))pb_fill(44+col*280,321+row*25,272,23,0xff314f68);
        wx_font_printat(12+row,3+col*28,"%-5s %.19s",keys[i],prompts[i].name);
    }
}
static const char* spell_name(const WxSpellBook* b,unsigned id){const WxKnownSpell* s=wx_spellbook_find(b,id);return s&&s->state==1?s->info.name:"Unlisted spell";}
void wx_spell_ui_draw(const WxSpellUi* u,const WxSpellBook* b,const WxGame* g,const uint8_t bindings[24],unsigned form){
    if(!u->open)return;pb_fill(24,30,592,420,0xff17212a);wx_font_clear();
    unsigned count=b->loaded==b->count?b->row_count:0;
    wx_font_printat(1,3,"SPELLBOOK  %u abilities",count);
    if(!b->file)wx_font_printat(3,3,"Spell details are not installed");
    else if(!b->count)wx_font_printat(3,3,"Waiting for learned spells");
    else if(b->loaded<b->count)wx_font_printat(3,3,"Loading spell details: %u/%u",b->loaded,b->count);
    if(!u->screen){
        unsigned first=(u->selected/7)*7;
        for(unsigned i=first;i<count&&i<first+7;i++){
            const WxKnownSpell* s=wx_spellbook_selected(b,i);
            wx_font_printat(3+i-first,3,"%s %.39s%s",i==u->selected?">":" ",s->state==1?s->info.name:s->state==2?"Details unavailable":"Loading...",s->info.attributes&WX_SPELL_PASSIVE?" [P]":"");
        }
        if(u->selected<count){const WxKnownSpell* s=wx_spellbook_selected(b,u->selected);wx_font_printat(11,3,"%.23s  %s",s->info.rank,s->info.attributes&WX_SPELL_PASSIVE?"Passive":"Active");}
        wx_font_printat(12,3,"D-pad: browse  Left/right: page");
        wx_font_printat(14,3,"A: assign to controller  B: close");
    }else if(u->selected<count){
        const WxKnownSpell* s=wx_spellbook_selected(b,u->selected);unsigned slot=wx_action_slot(bindings[u->slot],form);
        uint32_t binding=slot<120?g->actions[slot]:0,id=binding&0xffffff,type=binding>>24;
        if(u->screen==2&&u->clear)wx_font_printat(3,3,"Clear selected controller action");
        else {wx_font_printat(3,3,"Assign: %.41s",s->info.name);wx_font_printat(4,3,"%.23s",s->info.rank);}
        wx_font_printat(6,3,"Hold %s   Press %s",layers[u->slot/8],keys[u->slot%8]);
        wx_font_printat(8,3,"Current: %.42s",!binding?"Empty":type==0?spell_name(b,id):type==0x80?wx_item_name(g,id):"Macro or other action");
        if(u->screen==1){wx_font_printat(11,3,"Triggers + button or D-pad: choose control");wx_font_printat(12,3,"A: assign spell  X: clear control");wx_font_printat(14,3,"B: spell list");}
        else {wx_font_printat(11,3,u->clear?"Remove this control's current action?":"Replace this control's current action?");wx_font_printat(12,3,"Saved with this character on the realm");wx_font_printat(14,3,"A: apply assignment  B: cancel");}
    }
    wx_font_printat(15,3,"%.51s",u->message);
}
