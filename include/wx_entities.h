#ifndef WX_ENTITIES_H
#define WX_ENTITIES_H
#include <stdint.h>
#include <stddef.h>
#define WX_MAX_ENTITIES 512
#define WX_VISIBLE_ENTITIES 128
#define WX_UNIT_FIELDS 192
#define WX_UPDATE_LIMIT (1024u*1024u)
#define WX_SPLINE_POINTS 257
#ifdef __cplusplus
extern "C" {
#endif
typedef struct WxEntity {
    uint64_t guid;
    uint32_t fields[WX_UNIT_FIELDS];
    uint32_t quests[60],xp,next_xp,money;
    uint32_t inventory[78];
    // Vanilla player-only fields outside the compact common field window.
    // Visible items contain item entries, not ItemDisplayInfo IDs. Creation
    // defaults omitted fields to zero; deltas preserve untouched slots.
    uint32_t player_bytes,player_bytes2,visible_items[19];
    uint32_t explored[64]; // Vanilla PLAYER_EXPLORED_ZONES_1 = 0x457, private to self.
    float x,y,z,orientation;
    uint8_t type,positioned,animation;
} WxEntity;
typedef struct WxSpline {
    float points[WX_SPLINE_POINTS][3],distance[WX_SPLINE_POINTS];
    float facing[3],angle;
    uint64_t target;
    uint32_t started,duration,flags,count;
    uint8_t facing_type;
} WxSpline;
typedef struct WxEntities {
    WxEntity items[WX_MAX_ENTITIES];
    WxSpline paths[WX_MAX_ENTITIES];
    unsigned count,packets,unknown_updates;
} WxEntities;
// Caller supplies a separate scratch store. Failed packets do not change live state.
int wx_entities_apply(WxEntities* live,WxEntities* scratch,const uint8_t* data,size_t size,uint32_t now);
void wx_entities_destroy(WxEntities* entities,uint64_t guid);
int wx_entities_monster_move(WxEntities* entities,const uint8_t* data,size_t size,uint32_t now);
int wx_entities_relocate(WxEntities* entities,const uint8_t* data,size_t size,uint64_t* moved);
int wx_relocation_decode(const uint8_t* data,size_t size,WxEntity* pose);
void wx_entities_sample(const WxEntities* entities,unsigned index,uint32_t now,WxEntity* output);
// Bounded snapshot: self, selected target, owned corpse, then nearest objects.
// Self/corpse remain available even before a movement position arrives.
unsigned wx_entities_nearby(const WxEntities* entities,uint64_t self,uint64_t target,
    const float* position,uint32_t now,WxEntity* output,unsigned capacity);
int wx_update_inflate(const uint8_t* data,size_t size,uint8_t* output,size_t capacity,size_t* actual);
#ifdef __cplusplus
}
#endif
#endif
