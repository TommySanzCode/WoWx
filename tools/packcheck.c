#include "wx_runtime.h"
#include <stdlib.h>
void* wx_gpu_alloc(unsigned bytes){return malloc(bytes);}
void wx_gpu_free(void* p){free(p);}
unsigned wx_free_memory(void){return 64u*1024u*1024u;}
int main(int argc,char** argv){
    if(argc<2){fprintf(stderr,"Usage: wowx_packcheck pack...\n");return 2;}
    for(int i=1;i<argc;i++){
        WxScene scene;int ok=wx_pack_open(&scene,argv[i])&&wx_pack_verify(&scene);
        if(ok)printf("Verified %s: %u batches through the runtime loader\n",argv[i],scene.loads);
        else fprintf(stderr,"%s: %s\n",argv[i],scene.error);
        wx_pack_close(&scene);if(!ok)return 1;
    }
    return 0;
}
