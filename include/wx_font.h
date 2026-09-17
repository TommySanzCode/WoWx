#ifndef WX_FONT_H
#define WX_FONT_H
#include <stdint.h>
#define WX_FONT_ROWS 16
#define WX_FONT_COLS 60
#define WX_FONT_ATLAS_BYTES (128u*128u*4u)
#define WX_FONT_MAX_GLYPHS (WX_FONT_ROWS*WX_FONT_COLS)
typedef struct WxFontGrid { unsigned row,column;uint8_t cells[WX_FONT_ROWS][WX_FONT_COLS]; } WxFontGrid;
typedef struct WxFontVertex { float position[4],uv[2]; } WxFontVertex;
void wx_font_grid_clear(WxFontGrid* grid);
void wx_font_grid_write(WxFontGrid* grid,const char* text);
void wx_font_grid_at(WxFontGrid* grid,int row,int column);
void wx_font_atlas(uint32_t* pixels);
unsigned wx_font_vertices(const WxFontGrid* grid,WxFontVertex* output,unsigned capacity);
unsigned wx_font_count(const WxFontGrid* grid);
// Native backend. Initialization is once, all frame storage is bounded.
int wx_font_init(const char* configuration);
void wx_font_print(const char* format,...);
void wx_font_printat(int row,int column,const char* format,...);
void wx_font_clear(void);
void wx_font_draw(void);
unsigned wx_font_bytes(void);
unsigned wx_font_mode(void);
unsigned wx_font_glyphs(void);
unsigned wx_font_failures(void);
#endif
