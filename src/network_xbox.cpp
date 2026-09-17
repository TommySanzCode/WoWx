#include "wx_auth.h"
#include "wx_world.h"
#include "wx_lobby.h"
#include "wx_login.h"
#include <nxdk/net.h>
#include <nxdk/mount.h>
#include <xboxkrnl/xboxkrnl.h>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <atomic>
static std::atomic<int> network_ready{0};
static std::atomic<int> reconnect{1};
static std::atomic_flag front_lock=ATOMIC_FLAG_INIT;
static WxLoginView front{WX_LOGIN_ACCOUNT,0,0,0,0,0,1};
static WxLoginCommand pending{};
static unsigned generation;
struct FrontLocked {
    FrontLocked(){while(front_lock.test_and_set(std::memory_order_acquire))Sleep(0);}
    ~FrontLocked(){front_lock.clear(std::memory_order_release);}
};
static void erase_password(WxAuthConfig& config){volatile char* p=config.password;for(unsigned i=0;i<sizeof config.password;i++)p[i]=0;}
static bool load_config(WxAuthConfig& config){
    FILE* file=fopen("D:\\testauth.bin","rb");if(!file)return false;
    bool good=fread(&config,1,sizeof config,file)==sizeof config&&fgetc(file)==EOF;fclose(file);
    return good&&memchr(config.host,0,sizeof config.host)&&memchr(config.username,0,sizeof config.username)&&memchr(config.password,0,sizeof config.password);
}
extern "C" void wx_network_login_defaults(WxAuthConfig* output){
    if(!output)return;WxAuthConfig config{};
    if(!load_config(config)){strcpy(config.host,"10.0.2.2");config.port=3724;}
    {FrontLocked lock;if(front.mode!=2)erase_password(config);}
    *output=config;erase_password(config);
}
extern "C" void wx_network_login_view(WxLoginView* output){if(output){FrontLocked lock;*output=front;}}
extern "C" int wx_network_login_command(const WxLoginCommand* command){
    if(!command)return 0;FrontLocked lock;if(!front.mode)return 0;
    if(command->kind==WX_LOGIN_CANCEL){
        if(front.phase==WX_LOGIN_ACCOUNT||front.phase==WX_LOGIN_CANCELLING)return 0;
        erase_password(pending.config);pending={};pending.kind=WX_LOGIN_CANCEL;generation++;
        front.phase=WX_LOGIN_CANCELLING;front.cancellations++;front.revision++;wx_auth_cancel();wx_world_logout();return 1;
    }
    if(pending.kind)return 0;
    if(command->kind==WX_LOGIN_SUBMIT&&(front.phase==WX_LOGIN_ACCOUNT||front.phase==WX_LOGIN_ERROR)){
        const auto& c=command->config;
        if(!memchr(c.host,0,sizeof c.host)||!memchr(c.username,0,sizeof c.username)||!memchr(c.password,0,sizeof c.password)||!c.host[0]||!c.username[0]||strlen(c.username)>16||!c.password[0]||strlen(c.password)>16||!c.port||c.port>65535)return 0;
        pending=*command;generation++;front.phase=WX_LOGIN_AUTHENTICATING;front.error=0;front.attempts++;front.revision++;return 1;
    }
    if(command->kind==WX_LOGIN_SELECT&&front.phase==WX_LOGIN_REALMS){
        WxWorldSession check{};bool good=wx_auth_select(command->index,&check)!=0;
        volatile uint8_t* key=check.key;for(unsigned i=0;i<40;i++)key[i]=0;if(!good)return 0;
        pending=*command;generation++;front.selected=command->index;front.phase=WX_LOGIN_WORLD;front.revision++;return 1;
    }
    return 0;
}
extern "C" void wx_network_reconnect(void){
    FrontLocked lock;if(!front.mode){reconnect=1;return;}
    if(wx_auth_state()!=WX_AUTH_DONE){front.phase=WX_LOGIN_ACCOUNT;front.revision++;return;}
    erase_password(pending.config);pending={};pending.kind=WX_LOGIN_SELECT;pending.index=front.selected;
    generation++;front.phase=WX_LOGIN_WORLD;front.error=0;front.revision++;
}
extern "C" int wx_network_ready(void){return network_ready.load();}

// Development images carry fresh PC-generated entropy. Persist consumption before
// use; an unreadable/exhausted pool or failed durable write prevents authentication.
extern "C" int wx_random_bytes(uint8_t* output,size_t count){
    if(!count||count>64)return 0;
    FILE* pool=fopen("D:\\entropy.bin","rb");if(!pool)return 0;
    uint8_t id[16];if(fread(id,1,sizeof id,pool)!=sizeof id){fclose(pool);return 0;}
    if(!nxIsDriveMounted('E')&&!nxMountDrive('E',"\\Device\\Harddisk0\\Partition1\\")){fclose(pool);return 0;}
    CreateDirectoryA("E:\\WOWX",nullptr);
    char path[64]="E:\\WOWX\\";size_t at=strlen(path);
    for(int i=0;i<16;i++)snprintf(path+at+i*2,3,"%02x",id[i]);strcat(path,".rng");
    HANDLE file=CreateFileA(path,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){fclose(pool);return 0;}
    uint32_t used=0;DWORD actual=0;DWORD length=GetFileSize(file,nullptr);
    bool ok=length==0||(length==4&&ReadFile(file,&used,4,&actual,nullptr)&&actual==4);
    if(!ok||used>4096||count>4096-used){CloseHandle(file);fclose(pool);return 0;}
    uint32_t next=used+(uint32_t)count;
    SetFilePointer(file,0,nullptr,FILE_BEGIN);IO_STATUS_BLOCK io{};
    ok=WriteFile(file,&next,4,&actual,nullptr)&&actual==4&&NT_SUCCESS(NtFlushBuffersFile(file,&io));
    CloseHandle(file);
    ok=ok&&!fseek(pool,16+used,SEEK_SET)&&fread(output,1,count,pool)==count;
    fclose(pool);return ok;
}
static DWORD WINAPI worker(void*){
    if(nxNetInit(nullptr)!=0){FrontLocked lock;front.phase=WX_LOGIN_ERROR;front.error=1;front.revision++;return 0;}network_ready=1;
    while(1){
    if(front.mode){
        WxLoginCommand request{};unsigned token;
        {FrontLocked lock;request=pending;erase_password(pending.config);pending={};token=generation;}
        if(!request.kind){Sleep(50);continue;}
        if(request.kind==WX_LOGIN_CANCEL){wx_auth_forget();FrontLocked lock;if(token==generation){front.phase=WX_LOGIN_ACCOUNT;front.error=0;front.revision++;}continue;}
        if(request.kind==WX_LOGIN_SUBMIT){
            bool good=wx_auth_run(&request.config)!=0;erase_password(request.config);
            FrontLocked lock;if(token==generation){front.phase=good?WX_LOGIN_REALMS:WX_LOGIN_ERROR;front.error=good?0:2;front.revision++;}
        }else if(request.kind==WX_LOGIN_SELECT){
            WxWorldSession session{};bool good=wx_auth_select(request.index,&session)!=0;
            {FrontLocked lock;if(token!=generation)good=false;}
            if(good){wx_world_characters_open();good=wx_world_live(&session)!=0;}
            volatile uint8_t* key=session.key;for(unsigned i=0;i<40;i++)key[i]=0;
            FrontLocked lock;if(token==generation){front.phase=good?WX_LOGIN_REALMS:WX_LOGIN_ERROR;front.error=good?0:3;front.revision++;}
        }
        continue;
    }
    if(!reconnect.exchange(0)){Sleep(100);continue;}
    WxAuthConfig config{};bool loaded=load_config(config);
    if(loaded){
        if(wx_auth_run(&config)){
        WxWorldSession session{};if(wx_auth_session(&session))wx_world_live(&session);
        volatile uint8_t* key=session.key;for(size_t i=0;i<40;i++)key[i]=0;
        }
    }
    erase_password(config);
    }
    return 0;
}
extern "C" void wx_network_start(void){
    FILE* mode=fopen("D:\\LOGIN.BIN","rb");char tag[4]={};bool valid=false;
    if(mode){valid=fread(tag,1,4,mode)==4&&fgetc(mode)==EOF;fclose(mode);}
    if(valid&&!memcmp(tag,"NONE",4)){front.mode=0;front.phase=WX_LOGIN_DISABLED;}
    else if(valid&&!memcmp(tag,"WXLT",4))front.mode=2;
    HANDLE thread=CreateThread(nullptr,128*1024,worker,nullptr,0,nullptr);
    if(thread)CloseHandle(thread);else {front.phase=WX_LOGIN_ERROR;front.error=1;front.revision++;}
}

static DWORD WINAPI telemetry_worker(void*){if(nxNetInit(nullptr)==0)network_ready=1;return 0;}
extern "C" void wx_network_telemetry_start(void){
    HANDLE thread=CreateThread(nullptr,64*1024,telemetry_worker,nullptr,0,nullptr);if(thread)CloseHandle(thread);
}
