#include "wx_realm.h"
#include <string.h>
#include <math.h>
static uint32_t u32(const uint8_t* p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int string(const uint8_t* data,size_t size,size_t* at,char* output){
    size_t start=*at;
    while(*at<size&&data[*at]){
        if(data[*at]<32||data[*at]==127||*at-start>=255)return 0;
        ++*at;
    }
    if(*at==size||*at==start)return 0;
    if(output){memcpy(output,data+start,*at-start);output[*at-start]=0;}
    ++*at;return 1;
}
static int endpoint(char* address,uint16_t* output){
    char* colon=strchr(address,':');
    if(!colon||colon==address||!colon[1])return 0;
    // The native socket transport currently supports IPv4 endpoints. Keep DNS
    // names intact for the connection layer rather than truncating them here.
    for(char* p=address;p<colon;p++)if(!((*p>='0'&&*p<='9')||(*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||*p=='-'||*p=='.'))return 0;
    unsigned port=0;
    for(char* p=colon+1;*p;p++){if(*p<'0'||*p>'9'||port>6553)return 0;port=port*10+(unsigned)(*p-'0');}
    if(!port||port>65535)return 0;
    *colon=0;*output=(uint16_t)port;return 1;
}
static int scan(const uint8_t* data,size_t size,WxRealms* output){
    unsigned count=data[4];size_t at=5;
    for(unsigned i=0;i<count;i++){
        WxRealm realm={0};
        if(size-at<5)return 0;
        realm.type=u32(data+at);realm.flags=data[at+4];at+=5;
        if(!string(data,size,&at,realm.name)||!string(data,size,&at,realm.host)||!endpoint(realm.host,&realm.port))return 0;
        if(size-at<7)return 0;
        uint32_t bits=u32(data+at);memcpy(&realm.population,&bits,4);
        if(!isfinite(realm.population)||realm.population<0)return 0;
        realm.characters=data[at+4];realm.category=data[at+5];realm.id=data[at+6];at+=7;
        // Build 5875 does not append a version tuple, even with flag 0x04.
        // See pinned vMaNGOS AuthSocket::LoadRealmlistAndWriteIntoBuffer.
        if(output)output->items[i]=realm;
    }
    return at+2==size;
}
int wx_realms_parse(const uint8_t* data,size_t size,WxRealms* output){
    if(!data||!output||size<7||size>WX_REALMS_BYTES||!scan(data,size,NULL))return 0;
    // Two bounded passes avoid a 134 KiB stack scratch object or partial publish.
    memset(output,0,sizeof *output);output->count=data[4];return scan(data,size,output);
}
int wx_realm_available(const WxRealm* realm){return realm&&!(realm->flags&3)&&realm->port&&realm->host[0];}
const char* wx_realm_type(uint32_t type){switch(type){case 0:return "Normal";case 1:return "PvP";case 6:return "RP";case 8:return "RPPvP";default:return "Unknown";}}
const char* wx_realm_population(const WxRealm* realm){
    if(!realm)return "Unknown";if(realm->flags&2)return "Offline";if(realm->flags&1)return "Unavailable";
    // Match the pinned WoWee RealmScreen population categories.
    return realm->population>=2.5f?"Full":realm->population>=1.5f?"High":realm->population>=.5f?"Medium":"Low";
}
