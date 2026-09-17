#include "wx_lighting.h"
#include "wx_telemetry.h"
#include "wx_auth.h"
#include "wx_world.h"
#include "wx_cast.h"
#include "wx_actions.h"
#include <lwip/sockets.h>
#include <string.h>
static int descriptor=-1;
void wx_telemetry_draw(unsigned now,unsigned frame,unsigned fixture,unsigned scope,const WxDrawStats* s){
    if(descriptor<0||!s)return;
    _Static_assert(sizeof(WxDrawStats)==13*4,"Draw telemetry layout");
    uint32_t words[18]={0x31445857,now,frame,fixture,scope};memcpy(words+5,s,sizeof *s);
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
}
void wx_telemetry_material(unsigned now,unsigned frame,const unsigned counts[7],unsigned flags,unsigned fog,unsigned motion_draws,unsigned motion_hash,unsigned liquid_draws,unsigned sequence_draws,unsigned sequence_frames){
    if(descriptor<0)return;uint32_t words[18]={0x334d5857,now,frame,13};
    memcpy(words+4,counts,28);words[11]=flags;words[12]=fog;
    words[13]=motion_draws;words[14]=motion_hash;
    words[15]=liquid_draws;words[16]=sequence_draws;words[17]=sequence_frames;
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
}
void wx_telemetry_weather(unsigned now,unsigned frame,unsigned fixture,unsigned phase,const WxWeather* s,const WxWeatherMix* mix,unsigned environment,unsigned errors){
    if(descriptor<0||!s||!mix)return;
    uint32_t words[17]={0x31575857,now,frame,fixture,phase,s->type,s->sound,s->instant,s->revision,s->valid,mix->revision,mix->type,mix->snap,environment,errors};
    memcpy(words+15,&s->grade,4);memcpy(words+16,&mix->weight,4);
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
}
void wx_telemetry_send(const WxFrameStats* s){
    if(!wx_network_ready())return;
    if(descriptor<0){descriptor=socket(AF_INET,SOCK_DGRAM,0);if(descriptor<0)return;unsigned long nonblocking=1;ioctlsocket(descriptor,FIONBIO,&nonblocking);}
    // Original Xbox is little-endian IEEE-754. No target printf float support is required.
    uint32_t words[368]={0x5a545857,s->time_ms,s->frame_ms,s->free_bytes/1024,s->cache_bytes/1024,s->draws,s->triangles,s->loads,s->failures};
    memcpy(words+9,&s->x,5*sizeof(float));
    words[14]=s->pad.buttons;words[15]=s->pad.pressed;words[16]=s->pad.layer;
    words[17]=(uint32_t)s->pad.action;words[18]=(uint32_t)s->pad.slot;words[19]=s->replay_frame;
    words[20]=(uint32_t)s->replay_active;words[21]=(uint32_t)s->menu;words[22]=(uint32_t)s->saved;
    memcpy(words+23,&s->deadzone,sizeof(float));
    WxWorldView world;wx_world_view(&world);
    words[24]=world.active;words[25]=world.revision;words[26]=world.sent;words[27]=world.received;words[28]=world.map;
    if(s->portrait[9]==13)words[28]=s->fixture_map;
    memcpy(words+29,&world.x,4*sizeof(float));
    words[33]=s->npc_submitted;words[34]=s->npc_missing;words[35]=s->entity_count;
    words[36]=s->quest_screen;words[37]=s->quest_id;words[38]=s->completed_quest;words[39]=s->kills;
    words[40]=s->damage_dealt;words[41]=s->damage_received;words[42]=s->player_xp;words[43]=s->active_quests;
    words[44]=s->scenario_stage;words[45]=s->scenario_frame;words[46]=s->looted_items;words[47]=s->looted_money;
    words[48]=s->casts_accepted;words[49]=s->casts_rejected;words[50]=s->spell_hits;words[51]=s->last_spell;words[52]=s->inventory_count;words[53]=s->equipped_shield;
    words[54]=s->region_loaded;words[55]=s->region_transitions;words[56]=s->tile_x;words[57]=s->tile_y;words[58]=s->region_failures;
    words[59]=s->player_health;words[60]=s->player_flags;words[61]=s->position_revision;words[62]=s->inventory_money;
    words[63]=s->vendor_count;words[64]=s->bundles_bought;words[65]=s->items_sold;
    words[66]=world.appearance_revision;words[67]=world.equipment_ready_mask;
    words[68]=world.race|((uint32_t)world.character_class<<8)|((uint32_t)world.gender<<16);
    words[69]=world.skin|((uint32_t)world.face<<8)|((uint32_t)world.hair_style<<16)|((uint32_t)world.hair_color<<24);
    words[70]=world.facial_hair;words[71]=world.appearance_flags;words[72]=world.display_id;words[73]=world.native_display_id;
    for(unsigned i=0;i<19;i++){words[74+i*3]=world.equipment_entry[i];words[75+i*3]=world.equipment_display[i];words[76+i*3]=world.equipment_type[i];}
    words[131]=s->scenario_kind;
    words[132]=s->avatar_matched;words[133]=s->avatar_ready;words[134]=s->avatar_drawn;words[135]=s->avatar_missing;
    words[136]=s->avatar_revision;words[137]=s->avatar_bytes;words[138]=s->avatar_equipment;words[139]=s->camera_mm;
    words[140]=s->avatar_clip;words[141]=s->avatar_pose;
    words[142]=s->lobby_phase;words[143]=s->lobby_count;words[144]=s->character_screen;words[145]=s->character_selected;
    words[146]=s->character_row;words[147]=s->character_key;words[148]=s->character_result;words[149]=s->character_result_revision;
    words[150]=(uint32_t)world.guid;words[151]=(uint32_t)(world.guid>>32);words[152]=s->player_level;
    words[153]=(uint32_t)s->pad.connected;
    words[154]=s->journal_open;words[155]=s->journal_detail;words[156]=s->journal_story;
    words[157]=s->journal_id;words[158]=s->journal_ready;words[159]=s->journal_progress;
    words[160]=s->book_open;words[161]=s->book_screen;words[162]=s->book_count;words[163]=s->book_loaded;words[164]=s->book_failures;
    words[165]=s->book_spell;words[166]=s->book_slot;words[167]=s->book_server_slot;words[168]=s->book_binding;words[169]=s->action_revision;
    words[170]=s->avatar_select_attempts;words[171]=s->avatar_profile_changes;words[172]=s->avatar_select_failures;
    words[173]=s->actionbar_layer;for(unsigned i=0;i<8;i++){words[174+i]=s->actionbar_slots[i];words[182+i]=s->actionbar_bindings[i];}
    words[190]=s->utility_open;words[191]=s->utility_pending;words[192]=s->utility_selection;
    words[193]=s->utility_action;words[194]=s->utility_revision;words[195]=s->inventory_open;
    memcpy(words+196,s->profile,sizeof s->profile);
    memcpy(words+206,s->text,sizeof s->text);
    memcpy(words+214,s->map_ui,sizeof s->map_ui);
    memcpy(words+228,s->death,sizeof s->death);
    memcpy(words+246,s->soak,sizeof s->soak);
    memcpy(words+250,s->ui,sizeof s->ui);
    memcpy(words+254,s->login,sizeof s->login);
    memcpy(words+265,s->backdrop,sizeof s->backdrop);
    memcpy(words+273,s->effects,sizeof s->effects);
    memcpy(words+281,s->preview,sizeof s->preview);
    memcpy(words+299,s->customization,sizeof s->customization);
    memcpy(words+305,s->hud,sizeof s->hud);
    memcpy(words+322,s->portrait,sizeof s->portrait);
    memcpy(words+332,s->icons,sizeof s->icons);
    memcpy(words+342,s->cooldowns,sizeof s->cooldowns);
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
    static unsigned fault_revision=0;
    WxWorldFault fault;wx_world_fault(&fault);
    if(fault.revision!=fault_revision){
        uint32_t diagnostic[198]={0x31455857,fault.revision,fault.reason,fault.opcode,fault.size,fault.captured};
        memcpy(diagnostic+6,fault.data,fault.captured);
        sendto(descriptor,diagnostic,24+fault.captured,0,(struct sockaddr*)&host,sizeof host);
        fault_revision=fault.revision;
    }
}
#include "wx_region.h"
#include "wx_avatar.h"
void wx_telemetry_profile(unsigned now,unsigned frame,unsigned stage,unsigned cycles,unsigned completed,unsigned expected_failures,unsigned errors,unsigned work_ms,unsigned drawn,const WxAvatarSelection* selection,const WxAvatar* a){
    if(descriptor<0)return;
    uint32_t v[53]={0x31515857,now,frame,12,stage,cycles,completed,expected_failures,errors,work_ms,drawn,
        a->opening.phase,a->opening.pack.phase,a->profile_ready,a->ready,a->header.look[0],a->header.look[1],selection->look[0],selection->look[1],
        selection->attempts,selection->changes,selection->failures,selection->pending,selection->cancelled,
        a->opening.read_bytes,a->opening.read_ops,a->opening.scan_bytes,wx_avatar_pending_bytes(a),a->bytes+a->scene.bytes,
        a->scene.animation_index_reads,a->compose.phase,a->compose.read_bytes,a->compose.read_ops,a->compose.work_pixels,
        a->failures+a->scene.failures+a->compose.failures,a->count,a->atlas_hash};
    // All lanes are included so other staged consumers cannot be silently omitted.
    const WxStreamFrame* budget=wx_stream_frame_metrics();v[37]=budget->enabled;
    memcpy(v+38,&budget->total,16);memcpy(v+42,&budget->lane[WX_STREAM_AVATAR],16);
    v[46]=a->scene.streaming.read_bytes;v[47]=a->scene.streaming.read_ops;v[48]=a->scene.streaming.scan_bytes;
    v[49]=a->scene.index_bytes;v[50]=a->opening.pack.animation_count;v[51]=a->opening.pack.animation_reads;v[52]=a->compose.failures;
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,v,sizeof v,0,(struct sockaddr*)&host,sizeof host);
}
void wx_telemetry_actor_stream(unsigned now,unsigned frame,unsigned fixture,unsigned clip,const WxScene* actors,const WxAvatar* avatar,unsigned ready,unsigned fallback,unsigned drawn,unsigned actor_ms,unsigned gaps,unsigned switches){
    if(descriptor<0)return;
    uint32_t words[59]={0x334e5857,now,frame,fixture,clip,ready,fallback,drawn,avatar->ready,avatar->complete_clip,actor_ms,avatar->scene.loads,actors->loads};
    memcpy(words+13,&actors->streaming,40);memcpy(words+23,&avatar->scene.streaming,40);
    words[33]=actors->bytes;words[34]=avatar->scene.bytes+avatar->bytes;words[35]=actors->failures;words[36]=avatar->failures+avatar->scene.failures;
    words[37]=gaps;words[38]=switches;words[39]=actors->index_bytes;words[40]=avatar->scene.stream_job.phase;
    _Static_assert(sizeof(WxAnimationMetrics)==16,"Animation metrics changed");
    memcpy(words+41,&actors->animating,16);memcpy(words+45,&avatar->scene.animating,16);
    const WxAvatarCompose* j=&avatar->compose;
    words[49]=j->phase;words[50]=j->read_bytes;words[51]=j->read_ops;words[52]=j->work_pixels;
    words[53]=j->cancelled;words[54]=j->commits;words[55]=j->failures;words[56]=avatar->atlas_hash;words[57]=avatar->revision;words[58]=j->layer;
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
}
void wx_telemetry_stream(unsigned now,unsigned frame,unsigned fixture,unsigned stage,unsigned cycles,const WxScene* s,const WxRegion* r,unsigned region_ms,unsigned stream_ms,unsigned waits,unsigned errors,unsigned fog){
    if(descriptor<0)return;
    _Static_assert(sizeof(WxStreamMetrics)==40,"Streaming metrics changed");
    uint32_t words[76]={0x35535857,now,frame,fixture,stage,cycles,(unsigned)wx_stream_ready(s),s->stream_job.phase?(unsigned)s->stream_job.entry+1:0,s->stream_job.phase};
    memcpy(words+9,&s->streaming,sizeof s->streaming);
    words[19]=region_ms;words[20]=stream_ms;words[21]=waits;words[22]=errors;words[23]=r->pending.phase;words[24]=fog;
    words[25]=r->pending.read_bytes;words[26]=r->pending.read_ops;words[27]=r->pending.checked_frame;
    words[28]=wx_pack_pending_bytes(&r->pending);words[29]=s->index_bytes;
    words[30]=s->bytes;words[31]=r->pending.checked;
    _Static_assert(sizeof(WxStreamFrame)==84,"Frame quota metrics changed");
    memcpy(words+32,wx_stream_frame_metrics(),84);
    _Static_assert(sizeof(WxStreamTime)==40,"Streaming time metrics changed");
    memcpy(words+53,wx_stream_time_metrics(),40);
    words[63]=wx_stream_index_copy_bytes();words[64]=r->pending.join_total;words[65]=r->pending.join_progress;
    words[66]=r->pending.joined?r->pending.join_total:0;words[67]=r->pending.join_restarts;
    words[68]=wx_stream_selection_entries();words[69]=s->selection.phase;
    words[70]=s->selection.progress;words[71]=s->selection.count;
    words[72]=s->selection.commits;words[73]=s->selection.restarts;
    words[74]=s->selection.revision;words[75]=s->index_revision;
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
}
void wx_telemetry_actions(unsigned now,unsigned frame,unsigned fixture,unsigned step,const WxCastView* cast,const WxActionFeedback* actions,unsigned version){
    if(descriptor<0||!cast||!actions)return;
    _Static_assert(sizeof(WxCastView)==32&&sizeof(WxActionFeedback)==24,"Action telemetry layout changed");
    uint32_t words[66]={0x31465857,now,frame,fixture,step}; // WXF1, separate packet: WXTZ already fills the MTU
    memcpy(words+5,cast,32);for(unsigned i=0;i<8;i++)memcpy(words+13+i*6,actions+i,24);
    words[61]=version;words[62]=sizeof(WxCastState);words[63]=sizeof(WxSpellBook);words[64]=sizeof(WxCooldowns);words[65]=sizeof(WxInventory);
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
}

void wx_telemetry_lighting(unsigned now,unsigned frame,unsigned fixture,unsigned phase,float time,unsigned condition,const WxLighting* c,const WxLightSample* s,const WxFog* f,const WxLightPalette* palette,unsigned light_ms,unsigned sky_quads){
    if(descriptor<0)return;
    uint32_t words[34]={0x334c5857,now,frame,fixture,phase,condition,s->authored,s->volume[0],s->volume[1],s->profile[0],s->profile[1],s->profile[2],c->bytes,c->failures,f->enabled,f->environment,wx_fog_background(f),light_ms};
    memcpy(words+18,&time,4);memcpy(words+19,&f->start,4);memcpy(words+20,&f->end,4);
    words[21]=palette->mask;words[22]=sky_quads;
    for(unsigned b=0;b<WX_LIGHT_COLORS;b++)for(unsigned k=0;k<3;k++)words[23+b]|=(uint32_t)(palette->color[b][k]*255+.5f)<<(16-k*8);
    memcpy(words+31,palette->direction,12);
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
}

void wx_telemetry_environment(unsigned now,unsigned frame,unsigned fixture,unsigned stage,unsigned cycles,unsigned expected,unsigned liquid,const WxEnvironmentSample* sample,const WxScene* scene,unsigned sample_ms,unsigned errors,const float* camera){
    if(descriptor<0)return;unsigned bytes=0,pending=0;
    for(unsigned i=0;i<WX_REGION_FILES;i++){bytes+=scene->sources[i].environment.bytes;if(scene->sources[i].file&&scene->sources[i].version>=9&&scene->sources[i].environment.phase!=5)pending++;}
    uint32_t words[23]={0x31595857,now,frame,fixture,stage,cycles,expected,liquid,sample->environment,sample->known,sample->records,sample->liquid_type,sample->group,sample->source,bytes,pending,errors,sample_ms,(unsigned)wx_environment_ready(scene)};
    memcpy(words+19,&sample->depth,4);memcpy(words+20,camera,12);
    struct sockaddr_in host={0};host.sin_family=AF_INET;host.sin_port=htons(39001);host.sin_addr.s_addr=inet_addr("10.0.2.2");
    sendto(descriptor,words,sizeof words,0,(struct sockaddr*)&host,sizeof host);
}
