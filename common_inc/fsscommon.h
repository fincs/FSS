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

	FSSFIFO_LOCKCHANNELS, FSSFIFO_SETOUTPFLAGS, FSSFIFO_CAPSETUP, FSSFIFO_CAPREPLAYSETUP,
	FSSFIFO_CAPSTART, FSSFIFO_CAPREPLAYSTART, FSSFIFO_CAPSTOP, FSSFIFO_CAPREPLAYSTOP,

	FSSFIFO_PLAYERREAD, FSSFIFO_TRACKREAD, FSSFIFO_CHANNELREAD
};

#ifdef ARM7
#define fifoRetWait(ch) while(!fifoCheckValue32(ch))
#else
#define fifoRetWait(ch) do { while(!fifoCheckValue32(ch)) FeOS_WaitForIRQ(~0); } while(0)
#endif
#define fifoRetValue(ch) fifoGetValue32(ch)

#define fifoReturn(ch, x) do { fifoSendValue32(ch, (u32)(x)); return; } while(0)

static inline word_t fifoGetRetValue(int ch)
{
	fifoRetWait(ch);
	return fifoRetValue(ch);
}

enum { FMT_8BIT, FMT_16BIT, FMT_ADPCM, FMT_LOOP = 4 };
enum { PRM_VOL, PRM_PAN, PRM_TIMER, PRM_DUTY };

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
	msg_ptrwithparam msg;
	union
	{
		fss_chndata_t chnData;
		fss_trkdata_t trkData;
		fss_plydata_t plyData;
	};
	s16 globalVars[FSS_GLOBALVARCOUNT];
	s16 playerVars[FSS_PLAYERCOUNT][FSS_PLAYERVARCOUNT];
} fss_sharedwork_t;

#define FSS_SHAREDWORKSIZE ((sizeof(fss_sharedwork_t) + 31) &~ 31)

typedef struct
{
	int msgtype;
	volatile fss_sharedwork_t* sharedWork;
} msg_init;
