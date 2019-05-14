#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>

#include "sock.h"
#include "mem.h"


enum Operator{ge, le, gt, lt, eq, ne, op_unk};

typedef struct Split {
    u64 address;
    u64 value;
    size_t size;
    enum Operator op;
} Split;

typedef struct Splits {
    Split splits[200];
    int length;
} Splits;


int getSplitIndex();
bool doOperator(u64 param1, u64 param2, enum Operator op);
bool autoSplit(int index);
void splitterInit();
void addSplit(const char* buffer);
