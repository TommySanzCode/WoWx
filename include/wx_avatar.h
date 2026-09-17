#ifndef WX_AVATAR_H
#define WX_AVATAR_H
#include "wx_runtime.h"
#include "wx_world.h"
#include "wx_looks.h"

// Private, prepared assets; no MPQ paths or game data are compiled into the XBE.
#define WX_AVATAR_ITEMS 128
#define WX_AVATAR_PARTS 32
#define WX_AVATAR_ATLAS_BYTES 65536
#define WX_AVATAR_MIP_BYTES 87380
typedef struct WxAvatarHeader {
    char magic[4];
    uint32_t version,file_size,item_count,item_size;
    uint32_t look[7]; // race, sex, skin, face, hair style, hair color, facial hair
    uint32_t scalp,facial[3],body_texture,base_offset,items_offset,pack_size;
} WxAvatarHeader;
typedef struct WxAvatarItem {
    uint32_t display,slot,geoset[3],hide[5],component[2],overlay[8],cape;
} WxAvatarItem;
/* WXA v2 places these bindings between the original header and item table. */
typedef struct WxAvatarBindings {uint32_t hair_texture,extra_texture;} WxAvatarBindings;
#define WX_AVATAR_COMPOSE_PIXELS 8192u
typedef struct WxAvatarLayer {uint32_t offset,bytes,region,source;} WxAvatarLayer;
typedef struct WxAvatarCompose {
    uint32_t *body,*cape,*hair,*extra;
    WxAvatarLayer layers[80];
    uint32_t look[7],equipment[19],flags,families[WX_AVATAR_PARTS],ids[WX_AVATAR_PARTS];
    unsigned phase,count,missing,layer_count,layer,offset,mip_size,mip_base,mip_row,hash,hash_at,copy_at;
    unsigned read_bytes,read_ops,work_pixels,cancelled,commits,failures;
} WxAvatarCompose;
typedef struct WxAvatarOpen {
    WxPackOpen pack;
    FILE* looks_file;
    char metadata[256];
    unsigned phase,offset,checked,binding,hair_binding,extra_binding;
    unsigned read_bytes,read_ops,scan_bytes;
} WxAvatarOpen;
typedef struct WxAvatar {
    WxScene scene;
    FILE* file;
    WxAvatarHeader header;
    WxAvatarItem* items;
    uint8_t *canvas,*scratch;
    uint32_t *body,*cape;
    WxLooks looks;WxAvatarBindings bindings;uint32_t *hair,*extra;
    uint32_t families[WX_AVATAR_PARTS],ids[WX_AVATAR_PARTS];
    unsigned count,ready,missing,revision,bytes,failures,drawn,matched,equipment_mask,pose_hash,complete_clip;
    uint32_t equipment[19],flags;
    int appearance_ready;
    uint32_t atlas_hash; // Composed CPU RGBA, sampled only when appearance/items change.
    uint32_t published_look[7];
    WxAvatarCompose compose;
    WxAvatarOpen opening;
    uint32_t family_bits[136]; // 4,352 legal body/item families; idle membership.
    unsigned profile_ready;
    char error[96];
} WxAvatar;
typedef struct WxAvatarSelection {
    uint32_t look[7],world_revision;
    unsigned attempted,attempts,changes,failures,pending,cancelled;
} WxAvatarSelection;
int wx_avatar_open(WxAvatar* avatar,const char* pack,const char* metadata);
int wx_avatar_open_begin(WxAvatar* avatar,const char* pack,const char* metadata);
int wx_avatar_open_pump(WxAvatar* avatar);
unsigned wx_avatar_pending_bytes(const WxAvatar* avatar);
// Call only after previous GPU submissions finish. One profile stays resident.
int wx_avatar_select(WxAvatar* avatar,WxAvatarSelection* selection,const WxWorldView* world,const char* directory);
int wx_avatar_path(char* output,unsigned capacity,const char* directory,const uint32_t look[7],int metadata);
void wx_avatar_close(WxAvatar* avatar);
void wx_avatar_update(WxAvatar* avatar,const WxWorldView* world,unsigned clip,unsigned time_ms);
uint32_t* wx_avatar_texture(WxAvatar* avatar,const WxEntry* entry,uint32_t* original);
// Pure atlas helpers are also exercised by malformed-asset and compositing tests.
int wx_avatar_blend(uint8_t* canvas,const uint8_t* pixels,unsigned region);
void wx_avatar_mips(uint8_t* canvas,uint32_t* destination);
#endif
