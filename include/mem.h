#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>

#include "split.h"


Handle getDebugHandle();
u64 findHeapBase(Handle debugHandle);
u64 readMemory(Handle debugHandle, u64 address, size_t size);