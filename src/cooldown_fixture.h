/* Deterministic protocol injection for the isolated, credential-free GPU disc.
   It never sends commands to a server or changes normal input behavior. */
static void cooldown_fixture_write(uint8_t* data,unsigned* at,uint32_t value,unsigned bytes){
    for(unsigned i=0;i<bytes;i++)data[(*at)++]=(uint8_t)(value>>(i*8));
}
static void wx_cooldown_fixture_step(WxCooldowns* state,const WxCooldownCatalog* catalog,unsigned frame){
    if(frame%90)return;
    uint8_t data[96]={0};unsigned at=0,step=frame/90%8,opcode=0,now=frame*33u;
    if(step==0){
        opcode=0x12a;cooldown_fixture_write(data,&at,0,1);cooldown_fixture_write(data,&at,0,2);cooldown_fixture_write(data,&at,4,2);
        const unsigned rows[4][5]={{78,0,0,5000,0},{133,0,0,9000,0},{100,0,0,1,0x80000000u},{8690,6948,0,3600000,0}};
        for(unsigned i=0;i<4;i++){for(unsigned j=0;j<3;j++)cooldown_fixture_write(data,&at,rows[i][j],2);for(unsigned j=3;j<5;j++)cooldown_fixture_write(data,&at,rows[i][j],4);}
    }else if(step==1){
        opcode=0x134;cooldown_fixture_write(data,&at,1,4);cooldown_fixture_write(data,&at,0,4);
        cooldown_fixture_write(data,&at,133,4);cooldown_fixture_write(data,&at,7000,4);
    }else if(step==4){
        opcode=0x132;cooldown_fixture_write(data,&at,1,1);cooldown_fixture_write(data,&at,1,1); // packed caster
        cooldown_fixture_write(data,&at,1,1);cooldown_fixture_write(data,&at,1,1); // packed unit
        cooldown_fixture_write(data,&at,100,4);cooldown_fixture_write(data,&at,0,2); // flags
        cooldown_fixture_write(data,&at,0,1);cooldown_fixture_write(data,&at,0,1);cooldown_fixture_write(data,&at,0,2); // targets
    }else{
        opcode=step==3?0x135:0x1de;
        unsigned spell=step==2?78:step==5?133:step==7?8690:100;
        cooldown_fixture_write(data,&at,spell,4);cooldown_fixture_write(data,&at,1,4);cooldown_fixture_write(data,&at,0,4);
    }
    if(wx_cooldown_apply(state,catalog,NULL,(uint16_t)opcode,data,at,1,now)!=1)state->overflow++;
}
static void global_fixture_apply(WxCooldowns* s,const WxCooldownCatalog* c,unsigned opcode,const uint8_t* data,unsigned size,unsigned now){
    if(wx_cooldown_apply(s,c,NULL,(uint16_t)opcode,data,size,1,now)!=1)s->overflow++;
}
static void global_fixture_cast(WxCooldowns* s,const WxCooldownCatalog* c,unsigned spell,unsigned now,int start){
    uint8_t data[20]={0};unsigned at=0;
    for(unsigned i=0;i<4;i++)cooldown_fixture_write(data,&at,1,1);
    cooldown_fixture_write(data,&at,spell,4);cooldown_fixture_write(data,&at,0,2);
    if(start)cooldown_fixture_write(data,&at,3000,4);
    else{cooldown_fixture_write(data,&at,0,1);cooldown_fixture_write(data,&at,0,1);}
    cooldown_fixture_write(data,&at,0,2);
    global_fixture_apply(s,c,start?0x131:0x132,data,at,now);
}
static void global_fixture_spell_guid(WxCooldowns* s,const WxCooldownCatalog* c,unsigned spell,unsigned guid,unsigned opcode,unsigned now){
    uint8_t data[12]={0};unsigned at=0;
    if(opcode==0x2a6){cooldown_fixture_write(data,&at,guid,4);cooldown_fixture_write(data,&at,0,4);cooldown_fixture_write(data,&at,spell,4);}
    else{cooldown_fixture_write(data,&at,spell,4);cooldown_fixture_write(data,&at,guid,4);cooldown_fixture_write(data,&at,0,4);}
    global_fixture_apply(s,c,opcode,data,at,now);
}
static void global_fixture_modifier(WxCooldowns* s,const WxCooldownCatalog* c,unsigned spell,unsigned op,int value,unsigned now){
    const WxCooldownInfo* info=wx_cooldown_info(c,spell);unsigned bit=0;
    if(info)while(bit<64&&!(info->family_mask[bit/32]&(1u<<(bit%32))))bit++;
    if(!info||bit==64){s->missing++;return;}
    uint8_t data[6];unsigned at=0;cooldown_fixture_write(data,&at,bit,1);cooldown_fixture_write(data,&at,op,1);cooldown_fixture_write(data,&at,(uint32_t)value,4);
    global_fixture_apply(s,c,0x267,data,at,now);
}
static void wx_global_fixture_step(WxCooldowns* s,const WxCooldownCatalog* c,unsigned frame){
    unsigned step=frame/90%8,offset=frame%90,now=frame*33u;
    if(!offset){
        if(step==0){uint8_t initial[5]={0};global_fixture_apply(s,c,0x12a,initial,5,now);wx_cooldown_unit(s,8,1,2000);}
        if(step==2)global_fixture_modifier(s,c,133,21,-20,now);
        if(step==3){global_fixture_modifier(s,c,133,21,0,now);wx_cooldown_unit(s,8,.5f,2000);}
        if(step==0||step==1||step==2||step==3)global_fixture_cast(s,c,133,now,1);
        if(step==4){
            wx_cooldown_unit(s,8,1,2000);uint8_t data[24]={0};unsigned at=0;
            cooldown_fixture_write(data,&at,1,4);cooldown_fixture_write(data,&at,0,4);
            cooldown_fixture_write(data,&at,133,4);cooldown_fixture_write(data,&at,0,4);
            cooldown_fixture_write(data,&at,116,4);cooldown_fixture_write(data,&at,4000,4);
            global_fixture_apply(s,c,0x134,data,at,now);
        }
        if(step==5||step==6){
            global_fixture_spell_guid(s,c,step==5?116:100,1,0x1de,now);wx_cooldown_unit(s,1,1,2000);
            global_fixture_modifier(s,c,100,11,step==5?-50:0,now);global_fixture_cast(s,c,100,now,0);
        }
        if(step==7){
            global_fixture_spell_guid(s,c,100,1,0x1de,now);wx_cooldown_unit(s,8,1,2000);global_fixture_cast(s,c,133,now,1);
            global_fixture_spell_guid(s,c,133,2,0x2a6,now);
        }
    }
    if(step==1&&offset==15)global_fixture_spell_guid(s,c,133,1,0x2a6,now);
    if(step==2&&offset==15)global_fixture_cast(s,c,133,now,0);
    if(step==2&&offset==20)global_fixture_spell_guid(s,c,133,1,0x2a6,now);
}
