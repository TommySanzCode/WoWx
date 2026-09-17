#include "wx_input.h"
#include "wx_replay.h"
#include "wx_auth.h"
#include "wx_world.h"
#include <SDL.h>
#include <nxdk/mount.h>
#include <windows.h>
#include <math.h>
#include <string.h>
static SDL_GameController* controller;
static unsigned previous;
static WxControls controls;
static WxReplay replay;
static const SDL_GameControllerButton buttons[15]={
    SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_GUIDE,SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_LEFTSTICK,SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT};
void wx_pad_init(void){
    SDL_Init(SDL_INIT_GAMECONTROLLER);wx_controls_default(&controls);
    wx_replay_open(&replay,"D:\\input.rpl");
    if(!nxIsDriveMounted('E'))nxMountDrive('E',"\\Device\\Harddisk0\\Partition1\\");
    CreateDirectoryA("E:\\WOWX",NULL);
    FILE* file=fopen("E:\\WOWX\\controls.bin","rb");if(file){WxControls loaded;
        if(fread(&loaded,1,sizeof loaded,file)==sizeof loaded&&wx_controls_valid(&loaded))controls=loaded;fclose(file);}
}
float wx_pad_deadzone(void){return controls.deadzone;}
unsigned wx_pad_replay_frame(void){return replay.frame;}
int wx_pad_replay_active(void){return replay.active;}
void wx_pad_set_deadzone(float value){if(isfinite(value))controls.deadzone=fminf(.4f,fmaxf(.02f,value));}
int wx_pad_binding(int slot){return slot>=0&&slot<24?controls.actions[slot]:-1;}
void wx_pad_remap(int slot,int action){if(slot>=0&&slot<24&&action>=0&&action<120)controls.actions[slot]=(uint8_t)action;}
int wx_pad_save(void){FILE* file=fopen("E:\\WOWX\\controls.bin","wb");if(!file)return 0;
    int ok=fwrite(&controls,1,sizeof controls,file)==sizeof controls&&!fflush(file);int closed=fclose(file);if(!ok||closed)return 0;
    WxControls verify;file=fopen("E:\\WOWX\\controls.bin","rb");if(!file)return 0;
    ok=fread(&verify,1,sizeof verify,file)==sizeof verify&&wx_controls_valid(&verify)&&!memcmp(&verify,&controls,sizeof controls);
    fclose(file);return ok;}
void wx_pad_poll(WxPad* output){
    WxRawPad raw={0};SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type==SDL_CONTROLLERDEVICEADDED&&!controller)controller=SDL_GameControllerOpen(event.cdevice.which);
        if(event.type==SDL_CONTROLLERDEVICEREMOVED&&controller&&SDL_GameControllerFromInstanceID(event.cdevice.which)==controller){SDL_GameControllerClose(controller);controller=NULL;}
        if(event.type==SDL_CONTROLLERBUTTONDOWN&&controller&&SDL_GameControllerFromInstanceID(event.cbutton.which)==controller)
            for(int i=0;i<15;i++)if(event.cbutton.button==buttons[i])raw.latched|=1u<<i;
    }
    SDL_GameControllerUpdate();raw.connected=controller!=NULL;
    if(controller){
        for(int i=0;i<6;i++)raw.axes[i]=SDL_GameControllerGetAxis(controller,(SDL_GameControllerAxis)i);
        for(int i=0;i<15;i++)if(SDL_GameControllerGetButton(controller,buttons[i]))raw.buttons|=1u<<i;
    }
    // Keep the deterministic trace at frame zero until diagnostics can capture it.
    // Live input never depends on network initialization.
    WxWorldView world;wx_world_view(&world);
    if(replay.active&&replay.frame==0&&(!wx_network_ready()||!world.active))memset(&raw,0,sizeof raw);
    else wx_replay_apply(&replay,&raw);
    wx_input_process(&raw,&controls,&previous,output);
}
