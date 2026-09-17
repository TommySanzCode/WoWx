/* Synthetic state and Vanilla packets for the opt-in WXPF0005 disc only.
   Item quantities/charges deliberately describe a fixture item, not Hearthstone. */
static void action_fixture_packet(WxCastState* s,unsigned op,const uint8_t* data,unsigned size,unsigned now,unsigned* failures){
    if(wx_cast_apply(s,(uint16_t)op,data,size,1,now)!=1)(*failures)++;
}
static void action_fixture_start(WxCastState* s,unsigned duration,unsigned now,unsigned* failures){
    uint8_t data[16]={1,1,1,1,133,0,0,0};unsigned at=10;
    cooldown_fixture_write(data,&at,duration,4);cooldown_fixture_write(data,&at,0,2);
    action_fixture_packet(s,0x131,data,at,now,failures);
}
static void action_fixture_step(WxCastState* cast,WxCooldowns* cooldowns,const WxCooldownCatalog* catalog,
    WxSpellBook* book,WxEntity units[2],WxInventory* inventory,unsigned frame,unsigned* failures){
    unsigned step=frame/90%12,offset=frame%90,now=frame*33u;uint8_t data[16]={0};unsigned at=0;
    units[0].positioned=units[1].positioned=1;units[1].x=step==2?80:step==11?50:20;
    float reach=1.5f;memcpy(&units[0].fields[130],&reach,4);memcpy(&units[1].fields[130],&reach,4);
    units[0].fields[22]=step==10?0:100;units[0].fields[23]=step==1?0:step==11?20:1000;
    units[0].fields[24]=step==3?0:150;units[0].fields[138]=step==4?17u<<16:0;
    units[0].fields[162]=1000;units[0].fields[163]=100;
    memset(inventory,0,sizeof *inventory);
    if(step==5||step==6){inventory->count=1;inventory->items[0].entry=6948;inventory->items[0].count=step==5?3:1;inventory->items[0].charges[0]=-4;}
    wx_spellbook_request(book,689);
    if(!offset){
        memset(cast,0,sizeof *cast);wx_cooldown_reset(cooldowns);wx_cooldown_unit(cooldowns,step>=3&&step<=6?1:8,1,2000);
        if(step==6){cooldowns->items[0].item=6948;cooldowns->items[0].spells[0].spell=8690;cooldowns->items[0].spells[0].charges=-5;}
        if(step==11){global_fixture_modifier(cooldowns,catalog,133,14,-50,now);global_fixture_modifier(cooldowns,catalog,133,5,100,now);}
        if(step<=2||step==11)action_fixture_start(cast,step==11?1500:3000,now,failures);
        if(step>=7&&step<=9){cooldown_fixture_write(data,&at,689,4);cooldown_fixture_write(data,&at,step==8?UINT32_MAX:3000,4);action_fixture_packet(cast,0x139,data,at,now,failures);}
    }
    if((step==1||step==11)&&offset==15){
        cooldown_fixture_write(data,&at,1,4);cooldown_fixture_write(data,&at,0,4);cooldown_fixture_write(data,&at,500,4);action_fixture_packet(cast,0x1e2,data,at,now,failures);
    }
    if(step==1&&offset==45){cooldown_fixture_write(data,&at,133,4);cooldown_fixture_write(data,&at,2,1);cooldown_fixture_write(data,&at,7,1);action_fixture_packet(cast,0x130,data,at,now,failures);}
    if((step==2&&offset==45)||(step==11&&offset==55)){
        for(unsigned i=0;i<4;i++)cooldown_fixture_write(data,&at,1,1);
        cooldown_fixture_write(data,&at,133,4);cooldown_fixture_write(data,&at,0,2);cooldown_fixture_write(data,&at,0,4);action_fixture_packet(cast,0x132,data,at,now,failures);
    }
    if((step==8&&offset==45)||(step==9&&(offset==30||offset==60))){
        cooldown_fixture_write(data,&at,step==8?1500:offset==30?1800:0,4);action_fixture_packet(cast,0x13a,data,at,now,failures);
    }
}
