/* A fixed-size candidate set is private until the entire index is scored.
   Keep existing residents/payload jobs intact while this work is deferred. */
static float selection_distance(const float* a,const float* b){
    float result=0;for(unsigned i=0;i<3;i++){float d=a[i]-b[i];result+=d*d;}return result;
}
static void selection_begin(WxScene* s,const float* position){
    WxSelection* q=&s->selection;
    q->phase=1;q->revision=s->index_revision;q->progress=0;q->count=s->header.count;
    memcpy(q->position,position,sizeof q->position);memset(q->resident,0,sizeof q->resident);
    for(unsigned n=0;n<=WX_CACHE_SLOTS;n++){
        int entry=n<WX_CACHE_SLOTS?s->slots[n].entry:s->stream_job.phase?s->stream_job.entry:-1;
        if(entry<0)continue;unsigned key=(unsigned)entry+1,at=(key*2654435761u)>>22;
        while(q->resident[at]&&q->resident[at]!=key)at=(at+1)&1023;q->resident[at]=key;
    }
    for(unsigned i=0;i<WX_CACHE_SLOTS;i++){q->wanted[i]=-1;q->scores[i]=1e20f;}
}
static int selection_step(WxScene* s,const float* position){
    WxSelection* q=&s->selection;
    // Finish snapshots during normal movement so a moving player cannot starve
    // selection. The next pass catches up; collision still rejects missing
    // geometry. Large relocations/index changes restart from current state.
    if(q->phase&&(q->revision!=s->index_revision||selection_distance(position,q->position)>=32*32)){
        q->phase=0;q->restarts++;s->wanted_ready=0;
    }
    if(!q->phase){
        if(s->wanted_ready&&selection_distance(position,s->wanted_position)<4)return 1;
        selection_begin(s,position);
    }
    unsigned scanned=0;
    while(q->progress<q->count&&scanned<WX_STREAM_FRAME_SELECT_ENTRIES){
        unsigned n=q->count-q->progress;
        if(n>WX_STREAM_FRAME_SELECT_ENTRIES-scanned)n=WX_STREAM_FRAME_SELECT_ENTRIES-scanned;
        n=stream_budget_selection(n);if(!n)return 0;
        for(unsigned end=q->progress+n;q->progress<end;q->progress++){
            unsigned i=q->progress;const WxEntry* e=&s->entries[i];
            float dx=e->center[0]-q->position[0],dy=e->center[1]-q->position[1];
            float d=sqrtf(dx*dx+dy*dy)-e->radius;int collision=e->kind==WX_KIND_COLLISION;
            unsigned key=i+1,at=(key*2654435761u)>>22;
            while(q->resident[at]&&q->resident[at]!=key)at=(at+1)&1023;
            int retained=q->resident[at]==key;
            float required=collision?45:165,prefetch=collision?55:175,release=collision?65:195;
            if(d>(retained?release:prefetch))continue;
            float score=d+(d>required?200:0);
            int begin=collision?WX_RENDER_SLOTS:0,end_slot=collision?WX_CACHE_SLOTS:WX_RENDER_SLOTS;
            if(e->flags&WX_PLACEMENT_ID){int duplicate=0;
                for(int j=begin;j<end_slot&&q->wanted[j]>=0;j++){
                    const WxEntry* other=&s->entries[q->wanted[j]];
                    if((other->flags&WX_PLACEMENT_ID)&&other->reserved[0]==e->reserved[0]&&other->id==e->id&&other->kind==e->kind){duplicate=1;break;}
                }if(duplicate)continue;
            }
            for(int j=begin;j<end_slot;j++)if(score<q->scores[j]){
                for(int k=end_slot-1;k>j;k--){q->scores[k]=q->scores[k-1];q->wanted[k]=q->wanted[k-1];}
                q->scores[j]=score;q->wanted[j]=(int)i;break;
            }
        }scanned+=n;
    }
    if(q->progress<q->count)return 0;
    memcpy(s->wanted,q->wanted,sizeof s->wanted);memcpy(s->wanted_position,q->position,sizeof s->wanted_position);
    s->wanted_ready=1;q->phase=0;q->commits++;return 1;
}
