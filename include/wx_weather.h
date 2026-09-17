#ifndef WX_WEATHER_H
#define WX_WEATHER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum {WX_WEATHER_FINE,WX_WEATHER_RAIN,WX_WEATHER_SNOW,WX_WEATHER_STORM};
/* Vanilla 5875 weather is exactly 13 bytes, including the sound ID. Sound is
   retained for the audio service; it must not be mistaken for a change flag. */
typedef struct WxWeather {uint32_t type,sound,instant,revision,valid;float grade;} WxWeather;
typedef struct WxWeatherMix {uint32_t revision,type,snap,initialized;float weight;} WxWeatherMix;
int wx_weather_apply(WxWeather* state,uint16_t opcode,const void* bytes,unsigned size);
void wx_weather_reset(WxWeather* state);
void wx_weather_step(WxWeatherMix* mix,const WxWeather* state,float dt);
void wx_telemetry_weather(unsigned now,unsigned frame,unsigned fixture,unsigned phase,const WxWeather* state,const WxWeatherMix* mix,unsigned environment,unsigned errors);
#ifdef __cplusplus
}
#endif
#endif
