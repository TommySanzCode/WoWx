#ifndef WX_INPUT_H
#define WX_INPUT_H
#include "wx_runtime.h"
enum WxButton {WX_A,WX_B,WX_X,WX_Y,WX_BACK,WX_GUIDE,WX_START,WX_LSTICK,WX_RSTICK,WX_WHITE,WX_BLACK,WX_UP,WX_DOWN,WX_LEFT,WX_RIGHT};
typedef struct WxRawPad {int16_t axes[6];uint32_t buttons,latched;int connected;} WxRawPad;
typedef struct WxControls {uint32_t magic,version;float deadzone;uint8_t actions[24];} WxControls;
void wx_controls_default(WxControls* config);
int wx_controls_valid(const WxControls* config);
void wx_input_process(const WxRawPad* raw,const WxControls* config,unsigned* previous,WxPad* output);
float wx_pad_deadzone(void);
void wx_pad_set_deadzone(float value);
int wx_pad_binding(int slot);
void wx_pad_remap(int slot,int action);
int wx_pad_save(void);
#endif
