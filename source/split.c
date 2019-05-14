#include "split.h"

Splits splits;

int getSplitIndex() {
    send_msg("getsplitindex\r\n");
    char buffer[50];
    if(recv_msg(buffer, sizeof(buffer)) <= 0)
        return -1;
    for (int i = 0; i < sizeof(buffer); i++) 
        if (buffer[i] == '\r') buffer[i] = 0;
    return atoi(buffer);
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

bool autoSplit(int index) {
    if (index < 0 || index >= splits.length) return false;
    Split split = splits.splits[index];
    if (split.op == op_unk) return false;

    Handle debugHandle = getDebugHandle();
    if (debugHandle == 0) return false;

    u64 heapBase = findHeapBase(debugHandle);
    u64 val = readMemory(debugHandle, heapBase + split.address, split.size /*0x61BC93B6*/);
    svcCloseHandle(debugHandle);

    return (heapBase != 0 && doOperator(val, split.value, split.op));
}


void splitterInit() {
    char buffer[50];
    char ip[20];
    int port;
    splits.length = 0;

    memset(buffer, 0, sizeof(buffer));
    FILE* f = fopen("/splitter.txt", "r");
    if (f == 0) return;

    int i = 0;

    while (fgets(buffer, sizeof(buffer), f)) {
        for (int j = 0; j < sizeof(buffer); j++)
            if (buffer[j] == '\n') buffer[j] = 0;
        switch (i++) {
        case 0:
            strcpy(ip, buffer);
            break;
        case 1:
            port = atoi(buffer);
            break;
        default:
            addSplit(buffer);
            break;

        }
    }
    fclose(f);

    if (i < 2) return;
    tryConnect(ip, port);
    printf("length: %d\n", splits.length);
}

void addSplit(const char* buffer) {
    char operator[5];
    char size[5];
    int i = splits.length++;
    // 0x00010 >= u32 1234
    sscanf(buffer, "%lx %s %s %lu", &splits.splits[i].address, operator, size, &splits.splits[i].value);
    if      (strcmp(operator, "==") == 0) splits.splits[i].op = eq;
    else if (strcmp(operator, "!=") == 0) splits.splits[i].op = ne;
    else if (strcmp(operator, ">=") == 0) splits.splits[i].op = ge;
    else if (strcmp(operator, "<=") == 0) splits.splits[i].op = le;
    else if (strcmp(operator, ">") == 0)  splits.splits[i].op = gt;
    else if (strcmp(operator, "<") == 0)  splits.splits[i].op = lt;
    else                                  splits.splits[i].op = op_unk;

    if      (strcmp(size, "u8") == 0)  splits.splits[i].size = sizeof(u8);
    else if (strcmp(size, "u16") == 0) splits.splits[i].size = sizeof(u16);
    else if (strcmp(size, "u32") == 0) splits.splits[i].size = sizeof(u32);
    else if (strcmp(size, "u64") == 0) splits.splits[i].size = sizeof(u64);
    else                               splits.splits[i].size = sizeof(u64);
}
