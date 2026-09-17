/* Real prepared profiles through production selection/composition; no account. */
static void profile_fixture(void){
    wx_network_telemetry_start();wx_scene_init(&avatar.scene);
    static WxAvatarSelection selection;static WxWorldView subject;static WxOutfits outfits;
    unsigned errors=!wx_outfits_open(&outfits,"D:\\OUTFITS.WXO"),frame=0,stage=0,age=0,stable=0,cycles=0,completed=0,expected=0;
    unsigned last=GetTickCount(),presentation=pb_get_vbl_counter();
    const WxBackdropView view={{5,0,1.4f},{0,1,0},{0,0,1},{-1,0,0},{320,270},420,.1f,160};
    float camera[4]={5,0,1.4f,1},right[4]={0,1,0,0},up[4]={0,0,1,0},forward[4]={-1,0,0,0};
    for(;;){
        while((unsigned)(pb_get_vbl_counter()-presentation)<2)pb_wait_for_vbl();presentation=pb_get_vbl_counter();
        unsigned now=GetTickCount(),interval=now-last;last=now;while(pb_busy()){}
        unsigned index=stage<16?stage:stage==16?(age/5)%16:stage==18?15:0;
        memset(&subject,0,sizeof subject);subject.active=1;subject.guid=1;subject.race=index/2+1;subject.gender=index%2;subject.character_class=1;
        subject.revision=cycles*20+stage+1;subject.equipment_ready_mask=0x7ffff;
        const WxOutfit* outfit=wx_outfit_find(&outfits,subject.race,1,subject.gender);
        if(outfit)memcpy(subject.equipment_display,outfit->display,sizeof outfit->display);else errors++;
        wx_stream_frame_begin(8);unsigned begin=GetTickCount();
        wx_avatar_select(&avatar,&selection,&subject,stage==18?"D:\\MISSING":"D:\\");wx_avatar_update(&avatar,&subject,0,now);
        unsigned work=GetTickCount()-begin;wx_stream_frame_end();
        if(stage==18){if(selection.failures>expected)expected=selection.failures;}
        else if(avatar.failures||avatar.scene.failures||avatar.compose.failures||selection.failures>expected)errors++;
        if((selection.pending||!avatar.profile_ready)&&avatar.ready)errors++;
        if(avatar.ready&&(avatar.header.look[0]!=subject.race||avatar.header.look[1]!=subject.gender||
           avatar.published_look[0]!=subject.race||avatar.published_look[1]!=subject.gender))errors++;
        pb_reset();pb_target_back_buffer();push_base=NULL;pb_erase_depth_stencil_buffer(0,0,640,480);pb_fill(0,0,640,480,0xff303c48);
        wx_font_clear();wx_ui_clear(&interface);shaders();wx_fog_bind(NULL);draws=triangles=0;
        unsigned drawn=0;if(avatar.ready){unsigned before=draws;
            for(unsigned i=0;i<avatar.count;i++)render(&avatar.scene,camera,right,up,forward,avatar.ids[i],1,0,&view,&avatar);
            drawn=draws-before;if(!drawn)errors++;
        }
        wx_ui_center(&interface,1,320,25,WX_UI_GOLD,"OFFLINE CHARACTER PROFILE LOADING");
        wx_ui_center(&interface,0,320,55,WX_UI_WHITE,"Original assets; synthetic selection. No controller input.");
        wx_ui_textf(&interface,0,25,385,WX_UI_WHITE,"Race %u / sex %u / stage %u / cycle %u",subject.race,subject.gender,stage,cycles);
        wx_ui_textf(&interface,0,25,405,WX_UI_WHITE,"%u KiB free / %u ms frame / %u ms profile work",wx_free_memory()/1024,interval,work);
        wx_ui_textf(&interface,0,25,425,WX_UI_WHITE,"Loading %u / drawn %u / errors %u / cancellations %u",avatar.opening.phase,drawn,errors,selection.cancelled);
        while(pb_busy()){}pb_reset();wx_ui_draw(&interface);while(pb_busy()){}while(pb_finished()){}
        WxFrameStats stats={0};stats.time_ms=now;stats.frame_ms=interval;stats.free_bytes=wx_free_memory();stats.replay_frame=frame;
        stats.cache_bytes=avatar.bytes+avatar.scene.bytes+wx_avatar_pending_bytes(&avatar)+interface.bytes+wx_font_bytes()+cooldown_catalog.bytes;
        stats.draws=draws;stats.triangles=triangles;stats.failures=errors;stats.avatar_ready=avatar.ready;stats.avatar_bytes=avatar.bytes+avatar.scene.bytes;
        stats.avatar_revision=avatar.revision;stats.avatar_drawn=drawn;stats.portrait[9]=12;stats.scenario_stage=stage;stats.scenario_frame=age;
        wx_telemetry_send(&stats);wx_telemetry_profile(now,frame,stage,cycles,completed,expected,errors,work,drawn,&selection,&avatar);
        age++;int advance=0;
        if(stage==16)advance=age>=120;
        else if(stage==18)advance=selection.failures==expected&&avatar.failures&&!selection.pending&&age>=45;
        else{stable=avatar.ready&&drawn?stable+1:0;advance=stable>=45;if(advance&&stage<16)completed|=1u<<stage;}
        if(age>600){errors++;advance=1;}
        if(advance){stage++;age=stable=0;if(stage==20){stage=0;cycles++;}}
        frame++;
    }
}
