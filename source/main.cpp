#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <switch.h>
#include <twili.h>

#include "splitter.hpp"

extern "C"
{
#include "mp3.h"
#define INNER_HEAP_SIZE 0x400000
    extern u32 __start__;

    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

u32 __nx_applet_type = AppletType_None;

void __libnx_initheap(void)
{
    void *addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = (char *)addr;
    fake_heap_end = (char *)addr + size;
}

void __appInit(void)
{
    smInitialize();

    hidInitialize();
    hidPermitVibration(true);
    
    fsInitialize();
    fsdevMountSdmc();

    SetSysFirmwareVersion fw;
    setsysInitialize();
    setsysGetFirmwareVersion(&fw);
    hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
    setsysExit();
    pmdmntInitialize();

    twiliInitialize();
    socketInitializeDefault();
}

void __appExit(void)
{
    fsdevUnmountAll();
    fsExit();
    hidExit();
    pmdmntExit();
    socketExit();
    twiliExit();
    smExit();
}

int main(int argc, char *argv[])
{
    Splitter splitter = Splitter("/splitter.txt");
    mp3MutInit();

    while (appletMainLoop())
    {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        if (!(~kHeld & (KEY_ZL | KEY_ZR | KEY_L | KEY_R)))
        {
            if (kDown & KEY_PLUS)
                splitter.Connect();
            else if (kDown & KEY_A)
                splitter.Split();
            else if (kDown & KEY_B)
                splitter.Undo();
            else if (kDown & KEY_X)
                splitter.Skip();
            else if (kDown & KEY_Y)
                splitter.Reset();
        }

        if (kDown & KEY_MINUS)
        {
            playMp3("/connect.mp3");
        }

        svcSleepThread(1e+8L);
        splitter.Update();
    }

    return 0;
}
