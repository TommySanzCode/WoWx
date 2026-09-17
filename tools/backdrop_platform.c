/* Host-only adapter for validating/preparing private scenes. */
#include <stdlib.h>
unsigned wx_free_memory(void){return 64u*1024u*1024u;}
void* wx_gpu_alloc(unsigned bytes){return malloc(bytes);}
void wx_gpu_free(void* p){free(p);}
