#include "wx_realm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks;
#define CHECK(v) do{checks++;if(!(v)){fprintf(stderr,"Realm failure at %u: %s\n",__LINE__,#v);exit(1);}}while(0)
static uint8_t data[65536];static size_t at;
static WxRealms result,before;
static void begin(unsigned count){memset(data,0,sizeof data);data[4]=(uint8_t)count;at=5;}
static void entry(const char* name,const char* host,unsigned type,unsigned flags,float population){
    data[at++]=(uint8_t)type;data[at++]=(uint8_t)(type>>8);data[at++]=(uint8_t)(type>>16);data[at++]=(uint8_t)(type>>24);data[at++]=(uint8_t)flags;
    size_t size=strlen(name)+1;memcpy(data+at,name,size);at+=size;size=strlen(host)+1;memcpy(data+at,host,size);at+=size;
    memcpy(data+at,&population,4);at+=4;data[at++]=7;data[at++]=1;data[at++]=42;
}
static void end(void){data[at++]=2;data[at++]=0;}
static void invalid(void){before=result;CHECK(!wx_realms_parse(data,at,&result));CHECK(!memcmp(&before,&result,sizeof result));}
int main(void){
    begin(2);entry("Private Vanilla","10.0.2.2:8086",1,4,.75f);entry("Roleplay","realm.example:65535",6,2,2.f);end();
    CHECK(wx_realms_parse(data,at,&result));CHECK(result.count==2);
    CHECK(!strcmp(result.items[0].name,"Private Vanilla")&&!strcmp(result.items[0].host,"10.0.2.2"));
    CHECK(result.items[0].flags==4&&result.items[0].port==8086&&result.items[0].type==1&&result.items[0].population==.75f);
    CHECK(result.items[0].characters==7&&result.items[0].category==1&&result.items[0].id==42);
    CHECK(wx_realm_available(&result.items[0])&&!wx_realm_available(&result.items[1]));
    CHECK(!strcmp(result.items[1].host,"realm.example")&&result.items[1].port==65535);
    CHECK(!strcmp(wx_realm_type(0),"Normal")&&!strcmp(wx_realm_type(1),"PvP")&&!strcmp(wx_realm_type(6),"RP")&&!strcmp(wx_realm_type(8),"RPPvP")&&!strcmp(wx_realm_type(999),"Unknown"));
    CHECK(!strcmp(wx_realm_population(&result.items[0]),"Medium")&&!strcmp(wx_realm_population(&result.items[1]),"Offline"));
    WxRealm category=result.items[0];category.population=1.25f;CHECK(!strcmp(wx_realm_population(&category),"Medium"));
    category.population=2.25f;CHECK(!strcmp(wx_realm_population(&category),"High"));category.population=2.5f;CHECK(!strcmp(wx_realm_population(&category),"Full"));
    before=result;for(size_t length=0;length<at;length++){CHECK(!wx_realms_parse(data,length,&result));CHECK(!memcmp(&before,&result,sizeof result));}
    data[at++]=0;invalid();
    const char* addresses[]={"",":8086","10.0.2.2:","10.0.2.2:0","10.0.2.2:-1","10.0.2.2:65536","10.0.2.2:99999999999","10.0.2.2:8086x","host:1:2","a b:8086","http://host:8086"};
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;i++){begin(1);entry("Realm",addresses[i],0,0,0);end();invalid();}
    const float bad[]={-1.f,INFINITY,-INFINITY,NAN};for(unsigned i=0;i<4;i++){begin(1);entry("Realm","127.0.0.1:1",0,0,bad[i]);end();invalid();}
    begin(1);entry("","127.0.0.1:1",0,0,0);end();invalid();
    begin(1);entry("Line\nbreak","127.0.0.1:1",0,0,0);end();invalid();
    begin(1);entry("Realm","127.0.0.1:1",0,4,0);memset(data+at,0,5);at+=5;end();invalid();
    char name[257];memset(name,'R',255);name[255]=0;
    begin(1);entry(name,"127.0.0.1:1",0,0,0);end();CHECK(wx_realms_parse(data,at,&result)&&strlen(result.items[0].name)==255);
    name[255]='R';name[256]=0;begin(1);entry(name,"127.0.0.1:1",0,0,0);end();invalid();
    begin(255);for(unsigned i=0;i<255;i++)entry("Realm","127.0.0.1:8086",i,i==254?4:0,i/128.f);end();
    CHECK(wx_realms_parse(data,at,&result)&&result.count==255&&result.items[254].type==254&&result.items[254].flags==4);
    begin(0);end();CHECK(wx_realms_parse(data,at,&result)&&!result.count);CHECK(!result.items[254].name[0]);
    begin(1);memset(data+at,'R',300);at+=300;invalid();
    CHECK(!wx_realms_parse(NULL,7,&result));CHECK(!wx_realms_parse(data,7,NULL));CHECK(!wx_realms_parse(data,65536,&result));
    CHECK(!wx_realm_available(NULL));printf("Vanilla realm boundaries: %u checks passed\n",checks);return 0;
}
