#ifndef WX_ENVIRONMENT_H
#define WX_ENVIRONMENT_H
#include <stdint.h>
#include <math.h>
#include <string.h>
#define WX_ENV_GROUP 1u
#define WX_ENV_LIQUID 2u
#define WX_ENV_MAX_RECORDS 2048u
#define WX_ENV_MAX_BYTES (2u*1024u*1024u)
/* WXP v9 world-only footer. Offsets inside records are blob-relative. The
   footer follows the blob at EOF; legacy packs have unknown environment data. */
typedef struct WxEnvironmentFooter {char magic[4];uint32_t count,bytes,offset;} WxEnvironmentFooter;
typedef struct WxEnvironmentRecord {
    uint32_t kind,group,flags,liquid_type;
    float inverse[12],lo[3],hi[3];
    uint32_t x_tiles,y_tiles,heights_offset,flags_offset;
    float corner[3],tile_size;
    uint32_t reserved[2];
} WxEnvironmentRecord;
typedef struct WxEnvironmentCache {
    WxEnvironmentFooter footer;
    void* data;
    unsigned phase,offset,checked,record,heights,next,bytes;
} WxEnvironmentCache;
typedef struct WxEnvironmentSample {
    unsigned environment,known,records,liquid_type,group,source;
    float depth;
} WxEnvironmentSample;
static inline int wx_environment_footer_valid(const WxEnvironmentFooter* f,unsigned file_bytes,unsigned index_end){
    return !memcmp(f->magic,"WXE1",4)&&f->count<=WX_ENV_MAX_RECORDS&&f->bytes<=WX_ENV_MAX_BYTES&&
        f->bytes>=f->count*sizeof(WxEnvironmentRecord)&&f->offset>=index_end&&
        f->offset<=file_bytes&&file_bytes-f->offset>=sizeof *f&&
        f->bytes==file_bytes-f->offset-sizeof *f&&(!f->count?!f->bytes:1);
}
static inline int wx_environment_record_valid(const WxEnvironmentRecord* r,unsigned bytes,unsigned next){
    if(r->kind!=WX_ENV_GROUP&&r->kind!=WX_ENV_LIQUID)return 0;
    for(unsigned k=0;k<12;k++)if(!isfinite(r->inverse[k])||fabsf(r->inverse[k])>100000)return 0;
    // World placements use rigid orthogonal transforms (terrain liquid grids
    // also exchange axes). Reject collapsed or scaled matrices, which would
    // break bounds, depth and overlap ordering.
    for(unsigned a=0;a<3;a++)for(unsigned b=a;b<3;b++){
        float dot=0;for(unsigned k=0;k<3;k++)dot+=r->inverse[a*4+k]*r->inverse[b*4+k];
        if(fabsf(dot-(a==b?1.f:0.f))>.002f)return 0;
    }
    for(unsigned k=0;k<3;k++)if(!isfinite(r->lo[k])||!isfinite(r->hi[k])||fabsf(r->lo[k])>100000||fabsf(r->hi[k])>100000||r->hi[k]<r->lo[k]||
        !isfinite(r->corner[k])||fabsf(r->corner[k])>100000)return 0;
    if(r->reserved[0]||r->reserved[1])return 0;
    if(r->kind==WX_ENV_GROUP)return !r->x_tiles&&!r->y_tiles&&!r->heights_offset&&!r->flags_offset&&!r->liquid_type&&r->tile_size==0;
    if(!r->x_tiles||!r->y_tiles||r->x_tiles>256||r->y_tiles>256||!isfinite(r->tile_size)||fabsf(r->tile_size-4.1666625f)>.00001f)return 0;
    unsigned heights=(r->x_tiles+1)*(r->y_tiles+1)*4,tiles=r->x_tiles*r->y_tiles;
    return next<=bytes&&heights<=bytes-next&&r->heights_offset==next&&r->flags_offset==next+heights&&
        tiles<=bytes-r->flags_offset&&((tiles+3)&~3u)<=bytes-r->flags_offset;
}
#ifdef __cplusplus
static_assert(sizeof(WxEnvironmentFooter)==16&&sizeof(WxEnvironmentRecord)==128);
#endif
#endif
