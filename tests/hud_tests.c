#include "wx_hud.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
static unsigned checks,objects,bytes;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"HUD FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return 48u*1024u*1024u-bytes;}
void* wx_gpu_alloc(unsigned size){unsigned* p=malloc(size+4);if(!p)return NULL;*p=size;bytes+=size;objects++;return p+1;}
void wx_gpu_free(void* pointer){if(pointer){unsigned* p=(unsigned*)pointer-1;bytes-=*p;objects--;free(p);}}
static WxEntities live,scratch;static WxUi ui;
static unsigned write32(uint8_t* p,uint32_t value){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(8*i));return 4;}
// A real Vanilla UPDATE_VALUES block, applied through the production parser.
static void update(unsigned field,uint32_t value){
    uint8_t packet[40]={1,0,0,0,0,0,1,1,2};unsigned at=9;
    at+=write32(packet+at,field<32?1u<<field:0);at+=write32(packet+at,field>=32?1u<<(field-32):0);
    at+=write32(packet+at,value);CHECK(wx_entities_apply(&live,&scratch,packet,at,10));
}
int main(int argc,char** argv){
    WxEntity* e=live.items;live.count=1;e->guid=1;e->type=4;
    e->fields[22]=79;e->fields[28]=79;e->fields[34]=2;
    WxHud h={0};
    for(unsigned type=0;type<5;type++){
        update(36,(type<<24)|(1<<8));update(23+type,315+type);update(29+type,1000+type);
        wx_hud_unit(&h.player,e,"Xboxer");
        CHECK(h.player.power_known&&h.player.power_type==type&&h.player.power==315+type&&h.player.max_power==1000+type);
        CHECK(wx_hud_power_display(type,h.player.power)==(type==1?31:315+type));
        CHECK(wx_hud_power_color(type)!=0xff808080&&*wx_hud_power_name(type));
    }
    update(36,255u<<24);wx_hud_unit(&h.player,e,"Xboxer");CHECK(!h.player.power_known&&!h.player.power&&!h.player.max_power);
    update(36,3u<<24);update(32,0);wx_hud_unit(&h.player,e,"Xboxer");CHECK(!h.player.power_known);
    CHECK(wx_hud_fraction(3,0)==0&&wx_hud_fraction(9,8)==1&&wx_hud_fraction(UINT32_MAX,UINT32_MAX)==1);
    CHECK(fabsf(wx_hud_fraction(50,100)-.5f)<.00001f);
    update(22,0);wx_hud_unit(&h.player,e,"Xboxer");CHECK(h.player.dead&&!h.player.ghost);
    e->fields[190]=16;wx_hud_unit(&h.player,e,"Xboxer");CHECK(h.player.ghost);e->fields[190]=0;
    wx_hud_unit(&h.player,NULL,"Old name");CHECK(!h.player.present&&!h.player.guid&&!h.player.name[0]);
    WxEntity object={0};object.guid=2;object.type=5;wx_hud_unit(&h.target,&object,"Object");CHECK(!h.target.present);
    object.type=3;object.fields[22]=32;object.fields[28]=40;object.fields[34]=8;object.fields[23]=70;object.fields[29]=110;
    wx_hud_unit(&h.target,&object,NULL);CHECK(!strcmp(h.target.name,"Unknown")&&h.target.power_known);
    char long_name[200];memset(long_name,'Z',199);long_name[199]=0;wx_hud_unit(&h.target,&object,long_name);CHECK(strlen(h.target.name)==95);
    unsigned metrics[17];wx_hud_metrics(&h,metrics);CHECK(metrics[7]==2&&metrics[9]==32&&metrics[13]==110&&metrics[16]==1);
    update(22,79);update(36,1u<<24);update(24,355);update(30,1000);wx_hud_unit(&h.player,e,"Xboxer");
    wx_hud_unit(&h.target,&object,"Kobold with a deliberately long name");h.xp=768;h.next_xp=900;
    if(argc>1){
        CHECK(wx_ui_open(&ui,argv[1])&&ui.sprite_count==WX_UI_SPRITES);unsigned allocated=bytes;
        CHECK(wx_hud_draw(&h,&ui)&&h.drawn&&h.quads==ui.quads&&h.quads<300&&!ui.failures&&bytes==allocated);
        for(unsigned i=0;i<ui.quads*4;i++){
            CHECK(isfinite(ui.vertices[i].position[0])&&isfinite(ui.vertices[i].position[1]));
            CHECK(ui.vertices[i].uv[0]>=0&&ui.vertices[i].uv[0]<=1&&ui.vertices[i].uv[1]>=0&&ui.vertices[i].uv[1]<=1);
        }
        if(argc==3){FILE* f=fopen(argv[2],"wb");CHECK(f);uint32_t header[]={0x31565157,ui.quads};
            CHECK(fwrite(header,1,8,f)==8&&fwrite(ui.vertices,sizeof(WxUiVertex)*4,ui.quads,f)==ui.quads&&!fclose(f));}
        unsigned quads=ui.quads;wx_ui_clear(&ui);wx_hud_unit(&h.target,NULL,NULL);CHECK(wx_hud_draw(&h,&ui)&&h.quads<quads);
        wx_ui_clear(&ui);h.player.dead=1;CHECK(wx_hud_draw(&h,&ui)&&!ui.failures);
        wx_ui_clear(&ui);h.player.ghost=1;CHECK(wx_hud_draw(&h,&ui)&&!ui.failures);
        wx_ui_clear(&ui);h.player.dead=h.player.ghost=0;h.player.health=h.player.max_health=h.player.power=h.player.max_power=UINT32_MAX;
        h.xp=h.next_xp=UINT32_MAX;
        CHECK(wx_hud_draw(&h,&ui)&&!ui.failures&&bytes==allocated);
        wx_ui_clear(&ui);ui.sprite_count=WX_UI_LEGACY_SPRITES;CHECK(!wx_hud_draw(&h,&ui)&&!ui.quads&&!h.drawn);
        wx_ui_close(&ui);CHECK(!bytes&&!objects);
        printf("HUD real atlas: %u submitted quads, fixed GPU allocation %u bytes. Host renderer command generation only.\n",quads,allocated);
    }
    printf("HUD resource updates, death, target clearing and draw boundaries: %u checks pass\n",checks);return 0;
}
