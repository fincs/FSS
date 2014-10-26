#pragma once
#ifdef ARM7
#include <nds.h>
#endif
#include <feos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARM7
typedef u8 byte_t;
typedef u16 hword_t;
typedef u32 word_t;
typedef s8 char_t;
typedef s16 short_t;
typedef s32 long_t;
#endif

#include "../include/fssdata.h"

extern int fssFifoCh;

enum
{
	FSSFIFO_INIT,

	FSSFIFO_PLAYSMP, FSSFIFO_PLAYPSG, FSSFIFO_PLAYNOISE,
	FSSFIFO_CHNISACTIVE, FSSFIFO_CHNSTOP, FSSFIFO_CHNSETPARAM,

	FSSFIFO_PLAYERALLOC, FSSFIFO_PLAYERSETUP, FSSFIFO_PLAYERPLAY,
	FSSFIFO_PLAYERSTOP, FSSFIFO_PLAYERSETPAUSE, FSSFIFO_PLAYERFREE,
	FSSFIFO_PLAYERSETPARAM,

	FSSFIFO_LOCKCHANNELS, FSSFIFO_SETOUTPFLAGS, FSSFIFO_CAPSETUP, FSSFIFO_CAPREPLAYSETUP,
	FSSFIFO_CAPSTART, FSSFIFO_CAPREPLAYSTART, FSSFIFO_CAPSTOP, FSSFIFO_CAPREPLAYSTOP,

	FSSFIFO_SETUPDFLAGS,

	FSSFIFO_MICSTART, FSSFIFO_MICGETPOS, FSSFIFO_MICSTOP,

	FSSFIFO_STRMSETUP, FSSFIFO_STRMSETSTATUS, FSSFIFO_STRMGETPOS
};

#define fifoReturn(ch, x) do { fifoSendValue32(ch, (u32)(x)); return; } while(0)

#ifdef ARM7
#define fifoRetWait(ch) while(!fifoCheckValue32(ch))
#define fifoRetValue(ch) fifoGetValue32(ch)

static inline word_t fifoGetRetValue(int ch)
{
	fifoRetWait(ch);
	return fifoRetValue(ch);
}
#else
#define fifoGetRetValue fifoGetRetValue32
#endif

enum { FMT_8BIT, FMT_16BIT, FMT_ADPCM, FMT_LOOP = 4 };
enum { PRM_VOL, PRM_PAN, PRM_TIMER, PRM_DUTY, PRM_TUNE };
enum { FMT_STM_MONO = (0 << 2), FMT_STM_STEREO = (1 << 2), FMT_STM_STEREO_INTERLEAVED = (3 << 2) };

typedef struct
{
	int msgtype;

	const void* data;
	u32 len;
	u16 lpoint;
	u16 timer;
	u8 fmt, vol, pan, prio;
} msg_playsmp;

typedef struct
{
	int msgtype;

	u16 timer;
	u8 duty, vol, pan, prio;
} msg_playpsg;

typedef struct
{
	int msgtype;

	u16 timer;
	u8 vol, pan, prio;
} msg_playnoise;

typedef struct
{
	int msgtype;
	int ch;
} msg_singlech;

typedef struct
{
	int msgtype;
	int ch, tParam, vParam;
} msg_chnsetparam;

typedef struct
{
	int msgtype;
	u8 prio;
} msg_playeralloc;

typedef struct
{
	int msgtype;
	int playerId;
	const byte_t* pSeq;
	const byte_t* pBnk;
	const byte_t* pWar[4];
} msg_playersetup;

typedef struct
{
	int msgtype;
	int playerId;
} msg_singleplayer;

typedef struct
{
	int msgtype;
	word_t mask;
} msg_lockchns;

typedef struct
{
	int msgtype;
	void* buffer;
	word_t bufSizeInWords;
	hword_t timer;
	hword_t flags;
} msg_capsetup;

typedef struct
{
	int msgtype;
	hword_t vol;
	char_t pan;
	byte_t x;
} msg_caprplsetup;

typedef struct
{
	int msgtype;
	int which;
} msg_generic;

typedef struct
{
	int msgtype;
	int which;
	int param;
} msg_genericWithParam;

typedef struct
{
	int msgtype;
	void* ptr;
	int param;
} msg_ptrwithparam;

typedef struct
{
	fss_chndata_t chnData[16];
	fss_trkdata_t trkData[FSS_TRACKCOUNT];
	fss_plydata_t plyData[FSS_PLAYERCOUNT];
	s16 globalVars[FSS_GLOBALVARCOUNT];
	s16 playerVars[FSS_PLAYERCOUNT][FSS_PLAYERVARCOUNT];
} fss_sharedwork_t;

static inline vs16* _FSS_GetVarPtr(volatile fss_sharedwork_t* sw, int plyId, int varId)
{
	if (varId < 0 || varId > (FSS_PLAYERVARCOUNT+FSS_GLOBALVARCOUNT))
		varId = 0;
	if (varId < FSS_PLAYERVARCOUNT)
		return &sw->playerVars[plyId][varId];
	return sw->globalVars + varId - FSS_PLAYERVARCOUNT;
}

#define FSS_SHAREDWORKSIZE ((sizeof(fss_sharedwork_t) + 31) &~ 31)

typedef struct
{
	int msgtype;
	volatile fss_sharedwork_t* sharedWork;
} msg_init;

typedef struct
{
	int msgtype;
	u16 timer, fmt;
	void* buffer;
	int length;
} msg_micstart;

typedef struct
{
	int msgtype;
	void* buf;
	int buflen; // in WORDS and PER CHANNEL
	u16 timer;
	u16 fmt;
} msg_strmsetup;
