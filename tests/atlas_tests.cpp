#include "character_atlas.hpp"
#include <cstdio>
#include <functional>
static unsigned checks;
static void check(bool ok){checks++;if(!ok)throw std::runtime_error("Character atlas test failed");}
static void rejected(const std::function<void()>& f){bool failed=false;try{f();}catch(const std::runtime_error&){failed=true;}check(failed);}
int main(){
    std::vector<uint8_t> canvas(256*256*4),source={255,0,0,255,0,0,255,128};
    wx_atlas::blend(canvas,source,2,1,{0,192,4,1});
    auto p=&canvas[192*256*4];check(p[0]==255&&p[4]==255&&p[10]==128&&p[11]==128&&p[14]==128);
    check(canvas[(191*256)*4]==0&&canvas[(192*256+4)*4]==0);
    source={0,255,0,128};wx_atlas::blend(canvas,source,1,1,{0,192,1,1});
    check(p[0]==127&&p[1]==128&&p[3]==255);
    auto before=canvas;source={255,0,255,255};wx_atlas::blend(canvas,source,1,1,{0,192,1,1});check(canvas==before);
    rejected([&]{wx_atlas::blend(canvas,source,1,1,{255,0,2,1});});
    rejected([&]{wx_atlas::blend(canvas,source,0,1,{0,0,1,1});});
    rejected([&]{wx_atlas::blend(canvas,source,2,1,{0,0,1,1});});
    rejected([&]{wx_atlas::blend(canvas,source,1,1,{0,0,UINT32_MAX,1});});
    rejected([&]{wx_atlas::blend(canvas,source,1,1,{UINT32_MAX,0,1,1});});
    for(auto r:wx_atlas::regions)wx_atlas::blend(canvas,source,1,1,r);
    check(canvas==before); // keyed layers do not alter any of the eight regions
    printf("Character atlas checks: %u passed\n",checks);return 0;
}
