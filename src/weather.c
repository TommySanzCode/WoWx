#include "wx_weather.h"
#include <math.h>
#include <string.h>
int wx_weather_apply(WxWeather* state,uint16_t opcode,const void* bytes,unsigned size){
    if(opcode!=0x2f4)return -1;
    if(!state||!bytes||size!=13)return 0;
    const uint8_t* wire=bytes;WxWeather next={0};
    memcpy(&next.type,wire,4);memcpy(&next.grade,wire+4,4);memcpy(&next.sound,wire+8,4);next.instant=wire[12];
    if(next.type>WX_WEATHER_STORM||!isfinite(next.grade)||next.grade<0||next.grade>1||next.instant>1)return 0;
    next.valid=1;next.revision=state->revision+1;*state=next;return 1;
}
void wx_weather_reset(WxWeather* state){
    if(!state)return;uint32_t revision=state->revision+1;memset(state,0,sizeof *state);state->revision=revision;state->instant=1;
}
void wx_weather_step(WxWeatherMix* mix,const WxWeather* state,float dt){
    if(!mix||!state)return;mix->snap=0;
    float target=state->valid&&state->type!=WX_WEATHER_FINE?state->grade:0;
    if(!isfinite(target)||target<0||target>1||state->type>WX_WEATHER_STORM)return;
    if(!mix->initialized||mix->revision!=state->revision){
        mix->revision=state->revision;mix->initialized=1;mix->type=state->type;
        if(state->instant||!state->valid){mix->weight=target;mix->snap=1;}
    }
    if(isfinite(dt)&&dt>0&&!mix->snap){mix->weight+=(target-mix->weight)*(1-expf(-.8f*fminf(dt,5)));
        if(fabsf(target-mix->weight)<.0001f)mix->weight=target;}
}
