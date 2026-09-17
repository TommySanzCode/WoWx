#include <assert.h>
#include <stdio.h>
#include "../src/index_batch.h"

/* Decode the emitted NV2A method stream independently, including odd tails,
   triangle boundaries, full-range 16-bit indices and output-buffer sentinels. */
static void check(unsigned total){
    uint16_t indices[10002];for(unsigned i=0;i<10002;i++)indices[i]=(uint16_t)(i*7919+65535);
    unsigned at=0,batches=0;
    while(at<total){
        unsigned n=total-at;if(n>WX_INDEX_BATCH_LIMIT)n=WX_INDEX_BATCH_LIMIT;
        uint32_t words[WX_INDEX_BATCH_WORDS+2];for(unsigned i=0;i<WX_INDEX_BATCH_WORDS+2;i++)words[i]=0xdeadbeef;
        uint32_t* end=wx_index_batch(words+1,indices+at,n);
        assert(end<=words+1+WX_INDEX_BATCH_WORDS&&*end==0xdeadbeef&&words[0]==0xdeadbeef);
        unsigned consumed=0,in_triangles=0;
        for(uint32_t* p=words+1;p<end;){
            uint32_t h=*p++;unsigned method=h&0x1fff,count=(h>>18)&2047,repeat=!!(h&0x40000000u);
            assert(count&&p+count<=end);
            if(method==0x17fc){assert(count==1&&!repeat);if(*p==5){assert(!in_triangles&&!consumed);in_triangles=1;}else{assert(*p==0&&in_triangles&&consumed==n);in_triangles=0;}}
            else if(method==0x1800){assert(repeat&&in_triangles);for(unsigned k=0;k<count;k++){
                assert(consumed+2<=n);assert((p[k]&65535)==indices[at+consumed++]);assert((p[k]>>16)==indices[at+consumed++]);}}
            else {assert(method==0x1808&&!repeat&&count==1&&in_triangles&&consumed==n-1);assert(*p==indices[at+consumed++]);}
            p+=count;
        }
        assert(!in_triangles&&consumed==n);at+=n;batches++;
    }
    assert(batches==(total+WX_INDEX_BATCH_LIMIT-1)/WX_INDEX_BATCH_LIMIT);
}
int main(void){
    for(unsigned n=3;n<=10002;n+=3)check(n);
    uint32_t out=0xabcdef;uint16_t in=42;
    unsigned invalid[]={0,1,2,1537,1539,UINT32_MAX};
    for(unsigned i=0;i<sizeof invalid/sizeof *invalid;i++)assert(wx_index_batch(&out,&in,invalid[i])==&out&&out==0xabcdef);
    puts("NV2A batches: exact topology, odd tails, method limits and bounds PASS");return 0;
}
