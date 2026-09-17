#include "wx_entities.h"
#include <zlib.h>
#include <string.h>
// zlib Z_SOLO on nxdk requires explicit allocation functions. This arena bounds
// its transient allocations and is owned exclusively by the networking thread.
typedef struct Arena {uint8_t bytes[128*1024];size_t used;} Arena;
static Arena arena;
static voidpf allocate(voidpf opaque,uInt count,uInt bytes){
    Arena* a=opaque;if(bytes&&count>(sizeof a->bytes-a->used)/bytes)return 0;
    size_t size=(size_t)count*bytes;if(size>sizeof a->bytes-a->used)return 0;
    void* out=a->bytes+a->used;a->used=(a->used+size+7)&~(size_t)7;return out;
}
static void release(voidpf opaque,voidpf address){(void)opaque;(void)address;}
int wx_update_inflate(const uint8_t* data,size_t size,uint8_t* output,size_t capacity,size_t* actual){
    if(actual)*actual=0;if(!data||size<5||size>65533||!output||!actual)return 0;
    uint32_t expected=(uint32_t)data[0]|(uint32_t)data[1]<<8|(uint32_t)data[2]<<16|(uint32_t)data[3]<<24;
    if(expected<5||expected>WX_UPDATE_LIMIT||expected>capacity)return 0;
    arena.used=0;z_stream stream={0};stream.zalloc=allocate;stream.zfree=release;stream.opaque=&arena;
    stream.next_in=(Bytef*)(data+4);stream.avail_in=(uInt)(size-4);stream.next_out=output;stream.avail_out=expected;
    if(inflateInit(&stream)!=Z_OK)return 0;
    int status=inflate(&stream,Z_FINISH);int ok=status==Z_STREAM_END&&stream.total_out==expected&&stream.avail_in==0;
    inflateEnd(&stream);if(ok)*actual=expected;return ok;
}
