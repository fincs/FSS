#pragma once

typedef struct tagSwavInfo
{
	u8  nWaveType;    // 0 = PCM8, 1 = PCM16, 2 = (IMA-)ADPCM
	u8  bLoop;        // Loop flag = TRUE|FALSE
	u16 nSampleRate;  // Sampling Rate
	u16 nTime;        // (ARM7_CLOCK / nSampleRate) [ARM7_CLOCK: 33.513982MHz / 2 = 1.6756991 E +7]
	u16 nLoopOffset;  // Loop Offset (expressed in words (32-bits))
	u32 nNonLoopLen;  // Non Loop Length (expressed in words (32-bits))
} SWAVINFO;

typedef struct
{
	u16 wavid;
	u16 warid;
	u8 tnote;
	u8 a,d,s,r;
	u8 pan;
} SBNKNOTEDEF;

#define INST_TYPE(a) ((a) & 0xFF)
#define INST_OFF(a) ((a) >> 8)

static inline const SWAVINFO* GetWavInWar(const u8* pWar, int id)
{
	return (const SWAVINFO*)(pWar + ((const int*)(pWar+60))[id]);
}

static inline u32 GetInstrInBnk(const u8* pBnk, int id)
{
	return *(const u32*)(pBnk + 60 + sizeof(u32)*id);
}

static inline const u8* GetInstData(const u8* pBnk, u32 instInfo)
{
	return pBnk + (int)INST_OFF(instInfo);
}

static inline const void* GetSample(const SWAVINFO* pWavInfo)
{
	return pWavInfo + 1;
}
