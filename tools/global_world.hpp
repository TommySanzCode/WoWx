// Uses the same pinned WoWee WDT/WMO/M2/BLP adapters as ordinary tile buildings.
static std::string worldJson(const std::string& text){
    std::string value="\"";for(unsigned char c:text){if(c=='\\'||c=='\"')value+='\\';
        if(c<32)throw std::runtime_error("Control character in world metadata");value+=c;}return value+'\"';
}
static unsigned worldNumber(const char* text,unsigned limit){
    if(!text||!*text||strspn(text,"0123456789")!=strlen(text))throw std::runtime_error("Invalid map parameter");
    auto value=std::stoul(text);if(value>limit)throw std::runtime_error("Map parameter out of range");return(unsigned)value;
}
static void globalWorld(Archives& a,const fs::path& target,unsigned mapId,const std::string& directory){
    DbcTable maps(a.read("DBFilesClient\\Map.dbc"));
    if(maps.text(maps.row(mapId),1)!=directory)throw std::runtime_error("Map ID/directory mismatch");
    auto bytes=a.read("World\\Maps\\"+directory+"\\"+directory+".wdt");
    auto layout=worldLayout(bytes);if(!layout.global())throw std::runtime_error("Selected map uses terrain; use --tile");
    auto info=parseWDT(bytes);
    if(!info.isWMOOnly()||info.rootWMOPath!=layout.root)throw std::runtime_error("WDT parser identity mismatch");
    ADTTerrain::WMOPlacement placement{};placement.uniqueId=layout.uniqueId;placement.flags=info.flags;placement.doodadSet=info.doodadSet;
    std::copy_n(info.position,3,placement.position);std::copy_n(info.rotation,3,placement.rotation);
    std::vector<Part> parts;WorldEnvironment environment;WorldModelCoverage coverage;wmo(a,parts,layout.root,placement,&coverage,&environment);
    // This is an inspection point on prepared horizontal collision geometry.
    // Actual gameplay positions always remain authoritative server coordinates.
    std::array<float,3> spawn{};float best=0;
    for(const auto& p:parts)if(p.e.kind==WX_KIND_COLLISION)for(size_t i=0;i+2<p.i.size();i+=3){
        for(unsigned j=0;j<3;j++)if(p.i[i+j]>=p.v.size())throw std::runtime_error("Invalid global WMO collision index");
        glm::vec3 a(p.v[p.i[i]].p[0],p.v[p.i[i]].p[1],p.v[p.i[i]].p[2]);
        glm::vec3 b(p.v[p.i[i+1]].p[0],p.v[p.i[i+1]].p[1],p.v[p.i[i+1]].p[2]);
        glm::vec3 c(p.v[p.i[i+2]].p[0],p.v[p.i[i+2]].p[1],p.v[p.i[i+2]].p[2]);
        auto normal=glm::cross(b-a,c-a);float area=glm::length(normal);
        if(area>best&&fabsf(normal.z)>.8f*area){auto center=(a+b+c)/3.f;for(unsigned k=0;k<3;k++)spawn[k]=center[k];best=area;}
    }
    if(!best)throw std::runtime_error("Global WMO has no horizontal collision surface");
    writePack(target,parts,false,spawn,&environment);
    std::ofstream report(target.string()+".coverage.json",std::ios::binary|std::ios::trunc);
    report<<"{\"version\":1,\"map\":"<<mapId<<",\"directory\":"<<worldJson(directory)<<",\"kind\":\"world_model\",\"root\":"<<worldJson(layout.root)
          <<",\"inspection_spawn\":["<<spawn[0]<<','<<spawn[1]<<','<<spawn[2]<<"],\"groups\":"<<coverage.groups<<",\"doodads\":"<<coverage.doodads
          <<",\"environment\":{\"records\":"<<environment.items.size()<<",\"bytes\":"<<environment.encode().size()<<"},\"parts\":"<<parts.size()<<",\"unsupported\":{\"liquid_depth_flow_groups\":"<<coverage.liquidGroups<<",\"authored_lights\":"<<coverage.lights
          <<",\"portals\":"<<coverage.portals<<",\"authored_fogs\":"<<coverage.fogs<<",\"complex_materials\":"<<coverage.complexMaterials
          <<"},\"effect_only_doodads\":[";
    unsigned effects=0;for(const auto& name:coverage.effectOnly){if(effects++)report<<',';report<<worldJson(name);}
    report<<"],\"unsupported_uv_rotation_scale\":[";unsigned unsupportedUV=0;for(const auto& name:coverage.extraUV){if(unsupportedUV++)report<<',';report<<worldJson(name);}
    report<<"],\"material_motion\":{";unsigned motions=0,dynamic=0,uv=0;
    for(const auto& p:parts)if(p.e.flags&WX_MATERIAL_MOTION){motions++;bool animated=false;
        for(const auto& t:p.materialMotion.tracks)if(t.count>1)animated=true;dynamic+=animated;uv+=p.materialMotion.tracks[WX_MOTION_UV].count>0;}
    report<<"\"blocks\":"<<motions<<",\"dynamic\":"<<dynamic<<",\"uv_translation\":"<<uv<<",\"resident_bytes_per_block\":"<<sizeof(WxMaterialMotion)<<"},\"vertex_lighting\":{";
    unsigned colored=0,baked=0,colorVertices=0;for(const auto& p:parts){colored+=!!(p.e.flags&WX_VERTEX_COLOR);baked+=!!(p.e.flags&WX_BAKED_LIGHT);colorVertices+=(unsigned)p.vertexColors.size();}
    report<<"\"colored_batches\":"<<colored<<",\"baked_batches\":"<<baked<<",\"vertices\":"<<colorVertices<<",\"bytes\":"<<colorVertices*4u<<"},\"material_batches\":[";unsigned modes[7]={0},unlit=0,unfogged=0,noDepthTest=0,noDepthWrite=0;
    for(const auto& p:parts)if(p.e.kind!=WX_KIND_COLLISION){modes[wx_material_blend(p.e.flags)]++;unlit+=!!(p.e.flags&WX_UNLIT);unfogged+=!!(p.e.flags&WX_UNFOGGED);noDepthTest+=!!(p.e.flags&WX_NO_DEPTH_TEST);noDepthWrite+=!!(p.e.flags&WX_NO_DEPTH_WRITE);}
    for(unsigned i=0;i<7;i++){if(i)report<<',';report<<modes[i];}
    unsigned liquidBatches=0,liquidTriangles=0;std::set<std::string> liquidTextures;
    for(const auto& part:parts)if(part.e.flags&WX_LIQUID){liquidBatches++;liquidTriangles+=(unsigned)part.i.size()/3;liquidTextures.insert(part.textureName);}
    report<<"],\"material_flags\":{\"unlit\":"<<unlit<<",\"unfogged\":"<<unfogged<<",\"no_depth_test\":"<<noDepthTest<<",\"no_depth_write\":"<<noDepthWrite
          <<"},\"limitations\":[\"No portal visibility, local lights/fog, terrain liquids or skeletal/effect-only doodad animation\",\"MOCV RGB follows pinned WoWee indoor ambient/outdoor tint policy, quantized to RGBA8; original client alpha fixup and exact visual parity remain unverified\",\"WMO liquid grids and original 30-frame textures rendered; depth opacity, authored flow UV, waves, reflection/refraction and exact original appearance remain unverified; no swimming. Single texture only; complex WMO shaders and UV rotation/scale remain unsupported\",\"Material motion uses idle sequence, exact step/linear keys and shared local/global clocks; actor clip material preparation remains separate\",\"Blended batches sorted within each scene draw; intersecting geometry and cross-actor ordering remain open\",\"Face culling remains disabled\",\"Preparation is not gameplay or full visual acceptance\"],\"liquids\":{\"groups\":"<<coverage.liquidGroups<<",\"batches\":"<<liquidBatches<<",\"triangles\":"<<liquidTriangles<<",\"texture_sequences\":"<<liquidTextures.size()<<",\"frames\":30,\"size\":64,\"period_ms\":1250},\"dependencies\":[";
    unsigned n=0;for(const auto& [name,size]:a.members){if(n++)report<<',';report<<"{\"path\":"<<worldJson(name)<<",\"bytes\":"<<size<<'}';}
    report<<"]}\n";report.close();if(!report)throw std::runtime_error("World dependency report write failed");
}
