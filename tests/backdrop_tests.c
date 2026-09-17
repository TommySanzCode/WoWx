#include "wx_backdrop.h"
#include "wx_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
static unsigned checks,outstanding,available=60u*1024u*1024u;static int fail_after=-1;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"BACKDROP FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return available;}
void* wx_gpu_alloc(unsigned bytes){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;uint32_t* p=malloc(bytes+8);if(!p)return NULL;p[0]=bytes;p[1+bytes/4]=0xfeedbeef;available-=bytes;outstanding++;return p+1;}
void wx_gpu_free(void* v){if(v){uint32_t* p=(uint32_t*)v-1;CHECK(p[1+p[0]/4]==0xfeedbeef);available+=p[0];outstanding--;free(p);}}
typedef struct Fixture{WxBackdropHeader h;WxVertex vertices[3];WxSkinVertex skin[3];uint16_t indices[4];WxBackdropBone bones[2];WxBackdropBatch batch;WxBackdropTexture texture;WxBackdropTrack tracks[3];WxBackdropKey keys[6];uint32_t pixel;WxBackdropEffects effects;WxBackdropAnchor anchor;WxBackdropEmitter emitter;WxBackdropLight light;uint32_t color;}Fixture;
static Fixture f;static WxBackdrop scene;
#define OFFSET(field) ((uint32_t)offsetof(Fixture,field))
static void fixture(void){
    memset(&f,0,sizeof f);memcpy(f.h.magic,"WXB1",4);f.h.version=1;f.h.file_size=sizeof f;f.h.duration_ms=1000;
    f.h.vertices=3;f.h.vertex_offset=OFFSET(vertices);f.h.skin_offset=OFFSET(skin);f.h.indices=3;f.h.index_offset=OFFSET(indices);
    f.h.bones=2;f.h.bone_offset=OFFSET(bones);f.h.batches=1;f.h.batch_offset=OFFSET(batch);f.h.textures=1;f.h.texture_offset=OFFSET(texture);
    f.h.tracks=3;f.h.track_offset=OFFSET(tracks);f.h.keys=6;f.h.key_offset=OFFSET(keys);
    f.h.camera[0]=3;f.h.fov=1;f.h.near_clip=.1f;f.h.far_clip=2000;
    for(unsigned i=0;i<3;i++){f.vertices[i].p[0]=1;f.vertices[i].n[2]=1;f.skin[i].weights[0]=255;f.skin[i].bones[0]=1;f.indices[i]=i;}
    f.bones[0]=(WxBackdropBone){-1,0,{0,0,0},0,WX_BACKDROP_NONE,WX_BACKDROP_NONE};
    f.bones[1]=(WxBackdropBone){0,0,{0,0,0},WX_BACKDROP_NONE,1,WX_BACKDROP_NONE};
    f.batch=(WxBackdropBatch){0,3,0,1,2,2,WX_BACKDROP_NONE,{1,1,1,1}};
    f.texture=(WxBackdropTexture){OFFSET(pixel),4,1,1,0,0};f.pixel=0xffffffff;
    f.tracks[0]=(WxBackdropTrack){0,2,1000,1,1};f.tracks[1]=(WxBackdropTrack){2,2,2000,1,2};f.tracks[2]=(WxBackdropTrack){4,2,1000,0,0};
    f.keys[0]=(WxBackdropKey){0,{0,0,0,0}};f.keys[1]=(WxBackdropKey){1000,{10,0,0,0}};
    f.keys[2]=(WxBackdropKey){0,{0,0,0,1}};f.keys[3]=(WxBackdropKey){2000,{0,0,1,0}};
    f.keys[4]=(WxBackdropKey){0,{.25f,0,0,0}};f.keys[5]=(WxBackdropKey){500,{.75f,0,0,0}};
}
static void effects_fixture(void){
    fixture();f.h.version=2;f.h.reserved=OFFSET(effects);
    f.effects=(WxBackdropEffects){1,OFFSET(emitter),1,OFFSET(light),OFFSET(color),WX_BACKDROP_PARTICLES};
    f.color=WX_BACKDROP_NONE;f.emitter.bone=0;f.emitter.type=1;f.emitter.blend=4;f.emitter.rows=f.emitter.columns=1;f.emitter.midpoint=.5f;
    for(unsigned i=0;i<WX_FX_TRACKS;i++)f.emitter.tracks[i]=WX_BACKDROP_NONE;
    f.emitter.tracks[WX_FX_RATE]=f.emitter.tracks[WX_FX_LIFE]=2;
    for(unsigned i=0;i<3;i++){f.emitter.scale[i]=.1f;for(unsigned j=0;j<4;j++)f.emitter.color[i][j]=1;}
    f.light.type=1;f.light.bone=WX_BACKDROP_NONE;f.light.ambient=f.light.diffuse=0;
    f.light.ambient_intensity=f.light.diffuse_intensity=2;f.light.start=f.light.end=f.light.enabled=WX_BACKDROP_NONE;
}
static void write_pack(void){FILE* file=fopen("backdrop-fixture.wxb","wb");CHECK(file);CHECK(fwrite(&f,1,sizeof f,file)==sizeof f);CHECK(!fclose(file));}
static void invalid(void){CHECK(!wx_backdrop_validate(&f,sizeof f));write_pack();CHECK(!wx_backdrop_open(&scene,"backdrop-fixture.wxb"));CHECK(!scene.ready&&!scene.bytes&&!outstanding);}
static void particle_flags(void){
    // A stationary local particle follows its moving bone; a released particle
    // stays where it was born. Check the actual rendered quad's centroid.
    for(unsigned local=0;local<2;local++){
        effects_fixture();f.emitter.flags=local?WX_PARTICLE_LOCAL:0;
        f.emitter.tracks[WX_FX_RATE]=WX_BACKDROP_NONE;
        write_pack();CHECK(wx_backdrop_open(&scene,"backdrop-fixture.wxb"));
        scene.simulation->active=1;scene.simulation->particles[0]=(WxParticle){{2,3,4},{0,0,0},0,10,0};
        wx_backdrop_update(&scene,100);float center[3]={0};
        for(unsigned k=0;k<4;k++)for(unsigned j=0;j<3;j++)center[j]+=scene.effect_vertices[k].p[j]*.25f;
        CHECK(fabsf(center[0]-(local?3:2))<.00001f&&fabsf(center[1]-3)<.00001f&&fabsf(center[2]-4)<.00001f);
        wx_backdrop_close(&scene);CHECK(!outstanding);
    }
    // The scene is viewed along X. A billboard has no X extent; an authored XY
    // quad does. Uniform bone scale doubles only opt-in billboard dimensions.
    for(unsigned xy=0;xy<2;xy++)for(unsigned scale=0;scale<2;scale++){
        effects_fixture();f.emitter.flags=(xy?WX_PARTICLE_XY_QUAD:0)|(scale?WX_PARTICLE_BONE_SCALE:0);
        f.emitter.tracks[WX_FX_RATE]=WX_BACKDROP_NONE;f.bones[0].translation=WX_BACKDROP_NONE;f.bones[0].scale=0;
        for(unsigned k=0;k<2;k++)for(unsigned j=0;j<3;j++)f.keys[k].value[j]=2;
        write_pack();CHECK(wx_backdrop_open(&scene,"backdrop-fixture.wxb"));
        scene.simulation->active=1;scene.simulation->particles[0]=(WxParticle){{0,0,0},{0,0,0},0,10,0};
        wx_backdrop_update(&scene,100);
        float expected=.1f*(scale?2:1);const float* p=scene.effect_vertices[0].p;
        CHECK(fabsf(p[0]-(xy?-expected*2:0))<.00001f);
        CHECK(fabsf(fabsf(p[1])-(xy?expected*2:expected))<.00001f);
        CHECK(fabsf(p[2]-(xy?0:-expected))<.00001f);
        wx_backdrop_close(&scene);CHECK(!outstanding);
    }
    WxParticle plane;
    for(unsigned type=1;type<=2;type++)for(unsigned up=0;up<2;up++){
        effects_fixture();f.emitter.type=type;f.emitter.flags=WX_PARTICLE_LOCAL|(up?WX_PARTICLE_SPHERE_UP:0);
        f.emitter.tracks[WX_FX_SPEED]=f.emitter.tracks[WX_FX_VERTICAL]=f.emitter.tracks[WX_FX_LENGTH]=f.emitter.tracks[WX_FX_WIDTH]=2;
        f.keys[4].value[0]=f.keys[5].value[0]=30;
        write_pack();CHECK(wx_backdrop_open(&scene,"backdrop-fixture.wxb"));wx_backdrop_update(&scene,66);CHECK(scene.simulation->active==1);
        const WxParticle* p=scene.simulation->particles;
        if(type==1){if(!up)plane=*p;else CHECK(!memcmp(&plane,p,sizeof plane));}
        if(type==2&&up)CHECK(p->velocity[0]==0&&p->velocity[1]==0&&fabsf(p->velocity[2]-30)<.00001f);
        wx_backdrop_close(&scene);CHECK(!outstanding);
    }
}
int main(int argc,char** argv){
    if(argc==3&&!strcmp(argv[1],"--scene")){
        CHECK(wx_backdrop_open(&scene,argv[2]));CHECK(scene.ready&&scene.bytes<=WX_BACKDROP_LIMIT);
        uint64_t seen=0;unsigned peak=0;float extent=0;
        for(unsigned t=33;t<=30000;t+=33){
            wx_backdrop_update(&scene,t);CHECK(scene.ready);
            for(unsigned i=0;i<scene.h.vertices;i++)for(unsigned j=0;j<3;j++)CHECK(isfinite(scene.vertices[i].p[j]));
            if(!scene.effects.emitters){CHECK(!scene.simulation);continue;}
            CHECK(scene.simulation&&scene.simulation->active<=WX_BACKDROP_PARTICLES);
            WxBackdropSimulation* fx=scene.simulation;
            CHECK(fx->quads==fx->active&&fx->dropped==0);
            if(fx->active>peak)peak=fx->active;
            for(unsigned i=0;i<scene.effects.emitters;i++)if(fx->count[i])seen|=UINT64_C(1)<<i;
            for(unsigned i=0;i<fx->quads*4;i++){
                const WxEffectVertex* v=scene.effect_vertices+i;
                for(unsigned j=0;j<3;j++){CHECK(isfinite(v->p[j]));if(fabsf(v->p[j])>extent)extent=fabsf(v->p[j]);}
                CHECK(v->uv[0]>=0&&v->uv[0]<=1&&v->uv[1]>=0&&v->uv[1]<=1);
                for(unsigned j=0;j<4;j++)CHECK(isfinite(v->color[j])&&v->color[j]>=0&&v->color[j]<=1);
            }
        }
        printf("Host simulated 30s: %s bytes=%u emitters=%u seen=0x%llx peak=%u born=%u drops=%u max_abs_coordinate=%.3f\n",
               argv[2],scene.bytes,scene.effects.emitters,(unsigned long long)seen,peak,scene.simulation?scene.simulation->born:0,scene.simulation?scene.simulation->dropped:0,extent);
        CHECK(!scene.effects.emitters||peak>0);wx_backdrop_close(&scene);CHECK(!outstanding);return 0;
    }
    if(argc==2){CHECK(wx_backdrop_open(&scene,argv[1]));CHECK(scene.ready&&scene.bytes<=WX_BACKDROP_LIMIT&&scene.h.vertices==7970&&scene.h.bones==57);
        for(unsigned t=0;t<36000;t+=33){wx_backdrop_update(&scene,t);CHECK(scene.ready);for(unsigned i=0;i<scene.h.vertices;i++)for(unsigned j=0;j<3;j++)CHECK(isfinite(scene.vertices[i].p[j]));
            if(scene.lighting&&t%3300==0){unsigned n=scene.h.vertices*16;void* reference=malloc(n);CHECK(reference);memcpy(reference,scene.lighting,n);
                unsigned vertex_bytes=scene.h.vertices*sizeof(WxVertex);void* original=malloc(vertex_bytes);CHECK(original);memcpy(original,scene.vertices,vertex_bytes);scene.geometry_ready=0;
                for(unsigned i=0;i<scene.h.vertices;i++)((uint8_t*)(scene.light_state+1))[i]|=2;
                wx_backdrop_update(&scene,t);CHECK(!memcmp(reference,scene.lighting,n));CHECK(!memcmp(original,scene.vertices,vertex_bytes));free(original);free(reference);}}

        if(scene.simulation){CHECK(scene.simulation->peak>500&&scene.simulation->peak<=WX_BACKDROP_PARTICLES);CHECK(scene.simulation->dropped==0);CHECK(scene.simulation->quads==scene.simulation->active);printf("Effects live=%u peak=%u born=%u drops=%u\n",scene.simulation->active,scene.simulation->peak,scene.simulation->born,scene.simulation->dropped);}
        printf("Actual title: bytes=%u updates=%u omitted_emitters=%u\n",scene.bytes,scene.updates,scene.h.omitted_particles);wx_backdrop_close(&scene);CHECK(!outstanding);printf("Actual scene finite/bounded checks: %u\n",checks);return 0;}
    particle_flags();
    // Cancellation must close the file and every CPU/GPU allocation at any stage.
    for(unsigned phase=1;phase<=4;phase++){
        fixture();write_pack();CHECK(wx_backdrop_begin(&scene,"backdrop-fixture.wxb")&&!scene.ready&&scene.bytes==sizeof f);
        unsigned pumps=0;
        while(scene.load_phase<phase){unsigned before=scene.load_read;wx_backdrop_pump(&scene,0);CHECK(scene.load_read-before<=WX_BACKDROP_IO_SLICE);CHECK(++pumps<20&&!scene.ready);}
        CHECK((phase==1)==(scene.pending!=NULL));wx_backdrop_close(&scene);CHECK(!scene.pending&&!scene.bytes&&!outstanding&&!scene.load_phase);
    }
    fixture();write_pack();CHECK(wx_backdrop_begin(&scene,"backdrop-fixture.wxb"));
    for(unsigned i=0;i<4;i++)wx_backdrop_pump(&scene,500);
    CHECK(scene.ready&&!scene.pending&&!scene.load_phase);wx_backdrop_close(&scene);CHECK(!outstanding);
    fixture();CHECK(wx_backdrop_validate(&f,sizeof f));write_pack();CHECK(wx_backdrop_open(&scene,"backdrop-fixture.wxb"));CHECK(outstanding==2);
    wx_backdrop_update(&scene,500);CHECK(fabsf(scene.vertices[0].p[0]-(5+sqrtf(.5f)))<.0001f);CHECK(fabsf(scene.vertices[0].p[1]-sqrtf(.5f))<.0001f);CHECK(fabsf(scene.colors[0][3]-.75f)<.0001f);
    wx_backdrop_update(&scene,1000);CHECK(fabsf(scene.vertices[0].p[0])<.0001f&&fabsf(scene.vertices[0].p[1]-1)<.0001f);CHECK(fabsf(scene.colors[0][3]-.25f)<.0001f);
    wx_backdrop_update(&scene,2000);CHECK(fabsf(scene.vertices[0].p[0]-1)<.0001f&&fabsf(scene.vertices[0].p[1])<.0001f);
    const float fallback[4]={3,4,5,6};float value[4];wx_backdrop_sample(&scene,WX_BACKDROP_NONE,777,fallback,value);CHECK(!memcmp(fallback,value,16));wx_backdrop_close(&scene);CHECK(!outstanding);
    effects_fixture();f.h.version=3;f.anchor=(WxBackdropAnchor){1,0,{2,3,4}};
    CHECK(wx_backdrop_validate(&f,sizeof f));write_pack();CHECK(wx_backdrop_open(&scene,"backdrop-fixture.wxb"));
    wx_backdrop_update(&scene,500);float stand[3];CHECK(wx_backdrop_anchor(&scene,stand));CHECK(fabsf(stand[0]-7)<.0001f&&stand[1]==3&&stand[2]==4);
    wx_backdrop_close(&scene);CHECK(!outstanding);
    f.anchor.bone=2;invalid();f.anchor.bone=WX_BACKDROP_NONE;f.anchor.position[1]=NAN;invalid();
    effects_fixture();f.h.version=3;f.anchor.present=2;invalid();
    effects_fixture();f.h.version=3;f.h.reserved=sizeof f-sizeof(WxBackdropEffects);invalid();
    fixture();f.h.version=2;invalid();fixture();f.h.file_size--;invalid();fixture();f.h.vertices=16385;invalid();fixture();f.h.indices=UINT32_MAX;invalid();
    fixture();f.h.vertex_offset=UINT32_MAX;invalid();fixture();f.h.skin_offset=1;invalid();fixture();f.h.bones=WX_BACKDROP_BONES+1;invalid();fixture();f.h.near_clip=0;invalid();fixture();f.h.fov=NAN;invalid();fixture();f.h.target[0]=3;invalid();
    fixture();f.bones[1].parent=1;invalid();fixture();f.bones[0].parent=-2;invalid();fixture();f.bones[0].translation=1;invalid();
    fixture();f.skin[0].weights[0]=0;invalid();fixture();f.skin[0].bones[0]=2;invalid();fixture();f.indices[0]=3;invalid();fixture();f.vertices[0].uv[0]=INFINITY;invalid();
    fixture();f.tracks[0].period_ms=0;invalid();fixture();f.tracks[0].count=UINT32_MAX;invalid();fixture();f.tracks[0].interpolation=2;invalid();fixture();f.keys[1].time_ms=0;invalid();fixture();f.keys[2].value[3]=0;invalid();
    fixture();f.batch.count=4;invalid();fixture();f.batch.texture=1;invalid();fixture();f.batch.blend=8;invalid();fixture();f.batch.alpha=0;invalid();
    fixture();f.texture.offset++;invalid();fixture();f.texture.size=5;invalid();fixture();f.texture.dimension=3;invalid();fixture();f.texture.gpu_offset=128;invalid();
    for(int i=0;i<2;i++){fixture();write_pack();fail_after=i;CHECK(!wx_backdrop_open(&scene,"backdrop-fixture.wxb")&&!outstanding&&!scene.bytes);}fail_after=-1;
    available=8u*1024u*1024u;fixture();write_pack();CHECK(!wx_backdrop_open(&scene,"backdrop-fixture.wxb")&&!outstanding);available=60u*1024u*1024u;
    fixture();write_pack();FILE* file=fopen("backdrop-fixture.wxb","ab");CHECK(file);fputc(0,file);fclose(file);CHECK(!wx_backdrop_open(&scene,"backdrop-fixture.wxb")&&!outstanding);
    effects_fixture();CHECK(wx_backdrop_validate(&f,sizeof f));write_pack();CHECK(wx_backdrop_open(&scene,"backdrop-fixture.wxb"));
    CHECK(outstanding==4&&scene.simulation&&scene.lighting);CHECK(wx_backdrop_view(&scene,320,100,300,300));CHECK(!wx_backdrop_view(&scene,320,100,400,300));
    wx_backdrop_update(&scene,33);CHECK(scene.simulation->active==0);wx_backdrop_close(&scene);CHECK(!outstanding);
    effects_fixture();f.keys[4].value[0]=f.keys[5].value[0]=1000000;write_pack();CHECK(wx_backdrop_open(&scene,"backdrop-fixture.wxb"));
    for(unsigned t=33;t<=330;t+=33)wx_backdrop_update(&scene,t);
    CHECK(scene.simulation->active==WX_BACKDROP_PARTICLES&&scene.simulation->dropped>0);
    CHECK(scene.simulation->quads==scene.simulation->active&&scene.simulation->count[0]==scene.simulation->active);
    WxParticle first=scene.simulation->particles[0];unsigned born=scene.simulation->born;
    wx_backdrop_update(&scene,10000);CHECK(scene.simulation->active==0&&scene.simulation->clock_skips==1);wx_backdrop_close(&scene);
    write_pack();CHECK(wx_backdrop_open(&scene,"backdrop-fixture.wxb"));for(unsigned t=33;t<=330;t+=33)wx_backdrop_update(&scene,t);
    CHECK(scene.simulation->born==born&&!memcmp(&first,scene.simulation->particles,sizeof first));wx_backdrop_close(&scene);CHECK(!outstanding);
    effects_fixture();f.h.reserved=UINT32_MAX;invalid();effects_fixture();f.effects.capacity++;invalid();effects_fixture();f.effects.emitters=65;invalid();
    effects_fixture();f.emitter.bone=2;invalid();effects_fixture();f.emitter.type=0;invalid();effects_fixture();f.emitter.rows=0;invalid();effects_fixture();f.emitter.color[0][3]=NAN;invalid();
    effects_fixture();f.emitter.scale[0]=-1;invalid();effects_fixture();f.emitter.tracks[0]=0;invalid();effects_fixture();f.emitter.flags=8;invalid();
    effects_fixture();f.light.type=2;invalid();effects_fixture();f.light.bone=2;invalid();effects_fixture();f.light.diffuse=2;invalid();effects_fixture();f.color=2;invalid();
    for(int i=0;i<4;i++){effects_fixture();write_pack();fail_after=i;CHECK(!wx_backdrop_open(&scene,"backdrop-fixture.wxb")&&!outstanding&&!scene.bytes&&!scene.simulation);}fail_after=-1;
    CHECK(!wx_backdrop_open(&scene,"absent.wxb")&&!outstanding);remove("backdrop-fixture.wxb");printf("Backdrop validation, interpolation, skinning and allocation: %u checks passed\n",checks);return 0;
}
