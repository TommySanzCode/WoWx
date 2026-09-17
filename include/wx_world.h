#ifndef WX_WORLD_H
#define WX_WORLD_H
#include "wx_auth.h"
#include "wx_entities.h"
#include "wx_lighting.h"
#include "wx_weather.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {WX_MOVE_FORWARD=1,WX_MOVE_BACK=2,WX_MOVE_LEFT=4,WX_MOVE_RIGHT=8,WX_MOVE_JUMP=0x2000};
void wx_world_clock(WxWorldClock* output);
void wx_world_weather(WxWeather* output);
typedef struct WxMovement {
    uint32_t flags,time_ms,fall_ms;
    float x,y,z,orientation,jump_speed,jump_cos,jump_sin,jump_xy;
} WxMovement;
typedef struct WxWorldView {
    uint64_t guid;
    uint32_t map,revision,received,sent,position_revision;
    float x,y,z,orientation;
    int active;
    char name[49];
    uint8_t race,character_class,gender;
    uint8_t skin,face,hair_style,hair_color,facial_hair;
    // Enumeration fallback until the first player create. Live item entries
    // resolve through bounded query metadata; unresolved slots have display 0.
    uint32_t equipment_display[20];
    uint8_t equipment_type[20];
    uint32_t equipment_entry[19],equipment_ready_mask,appearance_revision;
    uint32_t appearance_flags,display_id,native_display_id;
} WxWorldView;
struct WxGame;
void wx_player_appearance(WxWorldView* view,const WxEntity* entity,const struct WxGame* game);
// Development evidence: last rejected world packet only; never authentication.
typedef struct WxWorldFault {
    uint32_t revision,reason,opcode,size,captured;
    uint8_t data[768];
} WxWorldFault;
void wx_world_fault(WxWorldFault* output);
int wx_movement_valid(const WxMovement* movement);
unsigned wx_movement_encode(const WxMovement* movement,uint8_t* output,unsigned capacity);
uint16_t wx_movement_opcode(uint32_t previous_flags,uint32_t flags);
int wx_world_live(const WxWorldSession* session);
void wx_world_view(WxWorldView* output);
int wx_world_submit(const WxMovement* movement,uint32_t position_revision);
void wx_world_logout(void);
void wx_network_reconnect(void);
unsigned wx_world_entities(WxEntity* output,unsigned capacity);
unsigned wx_world_entity_count(void);
#ifdef __cplusplus
}
#endif
#endif
