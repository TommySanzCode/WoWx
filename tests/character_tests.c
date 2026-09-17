#include "wx_character.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Character test failed at %d\n",__LINE__);exit(1);}}while(0)
static void word(unsigned char* p,unsigned value){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(value>>(i*8));}
int main(void){
    unsigned char packet[512]={1,7};size_t start=9;memcpy(packet+start,"Fixture",8);start+=8;
    unsigned char* f=packet+start;f[0]=8;f[1]=8;f[2]=1;f[3]=2;f[4]=3;f[5]=4;f[6]=5;f[7]=6;f[8]=60;
    word(f+9,12);word(f+13,1);word(f+50+19*5,0x12345);f[54+19*5]=18;
    size_t n=start+150;WxCharacters characters;
    CHECK(wx_characters_parse(packet,n,&characters));const WxCharacter* c=&characters.items[0];
    CHECK(characters.count==1&&c->guid==7&&!strcmp(c->name,"Fixture"));
    CHECK(c->race==8&&c->character_class==8&&c->gender==1&&c->level==60);
    CHECK(c->skin==2&&c->face==3&&c->hair_style==4&&c->hair_color==5&&c->facial_hair==6);
    CHECK(c->display[19]==0x12345&&c->inventory_type[19]==18&&c->zone==12&&c->map==1);
    for(size_t short_size=0;short_size<n;short_size++){CHECK(!wx_characters_parse(packet,short_size,&characters));CHECK(characters.count==0);}
    CHECK(!wx_characters_parse(packet,n+1,&characters));f[2]=2;CHECK(!wx_characters_parse(packet,n,&characters));f[2]=1;
    f[0]=9;CHECK(!wx_characters_parse(packet,n,&characters));f[0]=8;
    word(f+17,0x7fc00000);CHECK(!wx_characters_parse(packet,n,&characters));word(f+17,0);
    packet[0]=2;memcpy(packet+n,packet+1,n-1);CHECK(!wx_characters_parse(packet,n*2-1,&characters));
    packet[n]=8;CHECK(wx_characters_parse(packet,n*2-1,&characters)&&characters.count==2);
    CHECK(characters.items[1].facial_hair==6&&characters.items[1].display[19]==0x12345);
    CHECK(wx_character_find(&characters,7)==characters.items&&wx_character_find(&characters,8)==characters.items+1);
    CHECK(!wx_character_find(NULL,7)&&!wx_character_find(&characters,0)&&!wx_character_find(&characters,9));
    characters.count=11;CHECK(!wx_character_find(&characters,7));
    CHECK(strstr(wx_character_delete_result(0x39),"deleted")&&strstr(wx_character_delete_result(0x3a),"refused"));
    CHECK(strstr(wx_character_delete_result(0x3b),"transfer"));
    CHECK(strstr(wx_character_delete_result(0),"not confirmed")&&strstr(wx_character_delete_result(0x47),"not confirmed"));
    printf("Vanilla character enumeration checks: %u passed\n",checks);return 0;
}
