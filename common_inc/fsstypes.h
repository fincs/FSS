#pragma once

#ifdef ARM7
typedef u8 byte_t;
typedef u16 hword_t;
typedef u32 word_t;
typedef s8 char_t;
typedef s16 short_t;
typedef s32 long_t;
#endif

#define FSS_PLAYERCOUNT 16
#define FSS_TRACKCOUNT 16
#define FSS_MAXTRACKS 32
#define FSS_PLAYERVARCOUNT 16
#define FSS_GLOBALVARCOUNT 16
#define FSS_DEFAULTVARVALUE -1
#define FSS_TRACKSTACKSIZE 3

typedef struct
{
	s16 globalVars[FSS_GLOBALVARCOUNT];
	s16 playerVars[FSS_PLAYERCOUNT][FSS_PLAYERVARCOUNT];
} fss_work_t;
