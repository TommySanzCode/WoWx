#include "wx_replay.h"
#include <string.h>
int wx_replay_open(WxReplay* replay,const char* path){
    memset(replay,0,sizeof *replay);replay->file=fopen(path,"rb");if(!replay->file)return 0;
    if(fread(&replay->header,1,sizeof replay->header,replay->file)!=sizeof replay->header)goto invalid;
    WxReplayHeader* h=&replay->header;
    if(memcmp(h->magic,"WXR1",4)||h->count>1024||h->frames>108000||(h->count==0)!=(h->frames==0))goto invalid;
    unsigned end=0;
    for(unsigned i=0;i<h->count;i++){
        WxReplayRecord record;
        if(fread(&record,1,sizeof record,replay->file)!=sizeof record||record.end_frame<=end||record.end_frame>h->frames||record.connected>1||record.buttons&~0x7fffu)goto invalid;
        end=record.end_frame;
    }
    if(end!=h->frames||fgetc(replay->file)!=EOF)goto invalid;
    if(!h->count){fclose(replay->file);replay->file=NULL;return 0;}
    if(fseek(replay->file,sizeof *h,SEEK_SET))goto invalid;
    replay->active=1;return 1;
invalid:replay->error=1;fclose(replay->file);replay->file=NULL;return 0;
}
int wx_replay_apply(WxReplay* replay,WxRawPad* output){
    if(!replay->active)return 0;
    if(replay->frame==replay->header.frames){replay->active=0;fclose(replay->file);replay->file=NULL;memset(output,0,sizeof *output);return 1;}
    replay->frame++;
    if(replay->frame>replay->record.end_frame&&fread(&replay->record,1,sizeof replay->record,replay->file)!=sizeof replay->record){
        replay->error=1;replay->active=0;fclose(replay->file);replay->file=NULL;memset(output,0,sizeof *output);return 1;
    }
    memset(output,0,sizeof *output);memcpy(output->axes,replay->record.axes,sizeof output->axes);
    output->buttons=replay->record.buttons;output->connected=(int)replay->record.connected;return 1;
}
