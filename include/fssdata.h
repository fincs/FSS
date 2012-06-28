#pragma once

typedef struct
{
	byte_t state;
	char_t trackId; // -1 = none
	u8 pan, key;
	u16 vol, patch;
} fss_chndata_t;

typedef struct
{
	u8 state, number, playerId, pad;
	const u8* pos;
	u8 patch, vol, expr, pan;
} fss_trkdata_t;

typedef struct
{
	u8 state, nTracks;
	u16 tempo;
	u8 trackIds[16]; // FSS_TRACKCOUNT
} fss_plydata_t;
