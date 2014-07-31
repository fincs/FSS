#pragma once
#include <feos.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FSS_BUILD
#define FSS_API FEOS_EXPORT
#else
#define FSS_API
#endif

FSS_API bool FSS_Startup();
FSS_API void FSS_Cleanup();

enum { SoundFormat_8Bit, SoundFormat_16Bit, SoundFormat_ADPCM, SoundFormat_Loop = 4 };
enum { DutyCycle_0 = 7, DutyCycle_12 = 0, DutyCycle_25, DutyCycle_37, DutyCycle_50, DutyCycle_62, DutyCycle_75, DutyCycle_87 };
#define SOUND_TIMER(n) (0x1000000/((int)(n)))
#define CAP_TIMER(n) SOUND_TIMER(n)
#define MIC_TIMER(n) (SOUND_TIMER(n)<<1)
#define TONE_TIMER(n) SOUND_TIMER((n)*8)
#define INVALID_SND_HANDLE (-1)
#define DEFAULT_PRIO 0x80
#define DEFAULT_STRM_BUF_SIZE 8192

#define __Mixer 0
#define __Chn1 1
#define __Chn3 2
#define OutFlags_LeftOutFrom(x) ((x)<<8)
#define OutFlags_RightOutFrom(x) ((x)<<10)

enum
{
	OutFlags_LeftDisableDry = OutFlags_LeftOutFrom(__Chn1),
	OutFlags_RightDisableDry = OutFlags_RightOutFrom(__Chn3),
	OutFlags_MixerNoCh1 = BIT(12),
	OutFlags_MixerNoCh3 = BIT(13)
};

enum
{
	CapId_Left = BIT(8),
	CapId_Right = BIT(9)
};

enum
{
	CapFlags_AddAssocChnToPrevChn = BIT(0),
	CapFlags_SourceIsMixer = 0,
	CapFlags_SourceIsPrevChn = BIT(1),
	CapFlags_Loop = 0,
	CapFlags_OneShot = BIT(2),
	CapFlags_Format_16Bit = 0,
	CapFlags_Format_8Bit = BIT(3)
};

enum
{
	CapStatus_Active = 1,
	CapStatus_Disabled = 0,
};

enum
{
	UpdFlags_Players = BIT(0),
	UpdFlags_Tracks = BIT(1),
	UpdFlags_Channels = BIT(2)
};

enum
{
	StreamFormat_8Bit = SoundFormat_8Bit,
	StreamFormat_16Bit = SoundFormat_16Bit,

	StreamFormat_Mono = (0 << 2),
	StreamFormat_SplitStereo = (1 << 2),
	StreamFormat_Stereo = (3 << 2) // interleaved
};

typedef struct
{
	const void* data;
	word_t length;
	hword_t loopPoint;
	hword_t format;
} fss_sample_t;

typedef struct _tag_fss_stream_t fss_stream_t;

typedef int (*fssStreamCallback)(void* userData, void* outBuf, const fss_stream_t* streamData, int reqSamples);

struct _tag_fss_stream_t
{
	u16 timer, format;
	int bufSampleCount;
	fssStreamCallback callback;
};

static inline void** __SBNK_GetWavLinkEntryPtr(const void* pBnk, int id)
{
	return (void**)((char*)pBnk + 24 + id*sizeof(void*));
}

static inline void FSS_SetBankWar(void* pBnk, int warId, const void* pWar)
{
	const void** pEnt = (const void**) __SBNK_GetWavLinkEntryPtr(pBnk, warId);
	*pEnt = pWar;
}

static inline void* FSS_GetBankWar(const void* pBnk, int warId)
{
	void** pEnt = __SBNK_GetWavLinkEntryPtr(pBnk, warId);
	return *pEnt;
}

#include "fssdata.h"

FSS_API int FSS_PlaySample(const fss_sample_t* pSample, int timer, int volume, int pan, int prio);
FSS_API int FSS_PlayTone(int duty, int timer, int volume, int pan, int prio);
FSS_API int FSS_PlayNoise(int timer, int volume, int pan, int prio);

enum { Param_Volume, Param_Pan, Param_Timer, Param_Duty, Param_Tuning, Param_Tempo=Param_Timer };

FSS_API bool FSS_ChnIsActive(int handle);
FSS_API void FSS_ChnStop(int handle);
FSS_API void FSS_ChnSetParam(int handle, int type, int param);

static inline void FSS_ChnSetVolume(int handle, int vol)
{
	FSS_ChnSetParam(handle, Param_Volume, vol);
}

static inline void FSS_ChnSetPan(int handle, int pan)
{
	FSS_ChnSetParam(handle, Param_Pan, pan);
}

static inline void FSS_ChnSetTimer(int handle, int timer)
{
	FSS_ChnSetParam(handle, Param_Timer, timer);
}

static inline void FSS_ChnSetDuty(int handle, int duty)
{
	FSS_ChnSetParam(handle, Param_Duty, duty);
}

// tuning = 1/64ths of a semitone
static inline void FSS_ChnSetTuning(int handle, int tuning)
{
	FSS_ChnSetParam(handle, Param_Tuning, tuning);
}

FSS_API int FSS_PlayerAlloc(int prio);
FSS_API bool FSS_PlayerSetup(int handle, const void* pSeq, const void* pBnk);
FSS_API void FSS_PlayerPlay(int handle);
FSS_API void FSS_PlayerStopEx(int handle, bool bKillChannels);
FSS_API void FSS_PlayerSetPause(int handle, bool bPause);
FSS_API void FSS_PlayerFree(int handle);
FSS_API void FSS_PlayerSetParam(int handle, int type, int param);

static inline void FSS_PlayerStop(int handle)
{
	FSS_PlayerStopEx(handle, false);
}

static inline void FSS_PlayerKillSnd(int handle)
{
	FSS_PlayerStopEx(handle, true);
}

static inline void FSS_PlayerPause(int handle)
{
	FSS_PlayerSetPause(handle, true);
}

static inline void FSS_PlayerResume(int handle)
{
	FSS_PlayerSetPause(handle, false);
}

FSS_API void FSS_LockChannels(word_t mask);
FSS_API void FSS_UnlockChannels(word_t mask);

FSS_API void FSS_SetOutputFlags(int flags);

FSS_API void FSS_SetupCapture(int flags, void* buffer, size_t bufSize, int timer);
FSS_API void FSS_SetupCapReplay(int cap, int volume, int pan);

FSS_API void FSS_StartCapture(int mask);
FSS_API void FSS_StartCapReplay(int mask);

FSS_API void FSS_StopCapReplay(int mask);
FSS_API void FSS_StopCapture(int mask);

FSS_API void FSS_SetUpdFlags(int flags);
FSS_API void FSS_PlayerRead(int handle, fss_plydata_t* pData);
FSS_API void FSS_TrackRead(int handle, fss_trkdata_t* pData);
FSS_API void FSS_ChannelRead(int handle, fss_chndata_t* pData);

FSS_API void FSS_MicStart(void* buffer, int sampCount, int format, int timer);
FSS_API void FSS_MicStop();
FSS_API int FSS_MicGetPos();

FSS_API bool FSS_StreamSetup(const fss_stream_t* pStream, void* userData);
FSS_API bool FSS_StreamExists();
FSS_API int  FSS_StreamSetStatus(bool bActive);
FSS_API bool FSS_StreamGetStatus();
FSS_API int  FSS_StreamMain();
FSS_API void FSS_StreamFree();

static inline int FSS_StreamStart()
{
	return FSS_StreamSetStatus(true);
}

static inline int FSS_StreamStop()
{
	return FSS_StreamSetStatus(false);
}

#ifdef __cplusplus
}
#endif
