#include "wx_lighting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned checks,available=64*1024*1024;
static int allocation_fail_after=-1,owned;
void* __real_malloc(size_t bytes);
void __real_free(void* pointer);
void* __wrap_malloc(size_t bytes){
    if(allocation_fail_after==0)return NULL;
    if(allocation_fail_after>0)allocation_fail_after--;
    void* p=__real_malloc(bytes);if(p)owned++;return p;
}
void __wrap_free(void* p){if(p)owned--;__real_free(p);}
unsigned wx_free_memory(void){return available;}
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Lighting FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
#define NEAR(a,b) CHECK(fabsf((a)-(b))<.002f)
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static WxLightProfile make_profile(unsigned id,unsigned color,float end,float fraction){
    WxLightProfile p={0};p.id=id;for(unsigned k=0;k<3;k++)p.bands[k].count=1;
    p.bands[0].value[0]=color;p.bands[1].value[0]=bits(end);p.bands[2].value[0]=bits(fraction);return p;
}
static void save(const char* path,WxLightVolume* v,unsigned nv,WxLightProfile* p,unsigned np){
    WxLightHeader h={0x314c5857,2,nv,np,32+nv*40+np*sizeof *p,{0}};FILE* f=fopen(path,"wb");CHECK(f!=NULL);
    CHECK(fwrite(&h,1,32,f)==32);CHECK(fwrite(v,40,nv,f)==nv);CHECK(fwrite(p,sizeof *p,np,f)==np);CHECK(!fclose(f));
}
static void malformed(const char* path,long offset,uint32_t value,WxLighting* c){
    FILE* f=fopen(path,"r+b");CHECK(f!=NULL);CHECK(!fseek(f,offset,SEEK_SET));CHECK(fwrite(&value,4,1,f)==1);CHECK(!fclose(f));
    WxLightVolume* old=c->volumes;unsigned failures=c->failures;CHECK(!wx_light_open(c,path));CHECK(c->volumes==old&&c->failures==failures+1);
}
int main(int argc,char** argv){
    WxLightProfile p[3]={make_profile(1,0xff0000,1000,.1f),make_profile(2,0x0000ff,100,.5f),make_profile(3,0x00ff00,80,-.5f)};
    WxLightVolume v[3]={{1,0,{0,0,0},0,0,{1,3,3}},{2,0,{0,0,0},10,20,{2,2,3}},{3,0,{0,0,0},2,5,{3,3,3}}};
    const char* path="lighting-tests.tmp";save(path,v,3,p,3);WxLighting c={0};CHECK(wx_light_open(&c,path));CHECK(c.bytes==3432);
    float pos[3]={30,0,0};WxLightSample a,b;wx_light_sample(&c,0,pos,1440,0,0,&a);CHECK(a.authored&&a.profile[2]==1&&!a.profile[0]);NEAR(a.fog.start,90);NEAR(a.fog.end,145);CHECK(wx_fog_background(&a.fog)==0xffff0000);
    pos[0]=15;wx_light_sample(&c,0,pos,1440,0,0,&a);NEAR(a.weight[0],.5);NEAR(a.fog.color[0],.5);NEAR(a.fog.color[2],.5);NEAR(a.fog.end,122.5);
    pos[0]=0;wx_light_sample(&c,0,pos,1440,0,0,&a);CHECK(a.volume[0]==3&&a.volume[1]==2);NEAR(a.fog.color[1],.5);NEAR(a.fog.color[2],.5);
    /* Boundary must tend to global color, never jump from normalized local. */
    pos[0]=19.99f;wx_light_sample(&c,0,pos,1440,0,0,&a);CHECK(a.fog.color[0]>.999f);pos[0]=20;wx_light_sample(&c,0,pos,1440,0,0,&b);CHECK(b.fog.color[0]==1);
    wx_light_sample(&c,0,pos,1440,1,0,&a);CHECK(a.profile[2]==3&&a.fog.start==-40&&wx_fog_valid(&a.fog));NEAR(wx_fog_visibility(&a.fog,0),2.f/3);
    wx_light_sample(&c,0,pos,1440,2,WX_FOG_UNDERWATER,&a);CHECK(a.profile[2]==3&&a.fog.enabled&&a.fog.environment==2);
    wx_light_sample(&c,0,pos,1440,0,WX_FOG_INDOOR,&a);CHECK(!a.authored&&!a.fog.enabled);
    wx_light_sample(&c,999,pos,1440,0,0,&a);CHECK(!a.authored&&wx_fog_background(&a.fog)==0xff7196b5);
    pos[0]=NAN;wx_light_sample(&c,0,pos,1440,0,0,&a);CHECK(!a.authored);pos[0]=30;
    p[0].bands[0].count=2;p[0].bands[0].time[0]=600;p[0].bands[0].time[1]=1800;p[0].bands[0].value[1]=0x0000ff;
    save(path,v,3,p,3);CHECK(wx_light_open(&c,path));wx_light_sample(&c,0,pos,1200,0,0,&a);NEAR(a.fog.color[0],.5);NEAR(a.fog.color[2],.5);
    wx_light_sample(&c,0,pos,2640,0,0,&a);NEAR(a.fog.color[0],.5);NEAR(a.fog.color[2],.5);
    for(unsigned t=0;t<2880;t+=7){wx_light_sample(&c,0,pos,(float)t,0,0,&a);wx_light_sample(&c,0,pos,t+2880.f,0,0,&b);CHECK(wx_fog_valid(&a.fog));CHECK(!memcmp(&a,&b,sizeof a));}
    wx_light_sample(&c,0,pos,-1,0,0,&a);wx_light_sample(&c,0,pos,2879,0,0,&b);CHECK(!memcmp(&a,&b,sizeof a));
    /* Authored palette follows the same volume blend; colors may remain valid
       even where fog bands are absent. Missing color bands stay explicit. */
    for(unsigned i=0;i<3;i++)for(unsigned k=0;k<WX_LIGHT_COLORS;k++){p[i].bands[k+3].count=1;p[i].bands[k+3].value[0]=i==0?0xff0000:0x0000ff;}
    p[0].bands[3].count=2;p[0].bands[3].time[1]=1440;p[0].bands[3].value[0]=0x200000;p[0].bands[3].value[1]=0xff0000;
    save(path,v,3,p,3);CHECK(wx_light_open(&c,path));pos[0]=30;
    wx_light_sample(&c,0,pos,0,0,0,&a);wx_light_sample(&c,0,pos,1440,0,0,&b);
    CHECK(a.palette.mask==255&&b.palette.mask==255);NEAR(a.palette.color[0][0],32.f/255);NEAR(b.palette.color[0][0],1);
    CHECK(a.palette.direction[2]<0&&b.palette.direction[2]>0);
    pos[0]=15;wx_light_sample(&c,0,pos,1440,0,0,&a);NEAR(a.palette.color[0][0],.5f);NEAR(a.palette.color[0][2],.5f);
    c.profiles[1].bands[3].count=0;wx_light_sample(&c,0,pos,1440,0,0,&a);CHECK(a.palette.mask==254);CHECK(wx_light_palette_valid(&a.palette));
    c.profiles[0].bands[1].count=0;pos[0]=30;wx_light_sample(&c,0,pos,1440,0,0,&a);CHECK(!a.authored&&a.palette.mask==255);
    /* Rotation preserves world-space Lambert response, independent of model scale. */
    for(unsigned t=0;t<2880;t+=13){wx_light_sample(&c,0,pos,(float)t,0,0,&a);CHECK(wx_light_palette_valid(&a.palette));
        for(unsigned j=0;j<16;j++){float angle=j*.4f,d[4],co=cosf(angle),si=sinf(angle);wx_light_direction(&a.palette,angle,d);
            NEAR(d[0],a.palette.direction[0]*co+a.palette.direction[1]*si);
            NEAR(d[0]*d[0]+d[1]*d[1]+d[2]*d[2],1);
            float rgb[3];wx_light_sky_color(&a.palette,&a.fog,(float)j/15,rgb);for(unsigned k=0;k<3;k++)CHECK(rgb[k]>=0&&rgb[k]<=1);
        }
        float rgb[3];wx_light_sky_color(&a.palette,&a.fog,-1,rgb);for(unsigned k=0;k<3;k++)NEAR(rgb[k],a.fog.color[k]);
        wx_light_sky_color(&a.palette,&a.fog,1,rgb);for(unsigned k=0;k<3;k++)NEAR(rgb[k],a.palette.color[WX_LIGHT_SKY_TOP][k]);
    }
    WxLightPalette blended={0};wx_light_blend(&blended,&b.palette,.033f);CHECK(wx_light_palette_valid(&blended));
    for(unsigned j=0;j<300;j++){wx_light_blend(&blended,&a.palette,.033f);CHECK(wx_light_palette_valid(&blended));}
    WxLightPalette unchanged=blended;wx_light_blend(&blended,&b.palette,NAN);CHECK(!memcmp(&blended,&unchanged,sizeof blended));
    /* Legacy files expand into the same bounded resident representation. */
    {FILE* f=fopen(path,"wb");CHECK(f);WxLightHeader h={0x314c5857,1,3,3,1064,{0}};CHECK(fwrite(&h,32,1,f)==1);CHECK(fwrite(v,40,3,f)==3);
        for(unsigned i=0;i<3;i++)CHECK(fwrite(&p[i],304,1,f)==1);CHECK(!fclose(f));CHECK(wx_light_open(&c,path));CHECK(c.bytes==3432);
        wx_light_sample(&c,0,pos,1440,0,0,&a);CHECK(a.authored&&!a.palette.mask);CHECK(wx_light_palette_valid(&a.palette));}
    const long offsets[]={0,4,8,12,16,20,32,36,44,52,56,60,32+120,32+124,32+128,32+160};
    for(unsigned i=0;i<sizeof offsets/sizeof *offsets;i++){save(path,v,3,p,3);malformed(path,offsets[i],0xffffffff,&c);}
    save(path,v,3,p,3);available=8*1024*1024+100;CHECK(!wx_light_open(&c,path));available=64*1024*1024;
    for(int fail=0;fail<2;fail++){
        int before=owned;WxLightVolume* old=c.volumes;allocation_fail_after=fail;
        CHECK(!wx_light_open(&c,path));CHECK(c.volumes==old&&owned==before);allocation_fail_after=-1;
    }
    for(unsigned size=0;size<32;size++){FILE* f=fopen(path,"wb");CHECK(f!=NULL);uint8_t b[32]={0};CHECK(fwrite(b,1,size,f)==size);fclose(f);CHECK(!wx_light_open(&c,path));}
    WxLightProfile q=p[0];q.bands[0].time[1]=600;CHECK(!wx_light_profile_valid(&q));q=p[0];q.bands[1].value[0]=bits(NAN);CHECK(!wx_light_profile_valid(&q));q=p[0];q.bands[2].value[0]=bits(1);CHECK(!wx_light_profile_valid(&q));
    /* Authoritative time examples, speed, exact packet shape and tick wrap. */
    WxWorldClock clock={0};NEAR(wx_clock_half_minutes(&clock,123),1440);
    uint8_t wire[]={10,50,115,22,137,136,136,60};CHECK(wx_clock_apply(&clock,0x42,wire,8,1000)==1);NEAR(wx_clock_half_minutes(&clock,1000),980);NEAR(wx_clock_half_minutes(&clock,61000),982);
    WxWorldClock old=clock;for(unsigned n=0;n<=12;n++)if(n!=8){CHECK(wx_clock_apply(&clock,0x42,wire,n,1000)==0);CHECK(!memcmp(&clock,&old,sizeof clock));}
    uint32_t data[2]={23*64+59,bits(1.f/60)};CHECK(wx_clock_apply(&clock,0x42,data,8,0xfffffff0)==1);NEAR(wx_clock_half_minutes(&clock,59984),0);
    data[0]=0;data[1]=bits(1);CHECK(wx_clock_apply(&clock,0x42,data,8,0)==1);NEAR(wx_clock_half_minutes(&clock,60000),120);
    data[1]=bits(0);CHECK(wx_clock_apply(&clock,0x42,data,8,0)==1);NEAR(wx_clock_half_minutes(&clock,60000),0);
    data[1]=bits(NAN);CHECK(!wx_clock_apply(&clock,0x42,data,8,0));data[1]=bits(-1);CHECK(!wx_clock_apply(&clock,0x42,data,8,0));
    data[1]=bits(1.f/60);for(unsigned k=0;k<5;k++){const uint32_t invalid[]={60,24<<6,7<<11,31<<14,12<<20};data[0]=invalid[k];CHECK(!wx_clock_apply(&clock,0x42,data,8,0));}
    CHECK(wx_clock_apply(&clock,0xffff,NULL,0,0)==-1);wx_light_close(&c);CHECK(!c.bytes&&!c.volumes&&!c.profiles);remove(path);
    if(argc>1){CHECK(wx_light_open(&c,argv[1]));unsigned authored=0,fallback=0;
        for(unsigned i=0;i<c.volume_count;i++)for(unsigned condition=0;condition<3;condition++)for(unsigned t=0;t<2880;t+=120){
            wx_light_sample(&c,c.volumes[i].map,c.volumes[i].position,(float)t,condition,0,&a);CHECK(wx_fog_valid(&a.fog));CHECK(wx_light_palette_valid(&a.palette));if(a.authored)authored++;else fallback++;
        }
        const float northshire[]={-8949.95f,-132.493f,83.5f};wx_light_sample(&c,0,northshire,1440,0,0,&a);CHECK(a.authored&&a.profile[2]==12);
        wx_light_sample(&c,0,northshire,0,0,0,&b);CHECK(wx_fog_background(&a.fog)!=wx_fog_background(&b.fog));
        printf("Real catalog: %u volumes / %u profiles / %u bytes; center/time/condition samples %u authored / %u fallback; Northshire noon %08x midnight %08x\n",c.volume_count,c.profile_count,c.bytes,authored,fallback,wx_fog_background(&a.fog),wx_fog_background(&b.fog));wx_light_close(&c);
    }
    printf("Lighting/clock: %u checks pass\n",checks);return 0;
}
