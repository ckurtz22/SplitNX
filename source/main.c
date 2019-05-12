#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <twili.h>
#include <switch.h>

#include "mem.h"
#include "sock.h"
#include "split.h"


#define INNER_HEAP_SIZE 0x400000
size_t nx_inner_heap_size = INNER_HEAP_SIZE;
char   nx_inner_heap[INNER_HEAP_SIZE];
u32 __nx_applet_type = AppletType_None;

void __libnx_initheap(void)
{
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	extern char* fake_heap_start;
    extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

// Init/exit services, update as needed.
void __attribute__((weak)) __appInit(void)
{
    Result rc;
    SetSysFirmwareVersion fw;

    if (R_FAILED(smInitialize())) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    if (R_FAILED(hidInitialize())) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_HID));
    if (R_FAILED(fsInitialize())) fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));
    if (R_FAILED(rc = twiliInitialize())) printf("twili failed to initialize\n");
    if (R_FAILED(rc = socketInitializeDefault())) printf("socket failed to initialize\n");

    if (R_SUCCEEDED(setsysInitialize())) {
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }
    //fsdevMountSdmc();
}

void __attribute__((weak)) userAppExit(void);

void __attribute__((weak)) __appExit(void)
{
    fsdevUnmountAll();
    fsExit();
    hidExit();
    socketExit();
    twiliExit();
    smExit();
}

int main(int argc, char* argv[])
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    Splits splits = {.length = 0};


	while (appletMainLoop()) {
		hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        //int index = getSplitIndex(sock);
        //if (autoSplit(splits, index)) send_msg(sock, "split\r\n");

        if (kHeld & KEY_ZL && kHeld & KEY_ZR && kHeld & KEY_L && kHeld && KEY_R) {
            if (kDown & KEY_PLUS) splitterInit(&sock, &splits);
            if (kDown & KEY_A) send_msg(sock, "startorsplit\r\n");
            if (kDown & KEY_B) send_msg(sock, "unsplit\r\n");
            if (kDown & KEY_X) send_msg(sock, "skipsplit\r\n");
            if (kDown & KEY_Y) send_msg(sock, "reset\r\n");
        }

        svcSleepThread(1e+8L);
	}
    
    return 0;
}
