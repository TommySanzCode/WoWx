#include "wx_preview.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks,allocated;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"PREVIEW FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return 48u*1024u*1024u-allocated;}
void* wx_gpu_alloc(unsigned bytes){unsigned* p=malloc(bytes+sizeof(unsigned));if(!p)return NULL;*p=bytes;allocated+=bytes;return p+1;}
void wx_gpu_free(void* data){if(data){unsigned* p=(unsigned*)data-1;allocated-=*p;free(p);}}
static WxPreview preview;
int main(int argc,char** argv){
    WxCharacterUi ui={0};WxCharacterLobby lobby={0};WxWorldView view;
    CHECK(!wx_preview_subject(&ui,&lobby,&view));
    ui.open=1;lobby.phase=WX_LOBBY_READY;lobby.revision=7;CHECK(!wx_preview_subject(&ui,&lobby,&view));
    lobby.characters.count=1;WxCharacter* c=lobby.characters.items;c->guid=123;c->race=1;c->character_class=1;c->gender=1;
    c->skin=2;c->face=3;c->hair_style=4;c->hair_color=5;c->facial_hair=6;c->flags=0x400;c->display[15]=1542;c->inventory_type[15]=13;strcpy(c->name,"Example");
    CHECK(wx_preview_subject(&ui,&lobby,&view));CHECK(view.guid==123&&view.revision==7&&view.active&&view.race==1&&view.gender==1);
    CHECK(view.skin==2&&view.face==3&&view.hair_style==4&&view.hair_color==5&&view.facial_hair==6);
    CHECK(view.equipment_display[15]==1542&&view.equipment_type[15]==13&&view.appearance_flags==0x400&&!strcmp(view.name,"Example"));
    ui.selected=1;CHECK(!wx_preview_subject(&ui,&lobby,&view));ui.selected=0;
    ui.screen=WX_CHARACTER_DELETE;ui.delete_guid=123;ui.selected=9;
    CHECK(wx_preview_subject(&ui,&lobby,&view)&&view.guid==123&&view.equipment_display[15]==1542);
    ui.screen=WX_CHARACTER_DELETE_KEYBOARD;CHECK(wx_preview_subject(&ui,&lobby,&view)&&view.guid==123);
    ui.delete_guid=999;CHECK(!wx_preview_subject(&ui,&lobby,&view));ui.selected=0;
    ui.screen=WX_CHARACTER_CREATE;ui.draft.race=3;ui.draft.gender=0;ui.draft.character_class=2;
    CHECK(wx_preview_subject(&ui,&lobby,&view)&&view.race==3&&!view.guid&&!view.equipment_display[15]);
    ui.draft.race=9;CHECK(!wx_preview_subject(&ui,&lobby,&view));ui.draft.race=3;
    lobby.phase=WX_LOBBY_PENDING;CHECK(!wx_preview_subject(&ui,&lobby,&view));lobby.phase=WX_LOBBY_READY;
    preview.active=1;preview.zoom=1;WxPad pad={0};pad.look_x=pad.look_y=1;
    for(unsigned i=0;i<1000;i++)wx_preview_input(&preview,&pad,.033f);
    CHECK(preview.zoom==1.8f&&fabsf(preview.rotation)<=3.141593f);
    pad.look_y=-1;for(unsigned i=0;i<1000;i++)wx_preview_input(&preview,&pad,.033f);CHECK(preview.zoom==.7f);
    float angle=preview.rotation;pad.look_x=NAN;wx_preview_input(&preview,&pad,NAN);CHECK(preview.rotation==angle);
    wx_preview_close(&preview);CHECK(!preview.active&&!allocated);
    if(argc==2){
        unsigned outfits=0;
        for(unsigned race=1;race<=8;race++)for(unsigned sex=0;sex<2;sex++){
            ui.draft.race=race;ui.draft.gender=sex;ui.draft.character_class=1;
            for(unsigned i=0;i<180;i++)wx_preview_update(&preview,&ui,&lobby,argv[1],i*33);
            CHECK(preview.active&&preview.avatar.ready&&preview.avatar.matched&&!preview.avatar.failures&&!preview.avatar.missing);
            CHECK(preview.avatar.scene.animation_index_reads==1);
            CHECK(preview.subject.race==race&&preview.subject.gender==sex&&preview.avatar.header.look[0]==race&&preview.avatar.header.look[1]==sex);
            {CHECK(preview.scene.ready&&!preview.background_missing);float mark[3];CHECK(wx_backdrop_anchor(&preview.scene,mark));
                printf("Race %u: stand %.3f %.3f %.3f; camera %.3f %.3f %.3f; focal %.3f\n",race,mark[0],mark[1],mark[2],preview.scene.view.camera[0],preview.scene.view.camera[1],preview.scene.view.camera[2],preview.scene.view.focal);}
            CHECK(wx_free_memory()>=8u*1024u*1024u);
            for(unsigned cl=1;cl<=11;cl++)if(wx_character_class_allowed(race,cl)){
                ui.draft.character_class=cl;
                for(unsigned i=0;i<32;i++)wx_preview_update(&preview,&ui,&lobby,argv[1],6000+i*33);
                CHECK(preview.outfit_ready&&preview.avatar.ready&&!preview.avatar.missing&&!preview.avatar.failures&&!preview.avatar.scene.failures);
                CHECK(preview.subject.character_class==cl&&preview.subject.equipment_display[15]);
                const WxOutfit* outfit=wx_outfit_find(&preview.outfits,race,cl,sex);CHECK(outfit);
                CHECK(!memcmp(preview.avatar.equipment,outfit->display,sizeof outfit->display));
                CHECK(preview.avatar.scene.bytes<=preview.avatar.scene.budget_bytes&&wx_free_memory()>=8u*1024u*1024u);outfits++;
            }
        }
        CHECK(outfits==80);printf("All %u legal race/class/sex starter outfits composed from actual client assets\n",outfits);
        // An unavailable appearance must release the previous character, once.
        ui.draft.skin=255;wx_preview_update(&preview,&ui,&lobby,argv[1],1000);CHECK(!preview.avatar.ready&&!preview.avatar.file&&preview.selection.failures==1);
        unsigned attempts=preview.selection.attempts;wx_preview_update(&preview,&ui,&lobby,argv[1],1033);CHECK(preview.selection.attempts==attempts);
        ui.open=0;wx_preview_update(&preview,&ui,&lobby,argv[1],1066);CHECK(!preview.active&&!preview.scene.ready&&!preview.avatar.bytes&&!allocated);
    }
    printf("Preview boundaries, controller and lifecycle: %u checks pass\n",checks);return 0;
}
