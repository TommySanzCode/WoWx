#include "wx_ui.h"
#include "wx_sky.h"
#include "wx_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
static unsigned checks,outstanding,free_bytes=60u*1024u*1024u;static int fail_after=-1;
#define CHECK(v) do{checks++;if(!(v)){fprintf(stderr,"UI FAIL %u: %s\n",__LINE__,#v);exit(1);}}while(0)
unsigned wx_free_memory(void){return free_bytes;}
void* wx_gpu_alloc(unsigned bytes){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;uint32_t* p=malloc(bytes+8);if(!p)return NULL;
    p[0]=bytes;p[1+bytes/4]=0xfeedbeef;free_bytes-=bytes;outstanding++;return p+1;}
void wx_gpu_free(void* value){if(value){uint32_t* p=(uint32_t*)value-1;CHECK(p[1+p[0]/4]==0xfeedbeef);free_bytes+=p[0];outstanding--;free(p);}}
typedef struct Fixture {WxUiHeader h;WxUiGlyph glyphs[WX_UI_GLYPHS];WxUiSprite sprites[WX_UI_SPRITES];uint32_t pixels[512*512];} Fixture;
static Fixture pack;static WxUi canvas;
static void fixture(void){memset(&pack,0,sizeof pack);pack.h=(WxUiHeader){{'W','X','U','1'},2,512,512,WX_UI_GLYPHS,WX_UI_SPRITES,offsetof(Fixture,glyphs),offsetof(Fixture,sprites),offsetof(Fixture,pixels),sizeof pack};
    for(unsigned i=0;i<WX_UI_GLYPHS;i++)pack.glyphs[i]=(WxUiGlyph){32+i%95,i/95,1,1,8,12,0,2,8*64,0};
    for(unsigned i=0;i<WX_UI_SPRITES;i++)pack.sprites[i]=(WxUiSprite){i,1,1,16,16,0};pack.sprites[WX_UI_BORDER].w=128;
}
static void write_pack(void){FILE* f=fopen("ui-fixture.wui","wb");CHECK(f);CHECK(fwrite(&pack,1,sizeof pack,f)==sizeof pack);CHECK(!fclose(f));}
static void rejected(void){write_pack();CHECK(!wx_ui_open(&canvas,"ui-fixture.wui"));CHECK(canvas.failures==1&&!canvas.ready&&!canvas.bytes&&!outstanding);}
int main(int argc,char** argv){
    CHECK(sizeof(WxUiHeader)==40&&sizeof(WxUiGlyph)==20&&sizeof(WxUiSprite)==12&&sizeof(WxUiVertex)==40);
    if(argc==2){CHECK(wx_ui_open(&canvas,argv[1]));CHECK(canvas.bytes==1376256);CHECK(wx_ui_width(&canvas,0,"World of Warcraft")>70);
        wx_ui_panel(&canvas,20,100,600,320);wx_ui_text(&canvas,0,30,125,WX_UI_WHITE,"Native Vanilla interface");CHECK(canvas.quads>20);wx_ui_close(&canvas);CHECK(!outstanding);printf("Actual UI pack: %u checks passed\n",checks);return 0;}
    fixture();write_pack();CHECK(wx_ui_open(&canvas,"ui-fixture.wui"));CHECK(outstanding==2&&canvas.ready&&canvas.bytes==1376256);
    CHECK(wx_ui_width(&canvas,0,"AB\nC")==16);wx_ui_text(&canvas,0,10,20,0xff123456,"AB");CHECK(canvas.quads==2);
    CHECK(canvas.vertices[0].position[0]==10&&canvas.vertices[0].position[1]==22&&canvas.vertices[4].position[0]==18);
    CHECK(fabsf(canvas.vertices[0].color[0]-18.f/255)<.00001f);
    wx_ui_clear(&canvas);wx_ui_image(&canvas,0,-10,-5,30,20,0xffffffff);CHECK(canvas.quads==1);
    CHECK(canvas.vertices[0].position[0]==0&&canvas.vertices[0].position[1]==0&&canvas.vertices[2].position[0]==20);
    CHECK(canvas.vertices[0].uv[0]>1.f/512&&canvas.vertices[0].uv[1]>1.f/512);
    wx_ui_image_region(&canvas,WX_UI_UNIT_FRAME,30,30,100,50,1,0,0,1,0xffffffff);CHECK(canvas.quads==2);
    CHECK(canvas.vertices[4].uv[0]>canvas.vertices[5].uv[0]);
    wx_ui_image_region(&canvas,WX_UI_STATUS_BAR,30,30,50,8,0,0,.5f,1,0xffffffff);CHECK(canvas.quads==3);
    CHECK(fabsf(canvas.vertices[9].uv[0]-canvas.vertices[8].uv[0]-8.f/512)<.00001f);
    wx_ui_image_region(&canvas,WX_UI_STATUS_BAR,0,0,10,10,-1,0,1,1,0xffffffff);
    wx_ui_image_region(&canvas,WX_UI_STATUS_BAR,0,0,10,10,0,NAN,1,1,0xffffffff);CHECK(canvas.quads==3);
    wx_ui_clear(&canvas);wx_ui_text_fit(&canvas,0,0,0,40,WX_UI_WHITE,"ABCDEFGH");CHECK(canvas.quads==5);
    wx_ui_clear(&canvas);wx_ui_text_fit(&canvas,0,0,0,16,WX_UI_WHITE,"ABCDEFGH");CHECK(!canvas.quads);
    wx_ui_text_fit(&canvas,0,0,0,40,WX_UI_WHITE,"AB");CHECK(canvas.quads==2);
    wx_ui_clear(&canvas);wx_ui_image(&canvas,0,-10,-5,30,20,0xffffffff);
    wx_ui_image(&canvas,0,640,0,20,20,0xffffffff);wx_ui_image(&canvas,0,0,480,20,20,0xffffffff);wx_ui_image(&canvas,0,NAN,0,20,20,0xffffffff);
    wx_ui_image(&canvas,0,0,0,INFINITY,20,0xffffffff);wx_ui_image(&canvas,0,-3e38f,0,3e38f,20,0xffffffff);CHECK(canvas.quads==1);
    wx_ui_clear(&canvas);wx_ui_rect(&canvas,0,0,640,480,0x80ffffff);CHECK(canvas.quads==1);
    for(unsigned i=1;i<4;i++)CHECK(canvas.vertices[i].uv[0]==canvas.vertices[0].uv[0]&&canvas.vertices[i].uv[1]==canvas.vertices[0].uv[1]);
    CHECK(fabsf(canvas.vertices[0].color[3]-128.f/255)<.00001f);
    wx_ui_clear(&canvas);wx_ui_panel(&canvas,20,30,64,80);CHECK(canvas.quads==15);
    // Horizontal screen travel follows the stored edge's vertical texture axis.
    CHECK(canvas.vertices[28].uv[0]==canvas.vertices[29].uv[0]&&canvas.vertices[28].uv[1]<canvas.vertices[29].uv[1]);
    CHECK(canvas.vertices[28].uv[0]<canvas.vertices[31].uv[0]&&canvas.vertices[28].uv[1]==canvas.vertices[31].uv[1]);
    CHECK(canvas.vertices[4].uv[0]<canvas.vertices[5].uv[0]&&canvas.vertices[4].uv[1]==canvas.vertices[5].uv[1]);
    wx_ui_panel(&canvas,0,0,NAN,80);wx_ui_panel(&canvas,0,0,80,INFINITY);CHECK(canvas.quads==15);
    wx_ui_clear(&canvas);CHECK(!canvas.batch_count);
    static uint32_t external[256*256];
    wx_ui_rect(&canvas,0,0,20,20,0xffffffff);
    wx_ui_texture(&canvas,external,256,-10,0,20,20,0,0,1,1,0xffffffff);
    wx_ui_text(&canvas,0,0,0,WX_UI_WHITE,"AB");
    CHECK(canvas.batch_count==3&&canvas.batches[0].count==1&&canvas.batches[1].first==1&&canvas.batches[1].pixels==external&&canvas.batches[2].count==2);
    CHECK(fabsf(canvas.vertices[4].uv[0]-.5f)<.00001f);
    unsigned count=canvas.quads;
    wx_ui_texture(&canvas,external,255,0,0,20,20,0,0,1,1,0xffffffff);
    wx_ui_texture(&canvas,NULL,256,0,0,20,20,0,0,1,1,0xffffffff);
    wx_ui_texture(&canvas,external,256,0,0,20,20,0,0,NAN,1,0xffffffff);CHECK(canvas.quads==count);
    wx_ui_clear(&canvas);
    WxLightPalette palette;WxFog fog;wx_light_fallback(&palette);wx_fog_fallback(&fog,WX_FOG_OUTDOOR);
    float right[4]={0,-1,0,0},up[4]={0,0,1,0},forward[4]={1,0,0,0};unsigned memory=free_bytes;
    CHECK(wx_sky_build(&canvas,&palette,&fog,right,up,forward)==192);CHECK(canvas.batch_count==1&&free_bytes==memory);
    for(unsigned i=0;i<canvas.quads*4;i++){WxUiVertex* v=canvas.vertices+i;CHECK(v->position[0]>=0&&v->position[0]<=640&&v->position[1]>=0&&v->position[1]<=480);
        if(v->position[1]>=240)for(unsigned k=0;k<3;k++)CHECK(fabsf(v->color[k]-fog.color[k])<.0001f);}
    CHECK(wx_sky_build(&canvas,&palette,&fog,right,up,forward)==0); /* Never overwrite queued UI. */
    wx_ui_clear(&canvas);fog.environment=WX_FOG_UNDERWATER;CHECK(wx_sky_build(&canvas,&palette,&fog,right,up,forward)==0);fog.environment=WX_FOG_OUTDOOR;
    for(unsigned j=0;j<9;j++){float pitch=(int)j*.35f-1.4f;forward[0]=cosf(pitch);forward[2]=sinf(pitch);up[0]=-sinf(pitch);up[2]=cosf(pitch);
        wx_ui_clear(&canvas);CHECK(wx_sky_build(&canvas,&palette,&fog,right,up,forward)==192);CHECK(!canvas.failures&&free_bytes==memory);}
    wx_ui_clear(&canvas);for(unsigned i=0;i<WX_UI_BATCHES;i++)wx_ui_texture(&canvas,i%2?external:canvas.pixels,256,0,0,1,1,0,0,1,1,0xffffffff);
    CHECK(canvas.batch_count==WX_UI_BATCHES&&canvas.quads==WX_UI_BATCHES);
    wx_ui_texture(&canvas,canvas.pixels,256,0,0,1,1,0,0,1,1,0xffffffff);CHECK(canvas.failures==1&&canvas.quads==WX_UI_BATCHES);canvas.failures=0;
    wx_ui_clear(&canvas);for(unsigned i=0;i<WX_UI_QUADS+1;i++)wx_ui_rect(&canvas,0,0,1,1,0xffffffff);
    CHECK(canvas.quads==WX_UI_QUADS&&canvas.failures==1);wx_ui_close(&canvas);CHECK(!outstanding&&free_bytes==60u*1024u*1024u);
    for(unsigned i=0;i<10;i++){fixture();((uint32_t*)&pack.h)[i]^=1;rejected();}
    fixture();pack.glyphs[0].code=0;rejected();fixture();pack.glyphs[1].font=3;rejected();fixture();pack.glyphs[2].x=512;rejected();
    fixture();pack.glyphs[3].w=513;rejected();fixture();pack.glyphs[4].h=65;rejected();fixture();pack.glyphs[5].advance=-1;rejected();
    fixture();pack.glyphs[6].advance=4097;rejected();fixture();pack.glyphs[7].oy=65;rejected();fixture();pack.glyphs[8].reserved=1;rejected();
    fixture();pack.sprites[0].id=2;rejected();fixture();pack.sprites[1].w=0;rejected();fixture();pack.sprites[2].y=512;rejected();
    fixture();pack.sprites[3].reserved=1;rejected();fixture();pack.sprites[WX_UI_BORDER].w=127;rejected();
    fixture();write_pack();FILE* f=fopen("ui-fixture.wui","ab");CHECK(f);fputc(0,f);fclose(f);CHECK(!wx_ui_open(&canvas,"ui-fixture.wui")&&!outstanding);
    fixture();f=fopen("ui-fixture.wui","wb");CHECK(f);fwrite(&pack,1,sizeof pack-1,f);fclose(f);CHECK(!wx_ui_open(&canvas,"ui-fixture.wui")&&!outstanding);
    for(int i=0;i<2;i++){fixture();fail_after=i;rejected();}fail_after=-1;
    fixture();free_bytes=9u*1024u*1024u;rejected();CHECK(free_bytes==9u*1024u*1024u);free_bytes=60u*1024u*1024u;
    CHECK(!wx_ui_open(&canvas,"does-not-exist.wui"));CHECK(!canvas.ready&&!outstanding);remove("ui-fixture.wui");printf("UI atlas and draw boundaries: %u checks passed\n",checks);return 0;
}
