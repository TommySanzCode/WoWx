#include "wx_backdrop.h"
#include <math.h>
#include <string.h>
#define AT(d,type,off) ((const type*)((const uint8_t*)(d)+(off)))
static int finite_array(const float* p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1000000)return 0;return 1;}
static int span(unsigned off,unsigned n,unsigned stride,unsigned size){return off>=sizeof(WxBackdropHeader)&&!(off&3)&&off<=size&&n<=(size-off)/stride;}
static int track(const void* d,const WxBackdropHeader* h,unsigned id,unsigned kind){return id==WX_BACKDROP_NONE||(id<h->tracks&&AT(d,WxBackdropTrack,h->track_offset)[id].kind==kind);}
unsigned wx_backdrop_effect_bytes(const WxBackdropHeader* h,const WxBackdropEffects* e){
    return (e->emitters?sizeof(WxBackdropSimulation)+e->capacity*4*sizeof(WxEffectVertex):0)+(e->lights?h->vertices*17+sizeof(WxBackdropLighting):0);
}
int wx_backdrop_effect_validate(const void* d,unsigned size){
    const WxBackdropHeader* h=d;if(h->version==1)return h->reserved==0;
    if(!span(h->reserved,1,sizeof(WxBackdropEffects),size))return 0;
    const WxBackdropEffects* e=AT(d,WxBackdropEffects,h->reserved);
    if(e->emitters>WX_BACKDROP_EMITTERS||e->lights>WX_BACKDROP_LIGHTS||e->capacity!=(e->emitters?WX_BACKDROP_PARTICLES:0)||
       !span(e->emitter_offset,e->emitters,sizeof(WxBackdropEmitter),size)||!span(e->light_offset,e->lights,sizeof(WxBackdropLight),size)||!span(e->color_offset,h->batches,4,size))return 0;
    const uint32_t* colors=AT(d,uint32_t,e->color_offset);for(unsigned i=0;i<h->batches;i++)if(!track(d,h,colors[i],1))return 0;
    const WxBackdropEmitter* emitters=AT(d,WxBackdropEmitter,e->emitter_offset);
    for(unsigned i=0;i<e->emitters;i++){
        const WxBackdropEmitter* p=emitters+i;
        if(!wx_particle_flags_supported(p->flags)||p->bone>=h->bones||p->texture>=h->textures||p->blend>7||p->type<1||p->type>2||
           !p->rows||!p->columns||p->rows>16||p->columns>16||p->tile_rotation>3||!finite_array(p->position,3)||
           !finite_array(&p->midpoint,16)||p->midpoint<0||p->midpoint>1)return 0;
        for(unsigned j=0;j<3;j++){
            if(p->scale[j]<0||p->scale[j]>100)return 0;
            for(unsigned c=0;c<4;c++)if(p->color[j][c]<0||p->color[j][c]>1)return 0;
        }
        for(unsigned j=0;j<WX_FX_TRACKS;j++)if(!track(d,h,p->tracks[j],0))return 0;
    }
    const WxBackdropLight* lights=AT(d,WxBackdropLight,e->light_offset);
    for(unsigned i=0;i<e->lights;i++){
        const WxBackdropLight* l=lights+i;if(l->type>1||(l->bone!=WX_BACKDROP_NONE&&l->bone>=h->bones)||!finite_array(l->position,3))return 0;
        const uint32_t ids[]={l->ambient,l->ambient_intensity,l->diffuse,l->diffuse_intensity,l->start,l->end,l->enabled};
        for(unsigned j=0;j<7;j++)if(!track(d,h,ids[j],j==0||j==2?1:0))return 0;
    }
    return 1;
}
static float scalar(const WxBackdrop* s,unsigned id,unsigned time,float fallback){float v[4],f[4]={fallback,0,0,0};wx_backdrop_sample(s,id,time,f,v);return v[0];}
static float clamp(float x,float a,float b){return fmaxf(a,fminf(b,x));}
static void transform(const float* m,const float* v,float* out,int point){for(unsigned j=0;j<3;j++)out[j]=m[j*4]*v[0]+m[j*4+1]*v[1]+m[j*4+2]*v[2]+(point?m[j*4+3]:0);}
static void normalize(float* v){float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n>.00001f)for(unsigned j=0;j<3;j++)v[j]/=n;}
int wx_backdrop_view(WxBackdrop* s,float x,float y,float width,float height){
    const float args[]={x,y,width,height};if(!s->ready||!finite_array(args,4)||width<=0||height<=0||x<0||y<0||x+width>640||y+height>480)return 0;
    WxBackdropView* v=&s->view;memcpy(v->camera,s->h.camera,12);
    for(unsigned j=0;j<3;j++)v->forward[j]=s->h.target[j]-v->camera[j];normalize(v->forward);
    v->right[0]=v->forward[1];v->right[1]=-v->forward[0];v->right[2]=0;normalize(v->right);
    v->up[0]=v->right[1]*v->forward[2];v->up[1]=-v->right[0]*v->forward[2];v->up[2]=v->right[0]*v->forward[1]-v->right[1]*v->forward[0];
    v->center[0]=x+width*.5f;v->center[1]=y+height*.5f;v->focal=height*.5f/tanf(s->h.fov*.5f);v->near_clip=s->h.near_clip;v->far_clip=s->h.far_clip;return 1;
}
static float random01(WxBackdropSimulation* f){unsigned x=f->rng;x^=x<<13;x^=x>>17;x^=x<<5;f->rng=x;return (float)(x>>8)*(1.0f/16777216);}
static void spawn(WxBackdrop* s,unsigned ei,const float* values){
    WxBackdropSimulation* f=s->simulation;
    if(f->active==s->effects.capacity){f->dropped++;return;}
    const WxBackdropEmitter* e=AT(s->data,WxBackdropEmitter,s->effects.emitter_offset)+ei;
    WxParticle* p=f->particles+f->active++;memset(p,0,sizeof *p);p->emitter=ei;p->life=clamp(values[WX_FX_LIFE],.001f,60);
    float position[3],direction[3]={0,0,1};memcpy(position,e->position,12);
    float a=(random01(f)*2-1)*values[WX_FX_VERTICAL]*.5f,b=(random01(f)*2-1)*values[WX_FX_HORIZONTAL]*.5f;
    direction[0]=sinf(a)*cosf(b);direction[1]=sinf(a)*sinf(b);direction[2]=cosf(a);
    if(e->type==1){position[0]+=(random01(f)-.5f)*values[WX_FX_LENGTH];position[1]+=(random01(f)-.5f)*values[WX_FX_WIDTH];}
    else{float radius=random01(f);position[0]+=direction[0]*radius*values[WX_FX_LENGTH]*.5f;position[1]+=direction[1]*radius*values[WX_FX_WIDTH]*.5f;position[2]+=direction[2]*radius*values[WX_FX_LENGTH]*.5f;}
    if(values[WX_FX_ZSOURCE]!=0){direction[0]=position[0]-e->position[0];direction[1]=position[1]-e->position[1];direction[2]=position[2]-e->position[2]+values[WX_FX_ZSOURCE];normalize(direction);}
    /* The up-direction flag is tested only in the sphere emitter. Plane assets
       also contain it; treating those as spheres changes the authored effect. */
    if(e->type==2&&(e->flags&WX_PARTICLE_SPHERE_UP)&&values[WX_FX_ZSOURCE]==0){direction[0]=direction[1]=0;direction[2]=1;}
    if(e->flags&WX_PARTICLE_LOCAL){memcpy(p->position,position,12);memcpy(p->velocity,direction,12);}
    else{transform(s->matrices[e->bone],position,p->position,1);transform(s->matrices[e->bone],direction,p->velocity,0);}
    float speed=values[WX_FX_SPEED]*(1+(random01(f)*2-1)*values[WX_FX_VARIATION]);
    for(unsigned j=0;j<3;j++)p->velocity[j]*=speed;
    if(!finite_array(p->position,8)){f->active--;f->dropped++;return;}
    f->born++;if(f->active>f->peak)f->peak=f->active;
}
static void particles(WxBackdrop* s,unsigned time){
    WxBackdropSimulation* f=s->simulation;if(!f)return;
    if(!f->clock_started){f->clock_started=1;f->last_time=time;f->rng=0x57201201;}
    unsigned elapsed=time-f->last_time;f->last_time=time;
    /* A pause, clock rewind or loading stall cannot create unbounded catch-up. */
    if(elapsed>250){elapsed=0;f->active=0;memset(f->accumulators,0,sizeof f->accumulators);f->clock_skips++;}
    const WxBackdropEmitter* emitters=AT(s->data,WxBackdropEmitter,s->effects.emitter_offset);
    float values[WX_BACKDROP_EMITTERS][WX_FX_TRACKS];
    for(unsigned e=0;e<s->effects.emitters;e++)for(unsigned j=0;j<WX_FX_TRACKS;j++)values[e][j]=scalar(s,emitters[e].tracks[j],time,j==WX_FX_ENABLED?1:0);
    /* At most eight 33ms integration slices, fixed storage, no frame allocations. */
    for(unsigned remaining=elapsed;remaining;){unsigned ms=remaining>33?33:remaining;remaining-=ms;float dt=ms*.001f;
        for(unsigned i=0;i<f->active;){WxParticle* p=f->particles+i;p->age+=dt;
            if(p->age>=p->life){*p=f->particles[--f->active];continue;}
            float g=values[p->emitter][WX_FX_GRAVITY];p->position[2]-=.5f*g*dt*dt;
            for(unsigned j=0;j<3;j++)p->position[j]+=p->velocity[j]*dt;p->velocity[2]-=g*dt;
            if(!finite_array(p->position,8)){*p=f->particles[--f->active];f->dropped++;continue;}i++;
        }
        for(unsigned ei=0;ei<s->effects.emitters;ei++){
            float* v=values[ei];if(v[WX_FX_ENABLED]<=0||v[WX_FX_LIFE]<=0)continue;
            float amount=f->accumulators[ei]+clamp(v[WX_FX_RATE],0,10000)*dt;unsigned count=(unsigned)amount;f->accumulators[ei]=amount-count;
            for(unsigned n=0;n<count;n++)spawn(s,ei,v);
        }
    }
    memset(f->count,0,sizeof f->count);
    for(unsigned i=0;i<f->active;i++)f->count[f->particles[i].emitter]++;
    unsigned at=0;for(unsigned e=0;e<s->effects.emitters;e++){f->first[e]=at;at+=f->count[e]*4;f->count[e]=0;}f->quads=f->active;
    /* Cache the quad basis once per emitter, not per particle. No frame heap
       allocations and no growth of the 2048-particle/GPU vertex pools. */
    struct Basis{float right[3],up[3],scale;} basis[WX_BACKDROP_EMITTERS];
    for(unsigned ei=0;ei<s->effects.emitters;ei++){
        const WxBackdropEmitter* e=emitters+ei;const float* m=s->matrices[e->bone];
        basis[ei].scale=(e->flags&WX_PARTICLE_BONE_SCALE)?sqrtf(m[0]*m[0]+m[4]*m[4]+m[8]*m[8]):1;
        for(unsigned j=0;j<3;j++){
            basis[ei].right[j]=(e->flags&WX_PARTICLE_XY_QUAD)?m[j*4]:s->view.right[j];
            basis[ei].up[j]=(e->flags&WX_PARTICLE_XY_QUAD)?m[j*4+1]:s->view.up[j];
        }
    }
    for(unsigned i=0;i<f->active;i++){
        const WxParticle* p=f->particles+i;const WxBackdropEmitter* e=emitters+p->emitter;
        float age=p->age/p->life,t;unsigned phase;
        if(age<e->midpoint){phase=0;t=e->midpoint>0?age/e->midpoint:1;}else{phase=1;t=e->midpoint<1?(age-e->midpoint)/(1-e->midpoint):1;}
        float size=(e->scale[phase]+(e->scale[phase+1]-e->scale[phase])*t)*basis[p->emitter].scale;
        float center[3];
        if(e->flags&WX_PARTICLE_LOCAL)transform(s->matrices[e->bone],p->position,center,1);
        else memcpy(center,p->position,12);
        float color[4];for(unsigned j=0;j<4;j++)color[j]=e->color[phase][j]+(e->color[phase+1][j]-e->color[phase][j])*t;
        /* Head-cell sequence across the authored sprite sheet; advanced repeat/
           decay ranges remain a documented fidelity gap. Never read past atlas. */
        unsigned cell=(unsigned)(age*e->rows*e->columns);if(cell>=e->rows*e->columns)cell=e->rows*e->columns-1;
        float u=(float)(cell%e->columns)/e->columns,v=(float)(cell/e->columns)/e->rows;
        static const float corners[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
        unsigned first=f->first[p->emitter]+4*f->count[p->emitter]++;
        for(unsigned k=0;k<4;k++){
            WxEffectVertex* out=s->effect_vertices+first+k;float a=corners[k][0],b=corners[k][1];
            for(unsigned j=0;j<3;j++)out->p[j]=center[j]+size*(a*basis[p->emitter].right[j]+b*basis[p->emitter].up[j]);
            memcpy(out->color,color,16);unsigned uv=(k+e->tile_rotation)&3;
            out->uv[0]=u+(corners[uv][0]+1)*.5f/e->columns;out->uv[1]=v+(1-corners[uv][1])*.5f/e->rows;
        }
    }
}
static void lighting(WxBackdrop* s,unsigned time){
    if(!s->lighting)return;
    WxBackdropLightSample sampled[WX_BACKDROP_LIGHTS]={0};
    const float zero[4]={0,0,0,0};const WxBackdropLight* lights=AT(s->data,WxBackdropLight,s->effects.light_offset);
    for(unsigned i=0;i<s->effects.lights;i++){
        const WxBackdropLight* l=lights+i;WxBackdropLightSample* out=sampled+i;float a[4],d[4];
        out->enabled=scalar(s,l->enabled,time,1)>0;out->type=l->type;
        wx_backdrop_sample(s,l->ambient,time,zero,a);wx_backdrop_sample(s,l->diffuse,time,zero,d);
        float ai=scalar(s,l->ambient_intensity,time,0),di=scalar(s,l->diffuse_intensity,time,0);
        for(unsigned j=0;j<3;j++){out->ambient[j]=a[j]*ai;out->diffuse[j]=d[j]*di;}
        float direction[3]={0,0,1};const float* input=l->type?l->position:direction;
        if(l->bone==WX_BACKDROP_NONE)memcpy(out->position,input,12);else transform(s->matrices[l->bone],input,out->position,l->type!=0);
        if(!l->type)normalize(out->position);out->start=scalar(s,l->start,time,0);out->end=scalar(s,l->end,time,0);
    }
    int lights_changed=memcmp(sampled,s->light_state->samples,sizeof sampled)!=0;
    if(lights_changed)memcpy(s->light_state->samples,sampled,sizeof sampled);
    uint8_t* dirty=(void*)(s->light_state+1);
    for(unsigned i=0;i<s->h.vertices;i++){
        if(!(dirty[i]&1)||(!(dirty[i]&2)&&!lights_changed))continue;dirty[i]=1;
        const WxVertex* v=s->vertices+i;float* out=s->lighting[i];out[0]=out[1]=out[2]=0;out[3]=1;
        float norm[3];memcpy(norm,v->n,12);normalize(norm);
        for(unsigned j=0;j<s->effects.lights;j++){
            const WxBackdropLightSample* l=sampled+j;if(!l->enabled)continue;float dir[3],attenuation=1;
            if(l->type){float length=0;for(unsigned k=0;k<3;k++){dir[k]=l->position[k]-v->p[k];length+=dir[k]*dir[k];}length=sqrtf(length);
                if(l->end>l->start)attenuation=clamp((l->end-length)/(l->end-l->start),0,1);
                if(length>.00001f)for(unsigned k=0;k<3;k++)dir[k]/=length;
            }else memcpy(dir,l->position,12);
            float ndl=fmaxf(0,norm[0]*dir[0]+norm[1]*dir[1]+norm[2]*dir[2]);
            for(unsigned k=0;k<3;k++)out[k]+=attenuation*(l->ambient[k]+l->diffuse[k]*ndl);
        }
        for(unsigned k=0;k<3;k++)out[k]=clamp(out[k],0,1);
    }
}
void wx_backdrop_effect_update(WxBackdrop* s,unsigned time){
    if(s->h.version<2)return;const uint32_t* colors=AT(s->data,uint32_t,s->effects.color_offset);
    for(unsigned i=0;i<s->h.batches;i++){float rgb[4];wx_backdrop_sample(s,colors[i],time,s->colors[i],rgb);memcpy(s->colors[i],rgb,12);}
    lighting(s,time);particles(s,time);
}
