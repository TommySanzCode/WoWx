#ifndef WX_UI_H
#define WX_UI_H
#include <stdint.h>
#define WX_UI_SIZE 512u
#define WX_UI_FONTS 3u
#define WX_UI_GLYPHS (95u*WX_UI_FONTS)
#define WX_UI_SPRITES 10u
#define WX_UI_LEGACY_SPRITES 7u
#define WX_UI_QUADS 2048u
#define WX_UI_BATCHES 128u
#define WX_UI_GOLD 0xffffd100u
#define WX_UI_WHITE 0xfff4e9d0u
enum {WX_UI_LOGO,WX_UI_BUTTON,WX_UI_BUTTON_DOWN,WX_UI_BUTTON_DISABLED,WX_UI_PANEL,WX_UI_BORDER,WX_UI_WHITE_PIXEL,
      WX_UI_UNIT_FRAME,WX_UI_STATUS_BAR,WX_UI_UNIT_SKULL};
typedef struct WxUiHeader {char magic[4];uint32_t version,width,height,glyph_count,sprite_count,glyph_offset,sprite_offset,pixel_offset,file_size;} WxUiHeader;
typedef struct WxUiGlyph {uint16_t code,font,x,y,w,h;int16_t ox,oy,advance,reserved;} WxUiGlyph;
typedef struct WxUiSprite {uint16_t id,x,y,w,h,reserved;} WxUiSprite;
typedef struct WxUiVertex {float position[4],color[4],uv[2];} WxUiVertex;
typedef struct WxUiBatch {const uint32_t* pixels;unsigned size,first,count;} WxUiBatch;

typedef struct WxUi {
    uint32_t* pixels;WxUiVertex* vertices;
    WxUiGlyph glyphs[WX_UI_GLYPHS];WxUiSprite sprites[WX_UI_SPRITES];
    unsigned ready,bytes,quads,failures,sprite_count;
    WxUiBatch batches[WX_UI_BATCHES];unsigned batch_count;
} WxUi;
int wx_ui_open(WxUi* ui,const char* path);
void wx_ui_close(WxUi* ui);
void wx_ui_clear(WxUi* ui);
float wx_ui_width(const WxUi* ui,unsigned font,const char* text);
void wx_ui_text(WxUi* ui,unsigned font,float x,float y,uint32_t color,const char* text);
void wx_ui_textf(WxUi* ui,unsigned font,float x,float y,uint32_t color,const char* format,...);
void wx_ui_center(WxUi* ui,unsigned font,float x,float y,uint32_t color,const char* text);
void wx_ui_image(WxUi* ui,unsigned sprite,float x,float y,float w,float h,uint32_t color);
void wx_ui_image_region(WxUi* ui,unsigned sprite,float x,float y,float w,float h,float u0,float v0,float u1,float v1,uint32_t color);
void wx_ui_text_fit(WxUi* ui,unsigned font,float x,float y,float width,uint32_t color,const char* text);
void wx_ui_rect(WxUi* ui,float x,float y,float w,float h,uint32_t color);
/* In-bounds opaque gradient, corners ordered TL/TR/BR/BL. */
void wx_ui_gradient(WxUi* ui,float x,float y,float w,float h,const float colors[4][3]);
void wx_ui_panel(WxUi* ui,float x,float y,float w,float h);
void wx_ui_button(WxUi* ui,float x,float y,float w,const char* label,int focused,int enabled);
// External swizzled square textures remain owned by their bounded cache.
void wx_ui_texture(WxUi* ui,const uint32_t* pixels,unsigned size,float x,float y,float w,float h,float u0,float v0,float u1,float v1,uint32_t color);
void wx_ui_draw(WxUi* ui);
#endif
