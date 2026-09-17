#ifndef WX_DEATH_H
#define WX_DEATH_H
#include "wx_world.h"
#include "wx_game.h"
typedef struct WxDeath {
    unsigned state,world_revision,delay_revision,ready_at,remaining_ms;
    unsigned query_at,query_attempts,known,map,in_range,ready;
    unsigned queries,releases,reclaims,healers,resurrections;
    uint64_t corpse;float position[3],distance;
} WxDeath;
// Return one when a corpse query should be queued. No network or timing API here.
int wx_death_update(WxDeath* death,const WxWorldView* world,const WxEntity* self,
    const WxEntity* entities,unsigned count,const float* position,const WxGame* game,unsigned now);
unsigned wx_death_action(const WxDeath* death);
#endif
