#ifndef WX_TRAIL_H
#define WX_TRAIL_H
/* Development replay breadcrumbs. Positions come from ordinary collision movement. */
#define WX_TRAIL_CAPACITY 4096
typedef struct WxTrail {unsigned count,failed;float points[WX_TRAIL_CAPACITY][3];} WxTrail;
void wx_trail_reset(WxTrail* trail,const float position[3]);
int wx_trail_record(WxTrail* trail,const float position[3],int endpoint);
#endif
