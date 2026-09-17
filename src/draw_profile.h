/* Guest submission wall time, not an independent CPU/GPU hardware timer.
   Explicit pushbuffer waits are nested in submission; final drain is separate.
   Only four pass boundaries and actual buffer wraps read the clock. */
static WxDrawStats draw_profile;
static unsigned draw_profile_active,draw_profile_start,draw_profile_mark;
static void draw_profile_begin(void){
    memset(&draw_profile,0,sizeof draw_profile);draw_profile_active=1;
    draw_profile_start=draw_profile_mark=GetTickCount();
}
static void draw_profile_pass(unsigned* elapsed){
    unsigned now=GetTickCount();*elapsed=now-draw_profile_mark;draw_profile_mark=now;
}
static void draw_profile_end(void){
    draw_profile.submit_ms=draw_profile_mark-draw_profile_start;draw_profile_active=0;
}
static void draw_profile_drain(void){
    unsigned start=GetTickCount();while(pb_busy()){}
    draw_profile.drain_ms=GetTickCount()-start;
}
