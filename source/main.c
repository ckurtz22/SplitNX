// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

// Include the main libnx system header, for Switch development
#include <switch.h>

enum Size{_u8, _u16, _u32, _u64, _unk};
enum Operator{ge, le, gt, lt, eq, ne, op_unk};

typedef struct Split {
    u64 address;
    u64 value;
    enum Size size;
    enum Operator operator;
} Split;

typedef struct Splits {
    Split splits[200];
    int length;
} Splits;

u32 scenarios[] = {
                    1013,
                    1023,
                    1026,
                    1032
                  };

// Sysmodules should not use applet*.
u32 __nx_applet_type = AppletType_None;

// Adjust size as needed.
#define INNER_HEAP_SIZE 0x540000
size_t nx_inner_heap_size = INNER_HEAP_SIZE;
char   nx_inner_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void)
{
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	// Newlib
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

// Init/exit services, update as needed.
void __attribute__((weak)) __appInit(void)
{
    Result rc;

    // Initialize default services.
    rc = smInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));
    
    // Initialize system for pmdmnt
    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    // Enable this if you want to use HID.
    rc = hidInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_HID));

    //Enable this if you want to use time.
    /*rc = timeInitialize();
    if (R_FAILED(rc))
        atalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_Time));
    __libnx_init_time();*/

    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    rc = socketInitializeDefault();
    if (R_FAILED(rc))
        fatalSimple(rc);

    fsdevMountSdmc();
}

void __attribute__((weak)) userAppExit(void);

void __attribute__((weak)) __appExit(void)
{
    // Cleanup default services.
    fsdevUnmountAll();
    fsExit();
    //timeExit();//Enable this if you want to use time.
    hidExit();// Enable this if you want to use HID.
    socketExit();
    smExit();
}

bool tryConnect(int* sock, const char* address, int port) {
    close(*sock);
    *sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_aton(address, &serv_addr.sin_addr); 
	return connect(*sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0;
}

void send_msg(int sock, const char* msg) {
	send(sock, msg, strlen(msg), 0);
}

int getSplitIndex(int sock) {
    send_msg(sock, "getsplitindex\r\n");
    char buffer[50];
    recv(sock, buffer, sizeof(buffer), 0);
    for (int i = 0; i < 50; i++) 
        if (buffer[i] == '\r') buffer[i] = 0;
    return atoi(buffer);
}

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

size_t enumToSize(enum Size size) {
    switch (size) {
        case _u8: return sizeof(u8);
        case _u16: return sizeof(u16);
        case _u32: return sizeof(u32);
        case _u64: return sizeof(u64);
        default: return sizeof(u64);
    }
}

bool doOperator(u64 param1, u64 param2, enum Operator op) {
    switch (op) {
        case eq: return param1 == param2;
        case ne: return param1 != param2;
        case lt: return param1 < param2;
        case gt: return param1 > param2;
        case le: return param1 <= param2;
        case ge: return param1 >= param2;
        default: return false;
    }

}

u64 readMemory(Handle debugHandle, u64 address, enum Size size) {
    u64 val;
    svcReadDebugProcessMemory(&val, debugHandle, address, enumToSize(size));
    return val;
}

bool autoSplit(Splits splits, int index) {
    if (index < 0 || index >= splits.length) return false;
    Split split = splits.splits[index];
    if (split.size == _unk || split.operator == op_unk) return false;

    Handle debugHandle = getDebugHandle();
    if (debugHandle == 0) return false;

    u64 heapBase = findHeapBase(debugHandle);
    u64 val = readMemory(debugHandle, heapBase + split.address, split.size /*0x61BC93B6*/);
    svcCloseHandle(debugHandle);

    return (heapBase != 0 && doOperator(val, split.value, split.operator));
}


void splitterInit(int* sock, Splits* splits) {
    char buffer[202][50];
    memset(buffer, 0, sizeof(buffer));

    FILE* f = fopen("/split/splitter.txt", "r");
    if (f == 0) return;

    int i = 0;
    while (i < 202 && fgets(buffer[i], sizeof(buffer[i]), f)) {
        for (int j = 0; j < sizeof(buffer[i]); j++)
            if (buffer[i][j] == '\n') buffer[i][j] = 0;
        i++;
    }
    fclose(f);

    if (i < 2) return;
    tryConnect(sock, buffer[0], atoi(buffer[1]));
    
    splits->length = i - 2;
    for (int j = 0; j < splits->length; j++) {
        char operator[5];
        char size[5];
        // 0x00010 >= u32 1234
        sscanf(buffer[j + 2], "%lx %s %s %lu", &splits->splits[j].address, operator, size, &splits->splits[j].value);
        if      (strcmp(operator, "==") == 0) splits->splits[j].operator = eq;
        else if (strcmp(operator, "!=") == 0) splits->splits[j].operator = ne;
        else if (strcmp(operator, ">=") == 0) splits->splits[j].operator = ge;
        else if (strcmp(operator, "<=") == 0) splits->splits[j].operator = le;
        else if (strcmp(operator, ">") == 0)  splits->splits[j].operator = gt;
        else if (strcmp(operator, "<") == 0)  splits->splits[j].operator = lt;
        else                                  splits->splits[j].operator = op_unk;
    
        if      (strcmp(size, "u8") == 0)  splits->splits[j].size = _u8;
        else if (strcmp(size, "u16") == 0) splits->splits[j].size = _u16;
        else if (strcmp(size, "u32") == 0) splits->splits[j].size = _u32;
        else if (strcmp(size, "u64") == 0) splits->splits[j].size = _u64;
        else                               splits->splits[j].size = _unk;
    }
}


int main(int argc, char* argv[])
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    Splits splits = {.length = 0};


	while (appletMainLoop()) {
		hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        int index = getSplitIndex(sock);
        if (autoSplit(splits, index)) send_msg(sock, "split\r\n");

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
