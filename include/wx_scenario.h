#ifndef WX_SCENARIO_H
#define WX_SCENARIO_H
#include "wx_world.h"
#include "wx_game.h"
#include "wx_input.h"
#include "wx_lobby.h"
#include "wx_spellbook.h"
#include "wx_trail.h"
// Development-only, state-driven controller replay. It never sends packets or
// changes position directly; movement and interactions use normal client input.
typedef struct WxScenario {unsigned enabled,stage,frame,stage_frame,previous,initial_xp,initial_level,initial_kills,loot_seen;uint64_t target;
    unsigned trade_item,trade_bundle,trade_price,initial_count,initial_money,expected_money;
    unsigned probe_frame,detour_until,detours;float probe_position[3];uint64_t created_character;
    uint64_t blocked_targets[8];unsigned blocked_cursor,initial_objectives,return_count;
    float return_points[128][3];WxTrail trail;char test_name[13];
    unsigned now_ms,soak_started,soak_begin_ms,soak_cycles,soak_limit_ms,soak_cycle_frame;} WxScenario;
void wx_scenario_init(WxScenario* scenario,const char* path);
void wx_scenario_input(WxScenario* scenario,const WxWorldView* world,const WxEntity* entities,unsigned count,
    const float* position,float yaw,float pitch,uint64_t selected,const WxGame* game,const WxCharacterLobby* lobby,const WxCharacterUi* ui,WxPad* pad);
const char* wx_scenario_status(const WxScenario* scenario);
const char* wx_death_scenario_status(const WxScenario* scenario);
void wx_death_scenario_input(WxScenario* scenario,const WxWorldView* world,const WxEntity* entities,unsigned count,
    const float* position,float yaw,uint64_t selected,const WxGame* game,const WxCharacterLobby* lobby,const WxCharacterUi* ui,WxRawPad* raw);
void wx_soak_scenario_input(WxScenario* scenario,const WxWorldView* world,const WxEntity* entities,unsigned count,
    const float* position,float yaw,uint64_t selected,const WxGame* game,const WxCharacterLobby* lobby,const WxCharacterUi* ui,WxRawPad* raw);
void wx_spell_scenario_input(WxScenario* s,const WxWorldView* world,const WxEntity* self,const float* position,const WxGame* game,const WxSpellBook* book,const WxSpellUi* ui,const uint8_t bindings[24],unsigned form,WxPad* pad);
const char* wx_journey_status(const WxScenario* scenario);
void wx_journey_load_route(WxScenario* scenario,const char* path);
void wx_journey_input(WxScenario* scenario,const WxWorldView* world,const WxEntity* entities,unsigned count,
    const float* position,float yaw,uint64_t selected,const WxGame* game,const WxCharacterLobby* lobby,const WxCharacterUi* ui,WxRawPad* raw);
#endif
