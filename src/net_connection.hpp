#pragma once
#include <cstdint>
#ifdef WX_XBOX
#include <nxdk/net.h>
#include <lwip/sockets.h>
#include <windows.h>
// lwIP's POSIX convenience macros collide with C++ library/member names.
#undef bind
#undef read
#undef write
#define WX_CLOSE lwip_close
using Socket=int;
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#define WX_CLOSE closesocket
using Socket=SOCKET;
#endif
struct Connection {
    Socket handle;
    uint32_t started=GetTickCount();
    uint32_t timeout_ms=15000;
    bool (*cancelled)()=nullptr;
    Connection():handle(::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)){}
    ~Connection(){if(handle!=(Socket)-1)WX_CLOSE(handle);}
    bool ready(bool writing){
        for(;;){
            if(cancelled&&cancelled())return false;
            uint32_t elapsed=GetTickCount()-started;if(elapsed>=timeout_ms)return false;
            uint32_t remaining=timeout_ms-elapsed;if(cancelled&&remaining>100)remaining=100;
            timeval limit{(long)(remaining/1000),(long)(remaining%1000)*1000};
            fd_set descriptors;FD_ZERO(&descriptors);FD_SET(handle,&descriptors);
            int selected=select((int)handle+1,writing?nullptr:&descriptors,writing?&descriptors:nullptr,nullptr,&limit);
            if(selected<0)return false;if(selected>0)return !(cancelled&&cancelled());
        }
    }
    bool connectTo(const char* host,uint16_t port){
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(port);address.sin_addr.s_addr=inet_addr(host);
        if(handle==(Socket)-1||address.sin_addr.s_addr==INADDR_NONE)return false;
        unsigned long nonblocking=1;if(ioctlsocket(handle,FIONBIO,&nonblocking))return false;
        int connected=connect(handle,(sockaddr*)&address,sizeof address);
        if(connected&&!ready(true))return false;
        int error=0;
#ifdef WX_XBOX
        socklen_t error_size=sizeof error;
#else
        int error_size=sizeof error;
#endif
        if(getsockopt(handle,SOL_SOCKET,SO_ERROR,(char*)&error,&error_size)||error)return false;
        nonblocking=0;return ioctlsocket(handle,FIONBIO,&nonblocking)==0;
    }
    bool receive(void* output,size_t bytes){
        auto* p=(char*)output;
        while(bytes){if(!ready(false))return false;int n=recv(handle,p,(int)bytes,0);if(n<=0)return false;p+=n;bytes-=n;}return true;
    }
    bool sendExact(const void* input,size_t bytes){
        auto* p=(const char*)input;
        while(bytes){if(!ready(true))return false;int n=::send(handle,p,(int)bytes,0);if(n<=0)return false;p+=n;bytes-=n;}return true;
    }
};
