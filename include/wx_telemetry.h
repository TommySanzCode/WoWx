#ifndef WX_TELEMETRY_H
#define WX_TELEMETRY_H
#include "wx_runtime.h"
enum WxProfileStage {WX_PROFILE_UPDATE,WX_PROFILE_MOVE,WX_PROFILE_STREAM,WX_PROFILE_CAMERA,
    WX_PROFILE_ACTORS,WX_PROFILE_DRAW,WX_PROFILE_UI,WX_PROFILE_PRESENT,WX_PROFILE_PACE,WX_PROFILE_WORK,WX_PROFILE_COUNT};
typedef struct WxFrameStats {
    unsigned time_ms,frame_ms,free_bytes,cache_bytes,draws,triangles,loads,failures;
    float x,y,z,yaw,pitch;
    WxPad pad;
    unsigned replay_frame;int replay_active,menu,saved;float deadzone;
    unsigned npc_submitted,npc_missing,entity_count;
    unsigned quest_screen,quest_id,completed_quest,kills,damage_dealt,damage_received,player_xp,active_quests;
    unsigned scenario_stage,scenario_frame,looted_items,looted_money;
    unsigned casts_accepted,casts_rejected,spell_hits,last_spell,inventory_count,equipped_shield;
    unsigned region_loaded,region_transitions,tile_x,tile_y,region_failures;
    unsigned player_health,player_flags,position_revision,inventory_money,vendor_count,bundles_bought,items_sold;
    unsigned scenario_kind;
    unsigned avatar_matched,avatar_ready,avatar_drawn,avatar_missing,avatar_revision,avatar_bytes,avatar_equipment,camera_mm,avatar_clip,avatar_pose;
    unsigned lobby_phase,lobby_count,character_screen,character_selected,character_row,character_key,character_result,character_result_revision,player_level;
    unsigned journal_open,journal_detail,journal_story,journal_id,journal_ready,journal_progress;
    unsigned book_open,book_screen,book_count,book_loaded,book_failures,book_spell,book_slot,book_server_slot,book_binding,action_revision;
    unsigned avatar_select_attempts,avatar_profile_changes,avatar_select_failures;
    unsigned actionbar_layer,actionbar_slots[8],actionbar_bindings[8];
    unsigned utility_open,utility_pending,utility_selection,utility_action,utility_revision,inventory_open;
    unsigned profile[WX_PROFILE_COUNT];
    unsigned text[8]; // scene drain, text submit/drain, swap, glyphs, mode, bytes, failures
    unsigned map_ui[14];
    unsigned death[18];
    unsigned soak[4];
    unsigned ui[4]; // loaded, allocation bytes, submitted quads, failures
    unsigned login[11]; // public UI/replay state only; never credentials/session keys
    unsigned backdrop[8]; // ready, CPU+GPU bytes, draws, triangles, updates, failures, loads, time
    unsigned preview[18]; // Above plus class, outfit, load phase/read bytes, index reads.
    unsigned effects[8]; // emitters, lights, live, peak, births, drops, clock skips, quads
    unsigned customization[6]; // Rendered look/facial, composition revision/hash, UI row/randomizations.
    unsigned hud[17]; // Unit/resource snapshot actually submitted by the HUD.
    unsigned portrait[10]; // Catalog, keys, draws, mask spans, CPU submission ms, fixture.
    unsigned icons[10]; // ready/index/images/bytes/loads/pending/failures/drawn/UI batches/upload ms.
    unsigned cooldowns[26]; // catalog/state metrics, eight remaining timers, held mask, fixture step, GCD/modifier metrics.
    unsigned fixture_map; // Offline WXPF0013 map; never changes authoritative world state.
} WxFrameStats;
void wx_telemetry_send(const WxFrameStats* stats);
typedef struct WxDrawStats {
    unsigned submit_ms,flush_wait_ms,flush_reset_ms,drain_ms,flushes;
    unsigned batches,indices,largest_batch,legacy_batches;
    unsigned world_ms,actors_ms,player_ms,liquid_ms;
} WxDrawStats;
/* scope 1: immediate drain in offline fixtures; 2: live drain after CPU UI work. */
void wx_telemetry_draw(unsigned now,unsigned frame,unsigned fixture,unsigned scope,const WxDrawStats* stats);
void wx_telemetry_environment(unsigned now,unsigned frame,unsigned fixture,unsigned stage,unsigned cycles,unsigned expected,unsigned liquid,const WxEnvironmentSample* sample,const WxScene* scene,unsigned sample_ms,unsigned errors,const float* camera);
void wx_telemetry_material(unsigned now,unsigned frame,const unsigned counts[7],unsigned flags,unsigned fog,unsigned motion_draws,unsigned motion_hash,unsigned liquid_draws,unsigned sequence_draws,unsigned sequence_frames);
struct WxCastView;struct WxActionFeedback;
void wx_telemetry_actions(unsigned now,unsigned frame,unsigned fixture,unsigned step,const struct WxCastView* cast,const struct WxActionFeedback* actions,unsigned catalog_version);
struct WxRegion;
struct WxAvatar;
struct WxAvatarSelection;
void wx_telemetry_profile(unsigned now,unsigned frame,unsigned stage,unsigned cycles,unsigned completed,unsigned expected_failures,unsigned errors,unsigned work_ms,unsigned drawn,const struct WxAvatarSelection* selection,const struct WxAvatar* avatar);
void wx_telemetry_actor_stream(unsigned now,unsigned frame,unsigned fixture,unsigned clip,const WxScene* actors,const struct WxAvatar* avatar,unsigned ready,unsigned fallback,unsigned drawn,unsigned actor_ms,unsigned gaps,unsigned switches);
void wx_telemetry_stream(unsigned now,unsigned frame,unsigned fixture,unsigned stage,unsigned cycles,const WxScene* scene,const struct WxRegion* region,unsigned region_ms,unsigned stream_ms,unsigned waits,unsigned errors,unsigned fog);
#endif
