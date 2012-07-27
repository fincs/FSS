#include "fss7.h"

static void* micBuffer;

#define BUF8  ((s8*) micBuffer)
#define BUF16 ((s16*)micBuffer)

int micPos = -1;
int micLength;
bool micOneShot;

static void _micTimer12();
static void _micTimer8();

void Cmd_MicStart(msg_micstart* data)
{
	micOn();
	micBuffer = data->buffer;
	micPos = 0;
	micLength = data->length;
	int fmt = data->fmt;
	micOneShot = (fmt & FMT_LOOP) ? false : true;
	//fmt &= ~FMT_LOOP;

	timerStart(0, ClockDivider_1, -(int)data->timer, fmt & FMT_16BIT ? _micTimer12 : _micTimer8);
	fifoReturn(fssFifoCh, 0);
}

static void _micStop()
{
	timerStop(0);
	micOff();
	micBuffer = NULL;
	micPos = -1;
	micLength = 0;
	micOneShot = false;
}

void Cmd_MicStop()
{
	_micStop();
	fifoReturn(fssFifoCh, 0);
}

void Cmd_MicGetPos()
{
	fifoReturn(fssFifoCh, micPos);
}

// This would be easier if C had templates...
#define MIC_TIMER(funcName, bufName, micFuncName, shift, flip) \
	void funcName() \
	{ \
		int cS = enterCriticalSection(); \
		bufName[micPos++] = (micFuncName() << (shift)) ^ (flip); \
		if (micPos == micLength) \
		{ \
			if (micOneShot) _micStop(); \
			else micPos = 0; \
		} \
		leaveCriticalSection(cS); \
	}

MIC_TIMER(_micTimer12, BUF16, micReadData12, 4, 0x8000);
MIC_TIMER(_micTimer8,  BUF8,  micReadData8,  0, 0x80);

