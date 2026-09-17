#include "wx_font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Font FAIL %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned address(unsigned x,unsigned y){unsigned n=0;for(unsigned bit=0;bit<7;bit++)n|=((x>>bit)&1)<<(2*bit)|((y>>bit)&1)<<(2*bit+1);return n;}
int main(void){
    WxFontGrid g;wx_font_grid_clear(&g);CHECK(!wx_font_count(&g)&&!g.row&&!g.column);
    wx_font_grid_write(&g,"ABC\bZ\rQ\nHi");CHECK(g.cells[0][0]=='Q'&&g.cells[0][1]=='B'&&g.cells[0][2]=='Z');
    CHECK(g.row==1&&g.column==2&&g.cells[1][0]=='H'&&g.cells[1][1]=='i');
    wx_font_grid_at(&g,-1,80);CHECK(g.row==1&&g.column==2);wx_font_grid_at(&g,15,59);wx_font_grid_write(&g,"X");
    CHECK(g.row==15&&!g.column&&g.cells[14][59]=='X'&&g.cells[15][59]==' ');
    CHECK(g.cells[0][0]=='H'&&g.cells[0][1]=='i');
    for(unsigned i=0;i<1000;i++)wx_font_grid_write(&g,"a\n");CHECK(g.row==15&&g.column==0);
    for(unsigned i=0;i<15;i++)CHECK(g.cells[i][0]=='a'&&g.cells[i][1]==' ');
    wx_font_grid_clear(&g);wx_font_grid_write(&g,"\b\b\t\001");CHECK(!g.row&&!g.column&&!wx_font_count(&g));
    wx_font_grid_write(&g,"A B");CHECK(wx_font_count(&g)==2);
    WxFontVertex v[9];memset(v,0x5a,sizeof v);WxFontVertex sentinel=v[8];
    CHECK(wx_font_vertices(&g,v,3)==0&&!memcmp(v+8,&sentinel,sizeof sentinel));
    CHECK(wx_font_vertices(&g,v,7)==4);CHECK(v[0].position[0]==20&&v[0].position[1]==25&&v[0].position[3]==1);
    CHECK(v[2].position[0]==28&&v[2].position[1]==41);CHECK(v[0].uv[0]==1.f/16&&v[0].uv[1]==4.f/16);
    CHECK(wx_font_vertices(&g,v,8)==8&&v[4].position[0]==40&&!memcmp(v+8,&sentinel,sizeof sentinel));
    wx_font_grid_clear(&g);wx_font_grid_at(&g,15,58);wx_font_grid_write(&g,"A");
    CHECK(wx_font_vertices(&g,v,8)==4&&v[0].position[0]==600&&v[0].position[1]==400);
    uint32_t* pixels=malloc(WX_FONT_ATLAS_BYTES+8);CHECK(pixels);pixels[0]=0x12345678;pixels[128*128+1]=0x87654321;
    wx_font_atlas(pixels+1);CHECK(pixels[0]==0x12345678&&pixels[128*128+1]==0x87654321);
    // Known A shape from the retained nxdk bitmap; transparent space and edges.
    const unsigned a[8]={0x7c,0xfe,0xee,0xfe,0xee,0xee,0xee,0xee};
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)CHECK(pixels[1+address(8+x,32+y)]==((a[y]&(0x80>>x))?0xffffffffu:0));
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)CHECK(!pixels[1+address(x,16+y)]);
    free(pixels);printf("Font layout, scrolling, atlas and vertex bounds: %u checks passed\n",checks);return 0;
}
