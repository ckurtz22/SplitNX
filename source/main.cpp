#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <switch.h>
#include <twili.h>

#include "splitter.hpp"

extern "C"
{
#define INNER_HEAP_SIZE 0x80000
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
    Result rc;
    rc = smInitialize();
    rc = twiliInitialize();

    rc = hidInitialize();
    rc = hidPermitVibration(true);
    
    rc = fsInitialize();
    rc = fsdevMountSdmc();

    SetSysFirmwareVersion fw;
    rc = setsysInitialize();
    rc = setsysGetFirmwareVersion(&fw);
    hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
    setsysExit();
    rc = pmdmntInitialize();

    // Who really needs those large buffers anyways
    SocketInitConfig cfg = *socketGetDefaultInitConfig();
    cfg.tcp_tx_buf_size = 0x40;
    cfg.tcp_rx_buf_size = 0x40;
    cfg.tcp_tx_buf_max_size = 0x0;
    cfg.tcp_rx_buf_max_size = 0x0;

    cfg.udp_tx_buf_size = 0x0; // Do we really need UDP in sockets?
    cfg.udp_tx_buf_size = 0x0;

    cfg.sb_efficiency = 1;

    rc = socketInitialize(&cfg);
}

void __appExit(void)
{
    socketExit();
    pmdmntExit();
    romfsExit();
    fsdevUnmountAll();
    fsExit();
    hidExit();
    twiliExit();
    smExit();
}

int main_sysmodule() {
    Splitter splitter = Splitter("/switch/SplitNX/splitter.txt");

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

        svcSleepThread(1e+8L);
        splitter.Update();
    }

    return 0;
}

int main_applet() {

    return 0;
}

int main(int argc, char *argv[])
{
    return main_sysmodule();
    return (R_SUCCEEDED(romfsInit()) ? main_applet() : main_sysmodule());
}