#pragma once

#define FSS_PLAYERCOUNT 16
#define FSS_TRACKCOUNT 16
#define FSS_MAXTRACKS 32
#define FSS_PLAYERVARCOUNT 16
#define FSS_GLOBALVARCOUNT 16
#define FSS_DEFAULTVARVALUE -1
#define FSS_TRACKSTACKSIZE 3

typedef struct
{
	byte_t state;
	char_t trackId; // -1 = none
	u8 pan, key;
	u16 vol, patch;
} fss_chndata_t;

typedef struct
{
	u8 state, number, playerId, _pad;
	const u8* pos;
	u8 patch, vol, expr, pan;
	s8 bend;
	u8 bendRange;
	u16 _pad2;
} fss_trkdata_t;

typedef struct
{
	u8 state, nTracks;
	u16 tempo;
	u8 trackIds[16]; // FSS_TRACKCOUNT
} fss_plydata_t;
