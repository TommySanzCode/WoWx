#include "wx_runtime.h"
#include <xboxkrnl/xboxkrnl.h>

unsigned wx_free_memory(void) {
    MM_STATISTICS m={0};
    m.Length=sizeof m;
    return NT_SUCCESS(MmQueryStatistics(&m)) ? m.AvailablePages*4096 : 0;
}
void* wx_gpu_alloc(unsigned bytes) {
    return MmAllocateContiguousMemoryEx(bytes,0,0x03ffafff,0,PAGE_READWRITE|PAGE_WRITECOMBINE);
}
void wx_gpu_free(void* memory) { if(memory) MmFreeContiguousMemory(memory); }
