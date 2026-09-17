#include "wx_sky.h"
#include <math.h>
#include <string.h>
unsigned wx_sky_build(WxUi* ui,const WxLightPalette* palette,const WxFog* fog,const float right[4],const float up[4],const float forward[4]){
    if(!ui||!ui->ready||ui->quads||!wx_light_palette_valid(palette)||!wx_fog_valid(fog)||fog->environment!=WX_FOG_OUTDOOR)return 0;
    enum {COLUMNS=16,ROWS=12};float colors[ROWS+1][COLUMNS+1][3];
    for(unsigned y=0;y<=ROWS;y++)for(unsigned x=0;x<=COLUMNS;x++){
        float sx=((float)x*40-320)/420,sy=(240-(float)y*40)/420;
        float altitude=(forward[2]+right[2]*sx+up[2]*sy)/sqrtf(1+sx*sx+sy*sy);
        wx_light_sky_color(palette,fog,altitude,colors[y][x]);
    }
    for(unsigned y=0;y<ROWS;y++)for(unsigned x=0;x<COLUMNS;x++){
        float corners[4][3];memcpy(corners[0],colors[y][x],12);memcpy(corners[1],colors[y][x+1],12);
        memcpy(corners[2],colors[y+1][x+1],12);memcpy(corners[3],colors[y+1][x],12);
        wx_ui_gradient(ui,x*40,y*40,40,40,corners);
    }
    return ui->quads;
}
