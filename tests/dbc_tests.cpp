#include "dbc_table.hpp"
#include "world_catalog.hpp"
#include <cstdio>
#include <functional>
static unsigned checks;
static void check(bool ok){checks++;if(!ok)throw std::runtime_error("DBC boundary test failed");}
static void word(std::vector<uint8_t>& b,unsigned at,uint32_t value){memcpy(b.data()+at,&value,4);}
static std::vector<uint8_t> fixture(){
    std::vector<uint8_t> b(40);memcpy(b.data(),"WDBC",4);word(b,4,2);word(b,8,2);word(b,12,8);word(b,16,4);
    word(b,20,17);word(b,24,1);word(b,28,18);word(b,32,2);b[37]='x';return b;
}
static void rejected(const std::function<void()>& fn){bool failed=false;try{fn();}catch(const std::runtime_error&){failed=true;}check(failed);}
int main(){
    DbcTable d(fixture());check(d.size()==2&&d.row(18)==1);check(d.text(0,1)=="x"&&d.text(1,1).empty());
    rejected([&]{d.row(19);});rejected([&]{d.value(2,1);});rejected([&]{d.value(1,2);});
    auto b=fixture();b.pop_back();rejected([&]{DbcTable bad(b);});
    b=fixture();word(b,4,UINT32_MAX);rejected([&]{DbcTable bad(b);});
    b=fixture();word(b,12,4);rejected([&]{DbcTable bad(b);});
    b=fixture();word(b,28,17);rejected([&]{DbcTable bad(b);});
    DbcTable composite(b,false);check(composite.size()==2&&composite.columns()==2&&composite.value(1,0)==17);rejected([&]{composite.row(17);});
    b=fixture();word(b,24,4);rejected([&]{DbcTable bad(b);bad.text(0,1);});
    b=fixture();b.back()='y';rejected([&]{DbcTable bad(b);});
    std::vector<uint8_t> wdt(12+12+8+32768);word(wdt,0,0x4d564552);word(wdt,4,4);word(wdt,8,18);
    word(wdt,12,0x4d504844);word(wdt,16,4);word(wdt,24,0x4d41494e);word(wdt,28,32768);word(wdt,32+(48*64+32)*8,1);
    auto tiles=worldTiles(wdt);check(tiles.size()==1&&tiles[0].first==32&&tiles[0].second==48);
    auto broken=wdt;broken.pop_back();rejected([&]{worldTiles(broken);});
    broken=wdt;word(broken,8,17);rejected([&]{worldTiles(broken);});
    broken=wdt;word(broken,28,32760);rejected([&]{worldTiles(broken);});
    broken=wdt;broken.push_back(0);rejected([&]{worldTiles(broken);});
    broken=wdt;word(broken,4,UINT32_MAX);rejected([&]{worldTiles(broken);});
    auto global=std::vector<uint8_t>(wdt.begin(),wdt.begin()+24);word(global,20,1);
    auto append=[&](uint32_t id,const std::vector<uint8_t>& bytes){auto at=global.size();global.resize(at+8+bytes.size());word(global,(unsigned)at,id);word(global,(unsigned)at+4,(uint32_t)bytes.size());memcpy(global.data()+at+8,bytes.data(),bytes.size());};
    const char path[]="World\\Wmo\\Dungeon.wmo";std::vector<uint8_t> name(path,path+sizeof path);
    append(0x4d574d4f,name);std::vector<uint8_t> modf(64);word(modf,4,123);append(0x4d4f4446,modf);
    auto layout=worldLayout(global);check(layout.global()&&layout.root==path&&layout.uniqueId==123&&layout.tiles.empty());
    for(size_t size=0;size<global.size();size++){broken.assign(global.begin(),global.begin()+size);rejected([&]{worldLayout(broken);});}
    broken=global;broken[24+8+name.size()-1]='x';rejected([&]{worldLayout(broken);});
    broken=global;broken[32]='\\';rejected([&]{worldLayout(broken);});
    broken=global;broken[32]='.';broken[33]='.';rejected([&]{worldLayout(broken);});
    broken=global;word(broken,(unsigned)broken.size()-64,1);rejected([&]{worldLayout(broken);});
    broken=global;word(broken,(unsigned)broken.size()-64+8,0x7fc00000);rejected([&]{worldLayout(broken);});
    broken=global;word(broken,20,0);rejected([&]{worldLayout(broken);});
    append(0x4d574d4f,name);rejected([&]{worldLayout(global);});
    printf("Vanilla DBC/WDT boundary checks: %u passed\n",checks);return 0;
}
