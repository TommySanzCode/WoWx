#include "wx_stream_scenario.h"
#include <math.h>
#include <string.h>
static void phase(WxStreamScenario* s,unsigned value){s->phase=value;s->age=s->waypoint=s->wait_streak=0;}
void wx_stream_scenario_init(WxStreamScenario* s){
    memset(s,0,sizeof *s);s->position[0]=-9048;s->position[1]=-160;s->position[2]=84.439598f;s->yaw=3.14159265f;
}
static int route(WxStreamScenario* s,WxScene* scene,const float points[][2],unsigned count){
    if(s->waypoint==count)return 1;
    float dx=points[s->waypoint][0]-s->position[0],dy=points[s->waypoint][1]-s->position[1],distance=hypotf(dx,dy);
    if(distance<.04f){s->waypoint++;return s->waypoint==count;}
    s->yaw=atan2f(dy,dx);float step=fminf(4.9f/30,distance),x=s->position[0]+dx*step/distance,y=s->position[1]+dy*step/distance,z=s->position[2];
    if(wx_walk(scene,s->position,x,y,&z)){s->position[0]=x;s->position[1]=y;s->position[2]=z;s->wait_streak=0;s->blocker=0;}
    else {s->waits++;s->wait_streak++;s->blocker=wx_walk_blocker(NULL);if(s->wait_streak>300){s->errors++;phase(s,8);}}
    return 0;
}
void wx_stream_scenario_step(WxStreamScenario* s,WxRegion* region,WxScene* scene){
    s->frame++;s->age++;
    static const float boundary[][2]={{-9085,-160},{-9048,-160}};
    static const float abbey[][2]={{-8923.5f,-142.5f},{-8920,-141},{-8918.5f,-139.5f},{-8914.5f,-140.5f},{-8912.5f,-143},{-8905,-160},
        {-8912.5f,-143},{-8914.5f,-140.5f},{-8918.5f,-139.5f},{-8920,-141},{-8923.5f,-142.5f},{-8934,-140}};
    static const float camp[][2]={{-8910.5f,-133.5f},{-8876,-146.5f},{-8803,-177},{-8876,-146.5f},{-8910.5f,-133.5f},{-8934,-140}};
    if(s->phase==0||s->phase==4){
        if(!region->pending.phase&&region->loaded&&wx_stream_ready(scene))phase(s,s->phase==0?1:5);
        else if(s->age>1800){s->errors++;phase(s,8);}
    }else if(s->phase==1||s->phase==2){if(s->age>=180)phase(s,s->phase+1);}
    else if(s->phase==3){if(route(s,scene,boundary,2)){
        s->position[0]=-8934;s->position[1]=-140;s->position[2]=83.227203f;s->yaw=0;phase(s,4);}}
    else if(s->phase==5){if(route(s,scene,abbey,sizeof abbey/sizeof abbey[0]))phase(s,6);}
    else if(s->phase==6){if(route(s,scene,camp,sizeof camp/sizeof camp[0])){s->cycles++;phase(s,7);}}
    else if(s->phase==7&&s->age>=90){unsigned frame=s->frame,cycles=s->cycles,waits=s->waits;wx_stream_scenario_init(s);s->frame=frame;s->cycles=cycles;s->waits=waits;}
}
