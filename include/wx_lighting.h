#ifndef WX_LIGHTING_H
#define WX_LIGHTING_H
#include "wx_fog.h"
#ifdef __cplusplus
extern "C" {
#endif
/* WXL1, little endian. All fog bands are resident: no per-frame allocation/I/O.
   Positions are in the same wire/world coordinates as the native terrain. */
enum {WX_LIGHT_MAX_VOLUMES=1024,WX_LIGHT_MAX_PROFILES=1024,WX_LIGHT_KEYS=16,
      WX_LIGHT_BUDGET=512*1024,WX_LIGHT_CLEAR=0,WX_LIGHT_RAIN=1,WX_LIGHT_WATER=2};
enum {WX_LIGHT_AMBIENT,WX_LIGHT_DIFFUSE,WX_LIGHT_SKY_TOP,WX_LIGHT_SKY_MIDDLE,
      WX_LIGHT_SKY_BAND1,WX_LIGHT_SKY_BAND2,WX_LIGHT_SKY_SMOG,WX_LIGHT_SUN,WX_LIGHT_COLORS};
typedef struct WxLightHeader {uint32_t magic,version,volumes,profiles,bytes,reserved[3];} WxLightHeader;
typedef struct WxLightVolume {uint32_t id,map;float position[3],inner,outer;uint32_t profile[3];} WxLightVolume;
/* Version 1 has three bands (304-byte profiles); version 2 adds eight RGB
   bands. Values are IEEE float bits only for fog end/start bands 1 and 2. */
typedef struct WxLightBand {uint32_t count;uint16_t time[16];uint32_t value[16];} WxLightBand;
typedef struct WxLightProfile {uint32_t id;WxLightBand bands[3+WX_LIGHT_COLORS];} WxLightProfile;
typedef struct WxLightPalette {float color[WX_LIGHT_COLORS][3],direction[3];uint32_t mask;} WxLightPalette;
void wx_light_fallback(WxLightPalette* palette);
int wx_light_palette_valid(const WxLightPalette* palette);
void wx_light_blend(WxLightPalette* current,const WxLightPalette* target,float dt);
void wx_light_direction(const WxLightPalette* palette,float actor_angle,float result[4]);
void wx_light_sky_color(const WxLightPalette* palette,const WxFog* fog,float altitude,float result[3]);
typedef struct WxLighting {
    WxLightVolume* volumes;WxLightProfile* profiles;
    uint32_t volume_count,profile_count,bytes,failures;
} WxLighting;
typedef struct WxLightSample {WxFog fog;uint32_t volume[2],profile[3],authored;float weight[2];WxLightPalette palette;} WxLightSample;
void wx_telemetry_lighting(unsigned now,unsigned frame,unsigned fixture,unsigned phase,float time,unsigned condition,const WxLighting* catalog,const WxLightSample* sample,const WxFog* fog,const WxLightPalette* palette,unsigned light_ms,unsigned sky_quads);
int wx_light_open(WxLighting* catalog,const char* path);
void wx_light_close(WxLighting* catalog);
int wx_light_profile_valid(const WxLightProfile* profile);
void wx_light_sample(const WxLighting* catalog,uint32_t map,const float position[3],float half_minutes,
                     unsigned condition,unsigned environment,WxLightSample* sample);
/* Grade blends the clear and inclement profiles. Explicit indoor/underwater
   requests take priority; automatic world-environment classification is separate. */
void wx_light_weather_sample(const WxLighting* catalog,uint32_t map,const float position[3],float half_minutes,
                             float weather_weight,unsigned environment,WxLightSample* sample);
/* Vanilla 0x42: packed calendar and minutes per real second. */
typedef struct WxWorldClock {uint32_t anchor_ms,packed,valid;float minutes,speed;} WxWorldClock;
int wx_clock_apply(WxWorldClock* clock,uint16_t opcode,const void* bytes,unsigned size,uint32_t now);
float wx_clock_half_minutes(const WxWorldClock* clock,uint32_t now);
#ifdef __cplusplus
}
#endif
#endif
