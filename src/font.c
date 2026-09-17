#include "wx_font.h"
#include "font_data.h"
#include <string.h>
void wx_font_grid_clear(WxFontGrid* g){g->row=g->column=0;memset(g->cells,' ',sizeof g->cells);}
static void newline(WxFontGrid* g){
    g->column=0;
    if(++g->row==WX_FONT_ROWS){
        memmove(g->cells,g->cells+1,(WX_FONT_ROWS-1)*WX_FONT_COLS);
        memset(g->cells[WX_FONT_ROWS-1],' ',WX_FONT_COLS);g->row=WX_FONT_ROWS-1;
    }
}
void wx_font_grid_at(WxFontGrid* g,int row,int column){
    if(row>=0&&row<WX_FONT_ROWS)g->row=(unsigned)row;
    if(column>=0&&column<WX_FONT_COLS)g->column=(unsigned)column;
}
void wx_font_grid_write(WxFontGrid* g,const char* text){
    while(*text){unsigned c=(uint8_t)*text++;
        if(c=='\n')newline(g);else if(c=='\r')g->column=0;
        else if(c=='\b'){if(g->column)g->column--;}
        else if(c>=32){g->cells[g->row][g->column++]=(uint8_t)c;if(g->column==WX_FONT_COLS)newline(g);}
    }
}
static unsigned morton(unsigned x,unsigned y){unsigned value=0;for(unsigned bit=0;bit<7;bit++)value|=((x>>bit)&1)<<(bit*2)|((y>>bit)&1)<<(bit*2+1);return value;}
void wx_font_atlas(uint32_t* pixels){
    for(unsigned c=0;c<256;c++)for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)
        pixels[morton((c%16)*8+x,(c/16)*8+y)]=(wx_font_bitmap[c*8+y]&(0x80>>x))?0xffffffff:0;
}
unsigned wx_font_vertices(const WxFontGrid* g,WxFontVertex* out,unsigned capacity){
    unsigned count=0;
    for(unsigned row=0;row<WX_FONT_ROWS;row++)for(unsigned col=0;col<WX_FONT_COLS;col++){
        unsigned c=g->cells[row][col],ink=0;for(unsigned i=0;i<8;i++)ink|=wx_font_bitmap[c*8+i];
        if(!ink)continue;if(capacity-count<4)return count;
        float x=20+col*10,y=25+row*25,u=(c%16)/16.f,v=(c/16)/16.f;
        // Pixel centers and nearest filtering reproduce the existing 8x16 glyphs.
        out[count++]=(WxFontVertex){{x,y,0,1},{u,v}};
        out[count++]=(WxFontVertex){{x+8,y,0,1},{u+1.f/16,v}};
        out[count++]=(WxFontVertex){{x+8,y+16,0,1},{u+1.f/16,v+1.f/16}};
        out[count++]=(WxFontVertex){{x,y+16,0,1},{u,v+1.f/16}};
    }return count;
}
unsigned wx_font_count(const WxFontGrid* g){
    unsigned count=0;for(unsigned row=0;row<WX_FONT_ROWS;row++)for(unsigned col=0;col<WX_FONT_COLS;col++){
        unsigned c=g->cells[row][col],ink=0;for(unsigned i=0;i<8;i++)ink|=wx_font_bitmap[c*8+i];count+=ink!=0;
    }return count;
}
