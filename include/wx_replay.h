#ifndef WX_REPLAY_H
#define WX_REPLAY_H
#include "wx_input.h"
typedef struct WxReplayHeader {char magic[4];uint32_t count,frames;} WxReplayHeader;
typedef struct WxReplayRecord {uint32_t end_frame;int16_t axes[6];uint32_t buttons,connected;} WxReplayRecord;
typedef struct WxReplay {FILE* file;WxReplayHeader header;WxReplayRecord record;uint32_t frame;int active,error;} WxReplay;
int wx_replay_open(WxReplay* replay,const char* path);
int wx_replay_apply(WxReplay* replay,WxRawPad* output);
unsigned wx_pad_replay_frame(void);
int wx_pad_replay_active(void);
#endif
