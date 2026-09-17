#pragma once
// One body/skeleton profile plus independent item components. This does not
// enumerate outfit combinations. Additional displays use the same preparation.
static void prepareAvatar(Archives& a,HumanoidTables& tables,const HumanoidLook& profile,
        const fs::path& output,const char* catalog=nullptr) {
    HumanoidLook bare=profile;bare.equipment={};bare.inventoryType={};
    auto path=playerModelPath(bare);auto model=M2Loader::load(a.read(path));
    assemblePlayer(a,tables,model,bare,true);
    for(const auto& b:model.batches)if(b.submeshId>=4096)throw std::runtime_error("Body geoset exceeds avatar component range");
    struct Requested {unsigned display,slot,type;};std::vector<Requested> requested;
    auto add=[&](unsigned display,unsigned slot,unsigned type){
        if(!display||slot>=19||!type||type>28)throw std::runtime_error("Invalid avatar item catalog entry");
        for(auto r:requested)if(r.display==display&&r.slot==slot){if(r.type!=type)throw std::runtime_error("Conflicting avatar item type");return;}
        if(requested.size()==WX_AVATAR_ITEMS)throw std::runtime_error("Avatar item catalog limit exceeded");
        requested.push_back({display,slot,type});
    };
    for(unsigned i=0;i<19;i++)if(profile.equipment[i])add(profile.equipment[i],i,profile.inventoryType[i]);
    // One reusable component catalogue covers every starter class for this look.
    for(const auto& outfit:startOutfits(a))if((outfit.key&255)==bare.race&&((outfit.key>>16)&255)==bare.sex)
        for(unsigned i=0;i<19;i++)if(outfit.display[i])add(outfit.display[i],i,outfit.type[i]);
    if(catalog){std::ifstream in(catalog);if(!in)throw std::runtime_error("Missing avatar item catalog");std::string line;
        while(std::getline(in,line)){line.resize(line.find('#')==std::string::npos?line.size():line.find('#'));std::istringstream row(line);
            row>>std::ws;if(row.eof())continue;unsigned display,slot,type;std::string extra;
            if(!(row>>display>>slot>>type)||(row>>extra))throw std::runtime_error("Malformed avatar item catalog");add(display,slot,type);}}
    WxAvatarHeader header{};memcpy(header.magic,"WXAV",4);header.version=2;header.item_size=sizeof(WxAvatarItem);WxAvatarBindings bindings{};
    unsigned look[]={bare.race,bare.sex,bare.skin,bare.face,bare.style,bare.color,bare.facial};std::copy_n(look,7,header.look);
    header.scalp=1;
    for(unsigned i=0;i<tables.hair.size();i++)if(tables.hair.value(i,1)==bare.race&&tables.hair.value(i,2)==bare.sex&&tables.hair.value(i,3)==bare.style){header.scalp=tables.hair.value(i,4);if(!header.scalp)header.scalp=1;break;}
    for(unsigned i=0;i<tables.facial.size();i++)if(tables.facial.value(i,0)==bare.race&&tables.facial.value(i,1)==bare.sex&&tables.facial.value(i,2)==bare.facial){
        header.facial[0]=tables.facial.value(i,6);header.facial[1]=tables.facial.value(i,8);header.facial[2]=tables.facial.value(i,7);break;}
    std::vector<WxAvatarItem> items(requested.size());
    std::vector<std::array<std::vector<uint8_t>,8>> overlays(requested.size());
    std::vector<std::vector<uint32_t>> capes(requested.size());
    for(unsigned i=0;i<requested.size();i++){
        auto request=requested[i];auto& item=items[i];item.display=request.display;item.slot=request.slot;auto row=tables.items.row(item.display);
        for(unsigned j=0;j<3;j++){item.geoset[j]=tables.items.value(row,6+j);if(item.geoset[j]>98)throw std::runtime_error("Invalid equipment geoset variant");}
        if(item.slot==0){auto mask=tables.items.value(row,12+bare.sex);if(mask){auto r=tables.helmVisibility.row(mask);for(unsigned j=0;j<5;j++)item.hide[j]=tables.helmVisibility.value(r,1+j);}}
        for(unsigned region=0;region<8;region++){
            auto name=tables.items.text(row,14+region);if(name.empty())continue;
            std::string base=std::string("Item\\TextureComponents\\")+wowee::pipeline::itemComponentDir(region)+"\\"+withoutExtension(name),chosen;
            for(const auto& suffix:{bare.sex?"_F.blp":"_M.blp","_U.blp",".blp"})if(!a.read(base+suffix,false).empty()){chosen=base+suffix;break;}
            if(chosen.empty())throw std::runtime_error("Missing avatar clothing: "+base);
            const auto& im=texture(a,chosen);auto r=wx_atlas::regions[region];auto& pixels=overlays[i][region];pixels.resize(r.w*r.h);
            for(unsigned y=0;y<r.h/2;y++)for(unsigned x=0;x<r.w/2;x++){
                auto src=(((2*y+1)*im.height/r.h)*im.width+(2*x+1)*im.width/r.w)*4;
                std::copy_n(im.data.data()+src,4,pixels.data()+(y*(r.w/2)+x)*4);
            }
        }
        auto attach=[&](unsigned side,unsigned point,const std::string& directory,bool racial){
            auto name=tables.items.text(row,1+side);if(name.empty())return;
            auto suffix=racial?wowee::core::raceGenderSuffix(bare.race,bare.sex):std::string();
            size_t first=model.batches.size();attachEquipment(a,model,racialAsset(a,directory,name,suffix,".m2"),
                racialAsset(a,directory,tables.items.text(row,3+side),suffix,".blp"),point);
            item.component[side]=4096+2*i+side;for(size_t k=first;k<model.batches.size();k++)model.batches[k].submeshId=item.component[side];
        };
        if(item.slot==0)attach(0,11,"Item\\ObjectComponents\\Head\\",true);
        if(item.slot==2){attach(0,5,"Item\\ObjectComponents\\Shoulder\\",true);attach(1,6,"Item\\ObjectComponents\\Shoulder\\",true);}
        if(item.slot==15||item.slot==16){bool shield=request.type==14;
            attach(0,shield?0:item.slot==16?2:1,shield?"Item\\ObjectComponents\\Shield\\":"Item\\ObjectComponents\\Weapon\\",false);
            if(!item.component[0])throw std::runtime_error("Missing held item model");}
        if(item.slot==14){std::string chosen;
            for(unsigned col:{bare.sex?4u:3u,bare.sex?3u:4u})for(const auto& candidate:wowee::pipeline::capeTextureCandidates(tables.items.text(row,col),bare.sex==1))
                if(chosen.empty()&&!a.read(candidate,false).empty())chosen=candidate;
            if(chosen.empty())throw std::runtime_error("Missing avatar cape");Part p;modelTexture(a,p,chosen);capes[i]=std::move(p.tex);}
    }
    std::vector<Part> parts;
    for(unsigned clip:{0u,4u,5u,6u,16u})animatedCreature(a,parts,path,{},1,1,clip,nullptr,0,nullptr,&model,true);
    auto metadata=output;metadata.replace_extension(".WXA");auto temporary=output;temporary+=".tmp";
    auto metaTemporary=metadata;metaTemporary+=".tmp";
    writePack(temporary,parts,true);header.pack_size=uint32_t(fs::file_size(temporary));
    for(const auto& p:parts)if(p.textureName=="__player_body__"){header.body_texture=p.e.texture_offset;break;}
    for(const auto& p:parts){if(p.textureName=="__player_hair__")bindings.hair_texture=p.e.texture_offset;if(p.textureName=="__player_extra__")bindings.extra_texture=p.e.texture_offset;}
    if(!header.body_texture)throw std::runtime_error("Avatar body has no atlas binding");
    std::ofstream out(metaTemporary,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot create avatar manifest");
    header.item_count=uint32_t(items.size());header.items_offset=sizeof header+sizeof bindings;
    out.write((char*)&header,sizeof header);out.write((char*)&bindings,sizeof bindings);out.write((char*)items.data(),items.size()*sizeof(WxAvatarItem));
    auto offset=[&](){auto pos=out.tellp();if(pos<0||pos>32*1024*1024)throw std::runtime_error("Avatar metadata exceeds limit");return uint32_t(pos);};
    header.base_offset=offset();const auto& body=images.at("__player_body__");
    for(unsigned y=0;y<128;y++)for(unsigned x=0;x<128;x++)out.write((const char*)body.data.data()+((2*y+1)*256+2*x+1)*4,4);
    for(unsigned i=0;i<items.size();i++){
        for(unsigned region=0;region<8;region++)if(!overlays[i][region].empty()){
            items[i].overlay[region]=offset();out.write((char*)overlays[i][region].data(),overlays[i][region].size());}
        if(!capes[i].empty()){items[i].cape=offset();out.write((char*)capes[i].data(),capes[i].size()*4);}
    }
    header.file_size=offset();out.seekp(0);out.write((char*)&header,sizeof header);out.write((char*)&bindings,sizeof bindings);out.write((char*)items.data(),items.size()*sizeof(WxAvatarItem));
    out.close();if(!out)throw std::runtime_error("Avatar metadata write failed");
    // Both files are complete before replacing the prepared pair. The launcher
    // hashes both, and runtime checks the companion pack size and every span.
    fs::rename(temporary,output);fs::rename(metaTemporary,metadata);
    char layerName[20];snprintf(layerName,sizeof layerName,"L%02X%02X.WXL",bare.race,bare.sex);
    prepareLooks(a,tables,bare.race,bare.sex,output.parent_path()/layerName);
    std::cout<<"Avatar components: "<<items.size()<<" items, "<<header.file_size<<" metadata bytes\n";
}
