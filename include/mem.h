#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>


Handle getDebugHandle();
u64 findHeapBase(Handle debugHandle);
u64 readMemory(u64 address, size_t size);