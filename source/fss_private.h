#pragma once
#include <fsscommon.h>
#include <fss.h>

void FSS_RawStreamSetup(void* buf, int buflen, int timer, int fmt);
void FSS_RawStreamSetStatus(bool bStatus);
int FSS_RawStreamGetPos();
