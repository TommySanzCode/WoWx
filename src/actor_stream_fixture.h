/* Synthetic units over the real traversal route; no server entities or input. */
typedef struct WxActorStreamFixture {
    WxWorldView player;unsigned errors,ever_ready,gaps,switches,last_clip,clip;
    unsigned appearance,last_appearance,revision,probe;uint32_t* published;
    uint32_t look[7],equipment[19];WxAvatarSelection selection;
} WxActorStreamFixture;
static void actor_fixture_open(WxActorStreamFixture* f){
    memset(f,0,sizeof *f);f->last_clip=f->last_appearance=UINT32_MAX;
    wx_scene_init(&actors);if(!wx_pack_open(&actors,"D:\\actors.wxp"))f->errors++;actors.budget_bytes=8u*1024u*1024u;
    if(!wx_avatar_open(&avatar,"D:\\A01000000000000.WXP","D:\\A01000000000000.WXA"))f->errors++;
    f->player.guid=1;f->player.active=1;f->player.equipment_ready_mask=0x7ffff;
    const uint32_t* look=avatar.header.look;f->player.race=look[0];f->player.gender=look[1];f->player.skin=look[2];f->player.face=look[3];
    f->player.hair_style=look[4];f->player.hair_color=look[5];f->player.facial_hair=look[6];
    for(unsigned i=0;i<avatar.header.item_count;i++)f->player.equipment_display[avatar.items[i].slot]=avatar.items[i].display;
    memcpy(f->look,look,sizeof f->look);memcpy(f->equipment,f->player.equipment_display,sizeof f->equipment);
    memset(actor_entities,0,sizeof actor_entities);world_entity_count=8;
}
static unsigned appearance_probe(void){
    unsigned h=avatar.atlas_hash;for(unsigned i=0;i<avatar.count;i++)h=(h^avatar.families[i])*16777619u;
    // Read actual published GPU memory at dispersed texels and the final mip.
    const unsigned pixels[]={0,17,511,4096,8192,16383,17476,21844};
    uint32_t* textures[]={avatar.body,avatar.cape,avatar.hair,avatar.extra};
    for(unsigned i=0;i<4;i++)if(textures[i])for(unsigned k=0;k<8;k++)h=(h^textures[i][pixels[k]])*16777619u;
    return h;
}
static void appearance_request(WxActorStreamFixture* f,unsigned frame){
    unsigned phase=frame/180%6,age=frame%180,key=phase*16+(phase==3&&age<64?(age/4)%2:0);
    if(key==f->last_appearance)return;f->last_appearance=key;
    uint32_t look[7];memcpy(look,f->look,sizeof look);memcpy(f->player.equipment_display,f->equipment,sizeof f->equipment);
    f->player.appearance_flags=phase==4?0xc00:0;
    if(phase==1){f->player.equipment_display[3]=f->player.equipment_display[7]=f->player.equipment_display[15]=f->player.equipment_display[16]=0;}
    if(phase==2||phase==3){
        if(!wx_looks_step(&avatar.looks,look,2,1)||!wx_looks_step(&avatar.looks,look,4,1))f->errors++;
        if(key&1)if(!wx_looks_step(&avatar.looks,look,3,1))f->errors++;
    }
    f->player.skin=look[2];f->player.face=look[3];f->player.hair_style=look[4];f->player.hair_color=look[5];f->player.facial_hair=look[6];
    if(!wx_avatar_select(&avatar,&f->selection,&f->player,"D:\\"))f->errors++;
}
static void actor_fixture_update(WxActorStreamFixture* f,unsigned frame,unsigned now,const float* position,const float* camera,const float* right,const float* forward,float angle){
    unsigned tick=frame%960;f->clip=tick<240?0:tick<480?5:tick<600?(frame&1?0:5):tick<780?16:6;
    if(f->clip!=f->last_clip){f->switches++;f->last_clip=f->clip;}
    memcpy(player,position,sizeof player);
    const unsigned displays[]={328,447,2410,2072,1859,4449,447,2072};
    for(unsigned i=0;i<8;i++){
        WxEntity* e=&actor_entities[i];e->guid=10+i;e->type=3;e->fields[131]=displays[i];e->fields[22]=f->clip==6?0:100;
        e->animation=f->clip;float scale=1;memcpy(e->fields+4,&scale,4);
        float side=((int)(i%4)-1.5f)*3,depth=6+(i/4)*5;
        // Unique templates at middle/far distances exercise cadence reduction;
        // two instances of 2072 demonstrate that the near request wins sharing.
        if(i==2||i==3)depth=48;if(i==5)depth=95;
        e->x=position[0]+forward[0]*depth+right[0]*side;e->y=position[1]+forward[1]*depth+right[1]*side;e->z=position[2];
        wx_floor(&scene,e->x,e->y,e->z+5,e->z-8,&e->z);e->orientation=angle+3.14159265f;
    }
    // Periodically focus the far template: selected units always get full rate.
    target_guid=frame%240<60?15:0;
    if(f->appearance)appearance_request(f,frame);
    actor_stream(now,camera,right,forward);wx_avatar_update(&avatar,&f->player,f->clip,now);
    if(f->appearance&&avatar.ready){
        unsigned probe=appearance_probe();
        if(f->revision==avatar.revision){if(f->published!=avatar.body||f->probe!=probe)f->errors++;}
        else if(memcmp(avatar.equipment,f->player.equipment_display,sizeof avatar.equipment)||memcmp(avatar.published_look,avatar.header.look,sizeof avatar.published_look)||avatar.flags!=f->player.appearance_flags)f->errors++;
        for(unsigned i=0;i<avatar.count;i++)if(!wx_animation_resident(&avatar.scene,avatar.ids[i]))f->errors++;
        f->revision=avatar.revision;f->published=avatar.body;f->probe=probe;
    }
    if(avatar.ready)f->ever_ready=1;else if(f->ever_ready)f->gaps++;
}
