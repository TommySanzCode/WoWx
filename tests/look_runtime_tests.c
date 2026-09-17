#include "wx_avatar.h"
#include <stdlib.h>
#include <string.h>
static unsigned checks,allocated,objects,available=48u*1024u*1024u;static int fail_after=-1;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"LOOK RUNTIME FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
unsigned wx_free_memory(void){return available-allocated;}
void* wx_gpu_alloc(unsigned bytes){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;unsigned* p=malloc(bytes+4);if(!p)return NULL;*p=bytes;allocated+=bytes;objects++;return p+1;}
void wx_gpu_free(void* pointer){if(pointer){unsigned* p=(unsigned*)pointer-1;allocated-=*p;objects--;free(p);}}
static uint32_t hash(const uint32_t* pixels,int rgba){uint32_t result=2166136261u;
    for(unsigned y=0;y<128;y++)for(unsigned x=0;x<128;x++){
        unsigned address=0;for(unsigned bit=0;bit<7;bit++)address|=((x>>bit)&1u)<<(2*bit)|((y>>bit)&1u)<<(2*bit+1);
        uint32_t value=pixels[address];for(unsigned i=0;i<4;i++){unsigned channel=rgba&&!(i&1)?2-i:i;result=(result^((value>>(channel*8))&255))*16777619u;}
    }return result;
}
static WxAvatar avatar;static WxAvatarSelection selection;
int main(int argc,char** argv){
    CHECK(argc==4);unsigned race=strtoul(argv[2],NULL,10),sex=strtoul(argv[3],NULL,10);char path[512],pack[512],metadata[512];
    snprintf(path,sizeof path,"%s/R%02X%02X.WXV",argv[1],race,sex);FILE* reference=fopen(path,"rb");CHECK(reference);
    uint32_t header[3];CHECK(fread(header,1,12,reference)==12&&!memcmp(header,"WXLR",4)&&header[1]&&header[1]<=8192&&header[2]==56);
    WxWorldView world={0};world.active=1;world.race=race;world.gender=sex;world.revision=1;FILE* original=NULL;unsigned maximum=0,max_steps=0;
    for(unsigned sample=0;sample<header[1];sample++){
        uint32_t record[14];CHECK(fread(record,1,sizeof record,reference)==sizeof record&&record[0]==race&&record[1]==sex);
        world.skin=record[2];world.face=record[3];world.hair_style=record[4];world.hair_color=record[5];world.facial_hair=record[6];
        unsigned opened=0;
        for(unsigned tick=0;tick<512;tick++){
            wx_stream_frame_begin(8);opened=wx_avatar_select(&avatar,&selection,&world,argv[1]);
            CHECK(wx_stream_frame_metrics()->total.read_bytes<=65536&&wx_stream_frame_metrics()->total.scan_bytes<=65536);wx_stream_frame_end();
            if(opened||!selection.pending)break;
        }CHECK(opened);if(!original)original=avatar.file;CHECK(original==avatar.file);
        unsigned steps=0;
        do{
            wx_stream_frame_begin(8);wx_avatar_update(&avatar,&world,0,(sample*256+steps)*33);
            CHECK(wx_stream_frame_metrics()->total.read_bytes<=65536&&wx_stream_frame_metrics()->total.read_ops<=16);
            CHECK(avatar.compose.work_pixels<=WX_AVATAR_COMPOSE_PIXELS);wx_stream_frame_end();steps++;
        }while(steps<256&&(!avatar.ready||avatar.compose.phase));
        if(steps>max_steps)max_steps=steps;
        if(!avatar.ready||avatar.compose.phase||avatar.compose.failures)fprintf(stderr,"Sample %u: phase %u layer %u/%u ready %u errors %u/%u/%u parts %u/%u loads %u\n",sample,avatar.compose.phase,avatar.compose.layer,avatar.compose.layer_count,avatar.ready,avatar.failures,avatar.scene.failures,avatar.compose.failures,avatar.count,avatar.compose.count,avatar.scene.loads);
        CHECK(avatar.ready&&avatar.matched&&!avatar.failures&&!avatar.scene.failures&&!avatar.missing&&!avatar.compose.phase&&!avatar.compose.failures);
        uint32_t actual=hash(avatar.body,0);if(actual!=record[7])fprintf(stderr,"Sample %u look %u/%u/%u/%u/%u: body %08x expected %08x\n",sample,record[2],record[3],record[4],record[5],record[6],actual,record[7]);
        CHECK(avatar.atlas_hash==hash(avatar.body,1));
        CHECK(actual==record[7]);CHECK(!record[8]||(avatar.hair&&hash(avatar.hair,0)==record[8]));CHECK(!record[9]||(avatar.extra&&hash(avatar.extra,0)==record[9]));
        CHECK(avatar.header.scalp==record[10]&&!memcmp(avatar.header.facial,record+11,12));
        CHECK(wx_free_memory()>=8u*1024u*1024u&&avatar.scene.bytes<=avatar.scene.budget_bytes);
        unsigned bytes=avatar.scene.bytes+avatar.bytes;if(bytes>maximum)maximum=bytes;
        for(unsigned i=0;i<avatar.scene.header.count;i++){const WxEntry* e=avatar.scene.entries+i;
            if(e->texture_offset==avatar.bindings.hair_texture)CHECK(wx_avatar_texture(&avatar,e,NULL)==avatar.hair);
            if(e->texture_offset==avatar.bindings.extra_texture)CHECK(wx_avatar_texture(&avatar,e,NULL)==avatar.extra);
        }
    }
    CHECK(fgetc(reference)==EOF);fclose(reference);unsigned buffers=2+!!avatar.hair+!!avatar.extra;
    wx_avatar_close(&avatar);CHECK(!allocated&&!objects&&!avatar.looks.file);
    uint32_t base[]={race,sex,0,0,0,0,0};CHECK(wx_avatar_path(pack,sizeof pack,argv[1],base,0)&&wx_avatar_path(metadata,sizeof metadata,argv[1],base,1));
    for(unsigned i=0;i<buffers;i++){fail_after=(int)i;CHECK(!wx_avatar_open(&avatar,pack,metadata)&&!allocated&&!objects&&!avatar.file&&!avatar.looks.file);}
    fail_after=-1;available=8u*1024u*1024u;CHECK(!wx_avatar_open(&avatar,pack,metadata)&&!allocated&&!objects);available=48u*1024u*1024u;
    CHECK(wx_avatar_open(&avatar,pack,metadata));wx_avatar_close(&avatar);CHECK(!allocated&&!objects);
    printf("Appearance %u/%u: %u reference compositions, %u checks, peak accounted avatar bytes %u, maximum %u staged updates; allocation failures release all resources. Host only.\n",race,sex,header[1],checks,maximum,max_steps);return 0;
}
