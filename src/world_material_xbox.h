/* World-only material state; title/UI passes keep their own shader contracts. */
static void world_motion_bind(const WxMotionState* state){
    uint32_t* p=begin_commands();p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,105);
    pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,8);memcpy(p,state,sizeof *state);p+=8;pb_end(p);
}
static void world_material_bind(unsigned flags,const WxFog* base,const float ambient[4],const float diffuse[4]){
    WxMaterial m=wx_material(flags);WxFog fog;
    if(base&&!m.unfogged){fog=*base;if(m.blend>=3)for(unsigned i=0;i<3;i++)fog.color[i]=m.fog_neutral;wx_fog_bind(&fog);}
    else wx_fog_bind(NULL);
    static const unsigned factor[]={NV097_SET_BLEND_FUNC_SFACTOR_V_ZERO,NV097_SET_BLEND_FUNC_SFACTOR_V_ONE,
        NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA,NV097_SET_BLEND_FUNC_SFACTOR_V_ONE_MINUS_SRC_ALPHA,
        NV097_SET_BLEND_FUNC_SFACTOR_V_DST_COLOR,NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_COLOR};
    float white[4]={1,1,1,0},black[4]={0};uint32_t* p=begin_commands();
    p=pb_push1(p,NV097_SET_BLEND_ENABLE,m.blend>=2);
    p=pb_push1(p,NV097_SET_BLEND_EQUATION,NV097_SET_BLEND_EQUATION_V_FUNC_ADD);
    p=pb_push1(p,NV097_SET_BLEND_FUNC_SFACTOR,factor[m.src]);p=pb_push1(p,NV097_SET_BLEND_FUNC_DFACTOR,factor[m.dst]);
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,m.depth_test);p=pb_push1(p,NV097_SET_DEPTH_MASK,m.depth_write);
    p=pb_push1(p,NV097_SET_ALPHA_TEST_ENABLE,m.blend==1);p=pb_push1(p,NV097_SET_ALPHA_FUNC,NV097_SET_ALPHA_FUNC_V_GREATER);p=pb_push1(p,NV097_SET_ALPHA_REF,100);
    p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,102);
    int baked=m.unlit||(flags&WX_BAKED_LIGHT);
    pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,8);memcpy(p,baked?white:ambient,16);p+=4;memcpy(p,baked?black:diffuse,16);p+=4;pb_end(p);
}
