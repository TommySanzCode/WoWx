/* Offline map-wide WMO routing, streamed geometry and collision. No account/input. */
static void global_world_fixture(void){
    typedef struct {uint32_t map;float p[3];} Point;
    Point points[32]={{0}};uint32_t header[3]={0};unsigned errors=0;
    FILE* file=fopen("D:\\GLOBAL.BIN","rb");
    int valid=file&&fread(header,1,12,file)==12&&header[0]==0x314d4757&&(header[1]>=1&&header[1]<=3)&&header[2]>0&&header[2]<=32;
    if(valid)valid=fread(points,sizeof(Point),header[2],file)==header[2]&&fgetc(file)==EOF;
    if(file)fclose(file);
    typedef struct {unsigned environment,liquid;float depth;} Expected;
    Expected expected[32]={{0}};int environment_test=header[1]>=2,terrain_test=header[1]==3;
    if(environment_test){unsigned eh[2]={0};file=fopen("D:\\ENVIRON.BIN","rb");
        valid=valid&&file&&fread(eh,1,8,file)==8&&eh[0]==0x314e4557&&eh[1]==header[2]&&fread(expected,sizeof(Expected),eh[1],file)==eh[1]&&fgetc(file)==EOF;
        if(file)fclose(file);if(!wx_light_open(&lighting,"D:\\LIGHT.WLF"))valid=0;
    }
    for(unsigned i=0;valid&&i<header[2];i++){
        valid=points[i].map<=999;
        for(unsigned k=0;k<3;k++)valid=valid&&isfinite(points[i].p[k])&&fabsf(points[i].p[k])<18000;
        if(i&&!environment_test)valid=valid&&points[i].map>points[i-1].map;
        if(environment_test)valid=valid&&expected[i].environment<=2&&isfinite(expected[i].depth)&&expected[i].depth>=0;
    }
    if(!valid){errors++;header[2]=1;}
    unsigned probe=0;file=fopen("D:\\MATERIAL.BIN","rb");
    if(file){probe=fread(&probe,1,4,file)==4&&probe==0x31504d57&&fgetc(file)==EOF;fclose(file);if(!probe)errors++;}
    unsigned liquid_test=0;file=fopen("D:\\LIQUID.BIN","rb");
    if(file){liquid_test=fread(&liquid_test,1,4,file)==4&&liquid_test==0x314c5157&&fgetc(file)==EOF;fclose(file);if(!liquid_test||!environment_test)errors++;}
    wx_network_telemetry_start();wx_scene_init(&scene);
    int installed=wx_region_open(&region,"D:\\world.wxi");
    wx_fog_fallback(&world_fog,WX_FOG_INDOOR);world_fog.enabled=0;
    unsigned stage=0,frame=0,age=0,held=0,cycles=0,waits=0;
    unsigned last=GetTickCount(),presentation=pb_get_vbl_counter();
    for(;;){
        while((unsigned)(pb_get_vbl_counter()-presentation)<2)pb_wait_for_vbl();presentation=pb_get_vbl_counter();
        unsigned now=GetTickCount(),interval=now-last;last=now;while(pb_busy()){}
        Point* point=&points[stage];
        wx_stream_frame_begin(1u|(scene.file&&!wx_stream_ready(&scene)?2u:0u));
        unsigned start=GetTickCount();int attached=installed&&wx_region_update(&region,&scene,point->map,point->p);
        unsigned region_ms=GetTickCount()-start;start=GetTickCount();wx_stream(&scene,point->p);
        unsigned stream_ms=GetTickCount()-start;wx_stream_frame_end();
        float floor=0;int grounded=attached&&wx_stream_ready(&scene)&&wx_floor(&scene,point->p[0],point->p[1],point->p[2]+.5f,point->p[2]-.5f,&floor);
        int material_probe=probe&&point->map==999;
        float yaw=material_probe?0:(environment_test?age:held)*.012f,tilt=material_probe?0:-.1f;
        if(liquid_test)tilt=expected[stage].depth>0?.25f:-.3f;
        wx_fog_fallback(&world_fog,WX_FOG_INDOOR);world_fog.enabled=0;
        if(material_probe){world_fog.enabled=(cycles&1);world_fog.start=8;world_fog.end=12;world_fog.color[0]=.1f;world_fog.color[1]=.2f;world_fog.color[2]=.3f;}
        float forward[4]={cosf(yaw)*cosf(tilt),sinf(yaw)*cosf(tilt),sinf(tilt),0};
        float right[4]={sinf(yaw),-cosf(yaw),0,0};
        float up[4]={-cosf(yaw)*sinf(tilt),-sinf(yaw)*sinf(tilt),cosf(tilt),0};
        float camera[4]={point->p[0],point->p[1],point->p[2]+(environment_test?0:1.7f),1};
        unsigned sample_start=GetTickCount();WxEnvironmentSample environment;wx_environment_sample(&scene,camera,&environment);unsigned sample_ms=GetTickCount()-sample_start;
        WxLightSample atmosphere={0};
        if(environment_test){wx_light_weather_sample(&lighting,point->map,camera,1440,0,environment.environment,&atmosphere);world_fog=atmosphere.fog;world_light=atmosphere.palette;
            int match=environment.known&&environment.environment==expected[stage].environment&&environment.liquid_type==expected[stage].liquid&&fabsf(environment.depth-expected[stage].depth)<.02f;
            if(wx_stream_ready(&scene)&&!match)errors++;grounded=attached&&wx_stream_ready(&scene)&&match;
        }
        pb_reset();pb_target_back_buffer();push_base=NULL;pb_erase_depth_stencil_buffer(0,0,640,480);
        pb_fill(0,0,640,480,environment_test?wx_fog_background(&world_fog):0xff182028);shaders();draws=triangles=0;wx_font_clear();wx_ui_clear(&interface);
        unsigned sky_quads=0;
        if(environment_test){wx_fog_bind(NULL);sky_quads=wx_sky_build(&interface,&world_light,&world_fog,right,up,forward);wx_ui_draw(&interface);while(pb_busy()){}wx_ui_clear(&interface);shaders();}
        start=GetTickCount();draw_profile_begin();if(attached)render(&scene,camera,right,up,forward,UINT32_MAX,1,0,NULL,NULL);
        draw_profile_pass(&draw_profile.world_ms);
        if(attached)render_liquids(&scene,camera,right,up,forward);
        draw_profile_pass(&draw_profile.liquid_ms);draw_profile_end();
        if(!attached){memset(render_material_counts,0,sizeof render_material_counts);render_material_flags=render_motion_draws=render_motion_hash=0;render_liquid_draws=render_sequence_draws=render_sequence_frames=0;}
        draw_profile_drain();unsigned draw_ms=GetTickCount()-start;unsigned world_draws=draws;
        if(grounded&&world_draws)held++;else waits++;
        if(attached&&(region.selected<0||region.cells[region.selected].map!=point->map||region.cells[region.selected].reserved!=(terrain_test?0:WX_REGION_GLOBAL_WMO)))errors++;
        if(age>1800){errors++;held=120;}
        wx_fog_bind(NULL);wx_ui_rect(&interface,12,10,616,65,0xe0101010);
        wx_ui_textf(&interface,0,20,15,WX_UI_GOLD,"%s / map %u / %u of %u / lap %u",terrain_test?"TERRAIN LIQUID":"GLOBAL WMO",point->map,stage+1,header[2],cycles);
        wx_ui_text(&interface,0,20,34,WX_UI_WHITE,"Offline prepared world model / no account or pad input");
        wx_ui_textf(&interface,0,20,53,WX_UI_WHITE,"%u KiB free / %u ms / ready %u / draws %u / errors %u",wx_free_memory()/1024,interval,grounded,world_draws,errors);
        if(environment_test){wx_ui_rect(&interface,12,396,616,48,0xff101010);
            wx_ui_textf(&interface,0,20,403,WX_UI_GOLD,"Camera environment %u / expected %u / known %u",environment.environment,expected[stage].environment,environment.known);
            wx_ui_textf(&interface,0,20,422,WX_UI_WHITE,"Liquid %u / depth %u mm / fog %u / records %u",environment.liquid_type,(unsigned)(environment.depth*1000),world_fog.enabled,environment.records);}
        if(liquid_test){wx_ui_rect(&interface,12,369,616,25,0xe0101010);wx_ui_textf(&interface,0,20,374,WX_UI_GOLD,"Original liquid patches %u / 30-frame textures %u",render_liquid_draws,render_sequence_draws);}
        if(material_probe){wx_ui_rect(&interface,12,370,616,74,0xff101010);
            wx_ui_text(&interface,0,20,378,WX_UI_GOLD,"Synthetic blend probe: 0  1  2  3  4  5  6 (left to right)");
            wx_ui_text(&interface,0,20,397,WX_UI_WHITE,(render_material_flags&WX_VERTEX_COLOR)?"RGBA8 vertex lighting / bottom baked / UI stays opaque.":"Top: lit/fogged. Bottom: unlit/unfogged. UI stays opaque.");
            wx_ui_textf(&interface,0,20,416,WX_UI_WHITE,"Fog %u / moving materials %u / hash %u",world_fog.enabled,render_motion_draws,render_motion_hash);}
        pb_reset();wx_ui_draw(&interface);while(pb_busy()){}while(pb_finished()){}
        WxFrameStats stats={0};stats.time_ms=now;stats.frame_ms=interval;stats.free_bytes=wx_free_memory();
        stats.cache_bytes=lighting.bytes+scene.bytes+scene.index_bytes+interface.bytes+cooldown_catalog.bytes+wx_font_bytes()+region.count*sizeof(WxRegionCell)+wx_pack_pending_bytes(&region.pending);
        stats.draws=world_draws;stats.triangles=triangles;stats.loads=scene.loads;stats.failures=lighting.failures+scene.failures+interface.failures+!installed+errors;
        stats.x=point->p[0];stats.y=point->p[1];stats.z=point->p[2];stats.fixture_map=point->map;
        if(region.selected>=0){stats.tile_x=region.cells[region.selected].x;stats.tile_y=region.cells[region.selected].y;}
        stats.replay_frame=frame;stats.scenario_stage=stage;stats.scenario_frame=held;stats.region_loaded=region.loaded;stats.region_transitions=region.transitions;stats.region_failures=region.failures;
        stats.portrait[9]=13;stats.profile[WX_PROFILE_DRAW]=draw_ms;stats.profile[WX_PROFILE_STREAM]=region_ms+stream_ms;stats.profile[WX_PROFILE_WORK]=GetTickCount()-now;
        wx_telemetry_send(&stats);wx_telemetry_stream(now,frame,13,stage,cycles,&scene,&region,region_ms,stream_ms,waits,errors,world_fog.enabled);
        wx_telemetry_draw(now,frame,13,1,&draw_profile);
        wx_telemetry_material(now,frame,render_material_counts,render_material_flags,world_fog.enabled,render_motion_draws,render_motion_hash,render_liquid_draws,render_sequence_draws,render_sequence_frames);
        if(environment_test){wx_telemetry_environment(now,frame,13,stage,cycles,expected[stage].environment,expected[stage].liquid,&environment,&scene,sample_ms,errors,camera);
            wx_telemetry_lighting(now,frame,13,stage,1440,environment.environment==2?WX_LIGHT_WATER:WX_LIGHT_CLEAR,&lighting,&atmosphere,&world_fog,&world_light,sample_ms,sky_quads);}
        frame++;age++;
        if(held>=(material_probe?300u:120u)){held=age=0;if(++stage>=header[2]){stage=0;cycles++;}}
    }
}
