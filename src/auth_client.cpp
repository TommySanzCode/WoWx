// Bounded Vanilla wire adapter around WoWee's reusable SRP implementation.
#include "wx_auth.h"
#include "auth/srp.hpp"
#include "wx_srp.hpp"
#include <atomic>
#include <cstring>
#include <algorithm>
#include "net_connection.hpp"
enum State {IDLE,CONNECTING,CHALLENGE,PROOF,REALMS,DONE,NETWORK_ERROR,PROTOCOL_ERROR,AUTH_ERROR,CRYPTO_ERROR,CANCELLED};
static std::atomic<int> state{IDLE};
static std::atomic<int> cancelled{0};
static std::atomic_flag auth_lock=ATOMIC_FLAG_INIT;
static WxWorldSession session{};
static WxRealms realm_list{};
static uint8_t realm_packet[WX_REALMS_BYTES];
struct AuthLocked {
    AuthLocked(){while(auth_lock.test_and_set(std::memory_order_acquire))Sleep(0);}
    ~AuthLocked(){auth_lock.clear(std::memory_order_release);}
};
extern "C" int wx_auth_realms(WxRealms* output){
    if(!output)return 0;AuthLocked lock;if(state.load()!=DONE)return 0;*output=realm_list;return 1;
}
extern "C" int wx_auth_select(unsigned index,WxWorldSession* output){
    if(!output)return 0;AuthLocked lock;
    if(state.load()!=DONE||index>=realm_list.count||!wx_realm_available(&realm_list.items[index]))return 0;
    const WxRealm& realm=realm_list.items[index];*output=session;
    strcpy(output->host,realm.host);output->port=realm.port;return 1;
}
extern "C" int wx_auth_session(WxWorldSession* output){return wx_auth_select(0,output);}
extern "C" unsigned wx_auth_state(void){return (unsigned)state.load();}
extern "C" void wx_auth_cancel(void){cancelled=1;}
extern "C" void wx_auth_forget(void){AuthLocked lock;state=IDLE;volatile uint8_t* key=session.key;for(unsigned i=0;i<40;i++)key[i]=0;session={};realm_list.count=0;}
static bool is_cancelled(){return cancelled.load()!=0;}
extern "C" const char* wx_auth_status(void){
    static const char* names[]={"offline","connecting","SRP challenge","verifying server","requesting realms","authenticated / realm list received","network error","invalid auth packet","login rejected","crypto failure","cancelled"};
    return names[state.load()];
}
static int fail(State reason){state=cancelled.load()?CANCELLED:reason;return 0;}
extern "C" int wx_auth_run(const WxAuthConfig* config){
    cancelled=0;wx_auth_forget();
    if(!config||!memchr(config->host,0,sizeof config->host)||!memchr(config->username,0,sizeof config->username)||!memchr(config->password,0,sizeof config->password)||!config->port||config->port>65535)return fail(PROTOCOL_ERROR);
    size_t length=strlen(config->username);if(!length||length>16||!strlen(config->password))return fail(PROTOCOL_ERROR);
    state=CONNECTING;
#ifndef WX_XBOX
    WSADATA startup;if(WSAStartup(MAKEWORD(2,2),&startup))return fail(NETWORK_ERROR);
#endif
    Connection c;c.cancelled=is_cancelled;if(c.handle==(Socket)-1)return fail(NETWORK_ERROR);
#ifdef WX_XBOX
    timeval timeout{5,0};
#else
    DWORD timeout=5000;
#endif
    setsockopt(c.handle,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof timeout);
    setsockopt(c.handle,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof timeout);
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons((uint16_t)config->port);
    address.sin_addr.s_addr=inet_addr(config->host);
    if(address.sin_addr.s_addr==INADDR_NONE)return fail(NETWORK_ERROR);
    unsigned long nonblocking=1;
    if(ioctlsocket(c.handle,FIONBIO,&nonblocking))return fail(NETWORK_ERROR);
    int connected=connect(c.handle,(sockaddr*)&address,sizeof address);
    if(connected&& !c.ready(true))return fail(NETWORK_ERROR);
    int error=0;
#ifdef WX_XBOX
    socklen_t error_size=sizeof error;
#else
    int error_size=sizeof error;
#endif
    if(getsockopt(c.handle,SOL_SOCKET,SO_ERROR,(char*)&error,&error_size)||error)return fail(NETWORK_ERROR);
    nonblocking=0;if(ioctlsocket(c.handle,FIONBIO,&nonblocking))return fail(NETWORK_ERROR);
    uint8_t challenge[64]={0,3,0,0,'W','o','W',0,1,12,1,0xf3,0x16,'6','8','x',0,'n','i','W',0,'S','U','n','e'};
    challenge[2]=(uint8_t)(30+length);challenge[33]=(uint8_t)length;
    for(size_t i=0;i<length;i++){char ch=config->username[i];if(ch>='a'&&ch<='z')ch-=32;challenge[34+i]=(uint8_t)ch;}
    if(!c.sendExact(challenge,34+length))return fail(NETWORK_ERROR);
    state=CHALLENGE;uint8_t response[119]={};
    if(!c.receive(response,3))return fail(NETWORK_ERROR);
    if(response[0]!=0)return fail(PROTOCOL_ERROR);if(response[2])return fail(AUTH_ERROR);
    // The only accepted parameter set is Vanilla's 32-byte modulus and g=7.
    if(!c.receive(response+3,33))return fail(NETWORK_ERROR);
    if(response[35]!=1)return fail(PROTOCOL_ERROR);
    if(!c.receive(response+36,2))return fail(NETWORK_ERROR);
    if(response[36]!=7||response[37]!=32)return fail(PROTOCOL_ERROR);
    if(!c.receive(response+38,81))return fail(NETWORK_ERROR);
    const uint8_t modulus[32]={0xb7,0x9b,0x3e,0x2a,0x87,0x82,0x3c,0xab,0x8f,0x5e,0xbf,0xbf,0x8e,0xb1,0x01,0x08,0x53,0x50,0x06,0x29,0x8b,0x5b,0xad,0xbd,0x5b,0x53,0xe1,0x89,0x5e,0x64,0x4b,0x89};
    if(memcmp(response+38,modulus,32)||response[118])return fail(PROTOCOL_ERROR);
    unsigned nonzero=0;for(int i=3;i<35;i++)nonzero|=response[i];if(!nonzero)return fail(PROTOCOL_ERROR);
    bool smaller=false;for(int i=31;i>=0;i--){if(response[3+i]<modulus[i]){smaller=true;break;}if(response[3+i]>modulus[i])break;}
    if(!smaller)return fail(PROTOCOL_ERROR);
    wowee::auth::SRP srp;srp.initialize(config->username,config->password);
    srp.feed({response+3,response+35},{7},{modulus,modulus+32},wx_srp_natural(response+70,32));
    if(!wx_crypto_healthy())return fail(CRYPTO_ERROR);
    auto A=srp.getA(),key=srp.getSessionKey();if(A.size()!=32||key.size()!=40)return fail(CRYPTO_ERROR);
    auto proofs=wx_srp_proofs(modulus,response+70,A.data(),response+3,key.data(),config->username);
    auto& M1=proofs.client;if(M1.size()!=20||proofs.server.size()!=20)return fail(CRYPTO_ERROR);
    uint8_t proof[75]={1};memcpy(proof+1,A.data(),32);memcpy(proof+33,M1.data(),20);
    state=PROOF;if(!c.sendExact(proof,sizeof proof)||!c.receive(response,2))return fail(NETWORK_ERROR);
    if(response[0]!=1)return fail(PROTOCOL_ERROR);if(response[1])return fail(AUTH_ERROR);
    if(!c.receive(response+2,24))return fail(NETWORK_ERROR);
    unsigned proof_difference=0;for(unsigned i=0;i<20;i++)proof_difference|=response[i+2]^proofs.server[i];
    if(proof_difference)return fail(CRYPTO_ERROR);
    state=REALMS;const uint8_t request[5]={0x10,0,0,0,0};
    if(!c.sendExact(request,sizeof request)||!c.receive(response,3))return fail(NETWORK_ERROR);
    size_t bytes=response[1]|(size_t(response[2])<<8);if(response[0]!=0x10||bytes<7)return fail(PROTOCOL_ERROR);
    if(!c.receive(realm_packet,bytes))return fail(NETWORK_ERROR);
    AuthLocked lock;
    if(cancelled.load())return fail(CANCELLED);
    if(!wx_realms_parse(realm_packet,bytes,&realm_list))return fail(PROTOCOL_ERROR);
    session={};strcpy(session.username,config->username);
    for(char* p=session.username;*p;p++)if(*p>='a'&&*p<='z')*p-=32;
    memcpy(session.key,key.data(),40);
    state=DONE;return 1;
}
