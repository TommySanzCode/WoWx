#include "wx_input.h"
#include <math.h>
#include <string.h>
void wx_controls_default(WxControls* config){
    memset(config,0,sizeof *config);config->magic=0x31435857;config->version=1;config->deadzone=.18f;
    for(int i=0;i<24;i++)config->actions[i]=(uint8_t)i;
}
int wx_controls_valid(const WxControls* config){
    if(config->magic!=0x31435857||config->version!=1||!isfinite(config->deadzone)||config->deadzone<.02f||config->deadzone>.40f)return 0;
    for(int i=0;i<24;i++)if(config->actions[i]>=120)return 0;return 1;
}
static void stick(int16_t x,int16_t y,float deadzone,float* out_x,float* out_y){
    float fx=x/32768.f,fy=-y/32768.f,length=sqrtf(fx*fx+fy*fy);
    if(length<=deadzone){*out_x=*out_y=0;return;}
    float strength=(fminf(length,1)-deadzone)/(1-deadzone);
    *out_x=fx/length*strength;*out_y=fy/length*strength;
}
void wx_input_process(const WxRawPad* raw,const WxControls* config,unsigned* previous,WxPad* output){
    memset(output,0,sizeof *output);output->action=-1;output->slot=-1;output->connected=raw->connected;
    if(!raw->connected){*previous=0;return;}
    output->buttons=raw->buttons;output->pressed=(raw->buttons&~*previous)|raw->latched;*previous=raw->buttons;
    stick(raw->axes[0],raw->axes[1],config->deadzone,&output->move_x,&output->move_y);
    stick(raw->axes[2],raw->axes[3],config->deadzone,&output->look_x,&output->look_y);
    output->layer=(raw->axes[4]>12000?1:0)|(raw->axes[5]>12000?2:0);
    const int buttons[8]={WX_A,WX_B,WX_X,WX_Y,WX_UP,WX_DOWN,WX_LEFT,WX_RIGHT};
    if(output->layer)for(int i=0;i<8;i++)if(output->pressed&(1u<<buttons[i])){
        output->slot=(output->layer-1)*8+i;output->action=config->actions[output->slot];break;
    }
}
