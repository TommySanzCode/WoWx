#ifndef WX_RUNTIME_H
#define WX_RUNTIME_H
#include "wx_pack.h"
#include "wx_environment.h"
#include "wx_material_motion.h"
#include <stdio.h>
typedef struct WxResident {
    int entry;
    int texture_slot;
    int pose_slot;
    WxVertex* vertices;
    uint16_t* indices;
    uint32_t* texture;
    uint32_t bytes;
    WxAnimation animation;
    WxVertex* bind_vertices;
    WxSkinVertex* skin;
    float* poses;
    unsigned animated_time,animated;
    WxMaterialMotion* material_motion;
    uint32_t* vertex_colors;
} WxResident;
typedef struct WxTexture {
    uint32_t* pixels;
    unsigned offset,bytes,references,width,height,levels,encoding,source;
} WxTexture;
typedef struct WxPoseBuffer { float* data;unsigned source,offset,bytes,references; } WxPoseBuffer;
#define WX_REGION_FILES 4
typedef struct WxPackSource { FILE* file; unsigned first,count,bytes,version; WxEnvironmentCache environment; } WxPackSource;
typedef struct WxPackPending {
    FILE* file;WxPackHeader header;WxEntry* entries;
    unsigned phase,offset,checked,read_bytes,read_ops,checked_frame,deferred;
    WxEntry* joined;
    const struct WxScene* join_scene;
    unsigned join_revision,join_count,join_total,join_progress,join_restarts;
} WxPackPending;
// Isolated pack opening. Entries/animation headers are unpublished until all
// ranges are validated; the completed table transfers to an empty scene.
typedef struct WxPackOpen {
    FILE* file;WxPackHeader header;WxEntry* entries;WxAnimation* animations;
    char path[256];
    unsigned phase,lane,offset,checked,animation_first,animation_count,animation_at,contiguous;
    unsigned read_bytes,read_ops,scan_bytes,animation_reads;
} WxPackOpen;

#define WX_STREAM_READ_BYTES 65536u
#define WX_STREAM_SCAN_BYTES 65536u
#define WX_STREAM_READ_OPS 8u
#define WX_STREAM_CHUNK_BYTES 32768u
enum {WX_STREAM_INDEX,WX_STREAM_WORLD,WX_STREAM_ACTORS,WX_STREAM_AVATAR,WX_STREAM_LANES};
#define WX_STREAM_FRAME_BYTES 65536u
#define WX_STREAM_FRAME_OPS 16u
#define WX_STREAM_FRAME_SCANS 65536u
#define WX_STREAM_FRAME_ALLOCATIONS 3u
#define WX_STREAM_FRAME_COPY_BYTES (256u*1024u)
#define WX_STREAM_FRAME_SELECT_ENTRIES 2048u
unsigned wx_stream_index_copy_bytes(void);
unsigned wx_stream_selection_entries(void);
unsigned wx_pack_pending_bytes(const WxPackPending* pending);
typedef struct WxStreamUse {unsigned read_bytes,read_ops,scan_bytes,allocations;} WxStreamUse;
typedef struct WxStreamFrame {unsigned enabled;WxStreamUse total,lane[WX_STREAM_LANES];} WxStreamFrame;
// Cooperative wall-clock slices supplement byte quotas. A filesystem call or
// allocation already in progress cannot be interrupted by this limit.
#define WX_STREAM_FRAME_MS 12u
typedef unsigned (*WxStreamClock)(void);
typedef struct WxStreamTime {unsigned enabled,yield_mask,limit_ms[4],observed_ms[4];} WxStreamTime;
void wx_stream_set_clock(WxStreamClock clock); // Configure outside a frame.
const WxStreamTime* wx_stream_time_metrics(void);
// Render-thread scope. Guaranteed shares; unused work flows forward in index,
// world, NPC, avatar order. End before unrelated UI work.
void wx_stream_frame_begin(unsigned lane_mask);
void wx_stream_frame_end(void);
const WxStreamFrame* wx_stream_frame_metrics(void);
int wx_stream_index_ready(void);
// Shared reservations for staged consumers outside pack.c. A zero grant means
// defer; charge before attempting I/O. Calls cannot replenish the frame quota.
unsigned wx_stream_read_grant(unsigned lane,unsigned wanted);
int wx_stream_allocation_grant(unsigned lane);
int wx_stream_frame_active(void);
int wx_stream_scan_grant(unsigned lane,unsigned bytes);
typedef struct WxStreamJob {
    int entry,slot;unsigned phase,offset,new_texture,new_pose;
} WxStreamJob;
typedef struct WxStreamMetrics {
    unsigned read_bytes,read_ops,scan_bytes,pending_bytes,completed,cancelled;
    unsigned max_read_bytes,max_read_ops,max_scan_bytes,deferred;
} WxStreamMetrics;
typedef struct WxAnimationMetrics {unsigned sampled,skipped,vertices,palettes;} WxAnimationMetrics;
typedef struct WxSelection {
    unsigned phase,revision,progress,count,commits,restarts;
    float position[3];
    int wanted[WX_CACHE_SLOTS];
    float scores[WX_CACHE_SLOTS];
    unsigned resident[1024];
} WxSelection;
typedef struct WxScene {
    FILE* file;
    WxPackSource sources[WX_REGION_FILES];
    WxPackHeader header;
    WxEntry* entries;
    unsigned index_bytes,index_revision; // Retained capacity and table mutation identity.
    WxResident slots[WX_CACHE_SLOTS];
    WxTexture textures[WX_CACHE_SLOTS];
    WxPoseBuffer poses[WX_CACHE_SLOTS];
    uint32_t bytes, peak_bytes, loads, failures, budget_bytes,animation_index_reads;
    int wanted[WX_CACHE_SLOTS],wanted_ready;
    float wanted_position[3];
    WxSelection selection;
    WxStreamJob stream_job;
    WxStreamMetrics streaming;
    WxAnimationMetrics animating;
    unsigned stream_needed; // Unfinished actor requests from the preceding tick.
    char error[128];
} WxScene;
typedef struct WxPad {
    float move_x, move_y, look_x, look_y;
    unsigned buttons, pressed, layer;
    int connected, action, slot;
} WxPad;
int wx_environment_ready(const WxScene* s);
void wx_environment_sample(const WxScene* s,const float* position,WxEnvironmentSample* out);
void wx_scene_init(WxScene* s);
int wx_pack_open(WxScene* s,const char* path);
int wx_pack_open_begin(WxPackOpen* job,const char* path,unsigned lane);
int wx_pack_open_pump(WxScene* scene,WxPackOpen* job);
void wx_pack_open_cancel(WxPackOpen* job);
int wx_pack_attach(WxScene* s,const char* path);
int wx_pack_attach_begin(WxPackPending* pending,const char* path,unsigned expected_bytes);
// -1 failure, 0 still preparing, otherwise committed source slot plus one.
int wx_pack_attach_pump(WxScene* scene,WxPackPending* pending);
void wx_pack_attach_cancel(WxPackPending* pending);
void wx_pack_detach(WxScene* s,unsigned source);
int wx_pack_verify(WxScene* s);
void wx_pack_close(WxScene* s);
void wx_stream(WxScene* s,const float* position);
// True only when every currently requested world batch has been committed.
int wx_stream_ready(const WxScene* s);
void wx_stream_displays(WxScene* s,const uint32_t* displays,unsigned count);
void wx_stream_animations(WxScene* s,const uint32_t* ids,unsigned count);
// Avatar components switch together. Keep the last complete clip until all
// replacements arrive; idle/run may share the existing bounded cache.
void wx_stream_avatar_animations(WxScene* s,const uint32_t* ids,unsigned count,unsigned previous_clip);
// Retain the complete published appearance while new families load. Held IDs
// are resident-only: they never generate extra requests or optional prefetches.
void wx_stream_avatar_transition(WxScene* s,const uint32_t* ids,unsigned count,const uint32_t* held,unsigned held_count);
int wx_animation_resident(const WxScene* scene,uint32_t id);
uint32_t wx_animation_fallback(const WxScene* scene,uint32_t id);
void wx_animate(WxScene* s,unsigned time_ms);
void wx_animate_ids(WxScene* s,unsigned time_ms,const uint32_t* ids,unsigned count);
// Interval zero samples the current time; distant idle/run clips may use 66 or
// 100 ms buckets. All parts and instances of an ID share the fastest request.
void wx_animate_scheduled(WxScene* s,unsigned time_ms,const uint32_t* ids,const unsigned* intervals,unsigned count);
unsigned wx_animation_interval(float distance_squared,unsigned clip,int focused);
int wx_ground(WxScene* s,float x,float y,float* z);
int wx_floor(WxScene* s,float x,float y,float top,float bottom,float* z);
int wx_walk(WxScene* s,const float* from,float x,float y,float* z);
float wx_camera_distance(WxScene* s,const float* focus,const float* forward,const float* right,const float* up,float desired);
unsigned wx_walk_blocker(int* entry); // 1 invalid step, 2 pending collision, 3 no floor, 4 wall
unsigned wx_texture_bytes(const WxEntry* entry);
unsigned wx_texture_frame_bytes(const WxEntry* entry);
unsigned wx_free_memory(void);
void* wx_gpu_alloc(unsigned bytes);
void wx_gpu_free(void* memory);
void wx_pad_init(void);
void wx_pad_poll(WxPad* p);
#endif
