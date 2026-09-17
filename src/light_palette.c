#include "wx_lighting.h"
#include <math.h>
#include <string.h>
void wx_light_fallback(WxLightPalette* p){
    if(!p)return;memset(p,0,sizeof *p);
    const float colors[WX_LIGHT_COLORS][3]={{.65f,.65f,.65f},{.35f,.35f,.35f},
        {.4f,.6f,.9f},{.44f,.59f,.71f},{.44f,.59f,.71f},{.44f,.59f,.71f},{.44f,.59f,.71f},{1,1,1}};
    memcpy(p->color,colors,sizeof colors);p->direction[0]=.3f;p->direction[1]=.4f;p->direction[2]=.86f;
}
int wx_light_palette_valid(const WxLightPalette* p){
    if(!p||p->mask>=1u<<WX_LIGHT_COLORS)return 0;
    for(unsigned b=0;b<WX_LIGHT_COLORS;b++)for(unsigned k=0;k<3;k++)if(!isfinite(p->color[b][k])||p->color[b][k]<0||p->color[b][k]>1)return 0;
    float len=0;for(unsigned k=0;k<3;k++){if(!isfinite(p->direction[k]))return 0;len+=p->direction[k]*p->direction[k];}
    return len>.98f&&len<1.02f;
}
void wx_light_blend(WxLightPalette* current,const WxLightPalette* target,float dt){
    if(!current||!wx_light_palette_valid(target))return;
    if(!wx_light_palette_valid(current)){*current=*target;return;}
    if(!isfinite(dt)||dt<=0)return;float amount=1-expf(-2*fminf(dt,5));
    for(unsigned b=0;b<WX_LIGHT_COLORS;b++)for(unsigned k=0;k<3;k++)current->color[b][k]+=(target->color[b][k]-current->color[b][k])*amount;
    float len=0;for(unsigned k=0;k<3;k++){current->direction[k]+=(target->direction[k]-current->direction[k])*amount;len+=current->direction[k]*current->direction[k];}
    if(len<.000001f)memcpy(current->direction,target->direction,sizeof current->direction);
    else {len=sqrtf(len);for(unsigned k=0;k<3;k++)current->direction[k]/=len;}
    current->mask=target->mask;
}
void wx_light_direction(const WxLightPalette* p,float angle,float result[4]){
    float co=cosf(angle),si=sinf(angle);
    result[0]=p->direction[0]*co+p->direction[1]*si;
    result[1]=p->direction[1]*co-p->direction[0]*si;
    result[2]=p->direction[2];result[3]=0;
}
void wx_light_sky_color(const WxLightPalette* p,const WxFog* fog,float altitude,float result[3]){
    /* Short view distances need the horizon to meet the fog exactly. Above
       that seam use the original DBC bands. These are sky colors, not a claim
       of complete Vanilla sky geometry, celestial bodies, clouds or weather. */
    const float heights[]={0,.08f,.2f,.5f,1};
    const float* colors[]={fog->color,p->color[WX_LIGHT_SKY_BAND2],p->color[WX_LIGHT_SKY_BAND1],p->color[WX_LIGHT_SKY_MIDDLE],p->color[WX_LIGHT_SKY_TOP]};
    if(!isfinite(altitude))altitude=0;altitude=fminf(1,fmaxf(0,altitude));
    unsigned band=0;while(band<3&&altitude>heights[band+1])band++;
    float t=(altitude-heights[band])/(heights[band+1]-heights[band]);
    for(unsigned k=0;k<3;k++)result[k]=colors[band][k]+(colors[band+1][k]-colors[band][k])*t;
}
