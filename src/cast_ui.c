#include "wx_cast.h"
void wx_cast_draw(WxUi* ui,const WxCastView* v,const WxSpellBook* book){
    if(!ui||!v||!v->phase||v->phase>WX_CAST_FAILED||(!v->infinite&&!v->duration))return;
    const WxKnownSpell* spell=wx_spellbook_find(book,v->spell);
    const char* name=spell&&spell->state==1?spell->info.name:"Casting";
    uint32_t color=v->phase==WX_CAST_CHANNEL?0xff40bd55:v->phase>=WX_CAST_INTERRUPTED?0xffdc3939:WX_UI_GOLD;
    float fraction=v->infinite?1:v->phase==WX_CAST_CHANNEL?(float)v->remaining/v->duration:v->phase==WX_CAST_PREPARING?(float)v->elapsed/v->duration:1;
    if(fraction<0)fraction=0;if(fraction>1)fraction=1;
    wx_ui_rect(ui,166,222,308,42,0xe0101010);wx_ui_rect(ui,170,244,300,14,0xff65594b);
    wx_ui_rect(ui,172,246,296,10,0xff17130e);wx_ui_rect(ui,172,246,296*fraction,10,color);
    wx_ui_text_fit(ui,0,172,225,v->phase>=WX_CAST_INTERRUPTED?150:220,WX_UI_WHITE,name);
    if(v->phase>=WX_CAST_INTERRUPTED)wx_ui_text(ui,0,330,225,color,v->phase==WX_CAST_FAILED?"Failed":"Interrupted");
    else if(v->phase==WX_CAST_COMPLETE)wx_ui_text(ui,0,406,225,WX_UI_WHITE,"Done");
    else if(v->infinite)wx_ui_text(ui,0,409,225,WX_UI_WHITE,"...");
    else wx_ui_textf(ui,0,410,225,WX_UI_WHITE,"%u.%us",v->remaining/1000,(v->remaining%1000)/100);
}
