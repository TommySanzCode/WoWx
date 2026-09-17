#ifndef WX_OUTFIT_H
#define WX_OUTFIT_H
#include <stdint.h>
#define WX_OUTFIT_LIMIT 96u
typedef struct WxOutfit {uint32_t key,display[19];uint8_t type[20];} WxOutfit;
typedef struct WxOutfitHeader {char magic[4];uint32_t version,count,entry_size;} WxOutfitHeader;
typedef struct WxOutfits {WxOutfit rows[WX_OUTFIT_LIMIT];unsigned count,ready;} WxOutfits;
#ifdef __cplusplus
extern "C" {
#endif
int wx_outfit_slot(unsigned type);
int wx_outfit_valid(const WxOutfit* outfit);
int wx_outfits_open(WxOutfits* outfits,const char* path);
const WxOutfit* wx_outfit_find(const WxOutfits* outfits,unsigned race,unsigned character_class,unsigned gender);
#ifdef __cplusplus
}
#endif
#endif
