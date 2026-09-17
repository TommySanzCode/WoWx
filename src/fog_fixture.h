/* Real Northshire pack, fixed camera pairs, no authentication or gameplay.
   Assets are resident before timing; this does not measure streaming/travel. */
static void fog_fixture(void){
    wx_network_telemetry_start();int loaded=wx_pack_open(&scene,"D:\\world.wxp");
    if(loaded){memcpy(player,scene.header.spawn,sizeof player);for(unsigned i=0;i<128;i++)wx_stream(&scene,player);}
    wx_fog_fallback(&world_fog,WX_FOG_OUTDOOR);
    unsigned frame=0,last=GetTickCount(),presentation=pb_get_vbl_counter();
    for(;;){
        while((unsigned)(pb_get_vbl_counter()-presentation)<2)pb_wait_for_vbl();presentation=pb_get_vbl_counter();
        unsigned now=GetTickCount(),interval=now-last;last=now;while(pb_busy()){}
        unsigned phase=frame/180%6;world_fog.enabled=phase&1;
        float angle=.15f+(phase/2)*1.57f,tilt=-.08f;
        float forward[4]={cosf(angle)*cosf(tilt),sinf(angle)*cosf(tilt),sinf(tilt),0};
        float right[4]={sinf(angle),-cosf(angle),0,0};
        float up[4]={-cosf(angle)*sinf(tilt),-sinf(angle)*sinf(tilt),cosf(tilt),0};
        float camera[4]={player[0],player[1],player[2]+3,1};
        pb_reset();pb_target_back_buffer();push_base=NULL;pb_erase_depth_stencil_buffer(0,0,640,480);
        pb_fill(0,0,640,480,wx_fog_background(&world_fog));shaders();draws=triangles=0;wx_font_clear();wx_ui_clear(&interface);
        unsigned draw_start=GetTickCount();if(loaded){render(&scene,camera,right,up,forward,UINT32_MAX,1,0,NULL,NULL);render_liquids(&scene,camera,right,up,forward);}
        while(pb_busy()){}unsigned draw_ms=GetTickCount()-draw_start;
        wx_fog_bind(NULL);wx_ui_rect(&interface,12,10,616,60,0xe0101010);
        wx_ui_textf(&interface,0,20,15,WX_UI_GOLD,"NORTHSHIRE FOG %s / fixed view %u",world_fog.enabled?"ON":"OFF",phase/2+1);
        wx_ui_text(&interface,0,20,34,WX_UI_WHITE,"90-145 yd outdoor fallback. Offline, resident assets.");
        wx_ui_textf(&interface,0,20,51,WX_UI_WHITE,"Free %u KiB / guest frame %u ms / no physical input",wx_free_memory()/1024,interval);
        pb_reset();wx_ui_draw(&interface);while(pb_busy()){}while(pb_finished()){}
        WxFrameStats stats={0};stats.time_ms=now;stats.frame_ms=interval;stats.free_bytes=wx_free_memory();
        stats.cache_bytes=scene.bytes+interface.bytes+wx_font_bytes()+cooldown_catalog.bytes;stats.draws=draws;stats.triangles=triangles;
        stats.loads=scene.loads;stats.failures=scene.failures+interface.failures+!loaded;stats.replay_frame=frame;
        stats.ui[0]=interface.ready;stats.ui[1]=interface.bytes;stats.ui[2]=interface.quads;stats.ui[3]=interface.failures;
        stats.portrait[9]=6;stats.profile[WX_PROFILE_DRAW]=draw_ms;stats.profile[WX_PROFILE_WORK]=draw_ms;
        wx_telemetry_send(&stats);frame++;
    }
}
