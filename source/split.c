#include "split.h"

#include <errno.h>


int getSplitIndex(int sock) {
    send_msg(sock, "getsplitindex\r\n");
    char buffer[50];
    recv(sock, buffer, sizeof(buffer), 0);
    for (int i = 0; i < 50; i++) 
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

bool autoSplit(Splits splits, int index) {
    if (index < 0 || index >= splits.length) return false;
    Split split = splits.splits[index];
    if (split.operator == op_unk) return false;

    Handle debugHandle = getDebugHandle();
    if (debugHandle == 0) return false;

    u64 heapBase = findHeapBase(debugHandle);
    u64 val = readMemory(debugHandle, heapBase + split.address, split.size /*0x61BC93B6*/);
    svcCloseHandle(debugHandle);

    return (heapBase != 0 && doOperator(val, split.value, split.operator));
}


void splitterInit(int* sock, Splits* splits) {
    if(tryConnect(sock, "192.168.1.117", 16834))
        printf("yes\n");
    else
        printf("no\n");
    
    /*
    char buffer[202][50];
    memset(buffer, 0, sizeof(buffer));

    FILE* f = fopen("/splitter.txt", "r");
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
    
        if      (strcmp(size, "u8") == 0)  splits->splits[j].size = sizeof(u8);
        else if (strcmp(size, "u16") == 0) splits->splits[j].size = sizeof(u16);
        else if (strcmp(size, "u32") == 0) splits->splits[j].size = sizeof(u32);
        else if (strcmp(size, "u64") == 0) splits->splits[j].size = sizeof(u64);
        else                               splits->splits[j].size = sizeof(u64);
    }
    */
}
