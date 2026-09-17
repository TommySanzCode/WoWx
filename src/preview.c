#include "wx_preview.h"
#include <math.h>
#include <string.h>
static int roster_screen(unsigned screen){return screen==WX_CHARACTER_LIST||screen==WX_CHARACTER_DELETE||screen==WX_CHARACTER_DELETE_KEYBOARD;}
int wx_preview_subject(const WxCharacterUi* ui,const WxCharacterLobby* lobby,WxWorldView* out){
    memset(out,0,sizeof *out);
    if(!ui->open||lobby->phase!=WX_LOBBY_READY)return 0;
    if(roster_screen(ui->screen)){
        const WxCharacter* c=ui->screen==WX_CHARACTER_LIST?
            (ui->selected<lobby->characters.count?&lobby->characters.items[ui->selected]:NULL):wx_character_find(&lobby->characters,ui->delete_guid);
        if(!c)return 0;
        out->guid=c->guid;memcpy(out->name,c->name,sizeof out->name);
        out->race=c->race;out->character_class=c->character_class;out->gender=c->gender;
        out->skin=c->skin;out->face=c->face;out->hair_style=c->hair_style;out->hair_color=c->hair_color;out->facial_hair=c->facial_hair;
        memcpy(out->equipment_display,c->display,sizeof out->equipment_display);memcpy(out->equipment_type,c->inventory_type,sizeof out->equipment_type);
        out->appearance_flags=c->flags;
    }else{
        const WxCharacterDraft* c=&ui->draft;memcpy(out->name,c->name,sizeof c->name);
        out->race=c->race;out->character_class=c->character_class;out->gender=c->gender;
        out->skin=c->skin;out->face=c->face;out->hair_style=c->hair_style;out->hair_color=c->hair_color;out->facial_hair=c->facial_hair;
    }
    if(!out->race||out->race>8||out->gender>1)return 0;
    out->active=1;out->revision=lobby->revision;return 1;
}
void wx_preview_input(WxPreview* p,const WxPad* pad,float dt){
    if(!p->active||!isfinite(dt)||dt<0)return;if(dt>.05f)dt=.05f;
    if(isfinite(pad->look_x))p->rotation=remainderf(p->rotation+fmaxf(-1,fminf(1,pad->look_x))*dt*2,6.283185307f);
    if(isfinite(pad->look_y))p->zoom=fmaxf(.7f,fminf(1.8f,p->zoom+fmaxf(-1,fminf(1,pad->look_y))*dt));
}
void wx_preview_close(WxPreview* p){
    wx_avatar_close(&p->avatar);wx_backdrop_close(&p->scene);memset(&p->selection,0,sizeof p->selection);
    p->active=p->race=p->background_missing=p->outfit_ready=0;memset(&p->subject,0,sizeof p->subject);
}
void wx_preview_update(WxPreview* p,const WxCharacterUi* ui,const WxCharacterLobby* lobby,const char* directory,unsigned time){
    WxWorldView subject;
    if(!wx_preview_subject(ui,lobby,&subject)){if(p->active)wx_preview_close(p);return;}
    if(!p->outfit_attempted){char path[256];size_t n=strlen(directory);
        int length=snprintf(path,sizeof path,"%s%sOUTFITS.WXO",directory,n&&directory[n-1]!='/'&&directory[n-1]!='\\'?"/":"");
        p->outfit_attempted=1;if(length>0&&(unsigned)length<sizeof path)wx_outfits_open(&p->outfits,path);
    }
    p->outfit_ready=roster_screen(ui->screen);
    if(!p->outfit_ready){const WxOutfit* outfit=wx_outfit_find(&p->outfits,subject.race,subject.character_class,subject.gender);
        if(outfit){memcpy(subject.equipment_display,outfit->display,sizeof outfit->display);memcpy(subject.equipment_type,outfit->type,sizeof outfit->type);p->outfit_ready=1;}}
    int changed=!p->active||p->subject.guid!=subject.guid||p->subject.race!=subject.race||p->subject.gender!=subject.gender;
    if(changed){p->rotation=0;p->zoom=1;p->changes++;}
    if(!p->active||p->race!=subject.race){
        static const char* names[]={"","HUMAN","ORC","DWARF","NIGHTELF","SCOURGE","TAUREN","DWARF","ORC"};
        wx_backdrop_close(&p->scene);p->race=subject.race;char path[256];size_t n=strlen(directory);
        int length=snprintf(path,sizeof path,"%s%s%s.WXB",directory,n&&directory[n-1]!='/'&&directory[n-1]!='\\'?"/":"",names[p->race]);
        p->background_missing=1;
        if(length>0&&(unsigned)length<sizeof path){FILE* f=fopen(path,"rb");if(f){fclose(f);p->background_missing=!wx_backdrop_begin(&p->scene,path);}}
    }
    p->active=1;p->subject=subject;
    int own_scope=!wx_stream_frame_active();if(own_scope)wx_stream_frame_begin(8);
    wx_avatar_select(&p->avatar,&p->selection,&p->subject,directory);wx_avatar_update(&p->avatar,&p->subject,0,time);
    if(own_scope)wx_stream_frame_end();
    if(p->scene.load_phase){wx_backdrop_pump(&p->scene,time);if(!p->scene.ready&&!p->scene.load_phase)p->background_missing=1;}
    wx_backdrop_update(&p->scene,time);
    if(!wx_backdrop_anchor(&p->scene,p->position))memset(p->position,0,sizeof p->position);
    WxBackdropView* v=&p->scene.view;
    if(p->scene.ready)wx_backdrop_view(&p->scene,0,0,640,480);
    else {*v=(WxBackdropView){{5,0,1.4f},{0,1,0},{0,0,1},{-1,0,0},{320,240},420,.1f,160};}
    // Shift the full scene behind the visible character area; GPU clips to 640x480.
    v->center[0]=roster_screen(ui->screen)?215:416;
    v->focal*=p->zoom;
    p->angle=atan2f(v->camera[1]-p->position[1],v->camera[0]-p->position[0])+p->rotation;
}
