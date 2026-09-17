#ifndef WX_LOOKS_H
#define WX_LOOKS_H
#include <stdint.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WX_LOOK_ROWS 512
#define WX_LOOK_BASE 10
#define WX_LOOK_MIPS 11
typedef struct WxLookHeader {
    char magic[4];uint32_t version,file_size,count,row_size,race,sex,data_offset;
} WxLookHeader;
typedef struct WxLookRow {
    uint8_t kind,variation,color,reserved;
    uint32_t offset[3],region[3],geoset[3];
} WxLookRow;
typedef struct WxLooks {FILE* file;WxLookHeader header;WxLookRow rows[WX_LOOK_ROWS];} WxLooks;
int wx_looks_open(WxLooks* looks,const char* path);
int wx_looks_header_valid(const WxLookHeader* header);
int wx_looks_row_valid(const WxLooks* looks,unsigned row);
void wx_looks_close(WxLooks* looks);
const WxLookRow* wx_looks_find(const WxLooks* looks,unsigned kind,unsigned variation,unsigned color);
int wx_looks_valid(const WxLooks* looks,const uint32_t look[7]);
/* Fields 2..6 are skin, face, style, color and facial feature. IDs may be sparse. */
unsigned wx_looks_choices(const WxLooks* looks,const uint32_t look[7],unsigned field,uint8_t ids[256]);
int wx_looks_step(const WxLooks* looks,uint32_t look[7],unsigned field,int direction);
/* Bounded catalog sampling. Changes only appearance and seed on success. */
int wx_looks_randomize(const WxLooks* looks,uint32_t look[7],uint32_t* seed);
unsigned wx_looks_bytes(unsigned region);
int wx_looks_read(WxLooks* looks,const WxLookRow* row,unsigned layer,void* destination,unsigned capacity);
#ifdef __cplusplus
}
#endif
#endif
