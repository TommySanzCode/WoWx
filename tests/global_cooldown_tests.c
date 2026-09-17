#include "wx_cooldown.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/cooldown_fixture.h"
static unsigned checks,at;static uint8_t wire[8192];static WxCooldowns state,saved;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Global cooldown FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static WxCooldownInfo info[]={
 {.spell=100,.category=44,.category_recovery=15000,.family=4,.family_mask={2,0}},
 {.spell=133,.gcd_category=133,.gcd_time=1500,.family=3,.family_mask={1,0},.damage_class=1},
 {.spell=300,.recovery=6000,.gcd_category=133,.gcd_time=1500,.family=3,.family_mask={0,0x80000000u},.damage_class=1},
 {.spell=301,.attributes=0x10,.gcd_category=133,.gcd_time=1500,.family=4,.family_mask={1,0}},
 {.spell=302,.gcd_category=132,.gcd_time=2000,.family=3,.family_mask={2,0}},
 {.spell=303,.gcd_category=133,.gcd_time=1500,.family=3,.family_mask={1,0},.attributes_ex3=0x20000000u},
 {.spell=304,.category=55,.recovery=100,.attributes=2,.gcd_category=133,.family=3,.category_flags=2},
 {.spell=305,.recovery=100,.attributes=2,.attributes_ex2=0x20000},
 {.spell=306,.recovery=6000,.family=3,.family_mask={3,0}},
 {.spell=307,.gcd_category=133,.gcd_time=1500,.damage_class=2},
 {.spell=308,.category=55,.attributes=2,.category_flags=2}
};
static WxCooldownCatalog catalog={info,sizeof info/sizeof info[0],sizeof info,1,0,2};
unsigned wx_free_memory(void){return 48u*1024u*1024u;}
static void w(uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)wire[at++]=(uint8_t)(v>>(8*i));}
static int apply(unsigned op,unsigned now){return wx_cooldown_apply(&state,&catalog,NULL,(uint16_t)op,wire,at,1,now);}
static WxCooldownView query(unsigned spell,unsigned now){return wx_cooldown_query(&state,&catalog,spell,now);}
static void reset(unsigned cls,float haste){wx_cooldown_reset(&state);wx_cooldown_unit(&state,cls,haste,2000);}
static void mod(unsigned type,unsigned bit,unsigned op,int32_t value){at=0;w(bit,1);w(op,1);w((uint32_t)value,4);CHECK(apply(type?0x267:0x266,0)==1);}
static void cast(unsigned spell,unsigned caster,unsigned unit,unsigned flags,int start){
    at=0;w(1,1);w(caster,1);w(1,1);w(unit,1);w(spell,4);w(flags,2);
    if(start)w(3000,4);else{w(0,1);w(0,1);}w(0,2);
    if(flags&0x20){w(1234,4);w(26,4);}
}
static void fail(unsigned spell,unsigned caster){at=0;w(caster,4);w(0,4);w(spell,4);}
static void cd(unsigned spell,unsigned duration){at=0;w(1,4);w(0,4);w(spell,4);w(duration,4);}
static void clear(unsigned spell){at=0;w(spell,4);w(1,4);w(0,4);}
static void malformed(unsigned op){
    saved=state;
    for(unsigned n=0;n<at;n++){CHECK(!wx_cooldown_apply(&state,&catalog,NULL,(uint16_t)op,wire,n,1,1000));CHECK(!memcmp(&state,&saved,sizeof state));}
    wire[at]=0;CHECK(!wx_cooldown_apply(&state,&catalog,NULL,(uint16_t)op,wire,at+1,1,1000));CHECK(!memcmp(&state,&saved,sizeof state));
}
int main(int argc,char** argv){
    CHECK(sizeof(WxCooldownInfo)==56&&sizeof(WxCooldowns)==65336);reset(8,1);
    cast(133,1,1,0x20,1);malformed(0x131);CHECK(apply(0x131,1000)==1);
    CHECK(query(133,1000).global&&query(300,1000).remaining==1500&&!query(100,1000).remaining&&!query(302,1000).remaining);
    CHECK(query(133,2499).remaining==1&&!query(133,2500).remaining);
    // A preparing interruption clears its own GCD. Another GUID/spell cannot.
    fail(133,2);saved=state;CHECK(apply(0x2a6,1100)==1&&!memcmp(&state,&saved,sizeof state));
    fail(133,1);malformed(0x2a6);CHECK(apply(0x2a6,1200)==1&&!query(133,1200).remaining&&state.gcd_cancels==1);
    cast(133,1,1,0,1);CHECK(apply(0x131,3000)==1);cast(133,1,1,0,0);CHECK(apply(0x132,3100)==1);
    fail(133,1);CHECK(apply(0x2a6,3200)==1&&query(300,3200).remaining==1300&&state.gcd_cancels==1); // no restart/clear after GO
    cast(133,1,1,0,1);CHECK(apply(0x131,5000)==1);cast(302,1,1,0,1);CHECK(apply(0x131,5100)==1);
    fail(133,1);CHECK(apply(0x2a6,5200)==1&&query(133,5200).remaining==1300);
    fail(302,1);CHECK(apply(0x2a6,5200)==1&&!query(302,5200).remaining&&query(133,5200).remaining==1300);
    reset(8,.25f);cast(133,1,1,0,1);CHECK(apply(0x131,0)==1&&query(133,0).duration==1000);
    reset(8,2);cast(133,1,1,0,1);CHECK(apply(0x131,0)==1&&query(133,0).duration==1500);
    reset(1,.25f);cast(301,1,1,0,1);CHECK(apply(0x131,0)==1&&query(301,0).duration==1500);
    reset(8,.25f);cast(307,1,1,0,1);CHECK(apply(0x131,0)==1&&query(307,0).duration==1500);
    // Server modifiers replace totals; the sign and all 64 family bits matter.
    reset(8,1);mod(1,63,11,-50);mod(1,63,11,-50);cast(300,1,1,0,0);CHECK(apply(0x132,0)==1&&query(300,0).remaining==3000);
    mod(0,63,11,-1000);cast(300,1,1,0,0);CHECK(apply(0x132,4000)==1&&query(300,4000).remaining==2500);
    mod(1,63,11,0);cast(300,1,1,0,0);CHECK(apply(0x132,8000)==1&&query(300,8000).remaining==5000);
    wx_cooldown_unit(&state,1,1,2000);cast(300,1,1,0,0);CHECK(apply(0x132,14000)==1&&query(300,14000).remaining==6000); // family isolation
    mod(1,1,11,-50);cast(100,1,1,0,0);CHECK(apply(0x132,21000)==1&&query(100,21000).remaining==7500); // category recovery receives mod if own recovery is zero
    reset(8,.5f);mod(0,0,21,-500);cast(133,1,1,0,1);CHECK(apply(0x131,0)==1&&query(133,0).duration==1000);
    reset(8,1);mod(1,0,21,-20);cast(133,1,1,0,1);CHECK(apply(0x131,0)==1&&query(133,0).duration==1200);
    reset(8,1);mod(0,0,21,-500);cast(303,1,1,0,1);CHECK(apply(0x131,0)==1&&query(303,0).duration==1500); // ignore-caster-modifiers attribute
    reset(8,1);mod(0,0,11,-500);mod(0,1,11,-500);cast(306,1,1,0,0);CHECK(apply(0x132,0)==1&&query(306,0).remaining==6000&&state.modifier_ambiguous==1);
    mod(0,1,11,0);cast(306,1,1,0,0);CHECK(apply(0x132,7000)==1&&query(306,7000).remaining==5500);
    reset(8,1);mod(0,63,11,INT32_MAX);mod(1,63,11,INT32_MAX);cast(300,1,1,0,0);CHECK(apply(0x132,0)==1&&query(300,0).remaining==INT32_MAX);
    reset(8,1);mod(0,63,11,INT32_MIN);cast(300,1,1,0,0);CHECK(apply(0x132,0)==1&&!query(300,0).remaining);
    at=0;w(63,1);w(28,1);w(123,4);malformed(0x266);CHECK(apply(0x266,0)==1);saved=state;
    wire[0]=64;CHECK(!apply(0x266,0)&&!memcmp(&state,&saved,sizeof state));wire[0]=0;wire[1]=29;CHECK(!apply(0x267,0)&&!memcmp(&state,&saved,sizeof state));
    // Zero-duration server hint must not erase a real school lockout.
    reset(8,1);cd(133,8000);CHECK(apply(0x134,0)==1);cd(133,0);CHECK(apply(0x134,100)==1);
    CHECK(query(133,100).remaining==7900&&!query(133,100).global&&query(300,100).remaining==1500&&query(300,100).global);
    clear(133);CHECK(apply(0x1de,200)==1&&query(133,200).remaining==1400&&query(133,200).global); // clear normal recovery, keep independent GCD
    reset(8,1);cast(304,1,1,0,0);CHECK(apply(0x132,0)==1&&query(304,0).remaining==2100&&query(133,0).remaining==2100); // ranged speed + category global flag
    cast(305,1,1,0,0);CHECK(apply(0x132,0)==1&&query(305,0).remaining==100);
    reset(8,1);cast(308,1,1,0,0);CHECK(apply(0x132,0)==1&&query(308,0).remaining==2000&&query(133,0).remaining==2000); // zero DBC recovery still uses the equipped ranged speed
    reset(8,1);cast(133,1,1,0,1);CHECK(apply(0x131,UINT32_MAX-499)==1&&query(133,0).remaining==1000);fail(133,1);CHECK(apply(0x2a6,0)==1&&!query(133,0).remaining);
    reset(8,1);mod(0,0,21,-500);cast(133,1,1,0,1);CHECK(apply(0x131,0)==1);at=0;w(0,1);w(0,2);w(0,2);CHECK(apply(0x12a,500)==1&&!query(133,500).remaining&&state.modifiers[0][21][0]==-500);
    // Full GCD storage preserves active categories, diagnoses overflow and reuses expired slots.
    reset(8,1);
    for(unsigned category=1;category<=WX_GCD_LIMIT+1;category++){
        info[1].gcd_category=category;cast(133,1,1,0,1);CHECK(apply(0x131,0)==1);
    }
    CHECK(state.overflow==1&&state.gcd_starts==WX_GCD_LIMIT);
    for(unsigned i=0;i<WX_GCD_LIMIT;i++)CHECK(state.global[i].category==i+1&&state.global[i].duration==1500);
    CHECK(apply(0x131,1500)==1&&state.gcd_starts==WX_GCD_LIMIT+1&&query(133,1500).remaining==1500);
    info[1].gcd_category=133;
    wx_cooldown_reset(&state);CHECK(!state.modifier_updates&&!state.modifiers[0][21][0]&&!state.global[0].duration&&state.cast_speed==1);
    wx_cooldown_unit(&state,99,NAN,INFINITY);CHECK(!state.player_family&&state.cast_speed==1&&!state.ranged_ms);
    if(argc>=2){WxCooldownCatalog real={0};CHECK(wx_cooldown_catalog_open(&real,argv[1])&&real.version==2);const WxCooldownInfo* fire=wx_cooldown_info(&real,133);const WxCooldownInfo* charge=wx_cooldown_info(&real,100);
        CHECK(fire&&fire->family==3&&fire->gcd_category==133&&fire->gcd_time==1500&&fire->family_mask[0]==0x40000001u&&fire->damage_class==1);
        CHECK(charge&&charge->family==4&&charge->category_recovery==15000);printf("Real v2 catalog: %u records, %u bytes; Fireball GCD %u ms; Charge mask %08x:%08x\n",real.count,real.bytes,fire->gcd_time,charge->family_mask[1],charge->family_mask[0]);wx_cooldown_reset(&state);
        for(unsigned frame=0;frame<2160;frame++){
            wx_global_fixture_step(&state,&real,frame);unsigned step=frame/90%8,delta=frame%90*33;
            uint32_t duration=step==3?1000:step==2?1200:1500;
            uint32_t expected=step==5||step==6||(step==1&&frame%90>=15)||delta>=duration?0:duration-delta;
            WxCooldownView view=wx_cooldown_query(&state,&real,133,frame*33);
            CHECK(view.remaining==expected&&view.global==(expected!=0)&&!state.missing&&!state.overflow&&!state.modifier_ambiguous);
            view=wx_cooldown_query(&state,&real,100,frame*33);
            CHECK(view.remaining==(step==5?7500-delta:step==6?15000-delta:0));
        }
        CHECK(state.modifier_updates==12&&state.gcd_starts==18&&state.gcd_cancels==3);
        wx_cooldown_catalog_close(&real);
    }
    if(argc==3){WxCooldownCatalog legacy={0};CHECK(wx_cooldown_catalog_open(&legacy,argv[2])&&legacy.version==1);CHECK(!wx_cooldown_info(&legacy,133)->gcd_time&&wx_cooldown_info(&legacy,100)->category_recovery==15000);wx_cooldown_catalog_close(&legacy);}
    printf("Global cooldowns, cast lifecycle, haste, modifiers, high bits, overflow and family boundaries: %u checks pass\n",checks);return 0;
}
