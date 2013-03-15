#include "fss7.h"

static void Snd_Timer();

void Snd_Init()
{
	register int i;

	// Power sound on
	powerOn(POWER_SOUND);
	writePowerManagement(PM_CONTROL_REG, (readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE) | PM_SOUND_AMP);
	REG_SOUNDCNT = 127 | SOUND_ENABLE;

	// Turn all channels off
	for (i = 0; i < 16; i ++)
		SCHANNEL_CR(i) = 0;

	// Install timer
	coopTimerStart(1, ClockDivider_64, -2728, Snd_Timer);

	// Clear track assignments
	for (i = 0; i < 16; i ++)
	{
		FSS_Channels[i].trackId = -1;
		FSS_NoteLengths[i] = -1;
	}
}

void Snd_Deinit()
{
	register int i;

	// Stop timer
	coopIrqSet(IRQ_TIMER1, NULL);
	irqDisable(IRQ_TIMER1);
	timerStop(1);

	// Turn all channels off
	for (i = 0; i < 16; i ++)
		SCHANNEL_CR(i) = 0;

	// Power sound off
	//REG_SOUNDCNT &= ~SOUND_ENABLE;
	REG_SOUNDCNT = 0;
	writePowerManagement(PM_CONTROL_REG, ( readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_AMP ) | PM_SOUND_MUTE );
	powerOff(POWER_SOUND);
}

void Chn_Kill(fss_channel_t* pCh, int nCh)
{
	pCh->state = CS_NONE;
	pCh->trackId = -1;
	pCh->prio = 0;
	SCHANNEL_CR(nCh) = 0;
	FSS_ChnVol[nCh] = 0;
	FSS_NoteLengths[nCh] = -1;
}

void Chn_Release(fss_channel_t* pCh, int nCh)
{
	FSS_NoteLengths[nCh] = -1;
	pCh->prio = 1;
	pCh->state = CS_RELEASE;
}

void Chn_Lock(u32 mask)
{
	FSS_ChnLockMask |= mask & 0xFFFF;
	int i;
	for (i = 0; i < 16; i ++) if (mask & BIT(i))
		Chn_Kill(FSS_Channels + i, i);
}

void Chn_Unlock(u32 mask)
{
	FSS_ChnLockMask &= ~mask;
}

void Snd_SetOutputFlags(int flags)
{
	u16 reg = REG_SOUNDCNT;
	reg &= ~0x0F00;
	reg |= flags & 0xF00;
	REG_SOUNDCNT = reg;
}

static void Snd_UpdChannel(fss_channel_t* pCh, int nCh);
static void Snd_UpdWorkData();

void Snd_Timer()
{
	register int i;
	//int cS = enterCriticalSection();

	Chn_UpdateTracks();

	for (i = 0; i < 16; i ++)
	{
		if (FSS_ChnLockMask & BIT(i))
			continue;
		Snd_UpdChannel(FSS_Channels + i, i);
	}

	for (i = 0; i < FSS_PLAYERCOUNT; i ++)
		Player_Run(i);

	Snd_UpdWorkData();

	//leaveCriticalSection(cS);
}

static int calcVolDivShift(int x)
{
	// VOLDIV(0) /1  >>0
	// VOLDIV(1) /2  >>1
	// VOLDIV(2) /4  >>2
	// VOLDIV(3) /16 >>4
	if (x < 3) return x;
	return 4;
}

void Snd_UpdWorkData()
{
	register int i;
	if (FSS_WorkUpdFlags & WUF_CHANNELS)
		for (i = 0; i < 16; i ++)
			Cmd_ChannelRead((fss_chndata_t*)&FSS_SharedWork->chnData[i], i);
	if (FSS_WorkUpdFlags & WUF_TRACKS)
		for (i = 0; i < FSS_TRACKCOUNT; i ++)
			Cmd_TrackRead((fss_trkdata_t*)&FSS_SharedWork->trkData[i], i);
	if (FSS_WorkUpdFlags & WUF_PLAYERS)
		for (i = 0; i < FSS_PLAYERCOUNT; i ++)
			Cmd_PlayerRead((fss_plydata_t*)&FSS_SharedWork->plyData[i], i);
}

void Snd_UpdChannel(fss_channel_t* pCh, int nCh)
{
	// Kill active channels that aren't physically active
	if (pCh->state > CS_START && !SCHANNEL_ACTIVE(nCh))
	{
		Chn_Kill(pCh, nCh);
		return;
	}

	u8* pFlag = &pCh->flags;
	const bool bNotInSustain = pCh->state != CS_SUSTAIN;
	const bool bInStart = pCh->state == CS_START;
	const bool bPitchSweep = pCh->sweepPitch != 0 && pCh->sweepLen && pCh->sweepCnt <= pCh->sweepLen;
	bool bModulation = pCh->modDepth != 0;
	bool bVolNeedUpdate = bNotInSustain || *pFlag & F_UPDVOL;
	bool bPanNeedUpdate = *pFlag & F_UPDPAN || bInStart;
	bool bTmrNeedUpdate = *pFlag & F_UPDTMR || bInStart || bPitchSweep;
	const int modType = pCh->modType;
	int modParam = 0;

	switch (pCh->state)
	{
		case CS_NONE: return;
		case CS_START:
		{
			SCHANNEL_CR(nCh) = 0;
			SCHANNEL_SOURCE(nCh) = pCh->reg.SOURCE;
			SCHANNEL_REPEAT_POINT(nCh) = pCh->reg.REPEAT_POINT;
			SCHANNEL_LENGTH(nCh) = pCh->reg.LENGTH;
			pCh->ampl = AMPL_THRESHOLD;
			pCh->state = CS_ATTACK;
			// Fall down
		}
		case CS_ATTACK:
		{
			pCh->ampl = ((int)pCh->ampl * (int)pCh->attackLvl) / 255;
			if (pCh->ampl != 0) break;
			pCh->state = CS_DECAY;
			break;
		}
		case CS_DECAY:
		{
			pCh->ampl -= (int)pCh->decayRate;
			int sustLvl = Cnv_Sust(pCh->sustainLvl) << 7;
			if (pCh->ampl > sustLvl) break;
			pCh->ampl = sustLvl;
			pCh->state = CS_SUSTAIN;
			break;
		}
		case CS_RELEASE:
		{
			pCh->ampl -= (int)pCh->releaseRate;
			if (pCh->ampl > AMPL_THRESHOLD) break;
			Chn_Kill(pCh, nCh);
			return;
		}
	}

	if (bModulation) do
	{
		if (pCh->modDelayCnt < pCh->modDelay)
		{
			pCh->modDelayCnt ++;
			bModulation = false;
			break;
		}

		switch (modType)
		{
			case 0: bTmrNeedUpdate = true; break;
			case 1: bVolNeedUpdate = true; break;
			case 2: bPanNeedUpdate = true; break;
		}

		// Get the current modulation parameter
		modParam = Cnv_Sine(pCh->modCounter >> 8) * pCh->modRange * pCh->modDepth;

		if (modType == 0)
			modParam = (s64)(modParam * 60) >> 14;
		else
			// This ugly formula whose exact meaning and workings I cannot figure out is used for volume/pan modulation.
			modParam = ((modParam &~ 0xFC000000) >> 8) | ((((modParam < 0 ? -1 : 0) << 6) | ((u32)modParam >> 26)) << 18);

		// Update the modulation variables

		u16 speed = (u16)pCh->modSpeed << 6;
		u16 counter = (pCh->modCounter + speed) >> 8;
	
		while (counter >= 0x80)
			counter -= 0x80;
	
		pCh->modCounter += speed;
		pCh->modCounter &= 0xFF;
		pCh->modCounter |= counter << 8;
	} while(0);

	if (bTmrNeedUpdate)
	{
		int totalAdj = pCh->extTune;
		if (bModulation && modType == 0) totalAdj += modParam;
		if (bPitchSweep)
		{
			int len = pCh->sweepLen;
			int cnt = pCh->sweepCnt;
			totalAdj += ((s64)pCh->sweepPitch*(len-cnt)) / len;
			if (!pCh->manualSweep)
				pCh->sweepCnt ++;
		}
		u16 tmr = pCh->reg.TIMER;

		if (totalAdj) tmr = Timer_Adjust(tmr, totalAdj);
		SCHANNEL_TIMER(nCh) = -tmr;
		*pFlag &= ~F_UPDTMR;
	}

	if (bVolNeedUpdate || bPanNeedUpdate)
	{
		u32 cr = pCh->reg.CR;
		if (bVolNeedUpdate)
		{
			int totalVol = pCh->ampl >> 7;
			totalVol += pCh->extAmpl;
			totalVol += pCh->velocity;
			if (bModulation && modType == 1) totalVol += modParam;
			totalVol += AMPL_K;
			if (totalVol < 0) totalVol = 0;

			cr &= ~(SOUND_VOL(0x7F) | SOUND_VOLDIV(3));
			cr |= SOUND_VOL((int)swiGetVolumeTable(totalVol));

			if      (totalVol < (-240 + AMPL_K)) cr |= SOUND_VOLDIV(3);
			else if (totalVol < (-120 + AMPL_K)) cr |= SOUND_VOLDIV(2);
			else if (totalVol < (-60 + AMPL_K))  cr |= SOUND_VOLDIV(1);

			FSS_ChnVol[nCh] = ((cr & SOUND_VOL(0x7F)) << 4) >> calcVolDivShift((cr & SOUND_VOLDIV(3)) >> 8);

			*pFlag &= ~F_UPDVOL;
		}

		if (bPanNeedUpdate)
		{
			int pan = pCh->pan;
			pan += pCh->extPan;
			if (bModulation && modType == 2) pan += modParam;
			pan += 64;
			if      (pan < 0) pan = 0;
			else if (pan > 127) pan = 127;

			cr &= ~SOUND_PAN(0x7F);
			cr |= SOUND_PAN(pan);
			*pFlag &= ~F_UPDPAN;
		}

		pCh->reg.CR = cr;
		SCHANNEL_CR(nCh) = cr;
	}
}

int Chn_Alloc(int type, int prio)
{
	static const byte_t pcmChnArray[] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
	static const byte_t psgChnArray[] = { 13, 12, 11, 10, 9, 8 };
	static const byte_t noiseChnArray[] = { 15, 14 };
	static const byte_t arraySizes[] = { sizeof(pcmChnArray), sizeof(psgChnArray), sizeof(noiseChnArray) };
	static const byte_t* const arrayArray[] = { pcmChnArray, psgChnArray, noiseChnArray };

	const byte_t* chnArray = arrayArray[type];
	const int arraySize = arraySizes[type];

	register int i;
	register int curChnNo = -1;
	for (i = 0; i < arraySize; i ++)
	{
		int thisChnNo = chnArray[i];
		if (FSS_ChnLockMask & BIT(thisChnNo))
			continue;
		fss_channel_t* thisChn = FSS_Channels + thisChnNo;
		fss_channel_t* curChn = FSS_Channels + curChnNo;
		if (curChnNo != -1 && thisChn->prio >= curChn->prio)
		{
			if (thisChn->prio != curChn->prio)
				continue;
			if (FSS_ChnVol[curChnNo] <= FSS_ChnVol[thisChnNo])
				continue;
		}
		curChnNo = thisChnNo;
	}

	if (curChnNo == -1 || prio < FSS_Channels[curChnNo].prio) return -1;
	FSS_NoteLengths[curChnNo] = -1;
	FSS_ChnVol[curChnNo] = 0;
	return curChnNo;
}

// This function was obtained through disassembly of Ninty's sound driver
u16 Timer_Adjust(u16 basetmr, int pitch)
{
	u64 tmr;
	int shift = 0;
	pitch = -pitch;

	while (pitch < 0)
	{
		shift --;
		pitch += 0x300;
	}

	while (pitch >= 0x300)
	{
		shift ++;
		pitch -= 0x300;
	}

	tmr = (u64)basetmr * ((u32)swiGetPitchTable(pitch) + 0x10000);
	shift -= 16;
	if (shift <= 0)
		tmr >>= -shift;
	else if (shift < 32)
	{
		if (tmr & ((~0ULL) << (32-shift))) return 0xFFFF;
		tmr <<= shift;
	}else return 0x10;

	if (tmr < 0x10) return 0x10;
	if (tmr > 0xFFFF) return 0xFFFF;
	return (u16)tmr;
}
