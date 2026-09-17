#include "wx_stream_scenario.h"
#include "actor_stream_fixture.h"
/* Cold-index, real-asset traversal. No account, entities, pad or server commands.
   Phase 4 explicitly relocates between two independent collision routes. */
static void stream_fixture(unsigned mode){
    int with_actors=mode==8||mode==11;
    wx_network_telemetry_start();wx_scene_init(&scene);
    int installed=wx_region_open(&region,"D:\\world.wxi");
    WxStreamScenario path;wx_stream_scenario_init(&path);wx_fog_fallback(&world_fog,WX_FOG_OUTDOOR);
    WxActorStreamFixture animated={0};if(with_actors)actor_fixture_open(&animated);animated.appearance=mode==11;
    unsigned last=GetTickCount(),presentation=pb_get_vbl_counter();
    for(;;){
        while((unsigned)(pb_get_vbl_counter()-presentation)<2)pb_wait_for_vbl();presentation=pb_get_vbl_counter();
        unsigned now=GetTickCount(),interval=now-last;last=now;while(pb_busy()){}
        wx_stream_frame_begin(1u|(scene.file&&!wx_stream_ready(&scene)?2u:0u)|
            (with_actors&&(!actors.loads||actors.stream_needed)?4u:0u)|
            (with_actors&&(!avatar.scene.loads||avatar.scene.stream_needed||avatar.compose.phase)?8u:0u));
        unsigned start=GetTickCount();if(installed)wx_region_update(&region,&scene,0,path.position);
        unsigned region_ms=GetTickCount()-start;start=GetTickCount();wx_stream(&scene,path.position);wx_animate(&scene,now);
        unsigned stream_ms=GetTickCount()-start;world_fog.enabled=path.phase!=1;
        float angle=path.yaw,tilt=-.08f;
        float forward[4]={cosf(angle)*cosf(tilt),sinf(angle)*cosf(tilt),sinf(tilt),0};
        float right[4]={sinf(angle),-cosf(angle),0,0};
        float up[4]={-cosf(angle)*sinf(tilt),-sinf(angle)*sinf(tilt),cosf(tilt),0};
        float camera[4]={path.position[0],path.position[1],path.position[2]+2,1};
        unsigned actor_ms=0;
        if(with_actors){
            float focus[3]={path.position[0],path.position[1],path.position[2]+1.6f};
            float distance=wx_camera_distance(&scene,focus,forward,right,up,6);
            for(unsigned i=0;i<3;i++)camera[i]=focus[i]-forward[i]*distance;
            start=GetTickCount();actor_fixture_update(&animated,path.frame,now,path.position,camera,right,forward,angle);actor_ms=GetTickCount()-start;
        }
        wx_stream_frame_end();
        pb_reset();pb_target_back_buffer();push_base=NULL;pb_erase_depth_stencil_buffer(0,0,640,480);
        pb_fill(0,0,640,480,wx_fog_background(&world_fog));shaders();draws=triangles=0;wx_font_clear();wx_ui_clear(&interface);
        start=GetTickCount();draw_profile_begin();if(region.loaded)render(&scene,camera,right,up,forward,UINT32_MAX,1,0,NULL,NULL);
        draw_profile_pass(&draw_profile.world_ms);
        if(with_actors)actor_render(camera,right,up,forward);
        draw_profile_pass(&draw_profile.actors_ms);
        if(with_actors)player_render(camera,right,up,forward,angle,0);
        draw_profile_pass(&draw_profile.player_ms);
        if(region.loaded)render_liquids(&scene,camera,right,up,forward);
        draw_profile_pass(&draw_profile.liquid_ms);draw_profile_end();draw_profile_drain();unsigned draw_ms=GetTickCount()-start;
        wx_fog_bind(NULL);wx_ui_rect(&interface,12,10,616,60,0xe0101010);
        wx_ui_textf(&interface,0,20,15,WX_UI_GOLD,"NORTHSHIRE STREAM / phase %u / fog %s / lap %u",path.phase,world_fog.enabled?"ON":"OFF",path.cycles);
        wx_ui_text(&interface,0,20,34,WX_UI_WHITE,with_actors?"Synthetic units / real assets. Offline route; no pad input.":"Offline collision route. Phase 4 relocates. No pad input.");
        wx_ui_textf(&interface,0,20,51,WX_UI_WHITE,"%u KiB free / %u ms frame / index %u / data %u ms",wx_free_memory()/1024,interval,region_ms,stream_ms);
        pb_reset();wx_ui_draw(&interface);while(pb_busy()){}while(pb_finished()){}
        WxFrameStats stats={0};stats.time_ms=now;stats.frame_ms=interval;stats.free_bytes=wx_free_memory();
        stats.cache_bytes=scene.bytes+interface.bytes+wx_font_bytes()+cooldown_catalog.bytes+scene.index_bytes+wx_pack_pending_bytes(&region.pending);
        if(with_actors)stats.cache_bytes+=actors.bytes+actors.index_bytes+avatar.bytes+avatar.scene.bytes;
        stats.draws=draws;stats.triangles=triangles;stats.loads=scene.loads;stats.failures=scene.failures+interface.failures+!installed;
        stats.x=path.position[0];stats.y=path.position[1];stats.z=path.position[2];stats.yaw=path.yaw;stats.pitch=tilt;
        stats.replay_frame=path.frame;stats.scenario_stage=path.phase;stats.scenario_frame=path.age;
        stats.region_loaded=region.loaded;stats.region_transitions=region.transitions;stats.region_failures=region.failures;
        if(region.selected>=0){stats.tile_x=region.cells[region.selected].x;stats.tile_y=region.cells[region.selected].y;}
        stats.ui[0]=interface.ready;stats.ui[1]=interface.bytes;stats.ui[2]=interface.quads;stats.ui[3]=interface.failures;
        stats.portrait[9]=mode;stats.profile[WX_PROFILE_DRAW]=draw_ms;stats.profile[WX_PROFILE_STREAM]=region_ms+stream_ms;stats.profile[WX_PROFILE_ACTORS]=actor_ms;
        if(with_actors){stats.failures+=actors.failures+avatar.failures+avatar.scene.failures+avatar.compose.failures+animated.errors;stats.npc_submitted=actor_drawn;stats.npc_missing=actor_missing;stats.entity_count=world_entity_count;
            stats.avatar_revision=avatar.revision;stats.avatar_ready=avatar.ready;stats.avatar_drawn=avatar.drawn;stats.avatar_missing=avatar.missing;stats.avatar_clip=avatar.complete_clip;stats.avatar_pose=avatar.pose_hash;}
        stats.profile[WX_PROFILE_WORK]=GetTickCount()-now;
        wx_telemetry_send(&stats);wx_telemetry_stream(now,path.frame,mode,path.phase,path.cycles,&scene,&region,region_ms,stream_ms,path.waits,path.errors,world_fog.enabled);
        wx_telemetry_draw(now,path.frame,mode,1,&draw_profile);
        if(with_actors){unsigned ready=0,fallback=0;
            for(unsigned i=0;i<actor_count;i++){uint32_t id=actor_animation(&actor_entities[actor_indices[i]]);ready+=wx_animation_resident(&actors,id)!=0;fallback+=wx_animation_fallback(&actors,id)!=UINT32_MAX;}
            wx_telemetry_actor_stream(now,path.frame,mode,animated.clip,&actors,&avatar,ready,fallback,actor_drawn,actor_ms,animated.gaps,animated.switches);}
        wx_stream_scenario_step(&path,&region,&scene);
    }
}
