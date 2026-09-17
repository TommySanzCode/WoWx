#ifndef WX_INDEX_BATCH_H
#define WX_INDEX_BATCH_H
#include <stdint.h>
#include <string.h>

/* NV2A non-incrementing ARRAY_ELEMENT16 packets. Each batch ends on a
   triangle AND a packed-index pair; only the final triangle can be odd.
   768 data words stay below the 11-bit method-count limit. No new allocation. */
#define WX_INDEX_BATCH_LIMIT 1536u
#define WX_INDEX_BATCH_WORDS (WX_INDEX_BATCH_LIMIT/2+7)
enum { WX_NV_BEGIN_END=0x17fc, WX_NV_INDEX16=0x1800, WX_NV_INDEX32=0x1808 };
/* The native caller supplies pb_push so pbkit debug validation sees headers. */
#ifndef WX_INDEX_METHOD
#define WX_INDEX_METHOD(p,method,count) (*(p)=((count)<<18)|(method))
#endif
static inline uint32_t* wx_index_batch(uint32_t* out,const uint16_t* indices,unsigned count){
    if(!count||count>WX_INDEX_BATCH_LIMIT||count%3)return out;
    WX_INDEX_METHOD(out++,WX_NV_BEGIN_END,1);*out++=5; // TRIANGLES
    unsigned pairs=count/2;
    WX_INDEX_METHOD(out++,0x40000000u|WX_NV_INDEX16,pairs);
    // Both the original Xbox and on-disc 16-bit indices are little endian.
    memcpy(out,indices,pairs*4);out+=pairs;
    if(count&1){WX_INDEX_METHOD(out++,WX_NV_INDEX32,1);*out++=indices[count-1];}
    WX_INDEX_METHOD(out++,WX_NV_BEGIN_END,1);*out++=0;
    return out;
}
#endif
