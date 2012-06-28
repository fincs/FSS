#include "fss_private.h"

int fssFifoCh = -1;
static int fssRefCount = 0;
static instance_t fssArm7Handle;

// The funny 32-byte stuff is because it's the size of a cache line
__attribute__((aligned(32))) static byte_t __sharedworkBuf[FSS_SHAREDWORKSIZE];
static fss_sharedwork_t* sharedWork;

FEOSINIT void initSharedWork()
{
	DC_FlushRange(__sharedworkBuf, sizeof(__sharedworkBuf));
	sharedWork = (fss_sharedwork_t*) memUncached(__sharedworkBuf);
	sharedWork->msg.ptr = &sharedWork->chnData;
}

static inline int _callARM7(int x, u8* y)
{
	fifoSendDatamsg(fssFifoCh, x, y);
	return (int) fifoGetRetValue(fssFifoCh);
}

#define CALL_ARM7() _callARM7(sizeof(msg), (u8*)&msg)

static inline int _callARM7_addr(void* addr)
{
	fifoSendAddress(fssFifoCh, addr);
	return (int) fifoGetRetValue(fssFifoCh);
}

#define CALL_ARM7_ADDR() _callARM7_addr(sharedWork)

bool FSS_Startup()
{
	if (fssRefCount++) return true;

	// Do the real startup
	fssArm7Handle = FeOS_LoadARM7("/data/FeOS/arm7/FSS7.fx2", &fssFifoCh);
	if (!fssArm7Handle)
	{
		fssRefCount--;
		return false;
	}
	return true;
}

void FSS_Cleanup()
{
	if (--fssRefCount) return;

	// Do the real cleanup
	FeOS_FreeARM7(fssArm7Handle, fssFifoCh);
	fssArm7Handle = NULL, fssFifoCh = -1;
}

int FSS_PlaySample(fss_sample_t* pSample, int timer, int volume, int pan, int prio)
{
	msg_playsmp msg;
	msg.msgtype = FSSFIFO_PLAYSMP;

	msg.timer = timer;
	msg.vol = volume;
	msg.pan = pan;
	msg.prio = prio;

	msg.data = pSample->data;
	msg.len = pSample->length;
	msg.lpoint = pSample->loopPoint;
	msg.fmt = pSample->format;

	return CALL_ARM7();
}

int FSS_PlayTone(int duty, int timer, int volume, int pan, int prio)
{
	msg_playpsg msg;
	msg.msgtype = FSSFIFO_PLAYPSG;

	msg.duty = duty;
	msg.timer = timer;
	msg.vol = volume;
	msg.pan = pan;
	msg.prio = prio;

	return CALL_ARM7();
}

int FSS_PlayNoise(int timer, int volume, int pan, int prio)
{
	msg_playnoise msg;
	msg.msgtype = FSSFIFO_PLAYNOISE;

	msg.timer = timer;
	msg.vol = volume;
	msg.pan = pan;
	msg.prio = prio;

	return CALL_ARM7();
}

bool FSS_ChnIsActive(int handle)
{
	msg_singlech msg;
	msg.msgtype = FSSFIFO_CHNISACTIVE;
	msg.ch = handle;

	return CALL_ARM7();
}

void FSS_ChnStop(int handle)
{
	msg_singlech msg;
	msg.msgtype = FSSFIFO_CHNSTOP;
	msg.ch = handle;

	CALL_ARM7();
}

static inline bool isPsg(int ch) { return ch >= 8 && ch <= 13; }

void FSS_ChnSetParam(int handle, int type, int param)
{
	if (type == Param_Duty && !isPsg(handle)) return;

	msg_chnsetparam msg;
	msg.msgtype = FSSFIFO_CHNSETPARAM;
	msg.ch = handle;
	msg.tParam = type;
	msg.vParam = param;

	CALL_ARM7();
}

int FSS_PlayerAlloc(int prio)
{
	msg_playeralloc msg;
	msg.msgtype = FSSFIFO_PLAYERALLOC;
	msg.prio = prio;

	return CALL_ARM7();
}

bool FSS_PlayerSetup(int handle, const void* pSeq, const void* pBnk, const void* const pWar[4])
{
	msg_playersetup msg;
	msg.msgtype = FSSFIFO_PLAYERSETUP;
	msg.playerId = handle;
	msg.pSeq = (const byte_t*) pSeq;
	msg.pBnk = (const byte_t*) pBnk;
	memcpy(msg.pWar, pWar, sizeof(msg.pWar));

	return CALL_ARM7();
}

void FSS_PlayerPlay(int handle)
{
	msg_singleplayer msg;
	msg.msgtype = FSSFIFO_PLAYERPLAY;
	msg.playerId = handle;

	CALL_ARM7();
}

void FSS_PlayerStopEx(int handle, bool bKillChannels)
{
	msg_genericWithParam msg;
	msg.msgtype = FSSFIFO_PLAYERSTOP;
	msg.which = handle;
	msg.param = bKillChannels;

	CALL_ARM7();
}

void FSS_PlayerSetPause(int handle, bool bPause)
{
	msg_genericWithParam msg;
	msg.msgtype = FSSFIFO_PLAYERSETPAUSE;
	msg.which = handle;
	msg.param = bPause;

	CALL_ARM7();
}

void FSS_PlayerFree(int handle)
{
	msg_generic msg;
	msg.msgtype = FSSFIFO_PLAYERFREE;
	msg.which = handle;

	CALL_ARM7();
}

void FSS_LockChannels(word_t mask)
{
	msg_generic msg;
	msg.msgtype = FSSFIFO_LOCKCHANNELS;
	msg.which = (mask & 0xFFFF) | BIT(16);

	CALL_ARM7();
}

void FSS_UnlockChannels(word_t mask)
{
	msg_generic msg;
	msg.msgtype = FSSFIFO_LOCKCHANNELS;
	msg.which = mask & 0xFFFF;

	CALL_ARM7();
}

void FSS_SetOutputFlags(int flags)
{
	msg_generic msg;
	msg.msgtype = FSSFIFO_SETOUTPFLAGS;
	msg.which = flags;

	CALL_ARM7();
}

void FSS_SetupCapture(int flags, void* buffer, size_t bufSize, int timer)
{
	msg_capsetup msg;
	msg.msgtype = FSSFIFO_CAPSETUP;
	msg.buffer = buffer;
	msg.bufSizeInWords = (bufSize+3) >> 2;
	memset(buffer, 0, bufSize);
	DC_FlushRange(buffer, bufSize);
	msg.timer = timer;
	msg.flags = flags;

	CALL_ARM7();
}

static inline hword_t cnvVol(int vol)
{
	if (vol > 0x3F8) return (vol >> 4) | (0 << 8);
	if (vol > 0x1FC) return (vol >> 3) | (1 << 8);
	if (vol > 0x07F) return (vol >> 2) | (2 << 8);
	else             return (vol >> 0) | (3 << 8);
}

void FSS_SetupCapReplay(int cap, int volume, int pan)
{
	msg_caprplsetup msg;
	msg.msgtype = FSSFIFO_CAPREPLAYSETUP;
	msg.vol = cnvVol(volume);
	msg.pan = pan;
	msg.x = (cap >> 8) - 1;

	CALL_ARM7();
}

void FSS_StartCapture(int mask)
{
	msg_generic msg;
	msg.msgtype = FSSFIFO_CAPSTART;
	msg.which = mask >> 8;

	CALL_ARM7();
}

void FSS_StartCapReplay(int mask)
{
	msg_generic msg;
	msg.msgtype = FSSFIFO_CAPREPLAYSTART;
	msg.which = mask >> 8;

	CALL_ARM7();
}

void FSS_StopCapReplay(int mask)
{
	msg_generic msg;
	msg.msgtype = FSSFIFO_CAPSTOP;
	msg.which = mask >> 8;

	CALL_ARM7();
}

void FSS_StopCapture(int mask)
{
	msg_generic msg;
	msg.msgtype = FSSFIFO_CAPREPLAYSTOP;
	msg.which = mask >> 8;

	CALL_ARM7();
}

/*
void FSS_PlayerRead(int handle, fss_plydata_t* pData)
{
	msg_ptrwithparam msg;
	msg.msgtype = FSSFIFO_PLAYERREAD;
	msg.ptr = pData;
	msg.param = handle;

	DC_FlushRange(pData, sizeof(fss_plydata_t));
	CALL_ARM7();
}

void FSS_TrackRead(int handle, fss_trkdata_t* pData)
{
	msg_ptrwithparam msg;
	msg.msgtype = FSSFIFO_TRACKREAD;
	msg.ptr = pData;
	msg.param = handle;

	DC_FlushRange(pData, sizeof(fss_trkdata_t));
	CALL_ARM7();
}

void FSS_ChannelRead(int handle, fss_chndata_t* pData)
{
	msg_ptrwithparam msg;
	msg.msgtype = FSSFIFO_CHANNELREAD;
	msg.ptr = pData;
	msg.param = handle;

	DC_FlushRange(pData, sizeof(fss_chndata_t));
	CALL_ARM7();
}
*/

void FSS_PlayerRead(int handle, fss_plydata_t* pData)
{
	sharedWork->msg.msgtype = FSSFIFO_PLAYERREAD;
	sharedWork->msg.param = handle;
	CALL_ARM7_ADDR();

	memcpy(pData, &sharedWork->plyData, sizeof(fss_plydata_t));
}

void FSS_TrackRead(int handle, fss_trkdata_t* pData)
{
	sharedWork->msg.msgtype = FSSFIFO_TRACKREAD;
	sharedWork->msg.param = handle;
	CALL_ARM7_ADDR();

	memcpy(pData, &sharedWork->trkData, sizeof(fss_trkdata_t));
}

void FSS_ChannelRead(int handle, fss_chndata_t* pData)
{
	sharedWork->msg.msgtype = FSSFIFO_CHANNELREAD;
	sharedWork->msg.param = handle;
	CALL_ARM7_ADDR();

	memcpy(pData, &sharedWork->chnData, sizeof(fss_chndata_t));
}
