#include "wx_entities.h"
#include "wx_inventory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static WxEntities live,scratch,before;
static uint8_t packet[1024];static unsigned length,checks;
#define CHECK(value) do{checks++;if(!(value)){fprintf(stderr,"Entity test failed at %d: %s\n",__LINE__,#value);exit(1);}}while(0)
static void byte(unsigned value){packet[length++]=(uint8_t)value;}
static void word(uint32_t value){for(unsigned i=0;i<4;i++)byte(value>>(i*8));}
static void real(float value){uint32_t bits;memcpy(&bits,&value,4);word(bits);}
static void create(void){
    length=0;word(1);byte(0);byte(2);byte(1);byte(7);byte(3);byte(0x60);
    word(0);word(100);real(-8940);real(-130);real(83);real(.5f);word(0);
    for(unsigned i=0;i<6;i++)real(i==1?7:2.5f);
    byte(5);word((1u<<3)|(1u<<22)|(1u<<28));word((1u<<2)|(1u<<3));word(0);word(0);word((1u<<3)|(1u<<19));
    word(69);word(100);word(120);word(2);word(7);word(299);word(2);
}
static void move_packet(void){
    length=0;byte(1);byte(7);real(0);real(0);real(0);word(123);byte(0);
    word(0);word(2000);word(2);real(10);real(0);real(0);word(20);
}
static void motion_tests(void){
    memset(&live,0,sizeof live);create();CHECK(wx_entities_apply(&live,&scratch,packet,length,1000));
    move_packet();CHECK(wx_entities_monster_move(&live,packet,length,1000));WxEntity e;
    wx_entities_sample(&live,0,2000,&e);CHECK(e.x==5&&e.y==0&&e.animation==4);
    wx_entities_sample(&live,0,2500,&e);CHECK(e.x==7.5f);
    wx_entities_sample(&live,0,3000,&e);CHECK(e.x==10&&e.animation==0);
    before=live;for(unsigned n=0;n<length;n++){CHECK(!wx_entities_monster_move(&live,packet,n,1000));CHECK(!memcmp(&before,&live,sizeof live));}
    packet[length]=0;CHECK(!wx_entities_monster_move(&live,packet,length+1,1000));
    packet[28]=2;CHECK(!wx_entities_monster_move(&live,packet,length,1000));move_packet();
    packet[19]=0;packet[20]=1;CHECK(wx_entities_monster_move(&live,packet,length,UINT32_MAX-500));
    wx_entities_sample(&live,0,499,&e);CHECK(e.x==5&&e.animation==5);
    length=0;byte(1);byte(7);real(7);real(2);real(0);word(124);byte(1);
    CHECK(wx_entities_monster_move(&live,packet,length,5000));wx_entities_sample(&live,0,6000,&e);
    CHECK(e.x==7&&e.y==2&&e.animation==0);
    // Creation update retains elapsed time and its complete ground path.
    length=0;word(1);byte(0);byte(2);byte(1);byte(7);byte(3);byte(0x20);
    word(0x400000);word(100);real(5);real(0);real(0);real(0);word(0);
    for(unsigned i=0;i<6;i++)real(2.5f);
    word(0);word(1000);word(2000);word(124);word(2);
    real(0);real(0);real(0);real(10);real(0);real(0);real(10);real(0);real(0);byte(0);
    CHECK(wx_entities_apply(&live,&scratch,packet,length,5000));wx_entities_sample(&live,0,5000,&e);CHECK(e.x==5);
    wx_entities_destroy(&live,7);CHECK(!live.paths[0].count&&!live.count);
    length=0;word(1);byte(0);byte(2);byte(1);byte(7);byte(4);byte(0x40);
    real(0);real(0);real(0);real(0);byte(37);
    for(unsigned i=0;i<37;i++)word(i==6?((1u<<6)|(1u<<7)|(1u<<8)):i==15?(1u<<6):i==22?((1u<<12)|(1u<<13)):i==36?(1u<<24):0);
    word(783);word(1);word(0);word(99);word(40);word(400);word(10);
    CHECK(wx_entities_apply(&live,&scratch,packet,length,1000));
    CHECK(live.items[0].inventory[0]==99&&live.items[0].quests[0]==783&&live.items[0].quests[1]==1&&live.items[0].xp==40&&live.items[0].next_xp==400&&live.items[0].money==10);
}
static void nearby_tests(void){
    memset(&live,0,sizeof live);static WxEntity out[WX_VISIBLE_ENTITIES+1];float p[3]={0};
    for(unsigned i=0;i<WX_MAX_ENTITIES;i++){
        live.items[i].guid=1000+i;live.items[i].type=3;live.items[i].positioned=1;live.items[i].x=(float)(WX_MAX_ENTITIES-i);
    }
    // Self arrives last, after far more objects than fit in the public snapshot.
    live.items[511].guid=7;live.items[511].type=4;live.items[511].positioned=0;
    live.items[510].guid=8;live.items[510].type=7;live.items[510].positioned=0;live.items[510].fields[6]=7;
    out[WX_VISIBLE_ENTITIES].guid=UINT64_MAX;
    CHECK(wx_entities_nearby(&live,7,1000,p,0,out,WX_VISIBLE_ENTITIES+1)==WX_VISIBLE_ENTITIES);
    CHECK(out[0].guid==7&&out[1].guid==1000&&out[2].guid==8);
    CHECK(out[3].guid==1509&&out[WX_VISIBLE_ENTITIES-1].guid==1385);
    CHECK(out[WX_VISIBLE_ENTITIES].guid==UINT64_MAX);
    CHECK(wx_entities_nearby(&live,7,1000,p,0,out,1)==1&&out[0].guid==7);
    CHECK(!wx_entities_nearby(&live,7,0,p,0,out,0));
    CHECK(!wx_entities_nearby(&live,7,0,p,0,0,128));
    CHECK(!wx_entities_nearby(0,7,0,p,0,out,128));
    CHECK(!wx_entities_nearby(&live,7,0,0,0,out,128));
    // Snapshot order follows distance from the current player position.
    p[0]=500;CHECK(wx_entities_nearby(&live,7,0,p,0,out,4)==4);
    CHECK(out[0].guid==7&&out[1].guid==8&&out[2].guid==1012&&out[3].guid==1011);
    // A moving creature's sampled position determines inclusion, and GUIDs
    // break equal-distance ties independently of server insertion order.
    memset(&live,0,sizeof live);p[0]=0;
    live.items[0].guid=50;live.items[0].type=3;live.items[0].positioned=1;live.items[0].x=100;
    live.items[1]=live.items[0];live.items[1].guid=40;
    live.paths[0].count=2;live.paths[0].duration=100;live.paths[0].points[0][0]=100;
    live.paths[0].distance[1]=100;
    CHECK(wx_entities_nearby(&live,7,0,p,100,out,1)==1&&out[0].guid==50&&out[0].x==0);
    memset(live.paths,0,sizeof live.paths);
    CHECK(wx_entities_nearby(&live,7,0,p,0,out,1)==1&&out[0].guid==40);
    live.items[0].type=1;CHECK(wx_entities_nearby(&live,7,50,p,0,out,128)==1&&out[0].guid==40);
}
static void appearance_packet(int create_player,unsigned kind){
    length=0;word(1);byte(0);byte(create_player?2:0);byte(1);byte(7);
    if(create_player){byte(kind);byte(0);}
    uint32_t masks[16]={0};
    unsigned fields[]={36,193,194,259,260,261,272,284,296,308,320,332,344,356,368,380,392,404,416,428,440,452,464,476,477,485};
    for(unsigned i=0;i<sizeof fields/sizeof fields[0];i++)masks[fields[i]/32]|=1u<<(fields[i]%32);
    byte(16);for(unsigned i=0;i<16;i++)word(masks[i]);
    for(unsigned i=0;i<sizeof fields/sizeof fields[0];i++)word(fields[i]==36?0x010103:fields[i]==193?0x04030201:fields[i]==194?0xAABBCC05:fields[i]+1000);
}
static void appearance_tests(void){
    memset(&live,0,sizeof live);appearance_packet(1,4);
    CHECK(wx_entities_apply(&live,&scratch,packet,length,0));
    CHECK(live.items[0].fields[36]==0x010103&&live.items[0].player_bytes==0x04030201&&live.items[0].player_bytes2==0xAABBCC05);
    for(unsigned i=0;i<19;i++)CHECK(live.items[0].visible_items[i]==1260+i*12);
    appearance_packet(0,0);before=live;
    for(unsigned n=0;n<length;n++){CHECK(!wx_entities_apply(&live,&scratch,packet,n,0));CHECK(!memcmp(&live,&before,sizeof live));}
    packet[length]=0;CHECK(!wx_entities_apply(&live,&scratch,packet,length+1,0));CHECK(!memcmp(&live,&before,sizeof live));
    // Explicit zero removes equipment, while an omitted adjacent slot persists.
    length=0;word(1);byte(0);byte(0);byte(1);byte(7);byte(15);
    for(unsigned i=0;i<15;i++)word(i==6?2:i==14?16:0);word(0);word(0);
    CHECK(wx_entities_apply(&live,&scratch,packet,length,0));
    CHECK(live.items[0].player_bytes==0&&live.items[0].player_bytes2==0xAABBCC05&&live.items[0].visible_items[16]==0&&live.items[0].visible_items[15]==1440);
    // Re-creation clears defaults, even when the create mask omits all fields.
    length=0;word(1);byte(0);byte(2);byte(1);byte(7);byte(4);byte(0);byte(0);
    CHECK(wx_entities_apply(&live,&scratch,packet,length,0)&&!live.items[0].player_bytes&&!live.items[0].visible_items[15]);
    appearance_packet(1,3);CHECK(wx_entities_apply(&live,&scratch,packet,length,0));
    CHECK(!live.items[0].player_bytes&&!live.items[0].player_bytes2&&!live.items[0].visible_items[0]);
}
static void exploration_tests(void){
    memset(&live,0,sizeof live);live.count=1;live.items[0].guid=7;live.items[0].type=4;
    length=0;word(1);byte(0);byte(0);byte(1);byte(7);byte(37);
    for(unsigned i=0;i<37;i++)word(i==34?(1u<<23):i==35?1:i==36?(1u<<22):0);
    word(0x80000000);word(123);word(0x80000001);
    CHECK(wx_entities_apply(&live,&scratch,packet,length,1000));
    CHECK(live.items[0].explored[0]==0x80000000&&live.items[0].explored[9]==123&&live.items[0].explored[63]==0x80000001);
    before=live;CHECK(!wx_entities_apply(&live,&scratch,packet,length-1,1000));CHECK(!memcmp(&live,&before,sizeof live));
    length=0;word(1);byte(0);byte(0);byte(1);byte(7);byte(35);
    for(unsigned i=0;i<35;i++)word(i==34?(1u<<23):0);word(1);
    CHECK(wx_entities_apply(&live,&scratch,packet,length,1001));CHECK(live.items[0].explored[0]==1&&live.items[0].explored[9]==123&&live.items[0].explored[63]==0x80000001);
}
int main(void){
    exploration_tests();
    // Real 5875 observer relocation captured when a rabbit respawned nearby.
    // This is not a request to transfer the local player to another map.
    const uint8_t relocation[]={0xdf,0xe8,0x38,0x01,0xd1,0x02,0x30,0xf1,0x00,0x01,0x00,0x00,0xad,0x62,0xac,0x00,0xcc,0xe1,0x08,0xc6,0x54,0x5a,0x5a,0xc3,0x25,0xb5,0xab,0x42,0x0c,0x29,0x3c,0xbd,0x00,0x00,0x00,0x00};
    uint64_t moved=0;CHECK(wx_entities_relocate(&live,relocation,sizeof relocation,&moved));CHECK(moved==UINT64_C(0xf1300002d10138e8)&&live.unknown_updates==1);
    live.items[0].guid=moved;live.count=1;live.paths[0].count=2;live.paths[0].duration=1000;
    CHECK(wx_entities_relocate(&live,relocation,sizeof relocation,&moved));CHECK(live.items[0].x<-8700&&live.items[0].z>80&&!live.paths[0].count);
    before=live;for(unsigned i=0;i<sizeof relocation;i++){CHECK(!wx_entities_relocate(&live,relocation,i,&moved));CHECK(!memcmp(&before,&live,sizeof live));}
    memset(&live,0,sizeof live);
    create();CHECK(wx_entities_apply(&live,&scratch,packet,length,1000));CHECK(live.count==1&&live.items[0].guid==7);
    CHECK(live.items[0].fields[22]==100&&live.items[0].fields[131]==299&&live.items[0].fields[147]==2);
    CHECK(live.items[0].x==-8940&&live.items[0].positioned);
    before=live;CHECK(!wx_entities_apply(&live,&scratch,packet,length-1,1000));CHECK(!memcmp(&before,&live,sizeof live));
    byte(0);CHECK(!wx_entities_apply(&live,&scratch,packet,length,1000));length--;
    packet[9]=0x80;CHECK(!wx_entities_apply(&live,&scratch,packet,length,1000));create();
    packet[18]=packet[19]=0;packet[20]=0x80;packet[21]=0x7f;CHECK(!wx_entities_apply(&live,&scratch,packet,length,1000));create();
    uint8_t compressed[2048],decoded[1024];uLongf size=sizeof compressed-4;
    CHECK(compress2(compressed+4,&size,packet,length,Z_BEST_SPEED)==Z_OK);
    for(unsigned i=0;i<4;i++)compressed[i]=(uint8_t)(length>>(i*8));size_t actual=0;
    CHECK(wx_update_inflate(compressed,size+4,decoded,sizeof decoded,&actual));CHECK(actual==length&&!memcmp(decoded,packet,length));
    CHECK(!wx_update_inflate(compressed,size+3,decoded,sizeof decoded,&actual));
    CHECK(!wx_update_inflate(compressed,size+4,decoded,length-1,&actual));
    compressed[size+4]=0;CHECK(!wx_update_inflate(compressed,size+5,decoded,sizeof decoded,&actual));
    compressed[3]=0x7f;CHECK(!wx_update_inflate(compressed,size+4,decoded,sizeof decoded,&actual));
    length=0;word(1);byte(0);byte(4);word(1);byte(1);byte(7);
    CHECK(wx_entities_apply(&live,&scratch,packet,length,1000)&&!live.count);
    create();for(unsigned i=0;i<WX_MAX_ENTITIES;i++)live.items[i].guid=100+i;live.count=WX_MAX_ENTITIES;before=live;
    CHECK(!wx_entities_apply(&live,&scratch,packet,length,1000));CHECK(!memcmp(&live,&before,sizeof live));
    motion_tests();
    memset(&live,0,sizeof live);live.items[0].guid=7;live.items[0].type=4;live.items[0].money=123;
    live.items[0].inventory[46]=8;live.items[1].guid=8;live.items[1].type=1;live.items[1].fields[3]=7073;live.items[1].fields[14]=2;
    WxInventory inventory;wx_inventory_snapshot(&live,7,&inventory);
    CHECK(inventory.count==1&&inventory.money==123&&inventory.items[0].entry==7073&&inventory.items[0].count==2&&inventory.items[0].bag==255&&inventory.items[0].slot==23);
    live.items[0].inventory[38]=9;live.items[2].guid=9;live.items[2].type=2;live.items[2].fields[3]=828;live.items[2].fields[48]=36;live.items[2].fields[120]=10;
    live.items[3].guid=10;live.items[3].type=1;live.items[3].fields[3]=7074;live.items[3].fields[14]=1;
    wx_inventory_snapshot(&live,7,&inventory);CHECK(inventory.count==3&&inventory.items[1].entry==7074&&inventory.items[1].bag==19&&inventory.items[1].slot==35);
    wx_entities_destroy(&live,10);wx_inventory_snapshot(&live,7,&inventory);CHECK(inventory.count==2);
    memset(&inventory,0,sizeof inventory);inventory.count=1;inventory.items[0].guid=9;inventory.items[0].count=4;inventory.money=34;
    CHECK(wx_inventory_sale_completed(&inventory,9,5,33));
    CHECK(!wx_inventory_sale_completed(&inventory,9,5,34));
    CHECK(!wx_inventory_sale_completed(&inventory,9,6,33));
    CHECK(!wx_inventory_sale_completed(&inventory,9,0,33));
    inventory.count=0;CHECK(wx_inventory_sale_completed(&inventory,9,1,33));
    nearby_tests();appearance_tests();
    printf("Entity update/decompression/motion checks: %u passed\n",checks);return 0;
}
