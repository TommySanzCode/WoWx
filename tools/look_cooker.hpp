#pragma once
#include "wx_looks.h"
// The 5875 CharSections layout and unavailable flag follow pinned vMaNGOS.
// Store layers once per race/sex; never enumerate complete appearance combinations.
static void prepareLooks(Archives& a,HumanoidTables& tables,unsigned race,unsigned sex,const fs::path& output){
    std::vector<WxLookRow> rows;std::vector<std::array<std::vector<uint8_t>,3>> payload;
    unsigned excluded=0,optionalMissing=0,unusedHair=0;
    HumanoidLook identity;identity.race=race;identity.sex=sex;auto sourceModel=M2Loader::load(a.read(playerModelPath(identity)));
    bool usesHair=false;for(const auto& tex:sourceModel.textures)if(tex.type==6)usesHair=true;
    auto resample=[&](const std::string& path,unsigned w,unsigned h){
        const auto& im=texture(a,path);std::vector<uint8_t> data(w*h*4);
        for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
            auto source=((uint64_t(2*y+1)*im.height/(2*h))*im.width+uint64_t(2*x+1)*im.width/(2*w))*4;
            std::copy_n(im.data.data()+source,4,data.data()+(y*w+x)*4);
        }return data;
    };
    auto region=[](std::string name){
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(name.find("facelower")!=std::string::npos||name.find("faciallower")!=std::string::npos||name.find("scalplower")!=std::string::npos)return 8u;
        if(name.find("faceupper")!=std::string::npos||name.find("facialupper")!=std::string::npos||name.find("scalpupper")!=std::string::npos)return 9u;
        if(name.find("pelvis")!=std::string::npos)return 5u;
        if(name.find("torso")!=std::string::npos)return 3u;
        throw std::runtime_error("Unrecognized appearance overlay: "+name);
    };
    for(unsigned i=0;i<tables.sections.size();i++){
        const auto& t=tables.sections;if(t.value(i,1)!=race||t.value(i,2)!=sex)continue;
        unsigned flags=t.value(i,9);if(flags&~1u)throw std::runtime_error("Unknown Vanilla section flags");
        if(flags&1){excluded++;continue;}
        unsigned kind=t.value(i,3),var=t.value(i,4),color=t.value(i,5);
        if(kind>4||var>255||color>255)throw std::runtime_error("Appearance row out of range");
        WxLookRow row{};row.kind=(uint8_t)kind;row.variation=(uint8_t)var;row.color=(uint8_t)color;
        std::array<std::vector<uint8_t>,3> data;
        for(unsigned j=0;j<3;j++){
            auto path=t.text(i,6+j);if(path.empty())continue;
            if(kind==3&&j==0&&!usesHair){unusedHair++;continue;}
            bool optional=kind==2||kind==4||(kind==3&&j);
            if(optional&&a.read(path,false).empty()){optionalMissing++;std::cout<<"Optional appearance overlay absent: "<<path<<'\n';continue;}
            if(kind==0&&j==0){row.region[j]=WX_LOOK_BASE;data[j]=resample(path,128,128);}
            else if((kind==0&&j==1)||(kind==3&&j==0)){
                Part p;modelTexture(a,p,path);row.region[j]=WX_LOOK_MIPS;data[j].resize(p.tex.size()*4);memcpy(data[j].data(),p.tex.data(),data[j].size());
            }else{
                if(kind==0)throw std::runtime_error("Unexpected third skin texture");
                row.region[j]=region(path);unsigned bytes=wx_looks_bytes(row.region[j]);
                data[j]=resample(path,64,bytes/(64*4));
            }
        }
        rows.push_back(row);payload.push_back(std::move(data));
    }
    for(unsigned i=0;i<tables.hair.size();i++)if(tables.hair.value(i,1)==race&&tables.hair.value(i,2)==sex){
        WxLookRow row{};row.kind=5;unsigned var=tables.hair.value(i,3),geo=tables.hair.value(i,4);
        if(var>255||geo>99)throw std::runtime_error("Hair geoset out of range");row.variation=(uint8_t)var;row.geoset[0]=geo?geo:1;
        rows.push_back(row);payload.push_back({});
    }
    for(unsigned i=0;i<tables.facial.size();i++)if(tables.facial.value(i,0)==race&&tables.facial.value(i,1)==sex){
        WxLookRow row{};row.kind=6;unsigned var=tables.facial.value(i,2);if(var>255)throw std::runtime_error("Facial style out of range");
        row.variation=(uint8_t)var;row.geoset[0]=tables.facial.value(i,6);row.geoset[1]=tables.facial.value(i,8);row.geoset[2]=tables.facial.value(i,7);
        for(auto geo:row.geoset)if(geo>99)throw std::runtime_error("Facial geoset out of range");
        rows.push_back(row);payload.push_back({});
    }
    if(rows.empty()||rows.size()>WX_LOOK_ROWS)throw std::runtime_error("Appearance index exceeds bound");
    fs::create_directories(output.parent_path());auto temporary=output;temporary+=".tmp";
    WxLookHeader header{{'W','X','L','K'},1,0,uint32_t(rows.size()),sizeof(WxLookRow),race,sex,uint32_t(sizeof(WxLookHeader)+rows.size()*sizeof(WxLookRow))};
    std::ofstream out(temporary,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot create appearance layers");
    out.write((char*)&header,sizeof header);out.write((char*)rows.data(),rows.size()*sizeof(WxLookRow));
    struct ByteLess {bool operator()(const std::vector<uint8_t>& a,const std::vector<uint8_t>& b)const{
        for(size_t i=0,n=std::min(a.size(),b.size());i<n;i++)if(a[i]!=b[i])return a[i]<b[i];return a.size()<b.size();}};
    std::map<std::vector<uint8_t>,uint32_t,ByteLess> shared;
    for(unsigned i=0;i<rows.size();i++)for(unsigned j=0;j<3;j++)if(!payload[i][j].empty()){
        auto& data=payload[i][j];auto found=shared.find(data);
        if(found!=shared.end())rows[i].offset[j]=found->second;
        else {auto at=out.tellp();if(at<0||at+std::streamoff(data.size())>64*1024*1024)throw std::runtime_error("Appearance layers exceed limit");
            rows[i].offset[j]=uint32_t(at);shared.emplace(data,uint32_t(at));out.write((char*)data.data(),data.size());}
    }
    header.file_size=uint32_t(out.tellp());out.seekp(0);out.write((char*)&header,sizeof header);out.write((char*)rows.data(),rows.size()*sizeof(WxLookRow));out.close();
    if(!out)throw std::runtime_error("Appearance write failed");
    WxLooks check{};if(!wx_looks_open(&check,temporary.string().c_str()))throw std::runtime_error("Appearance validation failed");
    uint32_t initial[]={race,sex,0,0,0,0,0};bool valid=wx_looks_valid(&check,initial);wx_looks_close(&check);
    if(!valid)throw std::runtime_error("Default appearance not available");fs::rename(temporary,output);
    std::cout<<"Appearance "<<race<<'/'<<sex<<": "<<rows.size()<<" rows, "<<shared.size()<<" unique layers, "<<header.file_size<<" bytes, "<<excluded<<" unavailable rows excluded, "<<optionalMissing<<" absent optional overlays, "<<unusedHair<<" unused hair references\n";
}
