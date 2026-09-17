#include "wx_weather.h"
#include "wx_lighting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks;
unsigned wx_free_memory(void){return 64u*1024u*1024u;}
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Weather FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
#define NEAR(a,b) CHECK(fabsf((a)-(b))<.0001f)
static void encode(uint8_t b[13],uint32_t type,float grade,uint32_t sound,unsigned instant){memcpy(b,&type,4);memcpy(b+4,&grade,4);memcpy(b+8,&sound,4);b[12]=instant;}
int main(int argc,char** argv){
    CHECK(sizeof(WxWeather)==24&&sizeof(WxWeatherMix)==20);WxWeather state={0};uint8_t packet[20]={0};
    encode(packet,1,.25f,8533,0);CHECK(wx_weather_apply(&state,0x2f4,packet,13)==1);CHECK(state.valid&&state.type==1&&state.sound==8533&&!state.instant&&state.revision==1);NEAR(state.grade,.25f);
    WxWeather before=state;
    for(unsigned size=0;size<20;size++)if(size!=13){CHECK(!wx_weather_apply(&state,0x2f4,packet,size));CHECK(!memcmp(&state,&before,sizeof state));}
    CHECK(!wx_weather_apply(NULL,0x2f4,packet,13));CHECK(!wx_weather_apply(&state,0x2f4,NULL,13));CHECK(wx_weather_apply(&state,0x42,NULL,0)==-1);
    const float bad[]={-.001f,1.001f,NAN,INFINITY,-INFINITY};
    for(unsigned i=0;i<5;i++){encode(packet,1,bad[i],8533,0);CHECK(!wx_weather_apply(&state,0x2f4,packet,13));CHECK(!memcmp(&state,&before,sizeof state));}
    encode(packet,4,.2f,8533,0);CHECK(!wx_weather_apply(&state,0x2f4,packet,13));encode(packet,1,.2f,8533,2);CHECK(!wx_weather_apply(&state,0x2f4,packet,13));
    WxWeatherMix mix={0};wx_weather_step(&mix,&state,0);CHECK(mix.initialized&&!mix.snap&&mix.weight==0);
    float last=0;for(unsigned i=0;i<240;i++){wx_weather_step(&mix,&state,1.f/30);CHECK(mix.weight>=last&&mix.weight<=.25f&&!mix.snap);last=mix.weight;}CHECK(mix.weight>.249f);
    encode(packet,2,.65f,8538,1);CHECK(wx_weather_apply(&state,0x2f4,packet,13)==1);wx_weather_step(&mix,&state,0);CHECK(mix.snap&&mix.type==2);NEAR(mix.weight,.65f);
    wx_weather_step(&mix,&state,0);CHECK(!mix.snap);
    encode(packet,3,1,8558,0);CHECK(wx_weather_apply(&state,0x2f4,packet,13)==1);wx_weather_step(&mix,&state,NAN);NEAR(mix.weight,.65f);
    wx_weather_step(&mix,&state,-1);NEAR(mix.weight,.65f);wx_weather_step(&mix,&state,.1f);CHECK(mix.weight>.65f&&mix.weight<1);
    encode(packet,0,1,0,0);CHECK(wx_weather_apply(&state,0x2f4,packet,13)==1);
    for(unsigned i=0;i<400;i++){float previous=mix.weight;wx_weather_step(&mix,&state,1.f/30);CHECK(mix.weight<=previous&&mix.weight>=0);}CHECK(mix.weight<.001f);
    uint32_t revision=state.revision;wx_weather_reset(&state);CHECK(!state.valid&&!state.type&&!state.sound&&!state.grade&&state.revision==revision+1);
    wx_weather_step(&mix,&state,0);CHECK(mix.snap&&mix.weight==0);wx_weather_step(&mix,&state,.033f);CHECK(!mix.snap);
    state.revision=UINT32_MAX;encode(packet,1,1,UINT32_MAX,1);CHECK(wx_weather_apply(&state,0x2f4,packet,13)==1);CHECK(!state.revision&&state.sound==UINT32_MAX);
    wx_weather_step(&mix,&state,0);CHECK(mix.snap&&mix.weight==1);wx_weather_reset(&state);wx_weather_step(&mix,&state,0);CHECK(mix.snap&&!mix.weight);
    if(argc>1){WxLighting catalog={0};CHECK(wx_light_open(&catalog,argv[1]));float pos[]={-8949.95f,-132.493f,83.5f};WxLightSample clear,rain,sample;
        wx_light_weather_sample(&catalog,0,pos,1440,0,0,&clear);wx_light_weather_sample(&catalog,0,pos,1440,1,0,&rain);CHECK(clear.profile[2]==12&&rain.profile[2]==13);
        wx_light_weather_sample(&catalog,0,pos,1440,.25f,0,&sample);NEAR(sample.fog.start,49.375f);
        CHECK(sample.palette.mask==255&&wx_light_palette_valid(&sample.palette));
        wx_light_weather_sample(&catalog,0,pos,1440,1,WX_FOG_UNDERWATER,&sample);CHECK(sample.profile[2]==10&&sample.fog.environment==2);
        wx_light_weather_sample(&catalog,0,pos,1440,1,WX_FOG_INDOOR,&sample);CHECK(!sample.authored&&!sample.fog.enabled&&!sample.palette.mask);
        wx_light_weather_sample(&catalog,0,pos,1440,NAN,0,&sample);CHECK(!memcmp(&sample,&clear,sizeof clear));
        for(unsigned i=0;i<catalog.volume_count;i++)for(unsigned t=0;t<2880;t+=240)for(unsigned g=0;g<=4;g++){
            wx_light_weather_sample(&catalog,catalog.volumes[i].map,catalog.volumes[i].position,(float)t,g*.25f,WX_FOG_OUTDOOR,&sample);
            CHECK(wx_fog_valid(&sample.fog)&&wx_light_palette_valid(&sample.palette));
        }
        wx_light_close(&catalog);
    }
    printf("Vanilla weather: %u checks pass\n",checks);return 0;
}
