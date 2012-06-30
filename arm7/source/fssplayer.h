#pragma once

typedef struct
{
	byte_t state, prio, nTracks, _pad_;
	hword_t tempo, tempoCount, tempoRate /* 8.8 fixed point */;
	s16 masterVol;

	s16 userVol;
	s16 userPitch;

	const byte_t* pSeqData;
	const byte_t* pBnk;
	const byte_t* pWar[4];

	byte_t trackIds[FSS_TRACKCOUNT];
} fss_player_t;

typedef struct
{
	byte_t state, num, prio, _pad;
	fss_player_t* ply;

	const u8* startPos;
	const u8* pos;
	const u8* stack[FSS_TRACKSTACKSIZE];
	u8 stackPos;
	u8 loopCount[FSS_TRACKSTACKSIZE];

	int wait;
	u16 patch;
	u8 portaKey, portaTime;
	s16 sweepPitch;
	u8 vol, expr;
	s8 pan; // -64..63
	u8 pitchBendRange;
	s8 pitchBend;
	s8 transpose;

	u8 modType, modSpeed, modDepth, modRange;
	u16 modDelay;
} fss_track_t;

enum { PS_NONE, PS_INIT, PS_PLAY, PS_PAUSE, PS_STOP };
#define MK_SIG(player, track) ((player) | ((track) << 4))
#define SIG_NULL 0xFF

enum { TS_ALLOCBIT = BIT(0), TS_MUTEBIT = BIT(1), TS_NOTEWAIT = BIT(2), TS_PORTABIT = BIT(3), TS_TIEBIT = BIT(4), TS_END = BIT(7) };
enum { TUF_VOL = BIT(0), TUF_PAN = BIT(1), TUF_TIMER = BIT(2), TUF_MOD = BIT(3) };

extern fss_player_t FSS_Players[FSS_PLAYERCOUNT];
extern fss_track_t FSS_Tracks[FSS_MAXTRACKS];
extern byte_t FSS_TrackUpdateFlags[FSS_MAXTRACKS];

typedef struct
{
	u32 CR, SOURCE;
	u16 TIMER, REPEAT_POINT;
	u32 LENGTH;
} sndreg_t;

enum { CS_NONE, CS_START, CS_ATTACK, CS_DECAY, CS_SUSTAIN, CS_RELEASE };
#define AMPL_K 723
#define AMPL_MIN (-AMPL_K)
#define AMPL_MAX 0
#define AMPL_THRESHOLD (AMPL_MIN<<7)

enum { F_UPDVOL = BIT(0), F_UPDPAN = BIT(1), F_UPDTMR = BIT(2) };
#define SOUND_VOLDIV(n) ((n) << 8)
#define SOUND_DUTY(n) ((n)<<24)
#define SOUND_FORMAT(n) (((int)(n))<<29)
#define SOUND_LOOP(a) ((a) ? SOUND_REPEAT : SOUND_ONE_SHOT)
#define SCHANNEL_ACTIVE(ch) (SCHANNEL_CR(ch) & SCHANNEL_ENABLE)

typedef struct
{
	sndreg_t reg;
	byte_t state;
	char_t trackId; // -1 = none
	byte_t prio;
	byte_t _pad;

	u8 flags;
	s8 pan; // -64 .. 63
	s16 extAmpl;

	s16 velocity;
	s8 extPan;
	u8 key;

	int ampl; // 7 fractionary bits
	int extTune; // in 64ths of a semitone

	u8 orgKey, patch;

	u8 modType, modSpeed, modDepth, modRange;
	u16 modDelay, modDelayCnt, modCounter;

	u32 sweepLen, sweepCnt;
	s16 sweepPitch;

	u8 attackLvl, sustainLvl;
	u16 decayRate, releaseRate;
} fss_channel_t;

extern fss_channel_t FSS_Channels[16];
extern u16 FSS_ChnVol[16];
extern int FSS_NoteLengths[16];
extern u16 FSS_ChnLockMask;
