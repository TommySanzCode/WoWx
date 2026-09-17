/* Included by pack.c: one render-thread frame, with no heap allocations. */
static WxStreamFrame frame_metrics;
static WxStreamUse frame_left[WX_STREAM_LANES];
static unsigned frame_active,frame_mask;
static unsigned frame_copy_bytes;
static unsigned frame_selection_entries;
unsigned wx_stream_index_copy_bytes(void){return frame_copy_bytes;}
unsigned wx_stream_selection_entries(void){return frame_selection_entries;}
static WxStreamClock frame_clock;
static WxStreamTime frame_time;
static unsigned time_started,time_start[WX_STREAM_LANES];
void wx_stream_set_clock(WxStreamClock clock){if(!frame_active)frame_clock=clock;}
const WxStreamTime* wx_stream_time_metrics(void){return &frame_time;}
static int stream_budget_time_ready(unsigned lane){
    if(!frame_active||!frame_clock)return 1;
    if(!(frame_mask&(1u<<lane))||!frame_time.limit_ms[lane])return 0;
    if(frame_time.yield_mask&(1u<<lane))return 0;
    unsigned now=frame_clock();
    if(!(time_started&(1u<<lane))){time_started|=1u<<lane;time_start[lane]=now;}
    // Unsigned subtraction also handles the native millisecond clock wrapping.
    unsigned elapsed=now-time_start[lane];frame_time.observed_ms[lane]=elapsed;
    if(elapsed<frame_time.limit_ms[lane])return 1;
    frame_time.yield_mask|=1u<<lane;return 0;
}
void wx_stream_frame_begin(unsigned mask){
    static const WxStreamUse shares[WX_STREAM_LANES]={
        {8192,2,16384,0},{24576,6,16384,1},{16384,4,16384,1},{16384,4,16384,1}};
    memset(&frame_metrics,0,sizeof frame_metrics);memset(frame_left,0,sizeof frame_left);
    memset(&frame_time,0,sizeof frame_time);time_started=0;
    frame_copy_bytes=0;
    frame_selection_entries=0;
    frame_time.enabled=frame_clock!=NULL;
    frame_active=1;frame_mask=mask&15;frame_metrics.enabled=1;
    unsigned receiver=WX_STREAM_WORLD;
    if(!(frame_mask&(1u<<receiver))){receiver=0;while(receiver<WX_STREAM_LANES&&!(frame_mask&(1u<<receiver)))receiver++;}
    for(unsigned i=0;i<WX_STREAM_LANES;i++){
        unsigned to=frame_mask&(1u<<i)?i:receiver;if(to==WX_STREAM_LANES)continue;
        static const unsigned milliseconds[WX_STREAM_LANES]={2,6,2,2};
        if(frame_clock)frame_time.limit_ms[to]+=milliseconds[i];
        frame_left[to].read_bytes+=shares[i].read_bytes;frame_left[to].read_ops+=shares[i].read_ops;
        frame_left[to].scan_bytes+=shares[i].scan_bytes;frame_left[to].allocations+=shares[i].allocations;
    }
}
void wx_stream_frame_end(void){frame_active=0;}
static unsigned stream_budget_index_copy(unsigned wanted){
    if(!stream_budget_time_ready(WX_STREAM_INDEX))return 0;
    unsigned n=wanted<WX_STREAM_CHUNK_BYTES?wanted:WX_STREAM_CHUNK_BYTES;
    if(frame_active){
        if(n>WX_STREAM_FRAME_COPY_BYTES-frame_copy_bytes)n=WX_STREAM_FRAME_COPY_BYTES-frame_copy_bytes;
        frame_copy_bytes+=n;
    }return n;
}
const WxStreamFrame* wx_stream_frame_metrics(void){return &frame_metrics;}
static unsigned stream_budget_selection(unsigned wanted){
    if(!stream_budget_time_ready(WX_STREAM_WORLD))return 0;
    unsigned n=wanted<32?wanted:32;
    if(frame_active){
        if(!(frame_mask&(1u<<WX_STREAM_WORLD)))return 0;
        if(n>WX_STREAM_FRAME_SELECT_ENTRIES-frame_selection_entries)n=WX_STREAM_FRAME_SELECT_ENTRIES-frame_selection_entries;
        frame_selection_entries+=n;
    }return n;
}
static void stream_budget_enter(unsigned lane){
    if(!frame_active||!(frame_mask&(1u<<lane)))return;
    for(unsigned i=0;i<lane;i++){
        frame_left[lane].read_bytes+=frame_left[i].read_bytes;frame_left[lane].read_ops+=frame_left[i].read_ops;
        frame_left[lane].scan_bytes+=frame_left[i].scan_bytes;frame_left[lane].allocations+=frame_left[i].allocations;
        memset(frame_left+i,0,sizeof frame_left[i]);
    }
}
static unsigned stream_budget_read_room(unsigned lane,unsigned wanted){
    if(!frame_active)return wanted;
    if(!stream_budget_time_ready(lane))return 0;
    if(!frame_left[lane].read_ops)return 0;
    return wanted<frame_left[lane].read_bytes?wanted:frame_left[lane].read_bytes;
}
static void stream_budget_read(unsigned lane,unsigned bytes){
    if(!frame_active)return;
    frame_left[lane].read_bytes-=bytes;frame_left[lane].read_ops--;
    frame_metrics.total.read_bytes+=bytes;frame_metrics.total.read_ops++;
    frame_metrics.lane[lane].read_bytes+=bytes;frame_metrics.lane[lane].read_ops++;
}
static unsigned stream_budget_scan_room(unsigned lane,unsigned wanted){
    if(!stream_budget_time_ready(lane))return 0;
    return !frame_active||wanted<frame_left[lane].scan_bytes?wanted:frame_left[lane].scan_bytes;
}
static void stream_budget_scan(unsigned lane,unsigned bytes){
    if(!frame_active)return;
    frame_left[lane].scan_bytes-=bytes;frame_metrics.total.scan_bytes+=bytes;frame_metrics.lane[lane].scan_bytes+=bytes;
}
static int stream_budget_allocate(unsigned lane){
    if(!frame_active)return 1;if(!stream_budget_time_ready(lane)||!frame_left[lane].allocations)return 0;
    frame_left[lane].allocations--;frame_metrics.total.allocations++;frame_metrics.lane[lane].allocations++;return 1;
}
int wx_stream_index_ready(void){return stream_budget_read_room(WX_STREAM_INDEX,sizeof(WxPackHeader))>=sizeof(WxPackHeader);}
unsigned wx_stream_read_grant(unsigned lane,unsigned wanted){
    if(lane>=WX_STREAM_LANES||!wanted)return 0;stream_budget_enter(lane);
    unsigned n=stream_budget_read_room(lane,wanted);if(n)stream_budget_read(lane,n);return n;
}
int wx_stream_allocation_grant(unsigned lane){
    if(lane>=WX_STREAM_LANES)return 0;stream_budget_enter(lane);return stream_budget_allocate(lane);
}
int wx_stream_frame_active(void){return frame_active!=0;}
int wx_stream_scan_grant(unsigned lane,unsigned bytes){
    if(lane>=WX_STREAM_LANES||!bytes)return 0;stream_budget_enter(lane);
    if(stream_budget_scan_room(lane,bytes)<bytes)return 0;stream_budget_scan(lane,bytes);return 1;
}
