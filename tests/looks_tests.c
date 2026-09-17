#include "wx_looks.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"LOOK FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct Fixture {WxLookHeader h;WxLookRow r[14];uint8_t pixels[65536];} Fixture;
static Fixture fixture;
static void init(void){
    memset(&fixture,0,sizeof fixture);fixture.h=(WxLookHeader){{'W','X','L','K'},1,sizeof fixture,14,sizeof(WxLookRow),1,0,offsetof(Fixture,pixels)};
    unsigned keys[][3]={{0,0,0},{0,0,3},{1,0,0},{1,5,3},{3,0,0},{3,4,7},
        {5,0,0},{5,4,0},{6,0,0},{6,2,0},{2,0,0},{2,2,0},{2,2,7},{4,0,0}};
    for(unsigned i=0;i<14;i++){fixture.r[i].kind=keys[i][0];fixture.r[i].variation=keys[i][1];fixture.r[i].color=keys[i][2];}
    for(unsigned i=0;i<2;i++){fixture.r[i].offset[0]=offsetof(Fixture,pixels);fixture.r[i].region[0]=WX_LOOK_BASE;}
    fixture.r[6].geoset[0]=1;fixture.r[7].geoset[0]=5;fixture.r[9].geoset[2]=3;
}
static void write_file(void){FILE* f=fopen("looks-fixture.wxl","wb");CHECK(f);CHECK(fwrite(&fixture,1,sizeof fixture,f)==sizeof fixture);fclose(f);}
static void reject(void){write_file();WxLooks l;CHECK(!wx_looks_open(&l,"looks-fixture.wxl")&&!l.file&&!l.header.count);}
static void exercise(WxLooks* l){
    uint32_t base[]={l->header.race,l->header.sex,0,0,0,0,0};CHECK(wx_looks_valid(l,base));
    uint8_t ids[256];unsigned options=0;
    for(unsigned field=2;field<7;field++){
        uint32_t v[7];memcpy(v,base,sizeof v);unsigned n=wx_looks_choices(l,v,field,ids);CHECK(n);
        for(unsigned i=0;i<n;i++){
            CHECK(wx_looks_valid(l,v));CHECK(wx_looks_step(l,v,field,1));options++;
            for(unsigned dependent=2;dependent<7;dependent++){uint32_t other[7];memcpy(other,v,sizeof other);
                CHECK(wx_looks_step(l,other,dependent,-1)&&wx_looks_valid(l,other));}
        }
    }
    uint32_t seed=0,again=0,first[7],last[7];unsigned distinct=0;memcpy(last,base,sizeof last);
    for(unsigned i=0;i<256;i++){
        uint32_t before[7];memcpy(before,last,sizeof before);
        CHECK(wx_looks_randomize(l,last,&seed)&&wx_looks_valid(l,last));
        CHECK(memcmp(before,last,sizeof before));
        CHECK(last[0]==base[0]&&last[1]==base[1]);if(!i)memcpy(first,last,sizeof first);else distinct|=memcmp(last,first,sizeof last)!=0;
    }
    uint32_t repeat[7];memcpy(repeat,base,sizeof repeat);
    CHECK(wx_looks_randomize(l,repeat,&again)&&!memcmp(repeat,first,sizeof repeat));
    CHECK(distinct);
    printf("Look catalogue %u/%u: %u rows, %u primary choices exercised\n",l->header.race,l->header.sex,l->header.count,options);
}
int main(int argc,char** argv){
    if(argc==2){WxLooks l;CHECK(wx_looks_open(&l,argv[1]));exercise(&l);wx_looks_close(&l);printf("%u appearance checks pass\n",checks);return 0;}
    init();write_file();WxLooks l;CHECK(wx_looks_open(&l,"looks-fixture.wxl"));exercise(&l);
    uint32_t v[]={1,0,0,0,0,0,0};CHECK(wx_looks_step(&l,v,2,1)&&v[2]==3&&v[3]==5);
    CHECK(wx_looks_step(&l,v,4,1)&&v[4]==4&&v[5]==7&&v[6]==2);
    uint32_t keep[7];memcpy(keep,v,sizeof v);CHECK(!wx_looks_step(&l,v,7,1)&&!memcmp(v,keep,sizeof v));
    v[2]=1;CHECK(!wx_looks_valid(&l,v));CHECK(!wx_looks_valid(NULL,v));
    uint32_t seed=1234;memcpy(keep,v,sizeof v);
    CHECK(!wx_looks_randomize(NULL,v,&seed)&&seed==1234&&!memcmp(keep,v,sizeof v));
    CHECK(!wx_looks_randomize(&l,v,NULL)&&!memcmp(keep,v,sizeof v));
    v[0]=8;memcpy(keep,v,sizeof v);CHECK(!wx_looks_randomize(&l,v,&seed)&&seed==1234&&!memcmp(keep,v,sizeof v));
    v[0]=1;memcpy(keep,v,sizeof v);unsigned saved_count=l.header.count;l.header.count=2;
    CHECK(!wx_looks_randomize(&l,v,&seed)&&seed==1234&&!memcmp(keep,v,sizeof v));l.header.count=saved_count;
    uint32_t bald[]={1,0,0,0,4,7,2};l.rows[7].variation=9;CHECK(wx_looks_valid(&l,bald));l.rows[7].variation=4;
    uint8_t pixels[65536];const WxLookRow* r=wx_looks_find(&l,0,0,0);
    CHECK(wx_looks_read(&l,r,0,pixels,sizeof pixels));CHECK(!wx_looks_read(&l,r,0,pixels,sizeof pixels-1));
    CHECK(!wx_looks_read(&l,r,3,pixels,sizeof pixels));wx_looks_close(&l);CHECK(!l.file);
    init();fixture.h.count=513;reject();init();fixture.h.file_size--;reject();init();fixture.h.row_size++;reject();
    init();fixture.h.data_offset--;reject();init();fixture.h.race=9;reject();init();fixture.h.sex=2;reject();
    init();fixture.r[1]=fixture.r[0];reject();init();fixture.r[0].reserved=1;reject();init();fixture.r[0].kind=7;reject();
    init();fixture.r[0].offset[0]=UINT32_MAX;reject();init();fixture.r[0].offset[0]++;reject();
    init();fixture.r[0].region[0]=WX_LOOK_MIPS;reject();init();fixture.r[0].offset[0]=0;reject();
    init();fixture.r[7].geoset[0]=100;reject();init();fixture.r[7].offset[0]=offsetof(Fixture,pixels);reject();
    init();fixture.r[0].geoset[0]=1;reject();init();fixture.r[4].color=1;write_file();CHECK(wx_looks_open(&l,"looks-fixture.wxl"));
    uint32_t zero[]={1,0,0,0,0,0,0};CHECK(!wx_looks_valid(&l,zero));wx_looks_close(&l);
    remove("looks-fixture.wxl");printf("Appearance boundaries and dependent choices: %u checks pass\n",checks);return 0;
}
