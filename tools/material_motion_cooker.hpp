#pragma once
#include "wx_material_motion.h"
#include "wx_backdrop.h"
#include "vanilla_scene.hpp"
static void motionTrack(WxMaterialMotion& out,unsigned which,const M2AnimationTrack& t,const M2Model& model,unsigned sequence){
    if(t.globalSequence < -1||t.interpolationType>1)throw std::runtime_error("Unsupported material track interpolation");
    unsigned selected=sequence,period=sequence<model.sequences.size()?model.sequences[sequence].duration:0;
    if(t.globalSequence>=0){if(unsigned(t.globalSequence)>=model.globalSequenceDurations.size())throw std::runtime_error("Invalid material global sequence");selected=0;period=model.globalSequenceDurations[t.globalSequence];}
    if(selected>=t.sequences.size())return;
    const auto& keys=t.sequences[selected];bool vec=which==WX_MOTION_RGB||which==WX_MOTION_UV;
    unsigned count=unsigned(vec?keys.vec3Values.size():keys.floatValues.size());
    if(count!=keys.timestamps.size())throw std::runtime_error("Material key/time mismatch");if(!count)return;
    std::vector<WxMotionKey> packed;
    for(unsigned i=0;i<count;i++){
        WxMotionKey k{};k.time_ms=keys.timestamps[i];if(vec)for(unsigned j=0;j<3;j++)k.value[j]=keys.vec3Values[i][j];else k.value[0]=keys.floatValues[i];
        for(float v:k.value)if(!std::isfinite(v)||fabsf(v)>65536)throw std::runtime_error("Nonfinite/out-of-bounds material value");
        if(i&&k.time_ms<=packed.back().time_ms)throw std::runtime_error("Unordered material times");packed.push_back(k);
    }
    bool constant=true;for(const auto& k:packed)if(memcmp(k.value,packed[0].value,16)){constant=false;break;}
    if(!period||constant){
        packed.resize(1);packed[0].time_ms=0;period=1;
        if(which==WX_MOTION_RGB){for(unsigned i=0;i<3;i++)out.color[i]=wx_motion_unit(packed[0].value[i]);return;}
        if(which==WX_MOTION_ALPHA){out.color[3]=wx_motion_unit(packed[0].value[0]);return;}
        if(which==WX_MOTION_WEIGHT){out.weight=wx_motion_unit(packed[0].value[0]);return;}
        if(packed[0].value[0]==0&&packed[0].value[1]==0)return;
    }
    if(period>86400000||out.key_count+packed.size()>WX_MOTION_KEYS)throw std::runtime_error("Material motion exceeds bounded key/period limit");
    auto& dst=out.tracks[which];dst={out.key_count,unsigned(packed.size()),period,unsigned(t.interpolationType)|(t.globalSequence>=0?2u:0u)};
    for(const auto& k:packed)out.keys[out.key_count++]=k;
}
static WxMaterialMotion materialMotion(const M2Model& model,const VanillaScene& vanilla,const M2Batch& b,unsigned sequence){
    WxMaterialMotion m{};m.magic=WX_MOTION_MAGIC;for(float& c:m.color)c=1;m.weight=1;
    auto [colors,offset]=vanilla.colors();if(colors!=model.colorRGB.size())throw std::runtime_error("Material RGB parser disagreement");
    if(b.colorIndex<colors){
        for(unsigned k=0;k<3;k++)m.color[k]=std::clamp(model.colorRGB[b.colorIndex][k],0.f,1.f);
        if(b.colorIndex<model.colorAlphas.size())m.color[3]=model.colorAlphas[b.colorIndex];
        motionTrack(m,WX_MOTION_RGB,vanilla.track(offset+b.colorIndex*56,1),model,sequence);
        motionTrack(m,WX_MOTION_ALPHA,vanilla.track(offset+b.colorIndex*56+28,4),model,sequence);
    }else if(b.colorIndex!=65535)throw std::runtime_error("Invalid material color binding");
    if(b.transparencyIndex<model.textureWeightLookup.size()){
        unsigned index=model.textureWeightLookup[b.transparencyIndex];
        if(index!=65535){auto [count,at]=vanilla.table(0x64,28,4096);if(index>=count)throw std::runtime_error("Invalid material weight binding");
            if(index<model.textureWeights.size())m.weight=model.textureWeights[index];
            motionTrack(m,WX_MOTION_WEIGHT,vanilla.track(at+index*28,4),model,sequence);}
    }
    // Vanilla can retain a lookup slot while the model has no UV-animation
    // table. The pinned renderer likewise gates lookup evaluation on a present
    // texture animation; an inert slot is not a missing required transform.
    if(!model.textureTransforms.empty()&&b.textureAnimIndex!=65535&&b.textureAnimIndex<model.textureTransformLookup.size()){
        unsigned index=model.textureTransformLookup[b.textureAnimIndex];
        if(index!=65535){if(index>=model.textureTransforms.size())throw std::runtime_error("Invalid material UV binding");
            motionTrack(m,WX_MOTION_UV,model.textureTransforms[index].translation,model,sequence);}
    }
    if(!wx_motion_valid(&m))throw std::runtime_error("Invalid exported material motion");return m;
}
static bool materialMotionNeeded(const WxMaterialMotion& m){
    for(unsigned k=0;k<4;k++)if(m.color[k]!=1)return true;
    return m.weight!=1||m.key_count;
}
