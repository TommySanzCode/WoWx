#pragma once
#include <cstdint>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// Strict binary Vanilla WDBC reader for asset preparation. Layout field numbers
// come from WoWee's pinned classic/dbc_layouts.json, not newer client layouts.
class DbcTable {
    std::vector<uint8_t> bytes;
    uint32_t count=0,fields=0,strsize=0;
    size_t strings=0;
    std::map<uint32_t,uint32_t> ids;
    bool indexed;
public:
    explicit DbcTable(std::vector<uint8_t> input,bool indexIds=true):bytes(std::move(input)),indexed(indexIds) {
        if(bytes.size()<20||memcmp(bytes.data(),"WDBC",4))throw std::runtime_error("Invalid WDBC header");
        uint32_t header[4];memcpy(header,bytes.data()+4,16);
        count=header[0];fields=header[1];strsize=header[3];
        if(!fields||fields>256||count>1000000||header[2]!=fields*4)throw std::runtime_error("Invalid WDBC dimensions");
        strings=20+uint64_t(count)*header[2];
        if(strings>bytes.size()||strsize!=bytes.size()-strings||!strsize||bytes.back()!=0)throw std::runtime_error("Invalid WDBC strings");
        if(indexed)for(uint32_t i=0;i<count;i++)if(!ids.emplace(value(i,0),i).second)throw std::runtime_error("Duplicate WDBC ID");
    }
    uint32_t size() const {return count;}
    uint32_t columns() const {return fields;}
    uint32_t row(uint32_t id) const {auto i=ids.find(id);if(i==ids.end())throw std::runtime_error("Missing WDBC ID "+std::to_string(id));return i->second;}
    uint32_t value(uint32_t row,uint32_t field) const {
        if(row>=count||field>=fields)throw std::runtime_error("WDBC field out of bounds");
        uint32_t v;memcpy(&v,bytes.data()+20+(size_t(row)*fields+field)*4,4);return v;
    }
    float real(uint32_t row,uint32_t field) const {auto u=value(row,field);float v;memcpy(&v,&u,4);return v;}
    std::string text(uint32_t row,uint32_t field) const {
        auto offset=value(row,field);if(offset>=strsize)throw std::runtime_error("WDBC string offset out of bounds");
        const char* p=(const char*)bytes.data()+strings+offset;
        const char* end=(const char*)memchr(p,0,strsize-offset);
        if(!end)throw std::runtime_error("Unterminated WDBC string");return {p,end};
    }
};
