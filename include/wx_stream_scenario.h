#ifndef WX_STREAM_SCENARIO_H
#define WX_STREAM_SCENARIO_H
#include "wx_region.h"
/* Offline deterministic traversal only. No account, network or pad injection. */
typedef struct WxStreamScenario {
    float position[3],yaw;
    unsigned phase,frame,age,waypoint,cycles,waits,errors,wait_streak,blocker;
} WxStreamScenario;
void wx_stream_scenario_init(WxStreamScenario* scenario);
void wx_stream_scenario_step(WxStreamScenario* scenario,WxRegion* region,WxScene* scene);
#endif
