#include "dbc_table.hpp"
#include "wx_outfit.h"
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
#include "outfit_catalog.hpp"
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x))throw std::runtime_error(#x);}while(0)
struct Source {std::vector<uint8_t> bytes;std::vector<uint8_t> read(const char* path){CHECK(std::string(path)=="DBFilesClient\\CharStartOutfit.dbc");return bytes;}};
static void word(std::vector<uint8_t>& bytes,unsigned at,uint32_t value){memcpy(bytes.data()+at,&value,4);}
static Source fixture(){Source s;s.bytes.resize(20+152+1);memcpy(s.bytes.data(),"WDBC",4);
    word(s.bytes,4,1);word(s.bytes,8,41);word(s.bytes,12,152);word(s.bytes,16,1);
    word(s.bytes,20,100);word(s.bytes,24,257); // Human male Warrior, packed byte schema.
    word(s.bytes,20+14*4,9891);word(s.bytes,20+26*4,4); // Shirt.
    word(s.bytes,20+15*4,9892);word(s.bytes,20+27*4,7); // Trousers.
    return s;
}
static void rejected(Source s){unsigned caught=0;try{startOutfits(s);}catch(const std::runtime_error&){caught=1;}CHECK(caught);}
static WxOutfits catalog;
static void invalid(WxOutfitHeader h,WxOutfit row,unsigned tail=0){
    std::ofstream file("outfit-fixture.wxo",std::ios::binary|std::ios::trunc);file.write((char*)&h,sizeof h);file.write((char*)&row,sizeof row);if(tail)file.put(0);file.close();
    CHECK(!wx_outfits_open(&catalog,"outfit-fixture.wxo")&&!catalog.ready&&!catalog.count);
}
int main(){
    auto source=fixture();auto rows=startOutfits(source);CHECK(rows.size()==1&&rows[0].key==257);
    CHECK(rows[0].display[3]==9891&&rows[0].type[3]==4&&rows[0].display[6]==9892&&rows[0].type[6]==7);
    writeOutfits(source,"outfit-fixture.wxo");CHECK(wx_outfits_open(&catalog,"outfit-fixture.wxo"));
    CHECK(wx_outfit_find(&catalog,1,1,0)&&!wx_outfit_find(&catalog,1,1,1)&&!wx_outfit_find(&catalog,9,1,0));
    source.bytes.pop_back();rejected(source);source=fixture();word(source.bytes,8,40);rejected(source);
    source=fixture();word(source.bytes,12,164);rejected(source);source=fixture();word(source.bytes,24,0x020101);rejected(source);
    source=fixture();word(source.bytes,20+27*4,4);rejected(source);
    auto row=rows[0];WxOutfitHeader h{{'W','X','O','F'},1,1,sizeof(WxOutfit)};
    invalid(h,row,1);h.version=2;invalid(h,row);h.version=1;h.count=97;invalid(h,row);h.count=2;invalid(h,row);h.count=1;
    h.entry_size--;invalid(h,row);h.entry_size++;row.type[19]=1;invalid(h,row);row=rows[0];row.type[3]=7;invalid(h,row);
    row=rows[0];row.display[3]=0;invalid(h,row);row=rows[0];row.key=0;invalid(h,row);
    row=rows[0];h.count=2;std::ofstream duplicate("outfit-fixture.wxo",std::ios::binary|std::ios::trunc);
    duplicate.write((char*)&h,sizeof h);duplicate.write((char*)&row,sizeof row);duplicate.write((char*)&row,sizeof row);duplicate.close();
    CHECK(!wx_outfits_open(&catalog,"outfit-fixture.wxo"));CHECK(!wx_outfit_find(&catalog,1,1,0));
    fs::remove("outfit-fixture.wxo");std::cout<<"Starter outfit schema, lookup and malformed input: "<<checks<<" checks pass\n";
}
