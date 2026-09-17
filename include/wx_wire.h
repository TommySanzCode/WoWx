#ifndef WX_WIRE_H
#define WX_WIRE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
typedef struct WxReader {const uint8_t* data;size_t size,at;int ok;} WxReader;
static inline uint32_t wx_read(WxReader* r,unsigned bytes){
    if(bytes>4||!r->ok||r->at>r->size||bytes>r->size-r->at){r->ok=0;return 0;}
    uint32_t out=0;for(unsigned i=0;i<bytes;i++)out|=(uint32_t)r->data[r->at++]<<(i*8);return out;
}
static inline uint64_t wx_guid(WxReader* r,int packed){
    uint64_t value=0;unsigned mask=packed?wx_read(r,1):255;
    for(unsigned i=0;i<8;i++)if(mask&(1u<<i))value|=(uint64_t)wx_read(r,1)<<(i*8);return value;
}
static inline float wx_real(WxReader* r){uint32_t bits=wx_read(r,4);float value;memcpy(&value,&bits,4);if(!isfinite(value))r->ok=0;return value;}
static inline void wx_string(WxReader* r,char* output,unsigned capacity){
    if(!r->ok||!capacity||r->at>=r->size){r->ok=0;return;}
    const uint8_t* end=memchr(r->data+r->at,0,r->size-r->at);
    if(!end){r->ok=0;return;}size_t length=(size_t)(end-r->data-r->at);
    if(length>=capacity){r->ok=0;return;}
    if(output){memcpy(output,r->data+r->at,length);output[length]=0;}r->at+=length+1;
}
#endif
