#include "wx_spellbook.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
int wx_spellbook_open(WxSpellBook* b,const char* path){
    memset(b,0,sizeof *b);b->file=fopen(path,"rb");if(!b->file)return 0;
    uint32_t h[4];if(fread(h,1,16,b->file)!=16||h[0]!=0x31535857||(h[1]!=1&&h[1]!=2)||!h[2]||h[2]>65535||h[3]!=(h[1]==1?108:sizeof(WxSpellInfo)))goto bad;
    if(fseek(b->file,0,SEEK_END)||ftell(b->file)!=(long)(16+h[2]*(2+h[3])))goto bad;
    b->version=h[1];b->record_bytes=h[3];
    if(wx_free_memory()<8u*1024u*1024u+h[2]*2+65536)goto bad;
    b->catalog_count=h[2];b->ids=malloc(h[2]*2);if(!b->ids)goto bad;
    if(fseek(b->file,16,SEEK_SET)||fread(b->ids,2,h[2],b->file)!=h[2])goto bad;
    for(unsigned i=0;i<h[2];i++)if(!b->ids[i]||(i&&b->ids[i]<=b->ids[i-1]))goto bad;
    return 1;
bad:wx_spellbook_close(b);b->failures=1;return 0;
}
void wx_spellbook_close(WxSpellBook* b){if(b->file)fclose(b->file);free(b->ids);memset(b,0,sizeof *b);}
static int lookup(WxSpellBook* b,unsigned id,WxSpellInfo* out){
    if(!b->file)return 0;unsigned lo=0,hi=b->catalog_count;
    while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(b->ids[mid]<id)lo=mid+1;else hi=mid;}
    if(lo==b->catalog_count||b->ids[lo]!=id)return 0;
    memset(out,0,sizeof *out);
    if(fseek(b->file,16+b->catalog_count*2+lo*b->record_bytes,SEEK_SET)||fread(out,1,b->record_bytes,b->file)!=b->record_bytes)return 0;
    if(!out->name[0]||!memchr(out->name,0,sizeof out->name)||!memchr(out->rank,0,sizeof out->rank))return 0;
    if(b->version==1)return 1;
    return out->metadata==1&&out->school<=6&&isfinite(out->min_range)&&isfinite(out->max_range)&&out->min_range>=0&&out->max_range>=out->min_range&&out->max_range<=1000000;
}
static int before(const WxKnownSpell* a,const WxKnownSpell* b){
    unsigned ap=a->state!=1?2:(a->info.attributes&WX_SPELL_PASSIVE)?1:0,bp=b->state!=1?2:(b->info.attributes&WX_SPELL_PASSIVE)?1:0;
    if(ap!=bp)return ap<bp;int order=strcmp(a->info.name,b->info.name);return order?order<0:a->id<b->id;
}
void wx_spellbook_sync(WxSpellBook* b,const WxGame* game){
    if(!b||!game||game->spell_count>512)return;
    unsigned different=b->count!=game->spell_count;
    for(unsigned i=0;!different&&i<b->count;i++)different=b->known[i].id!=game->spells[i];
    if(different){memset(b->known,0,sizeof b->known);b->count=game->spell_count;b->loaded=b->row_count=0;
        for(unsigned i=0;i<b->count;i++)b->known[i].id=game->spells[i];}
    // One bounded record read per frame; the small ID index is already resident.
    if(b->loaded<b->count){WxKnownSpell* spell=&b->known[b->loaded++];
        spell->state=lookup(b,spell->id,&spell->info)?1:2;
        if(spell->state!=1){memset(&spell->info,0,sizeof spell->info);b->failures++;}
        // Vanilla trade spells belong to their profession sublist, not this
        // top-level spellbook. This also excludes internal trade-marked helpers.
        if(!(spell->info.attributes&(WX_SPELL_HIDDEN|WX_SPELL_RECIPE))){
            unsigned at=b->row_count;while(at&&before(spell,&b->known[b->rows[at-1]])){b->rows[at]=b->rows[at-1];at--;}
            b->rows[at]=(uint16_t)(b->loaded-1);b->row_count++;
        }}else for(unsigned i=0;i<8;i++)if(b->extra[i].id&&!b->extra[i].state){
            WxKnownSpell* spell=&b->extra[i];spell->state=lookup(b,spell->id,&spell->info)?1:2;
            if(spell->state!=1){memset(&spell->info,0,sizeof spell->info);b->failures++;}break;
        }
}
const WxKnownSpell* wx_spellbook_find(const WxSpellBook* b,unsigned id){
    if(b&&id){for(unsigned i=0;i<b->count;i++)if(b->known[i].id==id)return &b->known[i];for(unsigned i=0;i<8;i++)if(b->extra[i].id==id)return &b->extra[i];}return NULL;
}
void wx_spellbook_request(WxSpellBook* b,unsigned id){
    if(!b||!id||id>65535||wx_spellbook_find(b,id))return;
    WxKnownSpell* s=&b->extra[b->extra_cursor++%8];memset(s,0,sizeof *s);s->id=(uint16_t)id;
}
const WxKnownSpell* wx_spellbook_selected(const WxSpellBook* b,unsigned selected){return b&&b->loaded==b->count&&selected<b->row_count?&b->known[b->rows[selected]]:NULL;}
int wx_action_prompt(const WxSpellBook* b,const WxGame* g,const uint8_t bindings[24],unsigned form,unsigned control,WxActionPrompt* out){
    if(!out)return 0;memset(out,0,sizeof *out);out->server_slot=120;
    const char* name="Unavailable";int ready=0;
    if(g&&bindings&&control<24&&(out->server_slot=wx_action_slot(bindings[control],form))<120){
        if(!g->actions_ready)name="Loading actions";
        else {ready=1;out->binding=g->actions[out->server_slot];unsigned id=out->binding&0xffffff,type=out->binding>>24;
            if(!id)name="Empty";
            else if(type==0){const WxKnownSpell* spell=wx_spellbook_find(b,id);
                name=!wx_has_spell(g,id)?"Unlearned spell":spell&&spell->state==1?spell->info.name:spell&&spell->state==2?"Spell name missing":"Loading spell name";
            }else if(type==0x80)name=wx_item_name(g,id);
            else name="Unsupported action";
        }
    }
    // Keep the fixed HUD cells intact even if external metadata has control bytes.
    unsigned i=0;for(;i+1<sizeof out->name&&name[i];i++)out->name[i]=(unsigned char)name[i]<32||(unsigned char)name[i]>126?' ':name[i];
    out->name[i]=0;return ready;
}
int wx_spell_ui_input(WxSpellUi* u,const WxSpellBook* b,const WxGame* g,const uint8_t bindings[24],unsigned form,const WxPad* p,WxCommand* out){
    if(!u||!b||!g||!bindings||!p||!out||!u->open)return 0;
    unsigned count=b->loaded==b->count?b->row_count:0;
    if(u->selected>=count){u->selected=0;u->screen=0;}
    if(u->slot>=24)u->slot=0;
    if(!p->layer&&(p->pressed&(1u<<WX_B))){if(u->screen)u->screen--;else u->open=0;return 0;}
    const WxKnownSpell* spell=wx_spellbook_selected(b,u->selected);
    if(u->screen==0){
        if(p->layer)return 0;
        if((p->pressed&(1u<<WX_UP))&&u->selected)u->selected--;
        if((p->pressed&(1u<<WX_DOWN))&&u->selected+1<count)u->selected++;
        if(p->pressed&(1u<<WX_LEFT))u->selected=u->selected>=7?u->selected-7:0;
        if((p->pressed&(1u<<WX_RIGHT))&&count)u->selected=u->selected+7<count?u->selected+7:count-1;
        spell=wx_spellbook_selected(b,u->selected);
        if(p->pressed&(1u<<WX_A)){
            if(!spell||spell->state!=1)snprintf(u->message,sizeof u->message,"Spell details are unavailable");
            else if(spell->info.attributes&WX_SPELL_PASSIVE)snprintf(u->message,sizeof u->message,"Passive abilities are always active");
            else {u->screen=1;u->message[0]=0;}
        }return 0;
    }
    if(!spell||spell->state!=1||(spell->info.attributes&WX_SPELL_PASSIVE)||!wx_has_spell(g,spell->id)){u->screen=0;return 0;}
    if(u->screen==1){
        if(p->slot>=0&&p->slot<24)u->slot=(unsigned)p->slot;
        if(p->layer)return 0;
        if((p->pressed&(1u<<WX_LEFT))&&u->slot)u->slot--;
        if((p->pressed&(1u<<WX_RIGHT))&&u->slot<23)u->slot++;
        if(p->pressed&((1u<<WX_A)|(1u<<WX_X))){
            if(!g->actions_ready||bindings[u->slot]>=120){snprintf(u->message,sizeof u->message,"Waiting for action bindings");return 0;}
            u->clear=(p->pressed&(1u<<WX_X))!=0;
            u->confirmed_slot=wx_action_slot(bindings[u->slot],form);u->confirmed_form=form;
            u->confirmed_old=g->actions[u->confirmed_slot];u->confirmed_spell=u->clear?0:spell->id;u->screen=2;
        }return 0;
    }
    if(!p->layer&&(p->pressed&(1u<<WX_A))){
        unsigned slot=wx_action_slot(bindings[u->slot],form);
        if(!g->actions_ready||(!u->clear&&spell->id!=u->confirmed_spell)||slot!=u->confirmed_slot||form!=u->confirmed_form||slot>=120||g->actions[slot]!=u->confirmed_old){
            u->screen=1;snprintf(u->message,sizeof u->message,"Bindings changed; review again");return 0;}
        *out=(WxCommand){0x128,0,u->confirmed_spell,slot};u->screen=0;
        snprintf(u->message,sizeof u->message,"Binding sent");return 1;
    }return 0;
}
