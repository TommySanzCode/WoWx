#ifndef WX_SPELLBOOK_H
#define WX_SPELLBOOK_H
#include "wx_game.h"
#include "wx_input.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WX_SPELL_PASSIVE 0x40u
#define WX_SPELL_HIDDEN 0x80u
#define WX_SPELL_RECIPE 0x20u
typedef struct WxSpellInfo {
    uint32_t attributes;char name[80],rank[24]; // v1's 108-byte prefix
    uint32_t metadata,attributes_ex,school,power_type,cost,cost_percent,cost_per_level,base_level,max_level,spell_level,range_index;
    float min_range,max_range;
    uint32_t target_a[3],target_b[3],stance_allow,stance_deny,attributes_ex2;
} WxSpellInfo;
typedef struct WxKnownSpell {uint16_t id,state;WxSpellInfo info;} WxKnownSpell;
typedef struct WxSpellBook {FILE* file;uint16_t* ids;unsigned catalog_count,count,loaded,failures,row_count,version,record_bytes,extra_cursor;uint16_t rows[512];WxKnownSpell known[512],extra[8];} WxSpellBook;
typedef struct WxSpellUi {unsigned open,screen,selected,slot,confirmed_slot,confirmed_form,confirmed_old,confirmed_spell,clear;char message[96];} WxSpellUi;
typedef struct WxActionPrompt {uint32_t binding;unsigned server_slot;char name[80];} WxActionPrompt;
int wx_action_prompt(const WxSpellBook* book,const WxGame* game,const uint8_t bindings[24],unsigned form,unsigned control,WxActionPrompt* out);
void wx_actionbar_draw(const WxActionPrompt prompts[8],unsigned layer,unsigned buttons);
int wx_spellbook_open(WxSpellBook* book,const char* path);
void wx_spellbook_close(WxSpellBook* book);
void wx_spellbook_sync(WxSpellBook* book,const WxGame* game);
void wx_spellbook_request(WxSpellBook* book,unsigned spell);
const WxKnownSpell* wx_spellbook_find(const WxSpellBook* book,unsigned id);
const WxKnownSpell* wx_spellbook_selected(const WxSpellBook* book,unsigned selected);
int wx_spell_ui_input(WxSpellUi* ui,const WxSpellBook* book,const WxGame* game,const uint8_t bindings[24],unsigned form,const WxPad* pad,WxCommand* command);
void wx_spell_ui_draw(const WxSpellUi* ui,const WxSpellBook* book,const WxGame* game,const uint8_t bindings[24],unsigned form);
#ifdef __cplusplus
}
#endif
#endif
