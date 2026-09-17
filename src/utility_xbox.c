#include "wx_utility.h"
#include <pbkit/pbkit.h>
#include "wx_font.h"
#include <stdlib.h>
static unsigned sector(int x,int y){return abs(y)>=abs(x)?(y<0?WX_UTILITY_BAGS:WX_UTILITY_QUESTS):(x>0?WX_UTILITY_SPELLS:WX_UTILITY_SETTINGS);}
void wx_utility_draw(const WxUtility* u){
    if(!u->open)return;pb_fill(24,50,592,388,0xff17212a);wx_font_clear();
    // A small scanline ring uses the existing pbkit rectangle primitive.
    for(int y=-132;y<132;y+=4){
        unsigned previous=0;int start=-132;
        for(int x=-132;x<=132;x+=4){
            int radius=x*x+y*y;unsigned part=x<132&&radius>=54*54&&radius<132*132?sector(x,y):0;
            if(part!=previous){if(previous)pb_fill(320+start,220+y,x-start,4,previous==u->selection?0xff476b86:0xff263b4b);start=x;previous=part;}
        }
    }
    wx_font_printat(2,22,"UTILITY MENU");
    wx_font_printat(4,28,"Bags");wx_font_printat(8,39,"Spellbook");wx_font_printat(12,27,"Quests");wx_font_printat(8,7,"Settings");
    wx_font_printat(7,26,"Choose");wx_font_printat(8,26,"a menu");
    wx_font_printat(14,3,"Left stick or D-pad: choose   B: cancel");
    wx_font_printat(15,3,"Release Black or press A: open");
}
