#pragma once
// Independent 256-square host composition -> reduced-pixel reference. The native
// path reads prepared layers and blends at 128 square; compare actual pixels.
static void lookReferences(Archives& a,HumanoidTables& tables,const fs::path& directory,unsigned race,unsigned sex){
    char name[32];snprintf(name,sizeof name,"L%02X%02X.WXL",race,sex);WxLooks catalog{};
    if(!wx_looks_open(&catalog,(directory/name).string().c_str()))throw std::runtime_error("Missing reference appearance catalogue");
    std::set<std::array<uint32_t,7>> samples;
    for(unsigned i=0;i<catalog.header.count;i++){
        const auto& row=catalog.rows[i];if(row.kind==4)continue;
        std::array<uint32_t,7> v={race,sex,0,0,0,0,0};
        if(row.kind==0||row.kind==1){v[2]=row.color;if(row.kind==1)v[3]=row.variation;}
        if(row.kind==2){v[6]=row.variation;v[5]=row.color;}
        if(row.kind==3){v[4]=row.variation;v[5]=row.color;}
        if(row.kind==5)v[4]=row.variation;
        if(row.kind==6)v[6]=row.variation;
        if(!wx_looks_find(&catalog,1,v[3],v[2]))for(unsigned j=0;j<catalog.header.count;j++){
            const auto& q=catalog.rows[j];if(q.kind==1&&q.color==v[2]){v[3]=q.variation;break;}}
        if(!wx_looks_find(&catalog,3,v[4],v[5]))for(unsigned j=0;j<catalog.header.count;j++){
            const auto& q=catalog.rows[j];if(q.kind==3&&((row.kind==2&&q.color==v[5])||(row.kind==5&&q.variation==v[4]))){v[4]=q.variation;v[5]=q.color;break;}}
        if(race!=6&&(!sex||race==4||race==5)&&!wx_looks_find(&catalog,2,v[6],v[5]))for(unsigned j=0;j<catalog.header.count;j++){
            const auto& q=catalog.rows[j];if(q.kind==2&&q.color==v[5]){v[6]=q.variation;break;}}
        if(!wx_looks_valid(&catalog,v.data())){
            // Some shipped facial-color rows have no matching hair-color row
            // (Night Elf 8/9), so no server-valid player can use them. Preserve
            // them in the catalogue; the independent DBC checker audits this.
            std::cout<<"Unreachable appearance row: "<<unsigned(row.kind)<<'/'<<unsigned(row.variation)<<'/'<<unsigned(row.color)<<'\n';continue;
        }samples.insert(v);
    }
    HumanoidLook identity;identity.race=race;identity.sex=sex;auto original=M2Loader::load(a.read(playerModelPath(identity)));
    bool hasHair=false,hasExtra=false;for(const auto& t:original.textures){hasHair|=t.type==6;hasExtra|=t.type==8;}
    auto hash=[](const BLPImage& im){uint32_t result=2166136261u;
        for(unsigned y=0;y<128;y++)for(unsigned x=0;x<128;x++){
            auto at=((uint64_t(2*y+1)*im.height/256)*im.width+uint64_t(2*x+1)*im.width/256)*4;
            const uint8_t* p=im.data.data()+at;for(unsigned c:{2u,1u,0u,3u})result=(result^p[c])*16777619u;
        }return result;
    };
    snprintf(name,sizeof name,"R%02X%02X.WXV",race,sex);std::ofstream out(directory/name,std::ios::binary|std::ios::trunc);
    uint32_t count=(uint32_t)samples.size(),stride=14*4;out.write("WXLR",4);out.write((char*)&count,4);out.write((char*)&stride,4);
    for(const auto& v:samples){HumanoidLook look;look.race=v[0];look.sex=v[1];look.skin=v[2];look.face=v[3];look.style=v[4];look.color=v[5];look.facial=v[6];
        auto model=original;assemblePlayer(a,tables,model,look,true);
        uint32_t record[14];std::copy(v.begin(),v.end(),record);record[7]=hash(images.at("__player_body__"));
        record[8]=hasHair?hash(images.at("__player_hair__")):0;record[9]=hasExtra?hash(images.at("__player_extra__")):0;
        const auto* hg=wx_looks_find(&catalog,5,v[4],0);record[10]=hg?hg->geoset[0]:1;
        std::copy_n(wx_looks_find(&catalog,6,v[6],0)->geoset,3,record+11);
        out.write((char*)record,sizeof record);
    }
    out.close();wx_looks_close(&catalog);if(!out)throw std::runtime_error("Appearance reference write failed");
    std::cout<<"Independent full-atlas references "<<race<<'/'<<sex<<": "<<count<<" appearances\n";
}
