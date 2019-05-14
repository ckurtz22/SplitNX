#include "mem.h"


Handle getDebugHandle() {
    Handle debugHandle;
    u64 pid;
    Result rc = pmdmntInitialize();
    if (R_FAILED(rc)) fatalSimple(rc);
    rc = pmdmntGetApplicationPid(&pid);
    pmdmntExit();
    if (R_FAILED(rc)) return 0;
    svcDebugActiveProcess(&debugHandle, pid);
    return debugHandle;
}

u64 findHeapBase(Handle debugHandle) {
    MemoryInfo memInfo = { 0 };
    u32 pageInfo;
    u64 lastAddr;
    do {   
        lastAddr = memInfo.addr;
        svcQueryDebugProcessMemory(&memInfo, &pageInfo, debugHandle, memInfo.addr + memInfo.size);
    } while (memInfo.type != MemType_Heap && lastAddr < memInfo.addr + memInfo.size);
    if (memInfo.type != MemType_Heap) return 0;
    return memInfo.addr;
}

u64 readMemory(u64 address, size_t size) {
    u64 val;
    Handle handle = getDebugHandle();
    if (handle == 0) return 0;

    u64 heapBase = findHeapBase(handle);

    svcReadDebugProcessMemory(&val, handle, heapBase + address, size);
    svcCloseHandle(handle);

    return val;
}
