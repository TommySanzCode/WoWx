#include "wx_fog.h"
#include "wx_font.h"
#include "wx_runtime.h"
#include <pbkit/pbkit.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define FONT_PROGRAM 96
#define VERTEX_BYTES (WX_FONT_MAX_GLYPHS*4u*sizeof(WxFontVertex))
static WxFontGrid grid;
static uint32_t* atlas;
static WxFontVertex* vertices;
static unsigned glyphs,failures;
unsigned wx_font_bytes(void){return atlas?WX_FONT_ATLAS_BYTES+VERTEX_BYTES:0;}
unsigned wx_font_mode(void){return atlas!=NULL;}
unsigned wx_font_glyphs(void){return glyphs;}
unsigned wx_font_failures(void){return failures;}
int wx_font_init(const char* configuration){
    wx_font_grid_clear(&grid);
    FILE* file=fopen(configuration,"rb");
    if(file){char mode[4];int good=fread(mode,1,4,file)==4&&fgetc(file)==EOF;fclose(file);
        if(good&&!memcmp(mode,"WXFL",4))return 0;
        if(!good||memcmp(mode,"NONE",4)){failures++;return 0;}
    }
    if(wx_free_memory()<8u*1024u*1024u+WX_FONT_ATLAS_BYTES+VERTEX_BYTES+65536){failures++;return 0;}
    atlas=wx_gpu_alloc(WX_FONT_ATLAS_BYTES);vertices=wx_gpu_alloc(VERTEX_BYTES);
    if(!atlas||!vertices){wx_gpu_free(atlas);wx_gpu_free(vertices);atlas=NULL;vertices=NULL;failures++;return 0;}
    wx_font_atlas(atlas);
    const uint32_t program[]={
#include "font.inl"
    };
    uint32_t* p=pb_begin();p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_LOAD,FONT_PROGRAM);pb_end(p);
    for(unsigned i=0;i<sizeof program/16;i++){
        p=pb_begin();pb_push(p++,NV097_SET_TRANSFORM_PROGRAM,4);memcpy(p,program+i*4,16);p+=4;pb_end(p);
    }return 1;
}
void wx_font_clear(void){wx_font_grid_clear(&grid);}
static void print_args(const char* format,va_list args){char text[512];vsnprintf(text,sizeof text,format,args);text[sizeof text-1]=0;wx_font_grid_write(&grid,text);}
void wx_font_print(const char* format,...){va_list args;va_start(args,format);print_args(format,args);va_end(args);}
void wx_font_printat(int row,int column,const char* format,...){wx_font_grid_at(&grid,row,column);va_list args;va_start(args,format);print_args(format,args);va_end(args);}
void wx_font_draw(void){
    wx_fog_bind(NULL);
    glyphs=0;
    if(!atlas){
        pb_erase_text_screen();
        for(unsigned row=0;row<WX_FONT_ROWS;row++){
            char text[WX_FONT_COLS+1];memcpy(text,grid.cells[row],WX_FONT_COLS);unsigned end=WX_FONT_COLS;
            while(end&&text[end-1]==' ')end--;text[end]=0;
            if(end)pb_printat(row,0,"%s",text);
        }
        glyphs=wx_font_count(&grid);
        pb_draw_text_screen();return;
    }
    unsigned count=wx_font_vertices(&grid,vertices,WX_FONT_MAX_GLYPHS*4);glyphs=count/4;if(!count)return;
    uint32_t* p=pb_begin();
    p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,FONT_PROGRAM);
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,0);p=pb_push1(p,NV097_SET_DEPTH_MASK,0);
    p=pb_push1(p,NV097_SET_CULL_FACE_ENABLE,0);p=pb_push1(p,NV097_SET_BLEND_ENABLE,0);
    p=pb_push1(p,NV097_SET_ALPHA_TEST_ENABLE,1);p=pb_push1(p,NV097_SET_ALPHA_FUNC,NV097_SET_ALPHA_FUNC_V_GREATER);p=pb_push1(p,NV097_SET_ALPHA_REF,127);
    pb_push(p++,NV097_SET_VERTEX_DATA_ARRAY_FORMAT,16);for(unsigned i=0;i<16;i++)*p++=2;
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT,2|(4<<4)|(sizeof(WxFontVertex)<<8));
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET,(uint32_t)vertices&0x03ffffff);
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+9*4,2|(2<<4)|(sizeof(WxFontVertex)<<8));
    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+9*4,(uint32_t)vertices[0].uv&0x03ffffff);
    float white[4]={1,1,1,1};pb_push(p++,NV097_SET_VERTEX_DATA4F_M+3*16,4);memcpy(p,white,sizeof white);p+=4;
    p=pb_push2(p,NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0),(uint32_t)atlas&0x03ffffff,0x0771062a);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0),512<<16);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0),(128<<16)|128);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_WRAP(0),0x00030303);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0),0x4003ffc0);
    p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_FILTER(0),0x01012000);
    for(unsigned i=1;i<4;i++)p=pb_push1(p,NV20_TCL_PRIMITIVE_3D_TX_ENABLE(i),0x0003ffc0);
    p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_QUADS);
    for(unsigned at=0;at<count;){unsigned n=count-at;if(n>256)n=256;p=pb_push1(p,NV097_DRAW_ARRAYS,((n-1)<<24)|at);at+=n;}
    p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);pb_end(p);
}
