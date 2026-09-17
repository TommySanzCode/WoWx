// Bounded glue scene export using the pinned WoWee M2/BLP parsers.
#include "wx_backdrop.h"
#include "vanilla_scene.hpp"
static void backdrop(Archives& a,const std::string& path,const fs::path& output){
    auto source=a.read(modelPathToM2(path));auto model=M2Loader::load(source);VanillaScene vanilla(source);
    if(!model.isValid()||model.cameras.empty()||model.sequences.empty()||model.bones.empty())throw std::runtime_error("Backdrop lacks authored geometry/camera/animation");
    if(!model.textureTransforms.empty())throw std::runtime_error("Backdrop UV animation requires an exporter extension");
    unsigned sequence=0;for(unsigned i=0;i<model.sequences.size();i++)if(model.sequences[i].id==0){sequence=i;break;}
    WxBackdropHeader h{};memcpy(h.magic,"WXB1",4);h.version=3;h.duration_ms=model.sequences[sequence].duration;
    WxBackdropAnchor anchor{};anchor.bone=WX_BACKDROP_NONE;
    for(const auto& at:model.attachments)if(at.id==0){anchor.present=1;anchor.bone=at.bone==65535?WX_BACKDROP_NONE:at.bone;
        for(unsigned j=0;j<3;j++)anchor.position[j]=at.position[j];break;}
    const auto& cam=model.cameras[0];for(unsigned j=0;j<3;j++){h.camera[j]=cam.positionBase[j];h.target[j]=cam.targetBase[j];}
    // Classic M2 camera FOV is not a direct vertical-radian projection value.
    // See open-realm M2_CameraView and WoW Model Viewer ModelCamera::setup:
    // https://github.com/corepunch/open-realm/blob/main/games/world-of-warcraft/renderer/m2/r_m2.c
    // https://wowmodelviewer.github.io/wowmodelviewer/ModelCamera_8cpp_source.html
    // The former uses 0.6 radians per stored unit; the latter ~34.5 degrees.
    h.fov=cam.fov*.6f;h.near_clip=cam.nearClip;h.far_clip=cam.farClip;
    h.omitted_particles=0;h.omitted_ribbons=model.ribbonEmitters.size();
    std::vector<WxBackdropTrack> tracks;std::vector<WxBackdropKey> keys;unsigned linearFallbacks=0;
    auto track=[&](const M2AnimationTrack& t,unsigned kind)->uint32_t{
        unsigned seq=sequence,period=h.duration_ms;
        if(t.globalSequence>=0){if(unsigned(t.globalSequence)>=model.globalSequenceDurations.size())throw std::runtime_error("Invalid global track");seq=0;period=model.globalSequenceDurations[t.globalSequence];}
        if(seq>=t.sequences.size())return WX_BACKDROP_NONE;
        const auto& k=t.sequences[seq];size_t count=kind==0?k.floatValues.size():kind==1?k.vec3Values.size():k.quatValues.size();
        if(k.timestamps.empty()||!count)return WX_BACKDROP_NONE;
        if(count!=k.timestamps.size())throw std::runtime_error("Invalid backdrop animation keys");
        bool stationary=!period;if(stationary){period=1;count=1;} // Pinned resolveTime samples zero-duration globals at t=0.
        WxBackdropTrack dst{uint32_t(keys.size()),uint32_t(count),period,t.interpolationType?1u:0u,kind};
        if(t.interpolationType>1)linearFallbacks++;
        for(unsigned i=0;i<count;i++){
            WxBackdropKey key{};key.time_ms=stationary?0:k.timestamps[i];
            if(kind==0)key.value[0]=k.floatValues[i];
            if(kind==1)for(unsigned c=0;c<3;c++)key.value[c]=k.vec3Values[i][c];
            if(kind==2){auto q=k.quatValues[i];float n=glm::dot(q,q);q=n>.000001f?glm::normalize(q):glm::quat(1,0,0,0);key.value[0]=q.x;key.value[1]=q.y;key.value[2]=q.z;key.value[3]=q.w;}
            keys.push_back(key);
        }
        unsigned id=tracks.size();tracks.push_back(dst);return id;
    };
    std::vector<WxBackdropBone> bones;
    unsigned billboards=0;
    for(const auto& b:model.bones){WxBackdropBone dst{};dst.parent=b.parentBone;dst.flags=b.flags;
        if(b.flags&0x78)billboards++;
        for(unsigned j=0;j<3;j++)dst.pivot[j]=b.pivot[j];
        dst.translation=track(b.translation,1);dst.rotation=track(b.rotation,2);dst.scale=track(b.scale,1);bones.push_back(dst);
    }
    if(billboards)throw std::runtime_error("Backdrop requires billboard bone support: "+std::to_string(billboards));
    std::vector<WxVertex> vertices;std::vector<WxSkinVertex> skin;
    for(const auto& v:model.vertices){vertices.push_back(vertex(v.position,v.normal,v.texCoords[0],glm::mat4(1)));WxSkinVertex weights{};memcpy(weights.bones,v.boneIndices,4);memcpy(weights.weights,v.boneWeights,4);skin.push_back(weights);}
    std::vector<WxBackdropBatch> batches;std::vector<Part> pixels;std::vector<WxBackdropTexture> textures;std::map<unsigned,unsigned> textureMap;unsigned gpu=0;
    auto texture=[&](unsigned ti)->unsigned{
        if(ti>=model.textures.size()||model.textures[ti].type!=0)throw std::runtime_error("Unsupported backdrop texture");
        if(!textureMap.count(ti)){
            Part p;modelTexture(a,p,model.textures[ti].filename);textureMap[ti]=pixels.size();
            WxBackdropTexture t{0,uint32_t(p.tex.size()*4),p.e.width,p.e.reserved[1],model.textures[ti].flags&3,gpu};
            gpu+=(t.size+127)&~127u;textures.push_back(t);pixels.push_back(std::move(p));
        }
        return textureMap.at(ti);
    };
    std::vector<uint32_t> batchColors;
    auto [colorCount,colorOffset]=vanilla.colors();
    if(colorCount!=model.colorRGB.size())throw std::runtime_error("Vanilla/pinned color table mismatch");
    for(unsigned i=0;i<colorCount;i++){
        auto rgb=vanilla.track(colorOffset+i*56,1);
        if(!rgb.sequences.empty()&&!rgb.sequences[0].vec3Values.empty()&&
           glm::length(glm::clamp(rgb.sequences[0].vec3Values[0],glm::vec3(0),glm::vec3(1))-model.colorRGB[i])>.00001f)
            throw std::runtime_error("Vanilla/pinned first material color mismatch");
    }
    for(const auto& b:model.batches){
        if(!b.indexCount||b.submeshLevel>0)continue;
        if(b.textureCount!=1||b.textureIndex>=model.textureLookup.size()||b.materialIndex>=model.materials.size())throw std::runtime_error("Unsupported backdrop material");
        unsigned ti=model.textureLookup[b.textureIndex];texture(ti);
        batchColors.push_back(b.colorIndex<colorCount?track(vanilla.track(colorOffset+b.colorIndex*56,1),1):WX_BACKDROP_NONE);
        const auto& mat=model.materials[b.materialIndex];WxBackdropBatch dst{b.indexStart,b.indexCount,textureMap.at(ti),mat.flags,mat.blendMode,WX_BACKDROP_NONE,WX_BACKDROP_NONE,{1,1,1,1}};
        if(b.colorIndex<model.colorRGB.size())for(unsigned k=0;k<3;k++)dst.color[k]=model.colorRGB[b.colorIndex][k];
        if(b.colorIndex<model.colorAlphaTracks.size())dst.alpha=track(model.colorAlphaTracks[b.colorIndex],0);
        else if(b.colorIndex<model.colorAlphas.size())dst.color[3]=model.colorAlphas[b.colorIndex];
        if(b.transparencyIndex<model.textureWeightLookup.size()){
            unsigned id=model.textureWeightLookup[b.transparencyIndex];
            if(id<model.textureWeightTracks.size())dst.weight=track(model.textureWeightTracks[id],0);
            else if(id<model.textureWeights.size())dst.color[3]*=model.textureWeights[id];
        }
        batches.push_back(dst);
    }
    WxBackdropEffects effects{};
    std::vector<WxBackdropEmitter> emitters;
    auto [emitterCount,emitterOffset]=vanilla.table(0x13c,504,WX_BACKDROP_EMITTERS);
    for(unsigned i=0;i<emitterCount;i++){
        auto e=vanilla.emitter(i);e.texture=texture(e.texture);
        for(unsigned j=0;j<10;j++)e.tracks[j]=track(vanilla.track(emitterOffset+i*504+0x34+j*28,0),0);
        e.tracks[WX_FX_ENABLED]=track(vanilla.track(emitterOffset+i*504+0x1dc,3),0);
        emitters.push_back(e);
    }
    std::vector<WxBackdropLight> lights;
    auto [lightCount,lightOffset]=vanilla.table(0x11c,212,WX_BACKDROP_LIGHTS);
    for(unsigned i=0;i<lightCount;i++){
        auto p=lightOffset+i*212;WxBackdropLight l{};l.type=vanilla.read<uint16_t>(p);
        auto bone=vanilla.read<int16_t>(p+2);l.bone=bone<0?WX_BACKDROP_NONE:unsigned(bone);
        for(unsigned j=0;j<3;j++)l.position[j]=vanilla.read<float>(p+4+j*4);
        uint32_t* ids=&l.ambient;
        for(unsigned j=0;j<7;j++){unsigned kind=(j==0||j==2)?1:j==6?3:0;ids[j]=track(vanilla.track(p+16+j*28,kind),kind==1?1:0);}
        lights.push_back(l);
    }
    std::vector<uint8_t> bytes(sizeof h);
    auto append=[&](const void* data,size_t size){while(bytes.size()&3)bytes.push_back(0);uint32_t off=bytes.size();auto p=static_cast<const uint8_t*>(data);if(size)bytes.insert(bytes.end(),p,p+size);return off;};
    h.vertices=vertices.size();h.vertex_offset=append(vertices.data(),vertices.size()*sizeof(WxVertex));
    h.skin_offset=append(skin.data(),skin.size()*sizeof(WxSkinVertex));
    h.indices=model.indices.size();h.index_offset=append(model.indices.data(),model.indices.size()*2);
    h.bones=bones.size();h.bone_offset=append(bones.data(),bones.size()*sizeof(WxBackdropBone));
    h.batches=batches.size();h.batch_offset=append(batches.data(),batches.size()*sizeof(WxBackdropBatch));
    h.tracks=tracks.size();h.track_offset=append(tracks.data(),tracks.size()*sizeof(WxBackdropTrack));
    h.keys=keys.size();h.key_offset=append(keys.data(),keys.size()*sizeof(WxBackdropKey));
    for(unsigned i=0;i<textures.size();i++)textures[i].offset=append(pixels[i].tex.data(),pixels[i].tex.size()*4);
    h.textures=textures.size();h.texture_offset=append(textures.data(),textures.size()*sizeof(WxBackdropTexture));
    effects.emitters=emitters.size();effects.emitter_offset=append(emitters.data(),emitters.size()*sizeof(WxBackdropEmitter));
    effects.lights=lights.size();effects.light_offset=append(lights.data(),lights.size()*sizeof(WxBackdropLight));
    effects.color_offset=append(batchColors.data(),batchColors.size()*4);effects.capacity=emitters.empty()?0:WX_BACKDROP_PARTICLES;
    h.reserved=append(&effects,sizeof effects);
    append(&anchor,sizeof anchor);
    h.file_size=bytes.size();memcpy(bytes.data(),&h,sizeof h);
    if(!wx_backdrop_validate(bytes.data(),bytes.size()))throw std::runtime_error("Backdrop validation line "+std::to_string(wx_backdrop_error(bytes.data(),bytes.size()))+"; resident "+std::to_string(h.file_size+gpu+h.vertices*sizeof(WxVertex)+wx_backdrop_effect_bytes(&h,&effects))+"; vertices "+std::to_string(h.vertices)+" bones "+std::to_string(h.bones)+" textures "+std::to_string(h.textures));
    fs::create_directories(output.parent_path());std::ofstream file(output,std::ios::binary|std::ios::trunc);file.write((const char*)bytes.data(),bytes.size());file.close();if(!file)throw std::runtime_error("Backdrop write failed");
    // Compare the native C evaluator to pinned WoWee sampling plus GLM transforms,
    // including times beyond the local sequence and independent global periods.
    WxBackdrop native{};if(!wx_backdrop_open(&native,output.string().c_str()))throw std::runtime_error("Backdrop readback failed");
    float error=0;
    for(unsigned time:{0u,17u,333u,1667u,13334u,39999u,40001u,79991u,123456u}){
        wx_backdrop_update(&native,time);if(!native.ready)throw std::runtime_error("Native backdrop animation rejected");
        std::vector<glm::mat4> pose(model.bones.size());namespace sampler=wowee::rendering::m2_track;
        for(unsigned i=0;i<model.bones.size();i++){
            const auto& b=model.bones[i];float local=float(time%h.duration_ms);
            auto tr=sampler::sampleVec3(b.translation,sequence,local,float(time),model.globalSequenceDurations,glm::vec3(0));
            auto rot=sampler::sampleQuat(b.rotation,sequence,local,float(time),model.globalSequenceDurations);
            auto scale=sampler::sampleVec3(b.scale,sequence,local,float(time),model.globalSequenceDurations,glm::vec3(1));
            auto m=glm::translate(glm::mat4(1),b.pivot+tr)*glm::mat4_cast(rot)*glm::scale(glm::mat4(1),scale)*glm::translate(glm::mat4(1),-b.pivot);
            pose[i]=b.parentBone<0?m:pose[b.parentBone]*m;
            for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)error=std::max(error,std::abs(pose[i][c][r]-native.matrices[i][r*4+c]));
        }
        for(unsigned i=0;i<model.vertices.size();i++){
            const auto& v=model.vertices[i];unsigned sum=0;for(unsigned k=0;k<4;k++)sum+=v.boneWeights[k];glm::vec3 p(0);
            for(unsigned k=0;k<4;k++)if(v.boneWeights[k])p+=glm::vec3(pose[v.boneIndices[k]]*glm::vec4(v.position,1))*(float(v.boneWeights[k])/sum);
            for(unsigned c=0;c<3;c++)error=std::max(error,std::abs(p[c]-native.vertices[i].p[c]));
        }
    }
    wx_backdrop_close(&native);if(error>.003f)throw std::runtime_error("Native/pinned animation mismatch: "+std::to_string(error));
    std::cout<<"Pinned WoWee/GLM vs native C animation: nine independent timestamps pass; maximum error "<<error<<"\n";
    std::cout<<"Backdrop "<<path<<": "<<h.vertices<<" vertices, "<<h.bones<<" bones, "<<h.batches<<" batches, "<<h.textures<<" textures, "<<h.tracks<<" independent tracks / "<<h.keys<<" keys\n";
    std::cout<<"File "<<h.file_size<<" bytes; resident CPU+GPU "<<h.file_size+gpu+h.vertices*sizeof(WxVertex)+wx_backdrop_effect_bytes(&h,&effects)<<" bytes\n";
    std::cout<<"Pending fidelity: "<<h.omitted_particles<<" particle emitters, "<<h.omitted_ribbons<<" ribbons; "<<linearFallbacks<<" spline tracks use pinned sampler linear fallback; camera tracks and advanced particle motion remain pending\n";
    std::cout<<effects.emitters<<" bounded emitters, "<<effects.lights<<" authored lights, "<<batchColors.size()<<" animated material colors\n";
}
