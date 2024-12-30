#pragma once
#include "scratch.h"

void strescape(const char* str, size_t len, ScratchBuf* buf);

const char* strstrip(const char* str, const char* prefix);
