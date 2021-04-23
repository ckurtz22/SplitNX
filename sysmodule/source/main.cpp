#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <switch.h>

#include "dmntcht.h"
#include "splitter.hpp"
#include "ulog/ulog.h"

extern "C" {
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
    void* addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end = (char*)addr + size;
}

void __appInit(void)
{
    Result rc;
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = hidInitialize();
    if (R_FAILED(rc))
        fatalThrow(rc);

    rc = hidPermitVibration(true);
    if (R_FAILED(rc))
        fatalThrow(rc);

    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    rc = fsdevMountSdmc();
    if (R_FAILED(rc))
        fatalThrow(rc);

    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    dmntchtInitialize();

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
    romfsExit();
    fsdevUnmountAll();
    fsExit();
    hidExit();
    smExit();
}

void ensureAppDir()
{
    struct stat st = { 0 };
    if (stat("/switch/SplitNX", &st) == -1) {
        mkdir("/switch/SplitNX", 0700);
    }
}

static uint64_t get_time_us()
{
    struct timespec tp = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &tp);
    return static_cast<uint64_t>(tp.tv_sec) * 1000 * 1000 + tp.tv_nsec / 1000;
}

static int put_str(void* handle, const char* str)
{
    auto* file_handle = reinterpret_cast<FILE*>(handle);
    return fprintf(file_handle, "%s", str);
}

int main(int argc, char* argv[])
{
    ensureAppDir();
    logger_set_time_callback(get_time_us);
    auto* fptr = fopen("/switch/SplitNX/debug.log", "w");
    logger_init(fptr, put_str);
    Splitter splitter = Splitter("/switch/SplitNX/splitter.txt");

    LOGGER_DEBUG("Starting SplitNx.");

    while (appletMainLoop()) {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        if (!(~kHeld & (KEY_ZR | KEY_R)) && !(kHeld & KEY_ZL) && !(kHeld & KEY_L)) {
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
            else if (kDown & KEY_DLEFT)
                splitter.debug_first_mem();
            else if (kDown & KEY_MINUS)
                splitter.Reload("/switch/SplitNX/splitter.txt");
        }

        svcSleepThread(1e+8L);
        splitter.Update();
    }

    LOGGER_DEBUG("Closing SplitNx.");

    fclose(fptr);
    return 0;
}
