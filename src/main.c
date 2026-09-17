#include "wx_cast.h"
#include "wx_fog.h"
#include "wx_sky.h"
#include "wx_actions.h"
// Native NV2A renderer. Shader setup follows XboxDev/nxdk mesh sample.
#include <hal/video.h>
#include <hal/debug.h>
#include <pbkit/pbkit.h>
#include "wx_font.h"
#include <xboxkrnl/xboxkrnl.h>
#include <windows.h>
#include <SDL.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include "wx_runtime.h"
#include "wx_material.h"
#include "wx_region.h"
#include "wx_auth.h"
#include "wx_input.h"
#include "wx_replay.h"
#include "wx_telemetry.h"
#include "wx_spellbook.h"
#include "wx_world.h"
#include "wx_game.h"
#include "wx_dialog.h"
#include "wx_scenario.h"
#include "wx_inventory_ui.h"
#include "wx_avatar.h"
#include "wx_lobby.h"
#include "wx_journal.h"
#include "wx_utility.h"
#include "wx_map.h"
#include "wx_death.h"
#include "wx_login.h"
#include "wx_backdrop.h"
#include "wx_preview.h"
#include "wx_hud.h"
#include "wx_portrait.h"
#include "wx_icons.h"
#define MASK(mask,val) (((val)<<(ffs(mask)-1))&(mask))
static WxScene scene;
static WxRegion region;
static WxScene actors;
static WxAvatar avatar;
static WxCharacterLobby character_lobby;
static WxCharacterUi character_ui;
static WxJournalUi journal;
static WxUtility utility;
static WxMap world_map;
static WxUi interface;
static WxHud hud;
static WxPortraitCatalog portraits;
static WxIcons action_icons;
static WxCooldownCatalog cooldown_catalog;
static WxCooldownView action_cooldowns[8];
static unsigned cooldown_metrics[12];
static unsigned portrait_stats[10];
static WxLoginUi login_ui;
static WxLoginView login_view;
static WxRealms login_realms;
static WxLoginReplay login_replay;
static WxBackdrop title_scene;
static WxPreview preview;
static WxEntity actor_entities[128];
static unsigned actor_indices[32],actor_count,actor_missing,actor_drawn;
static unsigned world_entity_count;
static uint64_t target_guid;
static WxGame game;
static WxScenario scenario;
static WxInventory inventory;
static WxCastView cast_view;
static WxFog world_fog;
static WxLighting lighting;
static WxLightPalette world_light;
static WxWeatherMix weather_mix;
static WxActionFeedback action_feedback[8];
static char control_message[96];
static uint64_t resurrection_healer;
static WxDeath death;
static float player[3],yaw=0.15f,pitch=-0.18f;
static unsigned frames,draws,triangles;
#define WX_INDEX_METHOD(p,method,count) pb_push(p,method,count)
#include "index_batch.h"
#undef WX_INDEX_METHOD
#include "draw_profile.h"
_Static_assert(WX_NV_BEGIN_END==NV097_SET_BEGIN_END&&WX_NV_INDEX16==NV20_TCL_PRIMITIVE_3D_INDEX_DATA&&WX_NV_INDEX32==NV097_ARRAY_ELEMENT32,"NV2A index methods changed");
static uint32_t* push_base;
static uint32_t* begin_commands(void){
    uint32_t* p=pb_begin();if(!push_base)push_base=p;
    // Flush geometry well before the fixed 1 MiB command allocation fills.
    if(p>=push_base&&p-push_base>128*1024){
        pb_end(p);unsigned start=draw_profile_active?GetTickCount():0;while(pb_busy()){}
        unsigned drained=draw_profile_active?GetTickCount():0;pb_reset();p=pb_begin();push_base=p;
        if(draw_profile_active){draw_profile.flushes++;draw_profile.flush_wait_ms+=drained-start;draw_profile.flush_reset_ms+=GetTickCount()-drained;}
    }
    return p;
}
static void shaders(void){
    uint32_t program[]={
#include "vs.inl"
    };
    _Static_assert(sizeof(program)<=96*16,"World shader overlaps font program");
    uint32_t* p=begin_commands();
    p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,0);
    p=pb_push1(p,NV097_SET_TRANSFORM_EXECUTION_MODE,MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE,NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM)|MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE,NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN,0);
    p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_LOAD,0);pb_end(p);
    for(unsigned i=0;i<sizeof(program)/16;i++){p=begin_commands();pb_push(p++,NV097_SET_TRANSFORM_PROGRAM,4);memcpy(p,program+i*4,16);p+=4;pb_end(p);}
    p=begin_commands();
#include "ps.inl"
    pb_end(p);
}
static void attribute(int at,int size,const void* data){
    uint32_t* p=begin_commands();
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+at*4,MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE,NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F)|MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE,size)|MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE,sizeof(WxVertex)));
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+at*4,(uint32_t)data&0x03ffffff);pb_end(p);
}
#include "world_material_xbox.h"
static void vertex_color_attribute(const uint32_t* colors){
    uint32_t* p=begin_commands();
    // RGBA byte order, normalized to [0,1]. Missing streams use constant white.
    unsigned format=colors?NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_OGL|(4u<<4)|(4u<<8):2u;
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+3*4,format);
    if(colors)p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+3*4,(uint32_t)colors&0x03ffffff);
    else{const float white[4]={1,1,1,1};pb_push(p++,NV097_SET_VERTEX_DATA4F_M+3*16,4);memcpy(p,white,16);p+=4;}
    pb_end(p);
}
static unsigned render_material_counts[7],render_material_flags,render_motion_draws,render_motion_hash;
static unsigned render_liquid_draws,render_sequence_draws,render_sequence_frames;
static void render_pass(WxScene* current,const float camera[4],const float right[4],const float up[4],const float forward[4],uint32_t id,float scale,float angle,const WxBackdropView* framing,WxAvatar* appearance,int liquid_pass){
    if(!liquid_pass){
    memset(render_material_counts,0,sizeof render_material_counts);render_material_flags=0;
    render_motion_draws=0;render_motion_hash=2166136261u;
    render_liquid_draws=render_sequence_draws=render_sequence_frames=0;}
    unsigned material_now=GetTickCount();
    wx_fog_bind(framing?NULL:&world_fog);
    // actor_render divides camera coordinates by scale and multiplies the axes
    // by scale, so shader Z is already in world units. Do not scale fog twice.
    float projection[4]={320,240,420,1};if(framing){projection[0]=framing->center[0];projection[1]=framing->center[1];projection[2]=framing->focal;}uint32_t* p=begin_commands();
    p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,0);
    p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,96);
    WxLightPalette preview_light;const WxLightPalette* light=&world_light;
    if(framing||!wx_light_palette_valid(light)){wx_light_fallback(&preview_light);light=&preview_light;}
    float depth_range[4]={65535.f,.5f,1.f,0},ambient[4]={0},diffuse[4]={0},to_light[4];
    if(framing){depth_range[0]*=framing->far_clip/(framing->far_clip-framing->near_clip);depth_range[1]=framing->near_clip;}
    memcpy(ambient,light->color[WX_LIGHT_AMBIENT],12);memcpy(diffuse,light->color[WX_LIGHT_DIFFUSE],12);
    wx_light_direction(light,angle,to_light);
    // Explicit c0..c8 uniforms: actor rotation affects direction, never scale.
    const float* values[]={camera,right,up,forward,projection,depth_range,ambient,diffuse,to_light};
    for(unsigned i=0;i<9;i++){pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,4);memcpy(p,values[i],16);p+=4;}
    {
    // pb_target_back_buffer enables perspective depth (W buffering). Our
    // shader already emits projected Z, so interpolate that Z linearly while
    // retaining perspective-correct texture coordinates.
    p=pb_push1(p,NV097_SET_CONTROL0,NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE|NV097_SET_CONTROL0_TEXTURE_PERSPECTIVE_ENABLE);
    p=pb_push1(p,NV097_SET_BLEND_ENABLE,0);
    p=pb_push1(p,NV097_SET_CULL_FACE_ENABLE,0);
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,1);
    p=pb_push1(p,NV097_SET_DEPTH_FUNC,NV097_SET_DEPTH_FUNC_V_LEQUAL);
    p=pb_push1(p,NV097_SET_DEPTH_MASK,1);
    for(int i=1;i<4;i++)p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(i),0x0003ffc0);
    pb_push(p++,NV097_SET_VERTEX_DATA_ARRAY_FORMAT,16);for(int i=0;i<16;i++)*p++=2;
    }pb_end(p);
    WxMotionState previous_motion;wx_motion_evaluate(NULL,0,0,&previous_motion);world_motion_bind(&previous_motion);
    WxDrawOrder order[WX_CACHE_SLOTS];unsigned ordered=0,material_flags=UINT32_MAX;
    for(int n=0;n<WX_CACHE_SLOTS;n++){
        WxResident* r=&current->slots[n];if(r->entry<0)continue;WxEntry* e=&current->entries[r->entry];
        if(id!=UINT32_MAX&&e->id!=id)continue;
        if(e->kind==WX_KIND_COLLISION)continue;
        if(!!(e->flags&WX_LIQUID)!=liquid_pass)continue;
        float d[3]={e->center[0]-camera[0],e->center[1]-camera[1],e->center[2]-camera[2]};
        float depth=d[0]*forward[0]+d[1]*forward[1]+d[2]*forward[2];
        float side=fabsf(d[0]*right[0]+d[1]*right[1]+d[2]*right[2]);
        float radius=e->radius*scale;
        if(framing){if(depth+radius<framing->near_clip||depth-radius>framing->far_clip)continue;}
        else if(depth+radius<.5f||depth-radius>160.f||side>depth*.85f+radius*1.4f)continue;
        ordered=wx_material_order(order,ordered,n,depth,e->flags);
    }
    for(unsigned n=0;n<ordered;n++){
        WxResident* r=&current->slots[order[n].slot];WxEntry* e=&current->entries[r->entry];
        WxMotionState motion;wx_motion_evaluate(r->material_motion,(e->flags&WX_ANIMATED)?r->animated_time:material_now,material_now,&motion);
        if(motion.color[3]<=.001f)continue;
        if(memcmp(&motion,&previous_motion,sizeof motion)){world_motion_bind(&motion);previous_motion=motion;}
        if(r->material_motion){render_motion_draws++;const unsigned char* bytes=(const void*)&motion;
            for(unsigned k=0;k<sizeof motion;k++)render_motion_hash=(render_motion_hash^bytes[k])*16777619u;}
        unsigned flags=e->flags&(WX_ALPHA_TEST|WX_MATERIAL_FLAGS|WX_MATERIAL_V8);
        if(flags!=material_flags){world_material_bind(flags,framing?NULL:&world_fog,ambient,diffuse);material_flags=flags;}
        render_material_counts[wx_material_blend(flags)]++;render_material_flags|=flags|(e->flags&WX_MATERIAL_V10);
        p=begin_commands();
        unsigned logsize=0;while((1u<<logsize)<e->width)logsize++;
        unsigned levels=wx_texture_levels(e);
        unsigned format=(e->flags&WX_TEX_SWIZZLED)?(0x0000062a|(levels<<16)|(logsize<<20)|(logsize<<24)):0x0001122a;
        if(e->flags&(WX_TEX_DXT1|WX_TEX_DXT5))format=0x2a|((e->flags&WX_TEX_DXT1?0x0c:0x0f)<<8)|(levels<<16)|(logsize<<20)|(logsize<<24);
        uint32_t* pixels=appearance?wx_avatar_texture(appearance,e,r->texture):r->texture;
        if(e->flags&WX_TEXTURE_SEQUENCE){unsigned frame=wx_texture_frame(e,material_now);
            pixels=(uint32_t*)((unsigned char*)pixels+frame*wx_texture_frame_bytes(e));
            render_sequence_draws++;render_sequence_frames|=1u<<frame;}
        render_liquid_draws+=!!(e->flags&WX_LIQUID);
        p=pb_push2(p,NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0),(DWORD)pixels&0x03ffffff,format);
        p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0),(e->width*4)<<16);
        p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0),(e->width<<16)|e->height);
        unsigned wrap=e->kind==WX_KIND_TERRAIN?0x00030303:0x00010101;
        if(e->flags&WX_CLAMP_U)wrap=(wrap&~255u)|3u;if(e->flags&WX_CLAMP_V)wrap=(wrap&~65280u)|768u;
        p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_WRAP(0),wrap);
        p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0),0x4003ffc0);
        p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_FILTER(0),(levels>1?0x02062000:0x02022000));pb_end(p);
        attribute(0,3,r->vertices[0].p);attribute(2,3,r->vertices[0].n);attribute(9,2,r->vertices[0].uv);vertex_color_attribute(r->vertex_colors);
        if(draw_profile_active)draw_profile.legacy_batches+=(e->index_count+239)/240;
        for(unsigned at=0;at<e->index_count;){unsigned count=e->index_count-at;if(count>WX_INDEX_BATCH_LIMIT)count=WX_INDEX_BATCH_LIMIT;
            p=begin_commands();p=wx_index_batch(p,r->indices+at,count);pb_end(p);at+=count;
            if(draw_profile_active){draw_profile.batches++;draw_profile.indices+=count;if(count>draw_profile.largest_batch)draw_profile.largest_batch=count;}
        }draws++;triangles+=e->index_count/3;
    }
    // Blended/unlit/unfogged state cannot leak into later actors or UI/previews.
    world_material_bind(0,framing?NULL:&world_fog,ambient,diffuse);
    wx_motion_evaluate(NULL,0,0,&previous_motion);world_motion_bind(&previous_motion);
    vertex_color_attribute(NULL);
}
static void render(WxScene* current,const float camera[4],const float right[4],const float up[4],const float forward[4],uint32_t id,float scale,float angle,const WxBackdropView* framing,WxAvatar* appearance){
    render_pass(current,camera,right,up,forward,id,scale,angle,framing,appearance,0);
}
static void render_liquids(WxScene* current,const float camera[4],const float right[4],const float up[4],const float forward[4]){
    render_pass(current,camera,right,up,forward,UINT32_MAX,1,0,NULL,NULL,1);
}
static int actor_has(uint32_t id){for(unsigned i=0;i<actors.header.count;i++)if(actors.entries[i].id==id)return 1;return 0;}
static uint32_t actor_animation(const WxEntity* e){
    unsigned clip=!e->fields[22]?6:e->animation==0&&(e->fields[46]&0x80000)?16:e->animation;
    uint32_t id=(e->fields[131]<<8)|clip;return actor_has(id)?id:e->fields[131]<<8;
}
static int actor_visible(const WxEntity* e,uint32_t id,const float* camera,const float* right,const float* forward,float scale){
    float radius=0;
    for(unsigned j=0;j<actors.header.count;j++)if(actors.entries[j].id==id){
        const WxEntry* part=&actors.entries[j];const float* p=part->center;
        float bound=(sqrtf(p[0]*p[0]+p[1]*p[1]+p[2]*p[2])+part->radius)*scale;
        if(bound>radius)radius=bound;
    }
    float delta[3]={e->x-camera[0],e->y-camera[1],e->z-camera[2]};
    float depth=delta[0]*forward[0]+delta[1]*forward[1]+delta[2]*forward[2];
    float side=fabsf(delta[0]*right[0]+delta[1]*right[1]+delta[2]*right[2]);
    return !(depth+radius<.5f||depth-radius>160||side>depth*.85f+radius*1.4f);
}
static void actor_stream(unsigned now,const float* camera,const float* right,const float* forward){
    float distances[32];uint32_t animations[32];actor_count=actor_missing=0;
    unsigned count=world_entity_count;
    for(unsigned i=0;i<count;i++){
        WxEntity* e=&actor_entities[i];if(e->type!=3||e->fields[131]>0xffffff)continue;
        float dx=e->x-player[0],dy=e->y-player[1],dz=e->z-player[2];float d=dx*dx+dy*dy+dz*dz;
        if(d>125*125)continue;
        if(!actor_has(e->fields[131]<<8)){actor_missing++;continue;}
        unsigned j=0;while(j<actor_count&&distances[j]<=d)j++;if(j==32)continue;
        if(actor_count<32)actor_count++;
        for(unsigned k=actor_count-1;k>j;k--){actor_indices[k]=actor_indices[k-1];distances[k]=distances[k-1];}
        actor_indices[j]=i;distances[j]=d;
    }
    // Keep the selected unit in the same bounded cache even in a dense scene.
    for(unsigned i=0;i<count;i++)if(actor_entities[i].guid==target_guid&&actor_entities[i].type==3&&actor_entities[i].fields[131]<=0xffffff&&actor_has(actor_entities[i].fields[131]<<8)){
        unsigned j=0;while(j<actor_count&&actor_indices[j]!=i)j++;
        if(j==actor_count){if(actor_count<32)actor_count++;actor_indices[actor_count-1]=i;}break;
    }
    for(unsigned i=0;i<actor_count;i++){
        const WxEntity* e=&actor_entities[actor_indices[i]];
        animations[i]=actor_animation(e);
    }
    wx_stream_animations(&actors,animations,actor_count);
    // Keep the surrounding templates resident for camera turns, but skin only
    // templates actually needed by visible instances. Shared IDs update once.
    uint32_t visible[32];unsigned intervals[32],visible_count=0;
    for(unsigned i=0;i<actor_count;i++){
        const WxEntity* e=&actor_entities[actor_indices[i]];float scale;memcpy(&scale,&e->fields[4],4);
        uint32_t id=wx_animation_fallback(&actors,animations[i]);
        if(id==UINT32_MAX||!isfinite(scale)||scale<=0||scale>100||(e->guid!=target_guid&&!actor_visible(e,id,camera,right,forward,scale)))continue;
        float dx=e->x-camera[0],dy=e->y-camera[1],dz=e->z-camera[2];
        unsigned cadence=wx_animation_interval(dx*dx+dy*dy+dz*dz,animations[i]&255,e->guid==target_guid);
        unsigned j=0;while(j<visible_count&&visible[j]!=id)j++;
        if(j==visible_count){visible[j]=id;intervals[j]=cadence;visible_count++;}
        else if(cadence<intervals[j])intervals[j]=cadence;
    }
    wx_animate_scheduled(&actors,now,visible,intervals,visible_count);
}
static void actor_render(const float* camera,const float* right,const float* up,const float* forward){
    actor_drawn=0;
    for(unsigned i=0;i<actor_count;i++){
        const WxEntity* e=&actor_entities[actor_indices[i]];float scale;memcpy(&scale,&e->fields[4],4);
        if(!isfinite(scale)||scale<=0||scale>100)continue;
        uint32_t id=wx_animation_fallback(&actors,actor_animation(e));
        if(id==UINT32_MAX)continue;
        if(!actor_visible(e,id,camera,right,forward,scale))continue;
        float c=cosf(e->orientation),s=sinf(e->orientation),view[4][4]={{0}};
        float dx=camera[0]-e->x,dy=camera[1]-e->y;
        view[0][0]=(c*dx+s*dy)/scale;view[0][1]=(-s*dx+c*dy)/scale;view[0][2]=(camera[2]-e->z)/scale;view[0][3]=1;
        const float* axes[]={right,up,forward};
        for(unsigned k=0;k<3;k++){view[k+1][0]=(c*axes[k][0]+s*axes[k][1])*scale;view[k+1][1]=(-s*axes[k][0]+c*axes[k][1])*scale;view[k+1][2]=axes[k][2]*scale;}
        unsigned before=draws;render(&actors,view[0],view[1],view[2],view[3],id,scale,e->orientation,NULL,NULL);
        if(draws>before)actor_drawn++;
    }
}
static WxEntity* target_entity(void){
    for(unsigned i=0;i<world_entity_count;i++)if(actor_entities[i].guid==target_guid)return &actor_entities[i];return 0;
}
static void player_render(const float* camera,const float* right,const float* up,const float* forward,float angle,float jump){
    if(!avatar.ready)return;
    float c=cosf(angle),s=sinf(angle),view[4][4]={{0}};
    float dx=camera[0]-player[0],dy=camera[1]-player[1];
    view[0][0]=c*dx+s*dy;view[0][1]=-s*dx+c*dy;view[0][2]=camera[2]-player[2]-jump;view[0][3]=1;
    const float* axes[]={right,up,forward};
    for(unsigned k=0;k<3;k++){view[k+1][0]=c*axes[k][0]+s*axes[k][1];view[k+1][1]=-s*axes[k][0]+c*axes[k][1];view[k+1][2]=axes[k][2];}
    unsigned before=draws;
    for(unsigned i=0;i<avatar.count;i++){
        unsigned part_before=draws;render(&avatar.scene,view[0],view[1],view[2],view[3],avatar.ids[i],1,angle,NULL,&avatar);
        if(draws>part_before)for(unsigned j=0;j<avatar.header.item_count;j++)for(unsigned k=0;k<2;k++)
            if(avatar.items[j].component[k]&&avatar.items[j].component[k]==avatar.families[i])avatar.equipment_mask|=1u<<avatar.items[j].slot;
    }
    avatar.drawn=draws-before;
}
static void preview_render(void){
    if(!preview.active)return;
    wx_backdrop_draw(&preview.scene);draws+=preview.scene.draws;triangles+=preview.scene.triangles;
    WxAvatar* a=&preview.avatar;a->drawn=0;if(!a->ready||preview.scene.load_phase)return;
    const WxBackdropView* f=&preview.scene.view;float view[4][4]={{0}},c=cosf(preview.angle),s=sinf(preview.angle);
    float dx=f->camera[0]-preview.position[0],dy=f->camera[1]-preview.position[1];
    view[0][0]=c*dx+s*dy;view[0][1]=-s*dx+c*dy;view[0][2]=f->camera[2]-preview.position[2];view[0][3]=1;
    const float* axes[]={f->right,f->up,f->forward};
    for(unsigned k=0;k<3;k++){view[k+1][0]=c*axes[k][0]+s*axes[k][1];view[k+1][1]=-s*axes[k][0]+c*axes[k][1];view[k+1][2]=axes[k][2];}
    unsigned before=draws;for(unsigned i=0;i<a->count;i++)render(&a->scene,view[0],view[1],view[2],view[3],a->ids[i],1,preview.angle,f,a);
    a->drawn=draws-before;
}
// Clear only the circular portrait pixels, then test stencil while reusing the
// resident world mesh. The two references keep the portraits isolated. No render
// target, copied avatar, dynamic vertex buffer or per-frame heap allocation.
static unsigned portrait_render(WxScene* model,WxAvatar* appearance,const uint32_t* ids,unsigned count,uint32_t key,unsigned x,unsigned reference){
    WxPortraitView view;const WxPortraitEntry* entry=wx_portrait_find(&portraits,key);
    if(!wx_portrait_view(entry,&view)||!count)return 0;
    unsigned resident=0;for(unsigned i=0;i<count;i++)resident+=wx_animation_resident(model,ids[i])!=0;
    if(!resident)return 0;
    WxPortraitSpan spans[64];unsigned span_count=wx_portrait_spans(spans,x,32);
    for(unsigned i=0;i<span_count;i++){
        const WxPortraitSpan* r=&spans[i];uint32_t* p=begin_commands();
        p=pb_push1(p,NV097_SET_CLEAR_RECT_HORIZONTAL,((r->x+r->width-1)<<16)|r->x);
        p=pb_push1(p,NV097_SET_CLEAR_RECT_VERTICAL,((r->y+r->height-1)<<16)|r->y);
        p=pb_push1(p,NV097_SET_ZSTENCIL_CLEAR_VALUE,0xffffff00|reference);
        p=pb_push1(p,NV097_SET_COLOR_CLEAR_VALUE,0xff171914);
        p=pb_push1(p,NV097_CLEAR_SURFACE,0xf3);pb_end(p);
    }
    portrait_stats[7]+=span_count;
    uint32_t* p=begin_commands();p=pb_push1(p,NV097_SET_STENCIL_TEST_ENABLE,1);
    p=pb_push1(p,NV097_SET_STENCIL_MASK,0);p=pb_push1(p,NV097_SET_STENCIL_FUNC,0x202); // EQUAL
    p=pb_push1(p,NV097_SET_STENCIL_FUNC_REF,reference);p=pb_push1(p,NV097_SET_STENCIL_FUNC_MASK,255);
    p=pb_push1(p,NV097_SET_STENCIL_OP_FAIL,NV097_SET_STENCIL_OP_V_KEEP);
    p=pb_push1(p,NV097_SET_STENCIL_OP_ZFAIL,NV097_SET_STENCIL_OP_V_KEEP);
    p=pb_push1(p,NV097_SET_STENCIL_OP_ZPASS,NV097_SET_STENCIL_OP_V_KEEP);pb_end(p);
    WxBackdropView framing={0};framing.center[0]=x+32;framing.center[1]=64;framing.focal=view.focal;
    framing.near_clip=view.near_clip;framing.far_clip=view.far_clip;
    unsigned before=draws;
    for(unsigned i=0;i<count;i++)render(model,view.camera,view.right,view.up,view.forward,ids[i],1,0,&framing,appearance);
    p=begin_commands();p=pb_push1(p,NV097_SET_STENCIL_TEST_ENABLE,0);p=pb_push1(p,NV097_SET_STENCIL_MASK,255);pb_end(p);
    return draws-before;
}
static void hud_portraits(void){
    unsigned start=GetTickCount();portrait_stats[0]=portraits.ready;portrait_stats[1]=portraits.count;portrait_stats[2]=portraits.failures;
    if(hud.drawn){
        if(avatar.ready){portrait_stats[3]=WX_PORTRAIT_PLAYER|(avatar.header.look[0]<<1)|avatar.header.look[1];
            portrait_stats[4]=portrait_render(&avatar.scene,&avatar,avatar.ids,avatar.count,portrait_stats[3],50,1);}
        const WxEntity* target=target_entity();
        if(target&&target->type==3&&target->fields[131]<=0xffffff){uint32_t id=wx_animation_fallback(&actors,actor_animation(target));portrait_stats[5]=target->fields[131];
            if(id!=UINT32_MAX)portrait_stats[6]=portrait_render(&actors,NULL,&id,1,portrait_stats[5],398,2);}
    }
    portrait_stats[8]=GetTickCount()-start;
}
static WxEntity* self_entity(void){
    WxWorldView world;wx_world_view(&world);
    for(unsigned i=0;i<world_entity_count;i++)if(actor_entities[i].guid==world.guid)return &actor_entities[i];return 0;
}
static int death_state(void){const WxEntity* self=self_entity();return self?((self->fields[190]&16)?2:!self->fields[22]):0;}
static void command(uint16_t opcode,uint64_t guid,uint32_t value){
    WxCommand c={opcode,guid,value,0};
    if(!wx_world_command(&c))snprintf(control_message,sizeof control_message,"Command could not be queued");
    else {control_message[0]=0;if(opcode==0x216)death.queries++;else if(opcode==0x15a)death.releases++;
        else if(opcode==0x1d2)death.reclaims++;else if(opcode==0x21c)death.healers++;}
}
static void select_target(int direction,int close_only){
    unsigned candidates[128],count=0;float distances[128];int current=-1;
    for(unsigned i=0;i<world_entity_count;i++){
        WxEntity* e=&actor_entities[i];if(e->type!=3||!e->positioned||(e->fields[46]&0x02000000))continue;
        float dx=e->x-player[0],dy=e->y-player[1],dz=e->z-player[2],distance=dx*dx+dy*dy+dz*dz;
        if(distance>(close_only?36:10000))continue;
        if(close_only&&(e->fields[147]&2))distance-=10000;
        unsigned j=0;while(j<count&&(distances[j]<distance||(distances[j]==distance&&actor_entities[candidates[j]].guid<e->guid)))j++;
        for(unsigned k=count;k>j;k--){distances[k]=distances[k-1];candidates[k]=candidates[k-1];}
        distances[j]=distance;candidates[j]=i;count++;
    }
    for(unsigned i=0;i<count;i++)if(actor_entities[candidates[i]].guid==target_guid)current=(int)i;
    if(!count){snprintf(control_message,sizeof control_message,"No creature in range");return;}
    unsigned next=current<0?0:(unsigned)((current+(direction<0?-1:1)+(int)count)%(int)count);
    WxEntity* e=&actor_entities[candidates[next]];target_guid=e->guid;
    command(0x13d,target_guid,0);command(0x60,target_guid,e->fields[3]);
}
static void gameplay_input(const WxPad* pad){
    int dead=death_state();
    if(dead==1){
        if(!pad->layer&&(pad->pressed&(1u<<WX_A)))command(0x15a,0,0);
        return;
    }
    if(dead==2&&!pad->layer){
        if(resurrection_healer){
            if(pad->pressed&(1u<<WX_B)){resurrection_healer=0;control_message[0]=0;}
            else if(pad->pressed&(1u<<WX_A)){command(0x21c,resurrection_healer,0);resurrection_healer=0;}
            return;
        }
        if(pad->pressed&(1u<<WX_A)){
            unsigned action=wx_death_action(&death);
            if(action)command((uint16_t)action,death.corpse,0);
            else if(death.remaining_ms)snprintf(control_message,sizeof control_message,"Reclaim available in %u seconds",(death.remaining_ms+999)/1000);
            else snprintf(control_message,sizeof control_message,"Move closer to your body to reclaim it");
            return;
        }
    }
    if(!pad->layer){
        if(pad->pressed&((1u<<WX_Y)|(1u<<WX_RIGHT)))select_target(1,0);
        if(pad->pressed&(1u<<WX_LEFT))select_target(-1,0);
        if(pad->pressed&(1u<<WX_B)){WxCommand cancel;if(wx_cast_cancel(&cast_view,&cancel))wx_world_command(&cancel);else {command(0x142,0,0);command(0x13d,0,0);target_guid=0;}}
        if(pad->pressed&(1u<<WX_X)){
            if(!target_entity())select_target(1,1);WxEntity* e=target_entity();
            if(e){
                if(!e->fields[22])command(0x15d,e->guid,0);
                else if(dead==2&&(e->fields[147]&0x20)){resurrection_healer=e->guid;snprintf(control_message,sizeof control_message,"A: resurrect here (25%% durability loss)  B: cancel");}
                else if(dead==2){snprintf(control_message,sizeof control_message,"Find your body or a Spirit Healer");}
                else if(e->fields[147]&2)command(0x184,e->guid,0);
                else if(e->fields[147]&4)command(0x19e,e->guid,0);
                else if(e->fields[147])command(0x17b,e->guid,0);
                else command(0x141,e->guid,0);
            }
        }
    }
    if(!dead&&pad->action>=0&&pad->action<120){
        WxWorldView world;wx_world_view(&world);unsigned form=0;
        for(unsigned i=0;i<world_entity_count;i++)if(actor_entities[i].guid==world.guid)form=(actor_entities[i].fields[138]>>16)&255;
        uint32_t binding=wx_action_binding(&game,pad->action,form),id=binding&0xffffff,type=binding>>24;
        if(id==6603&&type==0&&target_entity())command(0x141,target_guid,0);
        else if(id&&type==0&&wx_has_spell(&game,id))command(0x12e,target_guid,id);
        else if(id&&type==0x80){
            for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].entry==id){
                WxCommand use={0xab,0,inventory.items[i].bag,inventory.items[i].slot};wx_world_command(&use);break;
            }
        }
        else snprintf(control_message,sizeof control_message,"This action slot is empty or not supported yet");
    }
}
static void gameplay_hud(uint64_t player_guid,unsigned map_id,const float* camera,const float* right,const float* up,const float* forward,unsigned action_layer,int unit_frames){
    for(unsigned i=0;!unit_frames&&i<world_entity_count;i++)if(actor_entities[i].guid==player_guid){
        WxEntity* e=&actor_entities[i];wx_font_printat(action_layer?8:11,2,"Health %u/%u  Level %u",e->fields[22],e->fields[28],e->fields[34]);break;
    }
    int dead=death_state();
    if(dead){
        if(dead==1)wx_font_printat(12,2,"You have died.  A: release spirit");
        else {
            if(death.known&&death.map==map_id)wx_font_printat(12,2,"Spirit: body %u yards away  Wait %u sec",(unsigned)death.distance,(death.remaining_ms+999)/1000);
            else wx_font_printat(12,2,"Spirit: return to your body or a Spirit Healer");
            wx_font_printat(14,2,death.ready?"A: reclaim body  Back: map":"Return to your body  Back: map  X: Spirit Healer");
        }
        wx_font_printat(15,2,"%.54s",control_message);return;
    }
    WxEntity* e=target_entity();
    if(e){
        const char* name=game.name_entry==e->fields[3]?game.target_name:"Querying target name";
        float dx=e->x-player[0],dy=e->y-player[1],dz=e->z-player[2];unsigned range=(unsigned)sqrtf(dx*dx+dy*dy+dz*dz);
        if(!unit_frames){wx_font_printat(action_layer?9:12,2,"Target: %.38s",name);
            wx_font_printat(action_layer?10:13,2,"Level %u  HP %u/%u  Range %u",e->fields[34],e->fields[22],e->fields[28],range);}
        else wx_ui_textf(&interface,0,286,123,WX_UI_GOLD,"Range: %u yd",range);
        float d[3]={e->x-camera[0],e->y-camera[1],e->z+1.5f-camera[2]};
        float depth=d[0]*forward[0]+d[1]*forward[1]+d[2]*forward[2];
        if(depth>1){
            int x=(int)(320+(d[0]*right[0]+d[1]*right[1]+d[2]*right[2])*420/depth);
            int y=(int)(240-(d[0]*up[0]+d[1]*up[1]+d[2]*up[2])*420/depth);
            if(x>8&&x<632&&y>8&&y<472){pb_fill(x-7,y-7,14,2,0xffffff00);pb_fill(x-7,y+5,14,2,0xffffff00);pb_fill(x-7,y-5,2,10,0xffffff00);pb_fill(x+5,y-5,2,10,0xffffff00);}
        }
    }
    if(!action_layer)wx_font_printat(14,2,"Y: target  X: interact  Hold Black: menus  LT/RT: actions");
    wx_font_printat(action_layer?7:15,2,"%.54s",control_message[0]?control_message:game.message);
}
#include "portrait_fixture.h"
static unsigned stream_clock_ms(void){return GetTickCount();}
int main(void){
    wx_stream_set_clock(stream_clock_ms);
    XVideoSetMode(640,480,32,REFRESH_DEFAULT);wx_pad_init();
    pb_size(1024*1024);
    int result=pb_init();if(result){debugPrint("WOWX pb_init failed: %d\n",result);Sleep(5000);return 1;}
    pb_show_front_screen();shaders();wx_font_init("D:\\FONT.BIN");wx_ui_open(&interface,"D:\\INTERFACE.WUI");wx_portrait_open(&portraits,"D:\\PORTRAIT.WPT");
    wx_cooldown_catalog_open(&cooldown_catalog,"D:\\COOLDOWN.WCD");wx_world_cooldown_catalog(&cooldown_catalog);
    int fixture_mode=portrait_fixture_enabled();if(fixture_mode)portrait_fixture((unsigned)fixture_mode);
    wx_fog_fallback(&world_fog,WX_FOG_OUTDOOR);
    wx_light_open(&lighting,"D:\\LIGHT.WLF");
    wx_network_start();
    {WxAuthConfig defaults;wx_network_login_defaults(&defaults);wx_login_init(&login_ui,&defaults);volatile char* secret=defaults.password;for(unsigned i=0;i<sizeof defaults.password;i++)secret[i]=0;}
    wx_map_open(&world_map,"D:\\");
    wx_scenario_init(&scenario,"D:\\scenario.bin");
    static WxSpellBook spellbook;static WxSpellUi spell_ui;wx_spellbook_open(&spellbook,"D:\\SPELLS.WXS");wx_icons_open(&action_icons,"D:\\ICONS.WIC");
    int loaded=wx_pack_open(&scene,"D:\\world.wxp");
    int actors_loaded=wx_pack_open(&actors,"D:\\actors.wxp");actors.budget_bytes=8u*1024u*1024u;
    static WxAvatarSelection avatar_selection;wx_scene_init(&avatar.scene);
    if(loaded){memcpy(player,scene.header.spawn,sizeof player);for(int i=0;i<32;i++)wx_stream(&scene,player);}
    int regional=wx_region_open(&region,"D:\\world.wxi");
    if(regional){wx_pack_close(&scene);wx_scene_init(&scene);loaded=wx_region_update(&region,&scene,0,player);}
    unsigned last=GetTickCount(),fps_start=last,fps_frames=0,fps=0,minfree=wx_free_memory();
    int menu=0,inventory_open=0,diagnostics=(scenario.enabled||wx_pad_replay_active()),last_action=-1,binding_slot=0,saved=0;float jump=0,jump_speed=0,fall_ms=0;WxPad pad={0};
    unsigned world_revision=0,position_revision=0;WxWorldView world={0};
    unsigned presentation=pb_get_vbl_counter();
    while(1){
        unsigned pace_start=GetTickCount();
        // Pace on every second 60 Hz vblank. Combining a 33 ms sleep with another
        // unconditional vblank wait was intermittently stretching frames to 50 ms.
        while((unsigned)(pb_get_vbl_counter()-presentation)<2)pb_wait_for_vbl();
        presentation=pb_get_vbl_counter();
        unsigned now=GetTickCount(),frame_ms=now-last;float dt=frame_ms/1000.f;last=now;if(dt>.05f)dt=.05f;
        unsigned profile[WX_PROFILE_COUNT]={0},phase_start=now,phase_end;
        profile[WX_PROFILE_PACE]=now-pace_start;
        wx_pad_poll(&pad);
        wx_world_view(&world);
        wx_world_characters(&character_lobby);wx_character_ui_sync(&character_ui,&character_lobby);
        character_ui.looks=&preview.avatar.looks;
        if(character_ui.open&&!character_ui.random_state)character_ui.random_state=now?now:1;
        int in_lobby=character_lobby.phase!=WX_LOBBY_CLOSED;
        wx_network_login_view(&login_view);
        int in_login=login_view.mode&&!world.active&&!in_lobby;
        if(in_login&&login_view.phase==WX_LOGIN_REALMS&&login_ui.revision!=login_view.revision){
            login_realms.count=0;wx_auth_realms(&login_realms);
        }
        world_entity_count=world.active?wx_world_entities(actor_entities,128):0;wx_world_game(&game);wx_world_inventory(&inventory);
        wx_world_cast(&cast_view);wx_spellbook_request(&spellbook,cast_view.spell);wx_spellbook_sync(&spellbook,&game);uint8_t spell_bindings[24];for(unsigned i=0;i<24;i++)spell_bindings[i]=(uint8_t)wx_pad_binding(i);
        unsigned spell_form=self_entity()?(self_entity()->fields[138]>>16)&255:0;
        if(!world.active)spell_ui.open=0;
        if(!death_state())resurrection_healer=0;
        if(world.active&&(world.revision!=world_revision||world.position_revision!=position_revision)){
            player[0]=world.x;player[1]=world.y;player[2]=world.z;yaw=world.orientation;
            world_revision=world.revision;position_revision=world.position_revision;jump=jump_speed=fall_ms=0;target_guid=0;
        }
        if(wx_death_update(&death,&world,self_entity(),actor_entities,world_entity_count,player,&game,now))command(0x216,0,0);
        scenario.now_ms=now;
        wx_scenario_input(&scenario,&world,actor_entities,world_entity_count,player,yaw,pitch,target_guid,&game,&character_lobby,&character_ui,&pad);
        wx_spell_scenario_input(&scenario,&world,self_entity(),player,&game,&spellbook,&spell_ui,spell_bindings,spell_form,&pad);
        wx_login_replay(&login_replay,&login_ui,&login_view,&character_lobby,&character_ui,world.active,&pad);
        if(in_login){
            WxLoginCommand request;menu=inventory_open=journal.open=spell_ui.open=0;
            if(wx_login_input(&login_ui,&login_view,&login_realms,&pad,&request)){
                if(!wx_network_login_command(&request))snprintf(login_ui.message,sizeof login_ui.message,"The login request could not be queued.");
                else if(request.kind==WX_LOGIN_SUBMIT){volatile char* secret=login_ui.config.password;for(unsigned i=0;i<sizeof login_ui.config.password;i++)secret[i]=0;}
            }
            volatile char* secret=request.config.password;for(unsigned i=0;i<sizeof request.config.password;i++)secret[i]=0;
            memset(&pad,0,sizeof pad);pad.action=pad.slot=-1;
        }
        wx_journal_sync(&journal,self_entity());
        if(scenario.enabled)dt=1.f/30.f;
        if(wx_pad_replay_active())dt=1.f/30.f;
        wx_map_input(&world_map,&pad,world.active&&!in_lobby&&!menu&&!journal.open&&!spell_ui.open&&!inventory_open&&!game.dialog.screen&&!utility.open&&!utility.pending,world.map,player[0],player[1],dt);
        wx_map_body(&world_map,death.state==2&&death.known,death.map,death.position[0],death.position[1]);
        unsigned utility_action=wx_utility_input(&utility,&pad,now,world.active&&!in_lobby&&!menu&&!journal.open&&!spell_ui.open&&!world_map.open&&!game.dialog.screen&&!death_state());
        if(utility.open)inventory_open=0;
        if(utility_action==WX_UTILITY_BAGS)inventory_open=!inventory_open;
        else if(utility_action){
            inventory_open=menu=journal.open=spell_ui.open=0;
            if(utility_action==WX_UTILITY_QUESTS)journal.open=1;
            if(utility_action==WX_UTILITY_SPELLS){memset(&spell_ui,0,sizeof spell_ui);spell_ui.open=1;}
            if(utility_action==WX_UTILITY_SETTINGS)menu=1;
        }
        if(in_lobby){
            if(character_ui.screen<=WX_CHARACTER_CREATE||character_ui.screen==WX_CHARACTER_APPEARANCE)wx_preview_input(&preview,&pad,dt);
            menu=inventory_open=journal.open=spell_ui.open=0;WxLobbyCommand choice;
            if(wx_character_ui_input(&character_ui,&character_lobby,&pad,&choice)&&!wx_world_character_command(&choice))
                snprintf(character_ui.message,sizeof character_ui.message,"The character request could not be queued");
        }
        if(!in_login&&!in_lobby&&(pad.pressed&(1u<<SDL_CONTROLLER_BUTTON_START))){menu=!menu;inventory_open=journal.open=spell_ui.open=0;}
        if(spell_ui.open){WxCommand edit;if(wx_spell_ui_input(&spell_ui,&spellbook,&game,spell_bindings,spell_form,&pad,&edit)&&!wx_world_command(&edit))snprintf(spell_ui.message,sizeof spell_ui.message,"Binding could not be queued");pad.pressed=0;pad.action=-1;}
        if(journal.open){wx_journal_input(&journal,&pad);pad.pressed=0;pad.action=-1;}
        if(inventory_open){
            wx_world_inventory(&inventory);
            if(pad.pressed&(1u<<WX_B)){inventory_open=0;pad.pressed&=~(1u<<WX_B);}
            else wx_inventory_input(&inventory,&pad);
        }
        if(!in_lobby&&(pad.pressed&(1u<<WX_RSTICK)))diagnostics=!diagnostics;
        if(menu){
            if(pad.slot>=0)binding_slot=pad.slot;
            if(!pad.layer){
                if(world.active&&(pad.pressed&(1u<<WX_A))){menu=0;journal.open=1;pad.pressed=0;}
                if(world.active&&(pad.pressed&(1u<<WX_WHITE))){menu=0;memset(&spell_ui,0,sizeof spell_ui);spell_ui.open=1;pad.pressed=0;}
                if(pad.pressed&(1u<<WX_B))menu=0;
                if(pad.pressed&(1u<<WX_UP)){wx_pad_set_deadzone(wx_pad_deadzone()+.02f);saved=0;}
                if(pad.pressed&(1u<<WX_DOWN)){wx_pad_set_deadzone(wx_pad_deadzone()-.02f);saved=0;}
                if(pad.pressed&(1u<<WX_LEFT)){wx_pad_remap(binding_slot,(wx_pad_binding(binding_slot)+119)%120);saved=0;}
                if(pad.pressed&(1u<<WX_RIGHT)){wx_pad_remap(binding_slot,(wx_pad_binding(binding_slot)+1)%120);saved=0;}
                if(pad.pressed&(1u<<WX_X))saved=wx_pad_save()?1:-1;
                if(pad.pressed&(1u<<WX_BACK)){if(world.active)wx_world_logout();else wx_network_reconnect();}
                if(pad.pressed&(1u<<WX_Y)){wx_world_characters_open();wx_network_reconnect();}
            }
        }
        if(!menu&&!journal.open&&!spell_ui.open&&!inventory_open&&!world_map.open&&world.active){
            if(game.dialog.screen){
                const WxEntity* self=0;for(unsigned i=0;i<world_entity_count;i++)if(actor_entities[i].guid==world.guid)self=&actor_entities[i];
                if(game.dialog.screen==WX_SCREEN_VENDOR)wx_vendor_input(&inventory,&game,&pad);
                else wx_dialog_input(&game,&pad,self);
            }else gameplay_input(&pad);
        }
        if(pad.action>=0)last_action=pad.action;
        phase_end=GetTickCount();profile[WX_PROFILE_UPDATE]=phase_end-phase_start;phase_start=phase_end;
        unsigned move_flags=0;float move_speed=0,move_cos=cosf(yaw),move_sin=sinf(yaw);
        if(!in_login&&!in_lobby&&!menu&&!journal.open&&!spell_ui.open&&!inventory_open&&!world_map.open&&!game.dialog.screen&&death_state()!=1&&!resurrection_healer){yaw+=pad.look_x*dt*2;yaw=fmodf(yaw+6.283185307f,6.283185307f);pitch+=pad.look_y*dt*1.3f;if(pitch>.7f)pitch=.7f;if(pitch<-.9f)pitch=-.9f;
            float dx=cosf(yaw)*pad.move_y+sinf(yaw)*pad.move_x,dy=sinf(yaw)*pad.move_y-cosf(yaw)*pad.move_x;
            float len=sqrtf(dx*dx+dy*dy);if(len>1){dx/=len;dy/=len;}
            float speed=pad.move_y<0?4.5f:7.f;
            float nx=player[0]+dx*dt*speed,ny=player[1]+dy*dt*speed,z=player[2];
            if(loaded&&wx_walk(&scene,player,nx,ny,&z)){
                player[0]=nx;player[1]=ny;player[2]=z;
                if(pad.move_y>.01f)move_flags|=WX_MOVE_FORWARD;if(pad.move_y<-.01f)move_flags|=WX_MOVE_BACK;
                if(pad.move_x<-.01f)move_flags|=WX_MOVE_LEFT;if(pad.move_x>.01f)move_flags|=WX_MOVE_RIGHT;
                move_speed=fminf(len,1)*speed;if(len>.001f){move_cos=dx/fminf(len,1);move_sin=dy/fminf(len,1);}
            }
            if(!death_state()&&!pad.layer&&(pad.pressed&(1u<<SDL_CONTROLLER_BUTTON_A))&&jump==0){jump_speed=7.95797334f;fall_ms=0;}
        }
        if(jump>0||jump_speed>0)fall_ms+=dt*1000;
        jump+=jump_speed*dt;jump_speed-=19.291105f*dt;if(jump<0){jump=0;jump_speed=0;}
        if(jump>0)move_flags|=WX_MOVE_JUMP;
        if(world.active){WxMovement movement={move_flags,now,(unsigned)fall_ms,player[0],player[1],player[2]+jump,yaw,
            7.95797334f,move_cos,move_sin,move_speed};wx_world_submit(&movement,world.position_revision);}
        phase_end=GetTickCount();profile[WX_PROFILE_MOVE]=phase_end-phase_start;phase_start=phase_end;
        while(pb_busy()) {}
        uint32_t icon_keys[24]={0};
        if(world.active&&game.actions_ready)for(unsigned i=0;i<24;i++)icon_keys[i]=wx_action_icon(&game,wx_action_binding(&game,spell_bindings[i],spell_form));
        unsigned icons_start=GetTickCount();wx_icons_update(&action_icons,icon_keys,24);unsigned icons_ms=GetTickCount()-icons_start;
        static int title_attempted;
        // Realm entry can close the lobby before the next world-view snapshot.
        // Never reload the title during that gap or while opening the world socket.
        int show_title=in_login&&login_view.phase!=WX_LOGIN_WORLD;
        if(show_title){if(!title_attempted){wx_backdrop_begin(&title_scene,"D:\\TITLE.WXB");title_attempted=1;}wx_backdrop_pump(&title_scene,now);wx_backdrop_update(&title_scene,now);}
        else if(title_attempted){wx_backdrop_close(&title_scene);title_attempted=0;}
        if(in_lobby&&(avatar.file||avatar.opening.phase)){wx_avatar_close(&avatar);memset(&avatar_selection,0,sizeof avatar_selection);}
        wx_preview_update(&preview,&character_ui,&character_lobby,"D:\\",now);
        wx_map_update(&world_map,self_entity()?self_entity()->explored:NULL);
        wx_stream_frame_begin(1u|(scene.file&&!wx_stream_ready(&scene)?2u:0u)|
            (actors_loaded&&world_entity_count&&(!actors.loads||actors.stream_needed)?4u:0u)|
            (!in_lobby&&world.active&&world.race&&(!avatar.file||!avatar.scene.loads||avatar.scene.stream_needed||avatar.compose.phase)?8u:0u));
        unsigned region_start=GetTickCount();if(regional)loaded=wx_region_update(&region,&scene,world.map,player);
        unsigned region_ms=GetTickCount()-region_start,stream_start=GetTickCount();
        if(scene.file){wx_stream(&scene,player);wx_animate(&scene,now);}
        unsigned stream_ms=GetTickCount()-stream_start;
        phase_end=GetTickCount();profile[WX_PROFILE_STREAM]=phase_end-phase_start;phase_start=phase_end;
        pb_reset();pb_target_back_buffer();push_base=NULL;
        float forward[4]={cosf(yaw)*cosf(pitch),sinf(yaw)*cosf(pitch),sinf(pitch),0};
        float right[4]={sinf(yaw),-cosf(yaw),0,0};
        float up[4]={-cosf(yaw)*sinf(pitch),-sinf(yaw)*sinf(pitch),cosf(pitch),0};
        float focus[3]={player[0],player[1],player[2]+1.6f+jump};
        // Recompute on camera movement or collision residency changes; a neutral
        // frame does not repeatedly scan the same building triangles.
        static float camera_key[5],distance=0;static unsigned camera_loads=UINT32_MAX,camera_count=UINT32_MAX;
        float key[5]={focus[0],focus[1],focus[2],yaw,pitch};
        if(memcmp(key,camera_key,sizeof key)||camera_loads!=scene.loads||camera_count!=scene.header.count){
            distance=wx_camera_distance(&scene,focus,forward,right,up,6);memcpy(camera_key,key,sizeof key);camera_loads=scene.loads;camera_count=scene.header.count;
        }
        float camera[4]={focus[0]-forward[0]*distance,focus[1]-forward[1]*distance,focus[2]-forward[2]*distance,1};
        phase_end=GetTickCount();profile[WX_PROFILE_CAMERA]=phase_end-phase_start;phase_start=phase_end;
        unsigned environment_start=GetTickCount();WxEnvironmentSample environment;wx_environment_sample(&scene,camera,&environment);unsigned environment_ms=GetTickCount()-environment_start;
        unsigned light_start=GetTickCount();WxWorldClock clock;WxLightSample atmosphere;WxWeather weather;wx_world_clock(&clock);wx_world_weather(&weather);
        wx_weather_step(&weather_mix,&weather,dt);
        wx_light_weather_sample(&lighting,world.map,camera,wx_clock_half_minutes(&clock,now),weather_mix.weight,environment.environment,&atmosphere);
        if(weather_mix.snap){world_fog=atmosphere.fog;world_light=atmosphere.palette;}
        else {wx_fog_blend(&world_fog,&atmosphere.fog,dt);wx_light_blend(&world_light,&atmosphere.palette,dt);}
        unsigned lighting_ms=GetTickCount()-light_start;
        pb_erase_depth_stencil_buffer(0,0,640,480);pb_fill(0,0,640,480,(in_login||in_lobby)?0xff080b0d:wx_fog_background(&world_fog));wx_font_clear();wx_ui_clear(&interface);

        if(actors_loaded)actor_stream(now,camera,right,forward);
        if(!in_lobby)wx_avatar_select(&avatar,&avatar_selection,&world,"D:\\");
        {const WxEntity* self=self_entity();unsigned clip=death_state()==1?6:move_flags?5:self&&(self->fields[46]&0x80000)?16:0;
            wx_avatar_update(&avatar,&world,clip,now);}
        wx_stream_frame_end();
        phase_end=GetTickCount();profile[WX_PROFILE_ACTORS]=phase_end-phase_start;phase_start=phase_end;
        draws=triangles=0;unsigned sky_quads=0;memset(&draw_profile,0,sizeof draw_profile);
        wx_fog_bind(NULL);
        if(in_login){wx_backdrop_draw(&title_scene);draws=title_scene.draws;triangles=title_scene.triangles;}
        else if(in_lobby)preview_render();
        else{
            unsigned sky_start=GetTickCount();sky_quads=wx_sky_build(&interface,&world_light,&world_fog,right,up,forward);
            wx_ui_draw(&interface);while(pb_busy()){}wx_ui_clear(&interface);shaders();lighting_ms+=GetTickCount()-sky_start;
            draw_profile_begin();
            if(loaded)render(&scene,camera,right,up,forward,UINT32_MAX,1,0,NULL,NULL);
            draw_profile_pass(&draw_profile.world_ms);
            if(actors_loaded)actor_render(camera,right,up,forward);
            draw_profile_pass(&draw_profile.actors_ms);
            if(world.active&&distance>1.3f)player_render(camera,right,up,forward,yaw,jump);
            draw_profile_pass(&draw_profile.player_ms);
            if(loaded)render_liquids(&scene,camera,right,up,forward);
            draw_profile_pass(&draw_profile.liquid_ms);draw_profile_end();
        }
        wx_fog_bind(NULL);
        phase_end=GetTickCount();profile[WX_PROFILE_DRAW]=phase_end-phase_start;phase_start=phase_end;
        unsigned free=wx_free_memory();if(free<minfree)minfree=free;
        if(diagnostics||menu){
        wx_font_print("\n  WOWX / Vanilla 1.12.1 / Xbox 64 MB\n");
        wx_font_print("  %s / development build\n",world.active?"ONLINE WORLD":"WORLD VIEWER");
        if(!menu){
        wx_font_print("  FPS %u  draws %u  triangles %u\n",fps,draws,triangles);
        wx_font_print("  Free %u KB  minimum %u KB\n",free/1024,minfree/1024);
        wx_font_print("  Cache %u KB  loads %u  failures %u\n",(lighting.bytes+scene.bytes+actors.bytes+avatar.scene.bytes+avatar.bytes+wx_font_bytes()+world_map.bytes+interface.bytes+action_icons.bytes+cooldown_catalog.bytes+title_scene.bytes+preview.scene.bytes+preview.avatar.bytes+preview.avatar.scene.bytes)/1024,scene.loads+actors.loads+avatar.scene.loads,lighting.failures+scene.failures+actors.failures+avatar.failures+avatar.scene.failures+interface.failures+action_icons.failures+cooldown_catalog.failures+title_scene.failures+preview.scene.failures+preview.avatar.failures+preview.avatar.scene.failures);
        wx_font_print("  Pad %s  layer %u  last action %d\n",pad.connected?"connected":"absent",pad.layer,last_action);
        wx_font_print("  Local server: %s\n",wx_auth_status());
        wx_font_print("  World: %s\n",wx_world_status());
        wx_font_print("  Movement packets %u / received %u\n",world.sent,world.received);
        wx_font_print("  NPCs %u pending %u / avatar %u missing %u\n",actor_drawn,actor_missing,avatar.drawn,avatar.missing);
        }
        if(wx_pad_replay_active())wx_font_printat(0,2,"AUTOMATED INPUT REPLAY / frame %u",wx_pad_replay_frame());
        if(!loaded||scene.error[0])wx_font_print("  %s\n",scene.error);
        if(regional&&region.error[0])wx_font_print("  %s\n",region.error);
        if(!actors_loaded||actors.error[0])wx_font_print("  %s\n",actors.error);
        if(avatar.error[0])wx_font_print("  %s\n",avatar.error);
        if(menu){
            if(!world.active)wx_font_print("  Login: %s\n",wx_auth_status());
            wx_font_print("  %s\n",wx_world_status());
            wx_font_print("\n  CONTROLS / Start or B: return\n  D-pad up/down: dead zone %u%%\n",(unsigned)(wx_pad_deadzone()*100+.5f));
            wx_font_print("  Triggers + button: choose binding\n  D-pad left/right: change action\n  Binding %d -> action %d\n",binding_slot+1,wx_pad_binding(binding_slot)+1);
            wx_font_print("  X: save  %s\n",saved>0?"Saved":saved<0?"Save failed":"");
            wx_font_print("  Back: %s local test character\n",world.active?"log out":"reconnect");
            wx_font_print("  Y: choose or create a character\n");
            wx_font_print("  A: quest journal  White: spellbook\n");
        }
        }
        if(!diagnostics&&!menu){
            if(scene.error[0]||!actors_loaded||actors.error[0]||(!loaded&&(!regional||region.error[0])))wx_font_print("  ASSET ERROR / press right stick for details\n");
            else if(!loaded)wx_font_print("  Loading terrain...\n");
        }
        if(!diagnostics&&!menu&&!world.active){wx_font_printat(1,2,"Local realm: %s",wx_auth_status());wx_font_printat(2,2,"World: %s",wx_world_status());wx_font_printat(4,2,"Start: connection menu");}
        if(world.active&&!menu&&!diagnostics&&!avatar.matched)wx_font_printat(2,2,"Player appearance not prepared");
        unsigned actionbar_layer=world.active&&!menu&&!diagnostics&&!in_lobby&&!inventory_open&&!journal.open&&!spell_ui.open&&!game.dialog.screen&&!death_state()?pad.layer:0;
        WxActionPrompt action_prompts[8]={{0}};
        memset(&hud,0,sizeof hud);memset(portrait_stats,0,sizeof portrait_stats);
        if(world.active&&!menu&&!diagnostics&&!in_lobby&&!inventory_open&&!journal.open&&!spell_ui.open&&!game.dialog.screen&&!world_map.open&&!utility.open){
            const WxEntity* self=self_entity();const WxEntity* target=target_entity();
            wx_hud_unit(&hud.player,self,world.name);
            wx_hud_unit(&hud.target,target,target&&game.name_entry==target->fields[3]?game.target_name:NULL);
            if(self){hud.xp=self->xp;hud.next_xp=self->next_xp;}
            wx_hud_draw(&hud,&interface);
        }
        if(world.active&&!menu)gameplay_hud(world.guid,world.map,camera,right,up,forward,actionbar_layer,hud.drawn);
        hud_portraits();
        memset(action_feedback,0,sizeof action_feedback);
        if(world.active&&!menu&&!in_lobby&&!world_map.open&&!journal.open&&!spell_ui.open&&!inventory_open&&!game.dialog.screen)wx_cast_draw(&interface,&cast_view,&spellbook);
        memset(action_cooldowns,0,sizeof action_cooldowns);wx_world_cooldowns(NULL,NULL,cooldown_metrics);
        if(actionbar_layer){for(unsigned i=0;i<8;i++)wx_action_prompt(&spellbook,&game,spell_bindings,spell_form,(actionbar_layer-1)*8+i,&action_prompts[i]);
            uint32_t cd_bindings[8];for(unsigned i=0;i<8;i++)cd_bindings[i]=action_prompts[i].binding;
            wx_world_cooldowns(cd_bindings,action_cooldowns,cooldown_metrics);
            WxActionModifiers modifiers[8];wx_world_action_modifiers(cd_bindings,modifiers);
            for(unsigned i=0;i<8;i++)action_feedback[i]=wx_action_feedback(cd_bindings[i],&spellbook,&game,&inventory,self_entity(),target_entity(),player,&modifiers[i]);
            if(!wx_actionbar_ui(&interface,&action_icons,&game,action_prompts,actionbar_layer,pad.buttons,action_cooldowns,action_feedback))wx_actionbar_draw(action_prompts,actionbar_layer,pad.buttons);}
        if(world.active&&!menu){if(game.dialog.screen==WX_SCREEN_VENDOR)wx_vendor_draw(&inventory,&game);else wx_dialog_draw(&game,&world);}
        if(inventory_open&&!menu)wx_inventory_draw(&inventory,&game);
        if(journal.open&&!menu)wx_journal_draw(&journal,self_entity(),&inventory,&world,now);
        if(spell_ui.open&&!menu)wx_spell_ui_draw(&spell_ui,&spellbook,&game,spell_bindings,spell_form);
        wx_utility_draw(&utility);
        wx_map_draw(&world_map);
        if(in_lobby){wx_character_ui_draw(&character_ui,&character_lobby,&interface);
            if(preview.active&&interface.ready&&character_ui.screen<=WX_CHARACTER_CREATE){
                if(preview.avatar.failures)wx_ui_center(&interface,0,character_ui.screen==WX_CHARACTER_LIST?215:416,375,0xffffc080,"Appearance is not prepared");
                else if(preview.avatar.missing)wx_ui_center(&interface,0,character_ui.screen==WX_CHARACTER_LIST?215:416,375,0xffffc080,"Some equipment is not prepared");
                if(character_ui.screen==WX_CHARACTER_CREATE&&!preview.outfit_ready)wx_ui_center(&interface,0,416,376,0xffffc080,"Starter outfit unavailable");
                if(preview.background_missing)wx_ui_center(&interface,0,character_ui.screen==WX_CHARACTER_LIST?215:416,398,0xffbfb9a8,"Racial background pending");
                else if(preview.scene.load_phase||preview.avatar.opening.phase||(!preview.avatar.ready&&preview.avatar.compose.phase))wx_ui_center(&interface,0,character_ui.screen==WX_CHARACTER_LIST?215:416,398,0xffbfb9a8,"Loading character scene...");
            }}
        if(in_login)wx_login_draw(&login_ui,&login_view,&login_realms,&interface);
        if(scenario.enabled)wx_font_printat(0,2,"AUTOMATED %s TEST: %s",scenario.enabled==17?"SOAK":scenario.enabled==16?"CORPSE":scenario.enabled==3?"TILE":scenario.enabled==4?"QUEST":scenario.enabled==5?"DEATH":scenario.enabled==6?"VENDOR":scenario.enabled==7?"GEAR":scenario.enabled==8?"CAMERA":scenario.enabled==9?"CHARACTER":scenario.enabled==15?"WALK":scenario.enabled==14?"QUEST LOOP":scenario.enabled==13?"AVATAR SWITCH":scenario.enabled==10?"STARTER QUEST":scenario.enabled==11?"CAMP QUEST":scenario.enabled==12?"SPELLBOOK":"COMBAT",wx_scenario_status(&scenario));
        // Debug text expands to many rectangle commands; give it its own
        // drained buffer instead of appending it to a dense 3D scene.
        phase_end=GetTickCount();profile[WX_PROFILE_UI]=phase_end-phase_start;phase_start=phase_end;
        unsigned text_stats[8]={0},text_start=phase_start;
        while(pb_busy()){}phase_end=GetTickCount();text_stats[0]=phase_end-text_start;text_start=phase_end;
        draw_profile.drain_ms=text_stats[0];
        pb_reset();wx_font_draw();wx_ui_draw(&interface);phase_end=GetTickCount();text_stats[1]=phase_end-text_start;text_start=phase_end;
        while(pb_busy()){}phase_end=GetTickCount();text_stats[2]=phase_end-text_start;text_start=phase_end;
        while(pb_finished()){}phase_end=GetTickCount();text_stats[3]=phase_end-text_start;
        profile[WX_PROFILE_PRESENT]=phase_end-phase_start;profile[WX_PROFILE_WORK]=phase_end-now;
        text_stats[4]=wx_font_glyphs();text_stats[5]=wx_font_mode();text_stats[6]=wx_font_bytes();text_stats[7]=wx_font_failures();
        WxFrameStats stats={now,frame_ms,free,lighting.bytes+scene.bytes+actors.bytes+avatar.scene.bytes+avatar.bytes+wx_font_bytes()+world_map.bytes+interface.bytes+action_icons.bytes+cooldown_catalog.bytes+title_scene.bytes+preview.scene.bytes+preview.avatar.bytes+preview.avatar.scene.bytes,draws,triangles,scene.loads+actors.loads+avatar.scene.loads,lighting.failures+scene.failures+actors.failures+avatar.failures+avatar.scene.failures+interface.failures+action_icons.failures+cooldown_catalog.failures+title_scene.failures+preview.scene.failures+preview.avatar.failures+preview.avatar.scene.failures,
            player[0],player[1],player[2],yaw,pitch,pad,wx_pad_replay_frame(),wx_pad_replay_active(),menu,saved,wx_pad_deadzone(),actor_drawn,actor_missing,wx_world_entity_count(),
            game.dialog.screen,game.dialog.quest,game.completed_quest,game.kills,game.damage_dealt,game.damage_received,0,0,scenario.stage,scenario.frame,game.looted_items,game.looted_money,game.casts_accepted,game.casts_rejected,game.spell_hits,game.last_spell,inventory.count,0,0,0,0,0,0,0,0,0,0,0,0,0,scenario.enabled,
            avatar.matched,avatar.ready,avatar.drawn,avatar.missing,avatar.revision,avatar.bytes+avatar.scene.bytes,avatar.equipment_mask,(unsigned)(distance*1000),avatar.ready?avatar.ids[0]&255:UINT32_MAX,avatar.pose_hash,
            character_lobby.phase,character_lobby.characters.count,character_ui.screen,character_ui.selected,character_ui.row,character_ui.key,character_lobby.result,character_lobby.result_revision,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,{0},{0},0,0,0,0,0,0,{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}};
        stats.cache_bytes+=scene.index_bytes+actors.index_bytes+region.count*sizeof(WxRegionCell)+wx_pack_pending_bytes(&region.pending);
        stats.journal_open=journal.open;stats.journal_detail=journal.detail;stats.journal_story=journal.story;
        stats.actionbar_layer=actionbar_layer;for(unsigned i=0;i<8;i++){stats.actionbar_slots[i]=action_prompts[i].server_slot;stats.actionbar_bindings[i]=action_prompts[i].binding;}
        stats.utility_open=utility.open;stats.utility_pending=utility.pending;stats.utility_selection=utility.selection;
        stats.utility_action=utility.action;stats.utility_revision=utility.revision;stats.inventory_open=inventory_open;
        memcpy(stats.profile,profile,sizeof profile);memcpy(stats.text,text_stats,sizeof text_stats);
        stats.map_ui[0]=world_map.open;stats.map_ui[1]=world_map.count?world_map.entries[world_map.selected].id:0;stats.map_ui[2]=world_map.ready;
        stats.map_ui[3]=world_map.loads;stats.map_ui[4]=world_map.failures;stats.map_ui[5]=world_map.bytes;stats.map_ui[6]=(unsigned)(world_map.zoom*1000);
        stats.map_ui[7]=(unsigned)(world_map.u*100000);stats.map_ui[8]=(unsigned)(world_map.v*100000);stats.map_ui[9]=world_map.marker;
        stats.map_ui[10]=world_map.marker?(unsigned)(world_map.player_u*100000):0;stats.map_ui[11]=world_map.marker?(unsigned)(world_map.player_v*100000):0;
        stats.map_ui[12]=world_map.overlays;stats.map_ui[13]=world_map.revision;
        stats.death[0]=death.state;stats.death[1]=death.known;stats.death[2]=death.map;stats.death[3]=(unsigned)death.corpse;stats.death[4]=(unsigned)(death.corpse>>32);
        stats.death[5]=(unsigned)(death.distance*1000);stats.death[6]=death.remaining_ms;stats.death[7]=death.ready;
        stats.death[8]=death.queries;stats.death[9]=death.releases;stats.death[10]=death.reclaims;stats.death[11]=death.healers;stats.death[12]=death.resurrections;
        stats.death[13]=world_map.body;stats.death[14]=world_map.body?(unsigned)(world_map.body_u*100000):0;stats.death[15]=world_map.body?(unsigned)(world_map.body_v*100000):0;
        stats.death[16]=death.query_attempts;stats.death[17]=death.in_range;
        stats.ui[0]=interface.ready;stats.ui[1]=interface.bytes;stats.ui[2]=interface.quads;stats.ui[3]=interface.failures;
        stats.login[0]=login_view.mode;stats.login[1]=login_view.phase;stats.login[2]=wx_auth_state();stats.login[3]=login_realms.count;
        stats.login[4]=login_ui.selected;stats.login[5]=login_ui.keyboard;stats.login[6]=login_ui.field;
        stats.login[7]=login_view.attempts;stats.login[8]=login_view.cancellations;stats.login[9]=login_replay.stage;stats.login[10]=login_replay.frame;
        stats.backdrop[0]=title_scene.ready;stats.backdrop[1]=title_scene.bytes;stats.backdrop[2]=title_scene.draws;stats.backdrop[3]=title_scene.triangles;
        stats.backdrop[4]=title_scene.updates;stats.backdrop[5]=title_scene.failures;stats.backdrop[6]=title_scene.loads;stats.backdrop[7]=title_scene.time_ms;
        stats.effects[0]=title_scene.effects.emitters;stats.effects[1]=title_scene.effects.lights;
        if(title_scene.simulation){const WxBackdropSimulation* fx=title_scene.simulation;
            stats.effects[2]=fx->active;stats.effects[3]=fx->peak;stats.effects[4]=fx->born;stats.effects[5]=fx->dropped;stats.effects[6]=fx->clock_skips;stats.effects[7]=fx->quads;}

        stats.preview[0]=preview.active;stats.preview[1]=preview.scene.ready;stats.preview[2]=preview.avatar.ready;
        stats.cache_bytes+=wx_avatar_pending_bytes(&avatar)+wx_avatar_pending_bytes(&preview.avatar);
        stats.avatar_bytes+=wx_avatar_pending_bytes(&avatar);
        stats.preview[3]=preview.avatar.drawn;stats.preview[4]=preview.scene.bytes+preview.avatar.bytes+preview.avatar.scene.bytes+wx_avatar_pending_bytes(&preview.avatar);
        stats.preview[5]=preview.subject.race|(preview.subject.gender<<8);stats.preview[6]=preview.avatar.pose_hash;
        stats.preview[7]=preview.changes;stats.preview[8]=preview.background_missing;stats.preview[9]=preview.avatar.missing;
        stats.preview[10]=preview.scene.failures+preview.selection.failures+preview.avatar.failures+preview.avatar.scene.failures;
        stats.preview[11]=(unsigned)((preview.rotation+3.141592654f)*1000);stats.preview[12]=(unsigned)(preview.zoom*1000);
        stats.preview[13]=preview.subject.character_class;stats.preview[14]=preview.outfit_ready;
        stats.preview[15]=preview.scene.load_phase;stats.preview[16]=preview.scene.load_read;stats.preview[17]=preview.avatar.scene.animation_index_reads;
        if(preview.active&&preview.avatar.ready&&preview.avatar.appearance_ready){
            const uint32_t* look=preview.avatar.header.look;
            stats.customization[0]=look[2]|(look[3]<<8)|(look[4]<<16)|(look[5]<<24);
            stats.customization[1]=look[6];stats.customization[2]=preview.avatar.revision;stats.customization[3]=preview.avatar.atlas_hash;
        }
        stats.customization[4]=character_ui.appearance_row;stats.customization[5]=character_ui.randomizations;
        unsigned icon_metrics[]={action_icons.ready,action_icons.count,action_icons.images,action_icons.bytes,action_icons.loads,action_icons.pending,action_icons.failures,action_icons.drawn,interface.batch_count,icons_ms};
        memcpy(stats.icons,icon_metrics,sizeof icon_metrics);
        wx_hud_metrics(&hud,stats.hud);memcpy(stats.portrait,portrait_stats,sizeof portrait_stats);
        stats.soak[0]=scenario.soak_cycles;stats.soak[1]=scenario.soak_started?now-scenario.soak_begin_ms:0;stats.soak[2]=scenario.soak_limit_ms;stats.soak[3]=scenario.soak_started;
        stats.avatar_select_attempts=avatar_selection.attempts;stats.avatar_profile_changes=avatar_selection.changes;stats.avatar_select_failures=avatar_selection.failures;
        stats.cooldowns[0]=cooldown_catalog.ready;stats.cooldowns[1]=cooldown_catalog.count;stats.cooldowns[2]=cooldown_catalog.bytes;
        memcpy(stats.cooldowns+3,cooldown_metrics,5*sizeof(unsigned));
        memcpy(stats.cooldowns+18,cooldown_metrics+5,5*sizeof(unsigned));stats.cooldowns[24]=cooldown_metrics[10];stats.cooldowns[25]=cooldown_metrics[11];
        for(unsigned i=0;i<8;i++){stats.cooldowns[8+i]=action_cooldowns[i].remaining;if(action_cooldowns[i].held)stats.cooldowns[16]|=1u<<i;if(action_cooldowns[i].global)stats.cooldowns[23]|=1u<<i;}
        stats.book_open=spell_ui.open;stats.book_screen=spell_ui.screen;stats.book_count=spellbook.count;stats.book_loaded=spellbook.loaded;stats.book_failures=spellbook.failures;
        const WxKnownSpell* observed_spell=wx_spellbook_selected(&spellbook,spell_ui.selected);stats.book_spell=observed_spell?observed_spell->id:0;stats.book_slot=spell_ui.slot;
        stats.book_server_slot=wx_action_slot(spell_bindings[spell_ui.slot<24?spell_ui.slot:0],spell_form);
        stats.book_binding=stats.book_server_slot<120?game.actions[stats.book_server_slot]:0;stats.action_revision=game.action_revision;
        if(journal.open&&journal.selected<journal.count){static WxQuestInfo observed;
            stats.journal_id=journal.ids[journal.selected];stats.journal_ready=wx_world_quest(stats.journal_id,&observed);
            if(stats.journal_ready){WxQuestProgress progress;wx_quest_progress(&observed,self_entity(),&inventory,&progress);stats.journal_progress=progress.creatures[0];}
        }
        for(unsigned i=0;i<inventory.count;i++)if(inventory.items[i].bag==255&&inventory.items[i].slot==16)stats.equipped_shield=inventory.items[i].entry;
        for(unsigned i=0;i<world_entity_count;i++)if(actor_entities[i].guid==world.guid){
            stats.player_health=actor_entities[i].fields[22];stats.player_flags=actor_entities[i].fields[190];
            stats.player_level=actor_entities[i].fields[34];
            stats.player_xp=actor_entities[i].xp;for(unsigned j=0;j<20;j++)if(actor_entities[i].quests[j*3])stats.active_quests++;break;
        }
        stats.position_revision=world.position_revision;stats.inventory_money=inventory.money;stats.vendor_count=game.dialog.vendor_count;
        stats.bundles_bought=game.bundles_bought;stats.items_sold=game.items_sold;
        stats.region_loaded=region.loaded;stats.region_transitions=region.transitions;stats.region_failures=region.failures;
        if(regional&&region.selected>=0){stats.tile_x=region.cells[region.selected].x;stats.tile_y=region.cells[region.selected].y;}
        wx_telemetry_send(&stats);
        wx_telemetry_environment(now,frames,0,0,0,UINT32_MAX,UINT32_MAX,&environment,&scene,environment_ms,0,camera);
        wx_telemetry_lighting(now,frames,0,0,wx_clock_half_minutes(&clock,now),environment.environment==WX_FOG_UNDERWATER?WX_LIGHT_WATER:weather_mix.weight>0?WX_LIGHT_RAIN:WX_LIGHT_CLEAR,&lighting,&atmosphere,&world_fog,&world_light,lighting_ms,sky_quads);
        wx_telemetry_weather(now,frames,0,0,&weather,&weather_mix,WX_FOG_OUTDOOR,0);
        wx_telemetry_stream(now,frames,0,0,0,&scene,&region,region_ms,stream_ms,0,0,world_fog.enabled);
        if(!in_login&&!in_lobby)wx_telemetry_draw(now,frames,0,2,&draw_profile);
        if(world.active)wx_telemetry_actions(now,stats.replay_frame,0,actionbar_layer,&cast_view,action_feedback,spellbook.version);
        if(world.active)wx_telemetry_actor_stream(now,frames,0,avatar.complete_clip<256?avatar.complete_clip:0,&actors,&avatar,0,0,actor_drawn,profile[WX_PROFILE_ACTORS],0,0);
        frames++;fps_frames++;if(now-fps_start>=1000){fps=fps_frames;fps_frames=0;fps_start=now;}
    }
}
