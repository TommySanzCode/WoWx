#include "cooldown_fixture.h"
#include "action_fixture.h"
#include "fog_fixture.h"
#include "lighting_fixture.h"
#include "stream_fixture.h"
#include "profile_fixture.h"
#include "global_world_fixture.h"
/* Included by main.c to exercise the production renderer without a realm/account.
   Opt-in only through an isolated disc's exact eight-byte PTTEST.BIN marker. */
static int portrait_fixture_enabled(void){
    FILE* f=fopen("D:\\PTTEST.BIN","rb");if(!f)return 0;
    char tag[8];int enabled=0;
    if(fread(tag,1,8,f)==8&&fgetc(f)==EOF){if(!memcmp(tag,"WXPF0013",8))enabled=13;else if(!memcmp(tag,"WXPF0012",8))enabled=12;else if(!memcmp(tag,"WXPF0011",8))enabled=11;else if(!memcmp(tag,"WXPF0010",8))enabled=10;
        else if(!memcmp(tag,"WXPF000",7)&&tag[7]>='1'&&tag[7]<='9')enabled=tag[7]-'0';}
    fclose(f);return enabled;
}
static void portrait_fixture(unsigned mode){
    if(mode==13)global_world_fixture();
    if(mode==12)profile_fixture();
    if(mode==9||mode==10)lighting_fixture(mode);
    if(mode==7||mode==8||mode==11)stream_fixture(mode);
    if(mode==6)fog_fixture();
    wx_network_telemetry_start(); // initializes UDP only; never starts the auth worker
    wx_scene_init(&actors);wx_pack_open(&actors,"D:\\actors.wxp");actors.budget_bytes=8u*1024u*1024u;
    wx_scene_init(&avatar.scene);static WxWorldView subject;static WxEntity units[2];
    static WxCooldowns fixture_cooldowns;static WxCastState fixture_cast;static WxInventory fixture_inventory;static unsigned fixture_failures;
    static WxSpellBook book;uint8_t bindings[24];for(unsigned i=0;i<24;i++)bindings[i]=i;
    if(mode>=2){
        wx_icons_open(&action_icons,"D:\\ICONS.WIC");wx_spellbook_open(&book,"D:\\SPELLS.WXS");
        uint32_t actions[]={6603,78,2457,6673,100,0x80000000|6948,0,0,133,116,686,172,403,331,585,2061,1752,2098,53,6673,1130,2973,5176,5185};
        memcpy(game.actions,actions,sizeof actions);game.actions_ready=1;game.item_names[0]=(WxItemName){6948,6418,0,1,"Hearthstone"};
        for(unsigned i=0;i<24;i++)if(actions[i]&&actions[i]<65536&&!wx_has_spell(&game,actions[i]))game.spells[game.spell_count++]=(uint16_t)actions[i];
    }
    unsigned index=UINT32_MAX,frame=0,last=GetTickCount(),presentation=pb_get_vbl_counter(),creatures=0;
    while(creatures<portraits.count&&!(portraits.entries[creatures].key&WX_PORTRAIT_PLAYER))creatures++;
    for(;;){
        while((unsigned)(pb_get_vbl_counter()-presentation)<2)pb_wait_for_vbl();presentation=pb_get_vbl_counter();
        unsigned now=GetTickCount(),interval=now-last;last=now;while(pb_busy()){}
        unsigned cycle=creatures?creatures*60:960,selected=mode>=2?0:(frame%cycle)*16/cycle;
        if(index!=selected){
            wx_avatar_close(&avatar);memset(&subject,0,sizeof subject);subject.guid=1;subject.active=1;
            char path[48],metadata[48];uint32_t look[7]={selected/2+1,selected%2,0,0,0,0,0};
            wx_avatar_path(path,sizeof path,"D:\\",look,0);wx_avatar_path(metadata,sizeof metadata,"D:\\",look,1);
            wx_avatar_open(&avatar,path,metadata);const uint32_t* chosen=avatar.header.look;
            subject.race=chosen[0];subject.gender=chosen[1];subject.skin=chosen[2];subject.face=chosen[3];subject.hair_style=chosen[4];subject.hair_color=chosen[5];subject.facial_hair=chosen[6];
            index=selected;
        }
        wx_avatar_update(&avatar,&subject,0,now);
        uint32_t key=creatures?portraits.entries[mode>=2?0:(frame/60)%creatures].key:0,id=key<<8;
        wx_stream_animations(&actors,&id,1);wx_animate_ids(&actors,now,&id,1);
        unsigned icons_start=GetTickCount(),icons_ms=0,layer=0;WxActionPrompt prompts[8]={{0}};
        if(mode>=2){uint32_t keys[24];for(unsigned i=0;i<24;i++)keys[i]=wx_action_icon(&game,game.actions[i]);
            wx_icons_update(&action_icons,keys,24);icons_ms=GetTickCount()-icons_start;wx_spellbook_sync(&book,&game);layer=1+(frame/120)%3;
            if(mode==3)layer=frame/90%8==1||frame/90%8==2||frame/90%8==5?2:frame/90%8==7?3:1;
            if(mode==5)layer=frame/90%12>=3&&frame/90%12<=6?1:2;
            if(mode==4)layer=frame/90%8==5||frame/90%8==6?1:2;
            for(unsigned i=0;i<8;i++)wx_action_prompt(&book,&game,bindings,0,(layer-1)*8+i,&prompts[i]);}
        if(mode>=3){if(mode==3)wx_cooldown_fixture_step(&fixture_cooldowns,&cooldown_catalog,frame);else if(mode==4)wx_global_fixture_step(&fixture_cooldowns,&cooldown_catalog,frame);
            for(unsigned i=0;i<8;i++)action_cooldowns[i]=wx_cooldown_query(&fixture_cooldowns,&cooldown_catalog,prompts[i].binding,frame*33u);}
        pb_reset();pb_target_back_buffer();push_base=NULL;pb_erase_depth_stencil_buffer(0,0,640,480);
        pb_fill(0,0,640,480,0xff303c48);for(unsigned y=0;y<160;y+=16)for(unsigned x=0;x<640;x+=16)if(((x+y)/16)&1)pb_fill(x,y,16,16,0xff3c4854);
        wx_font_clear();wx_ui_clear(&interface);shaders();draws=triangles=0;memset(portrait_stats,0,sizeof portrait_stats);
        memset(units,0,sizeof units);units[0].guid=1;units[0].type=4;units[1].guid=2;units[1].type=3;
        for(unsigned i=0;i<2;i++){units[i].fields[22]=79;units[i].fields[28]=100;units[i].fields[23]=70;units[i].fields[29]=100;units[i].fields[34]=2;}
        if(mode==5){
            action_fixture_step(&fixture_cast,&fixture_cooldowns,&cooldown_catalog,&book,units,&fixture_inventory,frame,&fixture_failures);
            strcpy(game.item_names[0].name,"Fixture item");
            float position[3]={0};for(unsigned i=0;i<8;i++){WxActionModifiers modifiers;wx_action_modifiers(&fixture_cooldowns,&cooldown_catalog,prompts[i].binding,&modifiers);
                action_feedback[i]=wx_action_feedback(prompts[i].binding,&book,&game,&fixture_inventory,&units[0],&units[1],position,&modifiers);}
            cast_view=wx_cast_query(&fixture_cast,frame*33u);
        }
        wx_hud_unit(&hud.player,&units[0],"Player fixture");wx_hud_unit(&hud.target,&units[1],"Creature fixture");hud.xp=768;hud.next_xp=900;wx_hud_draw(&hud,&interface);
        unsigned start=GetTickCount();portrait_stats[0]=portraits.ready;portrait_stats[1]=portraits.count;portrait_stats[2]=portraits.failures;
        portrait_stats[3]=WX_PORTRAIT_PLAYER|(subject.race<<1)|subject.gender;
        if(avatar.ready)portrait_stats[4]=portrait_render(&avatar.scene,&avatar,avatar.ids,avatar.count,portrait_stats[3],50,1);
        portrait_stats[5]=key;portrait_stats[6]=portrait_render(&actors,NULL,&id,1,key,398,2);
        portrait_stats[8]=GetTickCount()-start;portrait_stats[9]=mode;
        wx_ui_center(&interface,1,320,186,WX_UI_GOLD,mode==5?"OFFLINE ACTION / CAST FIXTURE":mode==4?"OFFLINE GCD / MODIFIER FIXTURE":mode==3?"OFFLINE COOLDOWN GPU FIXTURE":mode>=2?"OFFLINE ACTION ICON GPU FIXTURE":"OFFLINE PORTRAIT GPU FIXTURE");
        wx_ui_center(&interface,0,320,215,WX_UI_WHITE,"Synthetic units. No realm connection or character changes.");
        if(mode==1){wx_ui_textf(&interface,0,50,245,WX_UI_WHITE,"Race %u / sex %u    Creature display %u",subject.race,subject.gender,key);
        wx_ui_textf(&interface,0,50,270,WX_UI_WHITE,"Draw batches %u / %u    Mask rectangles %u",portrait_stats[4],portrait_stats[6],portrait_stats[7]);
        wx_ui_textf(&interface,0,50,295,WX_UI_WHITE,"Free %u KiB    Frame %u ms    Fixture frame %u",wx_free_memory()/1024,interval,frame);}
        else {if(mode==5)wx_cast_draw(&interface,&cast_view,&book);else wx_ui_center(&interface,0,320,242,WX_UI_GOLD,"Injected UI state; no physical controller input.");
            const unsigned button_ids[]={WX_A,WX_B,WX_X,WX_Y,WX_UP,WX_DOWN,WX_LEFT,WX_RIGHT};
            wx_actionbar_ui(&interface,&action_icons,&game,prompts,layer,1u<<button_ids[(frame/15)%8],mode>=3?action_cooldowns:NULL,mode==5?action_feedback:NULL);}
        while(pb_busy()){}pb_reset();wx_ui_draw(&interface);while(pb_busy()){}while(pb_finished()){}
        WxFrameStats stats={0};stats.time_ms=now;stats.frame_ms=interval;stats.free_bytes=wx_free_memory();
        stats.cache_bytes=actors.bytes+avatar.bytes+avatar.scene.bytes+interface.bytes+wx_font_bytes()+action_icons.bytes+cooldown_catalog.bytes;stats.draws=draws;stats.triangles=triangles;
        stats.failures=fixture_failures+book.failures+actors.failures+avatar.failures+avatar.scene.failures+interface.failures+portraits.failures+action_icons.failures+cooldown_catalog.failures;stats.replay_frame=frame;
        stats.ui[0]=interface.ready;stats.ui[1]=interface.bytes;stats.ui[2]=interface.quads;stats.ui[3]=interface.failures;
        stats.avatar_ready=avatar.ready;stats.avatar_bytes=avatar.bytes+avatar.scene.bytes;stats.avatar_pose=avatar.pose_hash;
        stats.actionbar_layer=layer;for(unsigned i=0;i<8;i++){stats.actionbar_slots[i]=prompts[i].server_slot;stats.actionbar_bindings[i]=prompts[i].binding;}
        unsigned metrics[]={action_icons.ready,action_icons.count,action_icons.images,action_icons.bytes,action_icons.loads,action_icons.pending,action_icons.failures,action_icons.drawn,interface.batch_count,icons_ms};memcpy(stats.icons,metrics,sizeof metrics);
        stats.cooldowns[0]=cooldown_catalog.ready;stats.cooldowns[1]=cooldown_catalog.count;stats.cooldowns[2]=cooldown_catalog.bytes;
        stats.cooldowns[3]=fixture_cooldowns.packets;stats.cooldowns[4]=fixture_cooldowns.revision;stats.cooldowns[5]=fixture_cooldowns.missing;stats.cooldowns[6]=fixture_cooldowns.overflow;stats.cooldowns[7]=sizeof fixture_cooldowns;
        if(mode>=3){stats.cooldowns[17]=frame/90%8;for(unsigned i=0;i<8;i++){stats.cooldowns[8+i]=action_cooldowns[i].remaining;if(action_cooldowns[i].held)stats.cooldowns[16]|=1u<<i;if(action_cooldowns[i].global)stats.cooldowns[23]|=1u<<i;}}
        stats.cooldowns[18]=cooldown_catalog.version;stats.cooldowns[19]=fixture_cooldowns.modifier_updates;stats.cooldowns[20]=fixture_cooldowns.modifier_ambiguous;
        stats.cooldowns[21]=fixture_cooldowns.gcd_starts;stats.cooldowns[22]=fixture_cooldowns.gcd_cancels;stats.cooldowns[24]=(unsigned)(fixture_cooldowns.cast_speed*1000);stats.cooldowns[25]=fixture_cooldowns.player_family;
        wx_hud_metrics(&hud,stats.hud);memcpy(stats.portrait,portrait_stats,sizeof portrait_stats);wx_telemetry_send(&stats);if(mode==5)wx_telemetry_actions(now,frame,5,frame/90%12,&cast_view,action_feedback,book.version);frame++;
    }
}
