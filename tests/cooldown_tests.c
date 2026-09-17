#include "wx_cooldown.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,available=48u*1024u*1024u;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Cooldown FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return available;}
static WxCooldowns state,before;static WxInventory inventory;static uint8_t wire[8192];static unsigned at;
static void w(uint32_t value,unsigned bytes){for(unsigned i=0;i<bytes;i++)wire[at++]=(uint8_t)(value>>(i*8));}
static void guid(uint64_t value){w((uint32_t)value,4);w((uint32_t)(value>>32),4);}
static WxCooldownInfo infos[]={{78,1,5000,2000,0},{100,1,15000,1000,0},{133,2,0,0,0},{200,1,6000,3000,WX_COOLDOWN_ON_EVENT},{201,0,0,0,WX_COOLDOWN_ON_EVENT},{8690,115,0,0,0}};
static WxCooldownCatalog catalog={infos,6,sizeof infos,1,0,2};
static int apply(unsigned op,unsigned now){return wx_cooldown_apply(&state,&catalog,&inventory,(uint16_t)op,wire,at,1,now);}
static WxCooldownView query(unsigned binding,unsigned now){return wx_cooldown_query(&state,&catalog,binding,now);}
static void malformed(unsigned op,unsigned now){
    before=state;
    // List packets may have valid shorter prefixes. All cuts through a scalar
    // or entry must fail without changing even the diagnostic counters.
    for(unsigned n=0;n<at;n++){
        if(op==0x134&&n>=8&&(n-8)%8==0)continue;
        CHECK(wx_cooldown_apply(&state,&catalog,&inventory,(uint16_t)op,wire,n,1,now)==0);
        CHECK(!memcmp(&state,&before,sizeof state));
    }
    wire[at]=0;CHECK(!wx_cooldown_apply(&state,&catalog,&inventory,(uint16_t)op,wire,at+1,1,now));CHECK(!memcmp(&state,&before,sizeof state));
}
static void initial(unsigned count){at=0;w(0,1);w(2,2);w(78,2);w(0,2);w(100,2);w(0,2);w(count,2);}
static void row(unsigned spell,unsigned item,unsigned category,unsigned duration,unsigned cat){w(spell,2);w(item,2);w(category,2);w(duration,4);w(cat,4);}
static void event(unsigned spell,uint64_t player){at=0;w(spell,4);guid(player);}
static void go(unsigned spell,unsigned caster,unsigned unit,unsigned flags){at=0;w(1,1);w(caster,1);w(1,1);w(unit,1);w(spell,4);w(flags,2);w(0,1);w(0,1);w(0,2);if(flags&0x20){w(1,4);w(2,4);}}
static void item_packet(unsigned id){
    at=0;w(id,4);w(0,4);w(0,4);memcpy(wire+at,"Test item",10);at+=10;w(0,1);w(0,1);w(0,1);
    for(unsigned i=0;i<6+34+15+9+1;i++)w(0,4);
    for(unsigned i=0;i<5;i++){w(i?0:8690,4);w(0,4);w(0,4);w(i?UINT32_MAX:3600000,4);w(i?0:115,4);w(i?UINT32_MAX:3600000,4);}
    w(0,4);w(0,1);for(unsigned i=0;i<14;i++)w(0,4);
}
static void catalog_file(unsigned bad){
    FILE* f=fopen("cooldown-test.tmp","wb");CHECK(f);uint32_t h[]={0x44435857,2,6,sizeof(WxCooldownInfo)};
    if(bad==1)h[2]=65536;if(bad==2)h[3]++;fwrite(h,1,sizeof h,f);
    WxCooldownInfo rows[6];memcpy(rows,infos,sizeof rows);if(bad==3)rows[1].spell=78;if(bad==4)rows[2].recovery=UINT32_MAX;if(bad==5)rows[3].category=65536;
    if(bad==7)rows[3].gcd_category=65536;if(bad==8)rows[3].gcd_time=UINT32_MAX;if(bad==9)rows[3].damage_class=4;if(bad==10)rows[3].family=65536;
    fwrite(rows,1,sizeof rows,f);if(bad==6)fputc(0,f);CHECK(!fclose(f));
}
int main(int argc,char** argv){
    CHECK(sizeof(WxCooldowns)==65336); // fixed CPU-only storage; no steady-state allocations
    initial(3);row(78,0,1,5000,2000);row(200,0,1,1,0x80000000u);row(8690,6948,115,3600000,3600000);malformed(0x12a,100);
    CHECK(apply(0x12a,100)==1);CHECK(query(78,100).held&&query(100,100).held);CHECK(query(0x80000000u|6948,1100).remaining==3599000);
    event(200,2);CHECK(apply(0x1de,100)==1&&query(78,100).held);event(200,1);malformed(0x1de,100);CHECK(apply(0x1de,100)==1);
    CHECK(query(78,1100).remaining==4000&&query(100,1100).remaining==1000);CHECK(!query(100,2100).remaining);
    initial(1);row(78,0,0,1000,0);CHECK(apply(0x12a,UINT32_MAX-499)==1);CHECK(query(78,0).remaining==500);CHECK(!query(78,500).remaining);CHECK(!query(6948|0x80000000u,0).remaining);
    at=0;guid(1);w(78,4);w(7000,4);w(133,4);w(2000,4);malformed(0x134,1000);CHECK(apply(0x134,1000)==1);CHECK(query(78,1500).remaining==6500&&query(133,1500).remaining==1500);
    event(78,1);CHECK(apply(0x1de,1600)==1&&!query(78,1600).remaining);
    go(100,1,1,0);malformed(0x132,2000);CHECK(apply(0x132,2000)==1&&query(100,2000).remaining==15000&&query(78,2000).remaining==1000);
    at=0;guid(1);w(100,4);w(4000,4);CHECK(apply(0x134,2000)==1&&query(100,3000).remaining==14000); // short lockout cannot erase longer recovery
    go(200,1,1,0x20);malformed(0x132,3000);CHECK(apply(0x132,3000)==1&&query(200,999999).held);
    event(200,1);malformed(0x135,4000);CHECK(apply(0x135,4000)==1&&!query(200,4000).held&&query(200,4000).remaining==6000);
    go(201,1,1,0);CHECK(apply(0x132,5000)==1&&query(201,5000).held);event(201,1);CHECK(apply(0x135,5001)==1&&!query(201,5001).held);
    item_packet(6948);malformed(0x58,7000);CHECK(apply(0x58,7000)==1);inventory.count=1;inventory.items[0].guid=5;inventory.items[0].entry=6948;
    go(8690,5,1,0);CHECK(apply(0x132,7000)==1&&query(0x80000000u|6948,8000).remaining==3599000);
    event(8690,1);CHECK(apply(0x1de,9000)==1&&!query(0x80000000u|6948,9000).remaining);
    // Reflected miss, packed unit/item targets, finite source/destination, string and ammo.
    go(78,1,1,0x20);at=10;w(1,1);guid(9);w(1,1);guid(10);w(11,1);w(2,1);w(2|0x10|0x20|0x40|0x2000,2);w(1,1);w(9,1);w(1,1);w(5,1);
    for(unsigned i=0;i<6;i++)w(0,4);w('x',1);w(0,1);w(1,4);w(2,4);malformed(0x132,10000);CHECK(apply(0x132,10000)==1);
    before=state; // wrong recipient is a no-op
    event(78,2);CHECK(apply(0x135,10000)==1&&!memcmp(&state,&before,sizeof state));
    initial(2);row(78,0,0,1,0);row(78,0,0,2,0);CHECK(!apply(0x12a,0));CHECK(!memcmp(&state,&before,sizeof state));
    wx_cooldown_reset(&state);initial(512);for(unsigned i=0;i<512;i++)row(i+1,0,0,100000,0);CHECK(apply(0x12a,0)==1);
    at=0;guid(1);w(600,4);w(1000,4);CHECK(apply(0x134,0)==1&&state.overflow==1&&query(1,1).remaining==99999);
    CHECK(apply(0x134,100000)==1&&query(600,100001).remaining==999); // reuse expired slots
    wx_cooldown_reset(&state);CHECK(!state.rows[0].spell&&!state.items[0].item&&!state.packets);
    for(unsigned bad=0;bad<=10;bad++){catalog_file(bad);WxCooldownCatalog c={0};CHECK(wx_cooldown_catalog_open(&c,"cooldown-test.tmp")==!bad);
        if(!bad)CHECK(c.bytes==sizeof infos&&wx_cooldown_info(&c,100)->recovery==15000);else CHECK(!c.rows&&!c.bytes&&c.failures==1);wx_cooldown_catalog_close(&c);}
    catalog_file(0);available=8u*1024u*1024u;WxCooldownCatalog c={0};CHECK(!wx_cooldown_catalog_open(&c,"cooldown-test.tmp")&&!c.rows);available=48u*1024u*1024u;CHECK(!remove("cooldown-test.tmp"));
    if(argc==2){CHECK(wx_cooldown_catalog_open(&c,argv[1]));const WxCooldownInfo* charge=wx_cooldown_info(&c,100);CHECK(charge&&(charge->recovery==15000||charge->category_recovery==15000));CHECK(wx_cooldown_info(&c,133)&&!wx_cooldown_info(&c,133)->recovery);
        printf("Vanilla catalog: %u records / %u bytes; Charge recovery %u + category %u ms\n",c.count,c.bytes,charge->recovery,charge->category_recovery);wx_cooldown_catalog_close(&c);}
    printf("Vanilla timers, item overrides, shared categories, reconnect, malformed packets and wraparound: %u checks pass\n",checks);return 0;
}
