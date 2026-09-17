/* Authored fog and non-unit actor scale acceptance, isolated from realm/pad.
   Rain/water/indoor conditions and accelerated clock are explicitly synthetic. */
static void lighting_fixture(unsigned fixture){
    wx_network_telemetry_start();int loaded=wx_pack_open(&scene,"D:\\world.wxp");
    wx_light_open(&lighting,"D:\\LIGHT.WLF");wx_scene_init(&actors);int models=wx_pack_open(&actors,"D:\\actors.wxp");
    actors.budget_bytes=8u*1024u*1024u;
    if(loaded){memcpy(player,scene.header.spawn,sizeof player);for(unsigned i=0;i<128;i++)wx_stream(&scene,player);}
    wx_fog_fallback(&world_fog,WX_FOG_OUTDOOR);memset(actor_entities,0,sizeof actor_entities);world_entity_count=3;
    unsigned frame=0,last=GetTickCount(),presentation=pb_get_vbl_counter(),weather_mode=fixture==10,weather_errors=0;
    WxWeather weather={0};WxWeatherMix weather_presentation={0};
    for(;;){
        while((unsigned)(pb_get_vbl_counter()-presentation)<2)pb_wait_for_vbl();presentation=pb_get_vbl_counter();
        unsigned now=GetTickCount(),interval=now-last;last=now;while(pb_busy()){}
        unsigned phase=frame/240%(weather_mode?10:8),age=frame%240,isolated=!weather_mode&&(phase==5||phase==6);
        const unsigned hours[]={12,0,6,12,12,12,12,12};unsigned condition=phase==3?WX_LIGHT_RAIN:phase==4?WX_LIGHT_WATER:WX_LIGHT_CLEAR;
        unsigned environment=phase==4?WX_FOG_UNDERWATER:phase==7?WX_FOG_INDOOR:WX_FOG_OUTDOOR;
        if(weather_mode){environment=phase==6?WX_FOG_INDOOR:phase==7?WX_FOG_UNDERWATER:WX_FOG_OUTDOOR;
            if(!age){
                const unsigned types[]={0,1,1,2,3,0,1,1,0,3},sounds[]={0,8533,8535,8538,8558,0,8535,8535,0,8556},instant[]={1,0,0,1,0,0,1,1,1,1};
                const float grades[]={0,.25f,1,.65f,.8f,0,1,1,0,.4f};
                uint8_t wire[13];memcpy(wire,types+phase,4);memcpy(wire+4,grades+phase,4);memcpy(wire+8,sounds+phase,4);wire[12]=instant[phase];
                if(phase==8)wx_weather_reset(&weather);
                else weather_errors+=wx_weather_apply(&weather,0x2f4,wire,13)!=1;
                if(phase==9){WxWeather before=weather;float bad=NAN;memcpy(wire+4,&bad,4);
                    weather_errors+=wx_weather_apply(&weather,0x2f4,wire,13)!=0||memcmp(&weather,&before,sizeof before)!=0;}
            }
            wx_weather_step(&weather_presentation,&weather,interval/1000.f);condition=weather_presentation.weight>0?WX_LIGHT_RAIN:WX_LIGHT_CLEAR;
            if(environment==WX_FOG_UNDERWATER)condition=WX_LIGHT_WATER;
        }
        WxWorldClock clock={0};uint32_t packet[2]={(weather_mode?12:hours[phase])<<6,0}; /* Frozen, valid Vanilla wire clock. */
        unsigned errors=!wx_clock_apply(&clock,0x42,packet,sizeof packet,now);float time=wx_clock_half_minutes(&clock,now);
        WxLightSample sample;unsigned start=GetTickCount();
        if(weather_mode)wx_light_weather_sample(&lighting,0,player,time,weather_presentation.weight,environment,&sample);
        else wx_light_sample(&lighting,0,player,time,condition,environment,&sample);
        if(isolated){sample.fog=(WxFog){{.35f,.42f,.5f},5,20,phase==6,WX_FOG_OUTDOOR};}
        if(weather_mode&&weather_presentation.snap){world_fog=sample.fog;world_light=sample.palette;}
        else {wx_fog_blend(&world_fog,&sample.fog,interval/1000.f);wx_light_blend(&world_light,&sample.palette,interval/1000.f);}
        unsigned light_ms=GetTickCount()-start;float angle=.15f,tilt=.12f;
        float forward[4]={cosf(angle)*cosf(tilt),sinf(angle)*cosf(tilt),sinf(tilt),0};
        float right[4]={sinf(angle),-cosf(angle),0,0};float up[4]={-cosf(angle)*sinf(tilt),-sinf(angle)*sinf(tilt),cosf(tilt),0};
        float camera[4]={player[0],player[1],player[2]+3,1};
        if(isolated){camera[0]=camera[1]=camera[2]=0;forward[0]=1;forward[1]=forward[2]=0;right[0]=right[2]=0;right[1]=-1;up[0]=up[1]=0;up[2]=1;}
        for(unsigned i=0;i<3;i++){
            WxEntity* e=&actor_entities[i];e->guid=10+i;e->type=3;e->fields[131]=447;e->fields[22]=100;e->animation=0;
            float scale=i==0?.5f:i==1?1:2;memcpy(e->fields+4,&scale,4);float side=((int)i-1)*4;
            e->x=camera[0]+forward[0]*12+right[0]*side;e->y=camera[1]+forward[1]*12+right[1]*side;e->z=camera[2]-scale;e->orientation=angle+3.14159265f;
        }
        /* Selection measures distance from player, including the synthetic view. */
        float saved[3];memcpy(saved,player,sizeof saved);if(isolated)memset(player,0,sizeof player);
        start=GetTickCount();actor_stream(now,camera,right,forward);unsigned actor_ms=GetTickCount()-start;memcpy(player,saved,sizeof saved);
        pb_reset();pb_target_back_buffer();push_base=NULL;pb_erase_depth_stencil_buffer(0,0,640,480);
        pb_fill(0,0,640,480,wx_fog_background(&world_fog));draws=triangles=0;wx_font_clear();wx_ui_clear(&interface);
        start=GetTickCount();unsigned sky_quads=wx_sky_build(&interface,&world_light,&world_fog,right,up,forward);
        wx_ui_draw(&interface);while(pb_busy()){}wx_ui_clear(&interface);shaders();light_ms+=GetTickCount()-start;
        start=GetTickCount();if(loaded&&!isolated)render(&scene,camera,right,up,forward,UINT32_MAX,1,0,NULL,NULL);actor_render(camera,right,up,forward);
        if(loaded&&!isolated)render_liquids(&scene,camera,right,up,forward);
        while(pb_busy()){}unsigned draw_ms=GetTickCount()-start;
        wx_fog_bind(NULL);wx_ui_rect(&interface,12,10,524,74,0xe0101010);
        const char* labels[]={"Authored noon","Authored midnight","Authored dawn","Synthetic rain condition","Synthetic underwater condition","Scale comparison: fog OFF","Scale comparison: fog ON","Indoor fallback: fog disabled"};
        const char* weather_labels[]={"Clear","Light rain, smooth","Heavy rain, smooth","Snow, instant","Storm, smooth","Clearing, smooth","Indoor override","Underwater override","Transfer reset","Malformed packet rejected"};
        wx_ui_textf(&interface,0,20,15,WX_UI_GOLD,"%s / %s",weather_mode?"WEATHER":"LIGHTING",weather_mode?weather_labels[phase]:labels[phase]);
        wx_ui_textf(&interface,0,20,33,WX_UI_WHITE,"Profile %u / fog %d to %d / scales 0.5, 1, 2",sample.profile[2],(int)world_fog.start,(int)world_fog.end);
        wx_ui_textf(&interface,0,20,51,WX_UI_WHITE,"%u KiB free / %u ms frame / %u KiB lighting",wx_free_memory()/1024,interval,lighting.bytes/1024);
        wx_ui_text(&interface,0,20,67,WX_UI_WHITE,"Offline; synthetic clock/conditions. No input.");
        pb_reset();wx_ui_draw(&interface);while(pb_busy()){}
        uint32_t wolf=447<<8;unsigned portrait_draws=portrait_render(&actors,NULL,&wolf,1,447,548,2);
        while(pb_busy()){}while(pb_finished()){}
        WxFrameStats stats={0};stats.time_ms=now;stats.frame_ms=interval;stats.free_bytes=wx_free_memory();
        stats.cache_bytes=scene.bytes+scene.index_bytes+actors.bytes+actors.index_bytes+lighting.bytes+interface.bytes+wx_font_bytes()+cooldown_catalog.bytes;
        stats.draws=draws;stats.triangles=triangles;stats.loads=scene.loads+actors.loads;
        stats.failures=scene.failures+actors.failures+lighting.failures+interface.failures+!loaded+!models+errors+weather_errors;
        stats.replay_frame=frame;stats.scenario_stage=phase;stats.scenario_frame=age;stats.portrait[9]=fixture;
        stats.portrait[5]=447;stats.portrait[6]=portrait_draws;
        stats.npc_submitted=actor_drawn;stats.npc_missing=actor_missing;stats.entity_count=3;stats.ui[0]=interface.ready;stats.ui[1]=interface.bytes;stats.ui[2]=interface.quads;stats.ui[3]=interface.failures;
        stats.profile[WX_PROFILE_DRAW]=draw_ms;stats.profile[WX_PROFILE_ACTORS]=actor_ms;stats.profile[WX_PROFILE_UPDATE]=light_ms;stats.profile[WX_PROFILE_WORK]=GetTickCount()-now;
        wx_telemetry_send(&stats);wx_telemetry_lighting(now,frame,fixture,phase,time,condition,&lighting,&sample,&world_fog,&world_light,light_ms,sky_quads);
        if(weather_mode)wx_telemetry_weather(now,frame,fixture,phase,&weather,&weather_presentation,environment,weather_errors);frame++;
    }
}
