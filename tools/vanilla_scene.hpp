// Local additions to the pinned loader. Vanilla 256 has u16 blend/type fields,
// 28-byte flat tracks, and inline lifetime colors; later M2 layouts differ.
// Format reference: wow-alchemy-m2 chunks/particle_emitter.rs (schema only).
#pragma once
struct VanillaScene {
    const std::vector<uint8_t>& bytes;
    explicit VanillaScene(const std::vector<uint8_t>& b):bytes(b){
        range(0,1,0x144);
        if(memcmp(b.data(),"MD20",4)||read<uint32_t>(4)!=256)
            throw std::runtime_error("Scene effects currently require Vanilla M2 version 256");
    }
    void range(size_t off,size_t count,size_t stride)const{
        if(off>bytes.size()||count>(bytes.size()-off)/stride)
            throw std::runtime_error("Truncated Vanilla scene table");
    }
    template<class T>T read(size_t off)const{range(off,1,sizeof(T));T v;memcpy(&v,bytes.data()+off,sizeof v);return v;}
    std::pair<uint32_t,uint32_t> table(size_t off,size_t stride,unsigned cap)const{
        auto n=read<uint32_t>(off),p=read<uint32_t>(off+4);
        if(n>cap)throw std::runtime_error("Vanilla scene table exceeds limit");range(p,n,stride);return {n,p};
    }
    std::pair<uint32_t,uint32_t> colors()const{return table(0x54,56,128);}
    M2AnimationTrack track(size_t off,unsigned kind)const{
        range(off,1,28);M2AnimationTrack t;
        t.interpolationType=read<uint16_t>(off);t.globalSequence=read<int16_t>(off+2);
        auto nr=read<uint32_t>(off+4),pr=read<uint32_t>(off+8),nt=read<uint32_t>(off+12),pt=read<uint32_t>(off+16),nk=read<uint32_t>(off+20),pk=read<uint32_t>(off+24);
        unsigned stride=kind==1?12:kind==3?1:kind==4?2:4;
        if(nr>4096||nt>65536||nt!=nk||t.interpolationType>1||t.globalSequence < -1)
            throw std::runtime_error("Unsupported or invalid Vanilla scene track");
        range(pr,nr,8);range(pt,nt,4);range(pk,nk,stride);
        if(!nt)return t;
        t.sequences.resize(nr?nr:1);
        for(unsigned i=0;i<t.sequences.size();i++){
            unsigned start=nr?read<uint32_t>(pr+i*8):0,end=nr?read<uint32_t>(pr+i*8+4):nt;
            if(start>end||end>nt)throw std::runtime_error("Invalid Vanilla track range");
            if(start==end)continue;auto& dst=t.sequences[i];unsigned origin=read<uint32_t>(pt+start*4);
            for(unsigned j=start;j<end;j++){
                unsigned stamp=read<uint32_t>(pt+j*4);
                if(stamp<origin||(j>start&&stamp<=read<uint32_t>(pt+(j-1)*4)))throw std::runtime_error("Invalid Vanilla track times");
                dst.timestamps.push_back(stamp-origin);
                if(kind==1)dst.vec3Values.emplace_back(read<float>(pk+j*12),read<float>(pk+j*12+4),read<float>(pk+j*12+8));
                else dst.floatValues.push_back(kind==3?float(read<uint8_t>(pk+j)):kind==4?std::clamp(float(read<int16_t>(pk+j*2))/32767.f,0.f,1.f):read<float>(pk+j*4));
            }
        }
        return t;
    }
    WxBackdropEmitter emitter(unsigned index)const{
        auto [n,p]=table(0x13c,504,WX_BACKDROP_EMITTERS);if(index>=n)throw std::runtime_error("Invalid emitter index");p+=index*504;
        WxBackdropEmitter e{};e.flags=read<uint32_t>(p+4);e.bone=read<uint16_t>(p+0x14);e.texture=read<uint16_t>(p+0x16);
        e.blend=read<uint16_t>(p+0x28);e.type=read<uint16_t>(p+0x2a);
        if(read<uint16_t>(p+0x2c)||!wx_particle_flags_supported(e.flags))
            throw std::runtime_error("Scene particle tail/type/flags require support");
        e.tile_rotation=read<uint16_t>(p+0x2e);e.rows=read<uint16_t>(p+0x30);e.columns=read<uint16_t>(p+0x32);
        for(unsigned j=0;j<3;j++){
            e.position[j]=read<float>(p+8+j*4);e.scale[j]=read<float>(p+0x15c+j*4);e.cells[j]=read<uint16_t>(p+0x168+j*2);
            uint32_t rgba=read<uint32_t>(p+0x150+j*4);
            e.color[j][0]=float((rgba>>16)&255)/255;e.color[j][1]=float((rgba>>8)&255)/255;e.color[j][2]=float(rgba&255)/255;e.color[j][3]=float(rgba>>24)/255;
        }
        e.midpoint=read<float>(p+0x14c);return e;
    }
};
