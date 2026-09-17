#include "wx_icons.h"
#include "wx_cast.h"
#include "wx_actions.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks,objects,available=48u*1024u*1024u;static int fail;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Icon FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return available;}
void* wx_gpu_alloc(unsigned bytes){if(fail)return NULL;unsigned* p=malloc(bytes+4);if(!p)return NULL;*p=bytes;available-=bytes;objects++;return p+1;}
void wx_gpu_free(void* v){if(v){unsigned* p=(unsigned*)v-1;available+=*p;objects--;free(p);}}
static WxIcons icons;static WxUi ui;static WxGame game;
static void fixture(unsigned bad){
    uint32_t h[]={0x43495857,1,70,70,24+70*8,24+70*8+70*4096};
    if(bad==1)h[2]=WX_ICON_INDEX_LIMIT+1;if(bad==2)h[3]=WX_ICON_IMAGE_LIMIT+1;if(bad==3)h[4]++;
    FILE* f=fopen("icons-test.tmp","wb");CHECK(f);fwrite(h,1,sizeof h,f);
    for(unsigned i=0;i<70;i++){uint32_t row[]={i,i};if(bad==4&&i==4)row[0]=3;if(bad==5&&i==4)row[1]=70;fwrite(row,1,sizeof row,f);}
    for(unsigned i=0;i<70;i++)for(unsigned j=0;j<1024;j++){uint32_t color=0xff000000|i;fwrite(&color,4,1,f);}
    if(bad==6)fputc(0,f);CHECK(!fclose(f));
}
int main(int argc,char** argv){
    fixture(0);CHECK(wx_icons_open(&icons,"icons-test.tmp")&&icons.bytes==262144+70*8);unsigned memory=available;
    uint32_t keys[32];for(unsigned i=0;i<32;i++)keys[i]=i+1;
    for(unsigned i=0;i<32;i++){unsigned before=icons.loads;wx_icons_update(&icons,keys,32);CHECK(icons.loads==before+1&&available==memory&&icons.pending==31-i);}
    for(unsigned i=0;i<32;i++)CHECK(wx_icons_slot(&icons,i+1)>=0);
    unsigned loads=icons.loads;for(unsigned i=0;i<100;i++)wx_icons_update(&icons,keys,32);CHECK(icons.loads==loads&&!icons.pending);
    for(unsigned i=0;i<32;i++)keys[i]=i+33;
    for(unsigned i=0;i<32;i++)wx_icons_update(&icons,keys,32);
    for(unsigned i=0;i<32;i++)CHECK(wx_icons_slot(&icons,keys[i])>=0);
    keys[0]=69;for(unsigned i=0;i<3;i++)wx_icons_update(&icons,keys,32);
    for(unsigned i=0;i<32;i++)CHECK(wx_icons_slot(&icons,keys[i])>=0); // pin requested slots during eviction
    keys[0]=60000;wx_icons_update(&icons,keys,1);CHECK(wx_icons_slot(&icons,60000)>=0&&wx_icons_slot(&icons,0)<0);
    CHECK(icons.slots[wx_icons_slot(&icons,60000)].image==0);icons.serial=UINT32_MAX;wx_icons_update(&icons,keys,1);CHECK(icons.serial==1&&!icons.failures);
    CHECK(!wx_icons_image(&icons,NULL,60000,0,0,32,0xffffffff));
    loads=icons.loads;wx_icons_update(&icons,keys,33);wx_icons_update(&icons,NULL,1);CHECK(icons.loads==loads);
    CHECK(wx_action_icon(&game,78)==78&&wx_action_icon(&game,0)==0&&wx_action_icon(&game,70000)==0&&wx_action_icon(&game,0x40000001)==0);
    CHECK(!wx_action_icon(NULL,0x80000001));game.item_names[0]=(WxItemName){6948,6418,0,1,"Hearthstone"};
    CHECK(wx_action_icon(&game,0x80000000|6948)==(WX_ICON_ITEM|6418));
    wx_icons_close(&icons);CHECK(!objects&&available==48u*1024u*1024u);
    for(unsigned bad=1;bad<=6;bad++){fixture(bad);CHECK(!wx_icons_open(&icons,"icons-test.tmp")&&!objects&&!icons.bytes&&!icons.ready);}
    fixture(0);fail=1;CHECK(!wx_icons_open(&icons,"icons-test.tmp")&&!objects);fail=0;
    available=8u*1024u*1024u;CHECK(!wx_icons_open(&icons,"icons-test.tmp")&&!objects);available=48u*1024u*1024u;
    CHECK(!remove("icons-test.tmp"));
    if(argc>=2){
        CHECK(wx_icons_open(&icons,argv[1]));uint32_t real[]={6603,78,2457,6673,100,20572,20594,WX_ICON_ITEM|6418};
        for(unsigned i=0;i<8;i++)wx_icons_update(&icons,real,8);
        for(unsigned i=0;i<8;i++)CHECK(wx_icons_slot(&icons,real[i])>=0);
        printf("Real icons: %u mappings / %u images, allocation %u bytes; %u uploads\n",icons.count,icons.images,icons.bytes,icons.loads);
        if(argc==3){
            CHECK(wx_ui_open(&ui,argv[2]));WxActionPrompt prompts[8]={0};
            for(unsigned i=0;i<8;i++){prompts[i].binding=i==7?0x80000000|6948:real[i];snprintf(prompts[i].name,sizeof prompts[i].name,"Ability %u",i);}
            CHECK(wx_actionbar_ui(&ui,&icons,&game,prompts,1,1u<<WX_A,NULL,NULL));CHECK(ui.batch_count==17&&ui.quads<600&&!ui.failures);
            unsigned at=0;for(unsigned i=0;i<ui.batch_count;i++){CHECK(ui.batches[i].first==at&&ui.batches[i].count);at+=ui.batches[i].count;}
            CHECK(at==ui.quads);for(unsigned i=0;i<ui.quads*4;i++){CHECK(isfinite(ui.vertices[i].position[0])&&isfinite(ui.vertices[i].uv[0]));CHECK(ui.vertices[i].uv[0]>=0&&ui.vertices[i].uv[0]<=1);}
            for(unsigned quarter=0;quarter<=4;quarter++){
                wx_ui_clear(&ui);WxCooldownView cd={quarter*1000,4000,0,0};
                unsigned drawn=wx_action_cooldown_ui(&ui,&cd,40,40);CHECK(drawn<=72);
                float area=0;for(unsigned q=0;q<ui.quads;q++){
                    WxUiVertex* v=ui.vertices+q*4;
                    if(fabsf(v[0].color[3]-184/255.f)<.001f){area+=(v[1].position[0]-v[0].position[0])*(v[2].position[1]-v[1].position[1]);
                        CHECK(v[0].position[0]>=40&&v[0].position[1]>=40&&v[2].position[0]<=72&&v[2].position[1]<=72);}
                }CHECK(fabsf(area-quarter*256.f)<.1f);
            }
            WxCooldownView cooldowns[8];for(unsigned i=0;i<8;i++)cooldowns[i]=(WxCooldownView){i?3599000:0,i?3600000:0,i==0,0};
            wx_ui_clear(&ui);CHECK(wx_actionbar_ui(&ui,&icons,&game,prompts,3,0,cooldowns,NULL));CHECK(ui.quads<900&&ui.batch_count<=17&&!ui.failures);
            wx_ui_clear(&ui);WxCooldownView invalid={4001,4000,0,0};CHECK(!wx_action_cooldown_ui(&ui,&invalid,0,0));
            wx_ui_clear(&ui);WxCooldownView gcd={750,1500,0,1};CHECK(wx_action_cooldown_ui(&ui,&gcd,0,0)==32);CHECK(ui.quads==32);
            WxActionFeedback feedback[8]={{0}};
            const unsigned flags[]={WX_ACTION_RESOURCE,WX_ACTION_FAR,WX_ACTION_NEAR,WX_ACTION_FORM,WX_ACTION_DEAD,WX_ACTION_UNLEARNED,WX_ACTION_COUNT_KNOWN,WX_ACTION_COUNT_KNOWN|WX_ACTION_CHARGES_KNOWN};
            for(unsigned i=0;i<8;i++){feedback[i].flags=flags[i];feedback[i].count=i==7?1:123;feedback[i].charges=4;}
            wx_ui_clear(&ui);CHECK(wx_actionbar_ui(&ui,&icons,&game,prompts,2,0,cooldowns,feedback));
            for(unsigned phase=1;phase<=WX_CAST_FAILED;phase++){WxCastView cast={133,phase,500,3000,2500,0,0,1};
                unsigned before=ui.quads;wx_cast_draw(&ui,&cast,NULL);CHECK(ui.quads>before&&ui.quads<=2048&&!ui.failures);}
            for(unsigned i=0;i<ui.quads*4;i++)CHECK(isfinite(ui.vertices[i].position[0])&&isfinite(ui.vertices[i].position[1]));
            wx_ui_clear(&ui);WxCastView invalid_cast={133,WX_CAST_PREPARING,0,0,0,0,0,1};wx_cast_draw(&ui,&invalid_cast,NULL);CHECK(!ui.quads);
            invalid_cast=(WxCastView){689,WX_CAST_CHANNEL,999,UINT32_MAX,UINT32_MAX,0,1,1};wx_cast_draw(&ui,&invalid_cast,NULL);CHECK(ui.quads>4&&!ui.failures);
            wx_ui_close(&ui);
        }
        wx_icons_close(&icons);CHECK(!objects);
    }
    printf("Bounded icon streaming, eviction, fallback, allocation failures and ordered UI: %u checks pass\n",checks);return 0;
}
