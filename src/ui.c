// Fixed atlas and command storage; game artwork stays in a private asset pack.
#include "wx_ui.h"
#include "wx_runtime.h"
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#define PIXELS (WX_UI_SIZE*WX_UI_SIZE*4u)
#define VERTICES (WX_UI_QUADS*4u*sizeof(WxUiVertex))
void wx_ui_close(WxUi* u){if(!u)return;wx_gpu_free(u->pixels);wx_gpu_free(u->vertices);u->pixels=NULL;u->vertices=NULL;u->bytes=u->ready=u->quads=u->sprite_count=u->batch_count=0;}
static int rectangle(unsigned x,unsigned y,unsigned w,unsigned h){return x<WX_UI_SIZE&&y<WX_UI_SIZE&&w<=WX_UI_SIZE-x&&h<=WX_UI_SIZE-y;}
int wx_ui_open(WxUi* u,const char* path){
    if(!u||!path)return 0;memset(u,0,sizeof *u);FILE* f=fopen(path,"rb");WxUiHeader h;
    if(!f)goto fail;
    if(fread(&h,1,sizeof h,f)!=sizeof h||memcmp(h.magic,"WXU1",4)||(h.version!=1&&h.version!=2)||h.width!=WX_UI_SIZE||h.height!=WX_UI_SIZE||
       h.glyph_count!=WX_UI_GLYPHS||h.sprite_count!=(h.version==1?WX_UI_LEGACY_SPRITES:WX_UI_SPRITES)||h.glyph_offset!=sizeof h||
       h.sprite_offset!=h.glyph_offset+sizeof u->glyphs||h.pixel_offset!=h.sprite_offset+h.sprite_count*sizeof(WxUiSprite)||h.file_size!=h.pixel_offset+PIXELS)goto fail;
    if(fseek(f,0,SEEK_END)||ftell(f)!=(long)h.file_size||fseek(f,h.glyph_offset,SEEK_SET)||
       fread(u->glyphs,1,sizeof u->glyphs,f)!=sizeof u->glyphs||fread(u->sprites,sizeof(WxUiSprite),h.sprite_count,f)!=h.sprite_count)goto fail;
    for(unsigned i=0;i<WX_UI_GLYPHS;i++){const WxUiGlyph* g=&u->glyphs[i];
        if(g->code!=32+i%95||g->font!=i/95||g->reserved||!rectangle(g->x,g->y,g->w,g->h)||
           g->w>64||g->h>64||g->ox< -64||g->ox>64||g->oy< -64||g->oy>64||g->advance<0||g->advance>4096)goto fail;
    }
    for(unsigned i=0;i<h.sprite_count;i++){const WxUiSprite* s=&u->sprites[i];
        if(s->id!=i||s->reserved||!s->w||!s->h||!rectangle(s->x,s->y,s->w,s->h))goto fail;
    }
    if(u->sprites[WX_UI_BORDER].w!=8*u->sprites[WX_UI_BORDER].h)goto fail;
    if(wx_free_memory()<8u*1024u*1024u+PIXELS+VERTICES+65536)goto fail;
    u->pixels=wx_gpu_alloc(PIXELS);u->vertices=wx_gpu_alloc(VERTICES);
    if(!u->pixels||!u->vertices||fread(u->pixels,1,PIXELS,f)!=PIXELS)goto fail;
    fclose(f);u->bytes=PIXELS+VERTICES;u->sprite_count=h.sprite_count;u->ready=1;return 1;
fail:if(f)fclose(f);wx_ui_close(u);u->failures++;return 0;
}
void wx_ui_clear(WxUi* u){u->quads=u->batch_count=0;}
static void quad_texture(WxUi* u,const uint32_t* pixels,unsigned size,float x,float y,float w,float h,float tx,float ty,float tw,float th,uint32_t color,int rotated){
    if(!u->ready||w<=0||h<=0||!isfinite(x)||!isfinite(y)||!isfinite(w)||!isfinite(h)||fabsf(x)>32768||fabsf(y)>32768||w>32768||h>32768)return;
    float x0=fmaxf(0,x),y0=fmaxf(0,y),x1=fminf(640,x+w),y1=fminf(480,y+h);if(x1<=x0||y1<=y0)return;
    if(u->quads==WX_UI_QUADS){u->failures++;return;}
    WxUiBatch* batch=u->batch_count?&u->batches[u->batch_count-1]:NULL;
    if(!batch||batch->pixels!=pixels||batch->size!=size){
        if(u->batch_count==WX_UI_BATCHES){u->failures++;return;}
        batch=&u->batches[u->batch_count++];*batch=(WxUiBatch){pixels,size,u->quads,0};
    }batch->count++;
    float ax[4]={(x0-x)/w,(x1-x)/w,(x1-x)/w,(x0-x)/w};
    float ay[4]={(y0-y)/h,(y0-y)/h,(y1-y)/h,(y1-y)/h};
    float rgba[4]={((color>>16)&255)/255.f,((color>>8)&255)/255.f,(color&255)/255.f,(color>>24)/255.f};
    WxUiVertex* v=u->vertices+u->quads++*4;
    v[0]=(WxUiVertex){{x0,y0,0,1},{0},{0}};v[1]=(WxUiVertex){{x1,y0,0,1},{0},{0}};
    v[2]=(WxUiVertex){{x1,y1,0,1},{0},{0}};v[3]=(WxUiVertex){{x0,y1,0,1},{0},{0}};
    for(unsigned i=0;i<4;i++){memcpy(v[i].color,rgba,sizeof rgba);
        v[i].uv[0]=(tx+(rotated?ay[i]:ax[i])*tw)/size;v[i].uv[1]=(ty+(rotated?ax[i]:ay[i])*th)/size;}
}
static void quad_ex(WxUi* u,float x,float y,float w,float h,float tx,float ty,float tw,float th,uint32_t color,int rotated){
    quad_texture(u,u->pixels,WX_UI_SIZE,x,y,w,h,tx,ty,tw,th,color,rotated);
}
void wx_ui_texture(WxUi* u,const uint32_t* pixels,unsigned size,float x,float y,float w,float h,float u0,float v0,float u1,float v1,uint32_t color){
    if(!u||!pixels||size<32||size>512||(size&(size-1))||!isfinite(u0)||!isfinite(v0)||!isfinite(u1)||!isfinite(v1)||
       u0<0||u0>1||u1<0||u1>1||v0<0||v0>1||v1<0||v1>1)return;
    quad_texture(u,pixels,size,x,y,w,h,u0*size,v0*size,(u1-u0)*size,(v1-v0)*size,color,0);
}
static void quad(WxUi* u,float x,float y,float w,float h,float tx,float ty,float tw,float th,uint32_t color){quad_ex(u,x,y,w,h,tx,ty,tw,th,color,0);}
static const WxUiGlyph* glyph(const WxUi* u,unsigned font,unsigned c){if(font>=WX_UI_FONTS)font=0;if(c<32||c>126)c='?';return &u->glyphs[font*95+c-32];}
float wx_ui_width(const WxUi* u,unsigned font,const char* text){
    if(!u||!u->ready||!text)return 0;float width=0,maximum=0;
    for(unsigned n=0;text[n]&&n<4096;n++){if(text[n]=='\n'){maximum=fmaxf(maximum,width);width=0;}else width+=glyph(u,font,(uint8_t)text[n])->advance/64.f;}
    return fmaxf(maximum,width);
}
void wx_ui_text(WxUi* u,unsigned font,float x,float y,uint32_t color,const char* text){
    if(!u||!u->ready||!text)return;float start=x;
    for(unsigned n=0;text[n]&&n<4096;n++){
        if(text[n]=='\n'){x=start;y+=font==2?32:font==1?24:19;continue;}
        const WxUiGlyph* g=glyph(u,font,(uint8_t)text[n]);quad(u,x+g->ox,y+g->oy,g->w,g->h,g->x,g->y,g->w,g->h,color);x+=g->advance/64.f;
    }
}
void wx_ui_textf(WxUi* u,unsigned font,float x,float y,uint32_t color,const char* format,...){char text[512];va_list args;va_start(args,format);vsnprintf(text,sizeof text,format,args);va_end(args);text[sizeof text-1]=0;wx_ui_text(u,font,x,y,color,text);}
void wx_ui_center(WxUi* u,unsigned font,float x,float y,uint32_t color,const char* text){wx_ui_text(u,font,x-wx_ui_width(u,font,text)*.5f,y,color,text);}
void wx_ui_image_region(WxUi* u,unsigned sprite,float x,float y,float w,float h,float u0,float v0,float u1,float v1,uint32_t color){
    if(!u||sprite>=u->sprite_count||!isfinite(u0)||!isfinite(v0)||!isfinite(u1)||!isfinite(v1)||
       u0<0||u0>1||u1<0||u1>1||v0<0||v0>1||v1<0||v1>1)return;
    const WxUiSprite* s=&u->sprites[sprite];quad(u,x,y,w,h,s->x+u0*s->w,s->y+v0*s->h,(u1-u0)*s->w,(v1-v0)*s->h,color);
}
void wx_ui_image(WxUi* u,unsigned sprite,float x,float y,float w,float h,uint32_t color){wx_ui_image_region(u,sprite,x,y,w,h,0,0,1,1,color);}
void wx_ui_text_fit(WxUi* u,unsigned font,float x,float y,float width,uint32_t color,const char* text){
    if(!u||!u->ready||!text||!isfinite(width)||width<=0)return;
    char line[128];unsigned n=0;float used=0;
    while(n<sizeof line-1&&text[n]&&text[n]!='\n'){
        float advance=glyph(u,font,(uint8_t)text[n])->advance/64.f;if(used+advance>width)break;
        line[n]=text[n];n++;used+=advance;
    }
    if(text[n]&&text[n]!='\n'){
        float dots=wx_ui_width(u,font,"...");if(dots>width)return;
        while(n&&(used+dots>width||n>sizeof line-4))used-=glyph(u,font,(uint8_t)line[--n])->advance/64.f;
        memcpy(line+n,"...",3);n+=3;
    }
    line[n]=0;wx_ui_text(u,font,x,y,color,line);
}
void wx_ui_rect(WxUi* u,float x,float y,float w,float h,uint32_t color){
    const WxUiSprite* s=&u->sprites[WX_UI_WHITE_PIXEL];
    // A constant center sample keeps stretched fills away from transparent padding.
    quad(u,x,y,w,h,s->x+s->w*.5f,s->y+s->h*.5f,0,0,color);
}
void wx_ui_gradient(WxUi* u,float x,float y,float w,float h,const float colors[4][3]){
    if(!u||!colors||x<0||y<0||x+w>640||y+h>480)return;
    for(unsigned i=0;i<4;i++)for(unsigned k=0;k<3;k++)if(!isfinite(colors[i][k])||colors[i][k]<0||colors[i][k]>1)return;
    unsigned before=u->quads;wx_ui_rect(u,x,y,w,h,0xffffffff);
    if(u->quads!=before+1)return;
    for(unsigned i=0;i<4;i++)memcpy(u->vertices[before*4+i].color,colors[i],3*sizeof(float));
}
static void edge_run(WxUi* u,const WxUiSprite* s,unsigned index,float x,float y,float span,int horizontal){
    if(span<=0)return;float step=fmaxf(16,span/64),e=s->h;unsigned count=(unsigned)ceilf(span/step);
    for(unsigned i=0;i<count;i++){float at=i*step,len=fminf(step,span-at),fraction=len/step;
        quad_ex(u,x+(horizontal?at:0),y+(horizontal?0:at),horizontal?len:16,horizontal?16:len,
            s->x+index*e,s->y,e,e*fraction,0xffffffff,horizontal);}
}
void wx_ui_panel(WxUi* u,float x,float y,float w,float h){
    if(!isfinite(x)||!isfinite(y)||!isfinite(w)||!isfinite(h)||w<32||h<32||w>32768||h>32768)return;
    wx_ui_image(u,WX_UI_PANEL,x+4,y+4,w-8,h-8,0xffe0e0e0);
    const WxUiSprite* s=&u->sprites[WX_UI_BORDER];float e=s->h;
    // Pinned WoWee WidgetRenderer backdrop convention: tiles 2/3 store the
    // horizontal edges sideways. Repeat each isolated tile, then cover joins.
    edge_run(u,s,0,x,y+16,h-32,0);edge_run(u,s,1,x+w-16,y+16,h-32,0);
    edge_run(u,s,2,x+16,y,w-32,1);edge_run(u,s,3,x+16,y+h-16,w-32,1);
    const float corners[4][2]={{x,y},{x+w-16,y},{x,y+h-16},{x+w-16,y+h-16}};
    for(unsigned i=0;i<4;i++)quad(u,corners[i][0],corners[i][1],16,16,s->x+(i+4)*e,s->y,e,e,0xffffffff);
}
void wx_ui_button(WxUi* u,float x,float y,float w,const char* label,int focused,int enabled){
    if(focused)wx_ui_rect(u,x-2,y+3,w+4,32,0xffc39743);
    wx_ui_image(u,enabled?WX_UI_BUTTON:WX_UI_BUTTON_DISABLED,x,y,w,38,0xffffffff);
    wx_ui_center(u,0,x+w*.5f-2,y+9,enabled?(focused?WX_UI_WHITE:WX_UI_GOLD):0xff888888,label);
}
