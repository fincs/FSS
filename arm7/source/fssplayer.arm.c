#include "fss7.h"
#include "sseqcmds.h"
#include "sbnkswar.h"

static void Player_ClearState(fss_player_t* ply)
{
	ply->tempo = 120;
	ply->tempoCount = 0;
	ply->tempoRate = 0x100;
	ply->masterVol = 0; // this is actually the highest level
	ply->userVol = 0;
	ply->userPitch = 0;
}

static int Player_Init(int handle, int prio)
{
	fss_player_t* ply = FSS_Players + handle;

	ply->state = PS_INIT;
	ply->prio = prio;
	ply->nTracks = 0;
	Player_ClearState(ply);

	return handle;
}

int Player_Alloc(int prio)
{
	register int i;
	for (i = 0; i < FSS_PLAYERCOUNT; i ++)
		if (FSS_Players[i].state == PS_NONE)
			return Player_Init(i, prio);
	return -1;
}

static void Player_FreeTracks(fss_player_t* ply)
{
	register int i;

	for (i = 0; i < ply->nTracks; i ++)
		Track_Free(ply->trackIds[i]);
	ply->nTracks = 0;
}

void Player_Free(int handle)
{
	fss_player_t* ply = FSS_Players + handle;

	if (ply->state == PS_PLAY)
		Player_Stop(handle, true);

	Player_FreeTracks(ply);

	ply->state = PS_NONE;
	ply->prio = 0;
}

int Track_Alloc()
{
	register int i;
	for (i = 0; i < FSS_MAXTRACKS; i ++)
	{
		fss_track_t* thisTrk = FSS_Tracks + i;
		if (!(thisTrk->state & TS_ALLOCBIT))
		{
			memset(thisTrk, 0, sizeof(fss_player_t));
			thisTrk->state = TS_ALLOCBIT;
			FSS_TrackUpdateFlags[i] = 0;
			return i;
		}
	}
	return -1;
}

void Track_Free(int handle)
{
	fss_track_t* trk = FSS_Tracks + handle;
	trk->state = 0;
	FSS_TrackUpdateFlags[handle] = 0;
}

static inline int read8(const u8** ppData)
{
	const u8* pData = *ppData;
	int x = *pData;
	*ppData = pData + 1;
	return x;
}

static inline int read16(const u8** ppData)
{
	int x = read8(ppData);
	x |= read8(ppData) << 8;
	return x;
}

static inline int read24(const u8** ppData)
{
	int x = read8(ppData);
	x |= read8(ppData) << 8;
	x |= read8(ppData) << 16;
	return x;
}

static int readvl(const u8** ppData)
{
	int x = 0;
	for (;;)
	{
		int data = read8(ppData);
		x = (x << 7) | (data & 0x7F);
		if (!(data & 0x80)) break;
	}
	return x;
}

static inline vs16* getVarPtr(int var, fss_track_t* trk)
{
	return _FSS_GetVarPtr(FSS_SharedWork, trk->ply-FSS_Players, var);
}

static int readovr(const u8** ppData, fss_track_t* trk)
{
	trk->state &= ~TS_OVRBIT;
	if (trk->state & TS_OVRVARBIT)
	{
		trk->state &= ~TS_OVRVARBIT;
		return *getVarPtr(read8(ppData), trk);
	}
	return trk->ovrValue;
}

static int read8_o(const u8** ppData, fss_track_t* trk)
{
	if (trk->state & TS_OVRBIT) return readovr(ppData, trk);
	return read8(ppData);
}

static int read16_o(const u8** ppData, fss_track_t* trk)
{
	if (trk->state & TS_OVRBIT) return readovr(ppData, trk);
	return read16(ppData);
}

static int readvl_o(const u8** ppData, fss_track_t* trk)
{
	if (trk->state & TS_OVRBIT) return readovr(ppData, trk);
	return readvl(ppData);
}

void Track_ClearState(fss_track_t* trk)
{
	trk->state = TS_ALLOCBIT | TS_NOTEWAIT;
	trk->prio = trk->ply->prio + 64;
	trk->pos = trk->startPos;
	trk->stackPos = 0;
	trk->wait = 0;
	trk->patch = 0;
	trk->portaKey = 60;
	trk->portaTime = 0;
	trk->sweepPitch = 0;
	trk->vol = 127;
	trk->expr = 127;
	trk->pan = 0;
	trk->pitchBendRange = 2;
	trk->pitchBend = 0;
	trk->transpose = 0;
	trk->a = 0xFF;
	trk->d = 0xFF;
	trk->s = 0xFF;
	trk->r = 0xFF;
	trk->modType = 0;
	trk->modRange = 1;
	trk->modSpeed = 16;
	trk->modDelay = 10;
	trk->modDepth = 0;
}

static void Player_InitTrack(int handle, fss_player_t* ply, const byte_t* pos, int n)
{
	fss_track_t* trk = FSS_Tracks + handle;
	trk->num = n;
	trk->ply = ply;
	trk->startPos = pos;
	Track_ClearState(trk);
}

void WTF(char* buf, u32 x)
{
	static const char* cnv = "0123456789ABCDEF";
	buf[7] = cnv[x & 0xF]; x >>= 4;
	buf[6] = cnv[x & 0xF]; x >>= 4;
	buf[5] = cnv[x & 0xF]; x >>= 4;
	buf[4] = cnv[x & 0xF]; x >>= 4;
	buf[3] = cnv[x & 0xF]; x >>= 4;
	buf[2] = cnv[x & 0xF]; x >>= 4;
	buf[1] = cnv[x & 0xF]; x >>= 4;
	buf[0] = cnv[x & 0xF];
}

void print32(u32 x)
{
	char buf[10]; buf[8] = '\n'; buf[9] = 0;
	WTF(buf, x);
	nocashMessage(buf);
}

void print32b(u32 x)
{
	char buf[9]; buf[8] = 0;
	WTF(buf, x);
	nocashMessage(buf);
}

bool Player_Setup(int handle, const byte_t* pSeq, const byte_t* pBnk, const byte_t* const pWar[4])
{
	fss_player_t* ply = FSS_Players + handle;

	if (ply->state == PS_PLAY)
		Player_Stop(handle, true);
	if (ply->state != PS_INIT)
		Player_FreeTracks(ply);

	ply->pSeqData = pSeq + ((u32*)pSeq)[6];
	ply->pBnk = pBnk;
	memcpy(ply->pWar, pWar, sizeof(ply->pWar));

	byte_t* tArray = ply->trackIds;
	
	int firstTrack = Track_Alloc();
	if (firstTrack == -1) return false;
	Player_InitTrack(firstTrack, ply, NULL, 0);

	int nTracks = 1;
	tArray[0] = firstTrack;

	const byte_t* pData = ply->pSeqData;
	if (*pData == SSEQ_CMD_TRACKALLOC)
	for (pData += 3; *pData == SSEQ_CMD_TRACKCONF;) // Prepare extra tracks
	{
		pData ++;
		int tNum = read8(&pData);
		const byte_t* pos = ply->pSeqData + read24(&pData);
		int newTrack = Track_Alloc();
		if (!newTrack) continue;
		Player_InitTrack(newTrack, ply, pos, tNum);
		tArray[nTracks++] = newTrack;
	}

	FSS_Tracks[firstTrack].startPos = pData;
	FSS_Tracks[firstTrack].pos = pData;
	ply->state = PS_STOP;
	ply->nTracks = nTracks;

	return true;
}

void Player_Play(int handle)
{
	fss_player_t* ply = FSS_Players + handle;
	ply->state = PS_PLAY;
}

void Player_Stop(int handle, bool bKillSound)
{
	register int i, j;
	fss_player_t* ply = FSS_Players + handle;

	ply->state = PS_STOP;
	Player_ClearState(ply);
	for (i = 0; i < ply->nTracks; i ++)
	{
		int trackId = ply->trackIds[i];
		Track_ClearState(FSS_Tracks + trackId);
		for (j = 0; j < 16; j ++)
		{
			fss_channel_t* chn = FSS_Channels + j;
			if (chn->state != CS_NONE && chn->trackId == trackId)
			{
				if (bKillSound)
					Chn_Kill(chn, j);
				else
					Chn_Release(chn, j);
			}
		}
	}
}

void Player_SetPause(int handle, bool bPause)
{
	register int i;
	fss_player_t* ply = FSS_Players + handle;

	if (ply->state == PS_PAUSE && !bPause)
		ply->state = PS_PLAY;
	else if (ply->state == PS_PLAY && bPause)
	{
		ply->state = PS_PAUSE;
		for (i = 0; i < ply->nTracks; i ++)
			Track_ReleaseAllNotes(ply->trackIds[i]);
	}
}

static void Track_Run(int handle);

void Player_Run(int handle)
{
	fss_player_t* ply = FSS_Players + handle;

	if (ply->state != PS_PLAY) return;

	while (ply->tempoCount > 240)
	{
		ply->tempoCount -= 240;
		register int i;
		for (i = 0; i < ply->nTracks; i ++)
			Track_Run(ply->trackIds[i]);
	}
	ply->tempoCount += ((int)ply->tempo * (int)ply->tempoRate) >> 8;
}

static inline void Chn_UpdateVol(fss_channel_t* chn, fss_track_t* trk)
{
	int vol = trk->ply->masterVol;
	vol += trk->ply->userVol;
	vol += Cnv_Sust(trk->vol);
	vol += Cnv_Sust(trk->expr);
	if (vol < -AMPL_K) vol = -AMPL_K;
	chn->extAmpl = vol;
}

static inline void Chn_UpdatePan(fss_channel_t* chn, fss_track_t* trk)
{
	chn->extPan = trk->pan;
}

static inline void Chn_UpdateTune(fss_channel_t* chn, fss_track_t* trk)
{
	int tune = trk->ply->userPitch;
	tune += ((int)chn->key - (int)chn->orgKey) * 64;
	tune += ((int)trk->pitchBend * (int)trk->pitchBendRange) >> 1;
	chn->extTune = tune;
}

static inline void Chn_UpdateMod(fss_channel_t* chn, fss_track_t* trk)
{
	chn->modType = trk->modType;
	chn->modSpeed = trk->modSpeed;
	chn->modDepth = trk->modDepth;
	chn->modRange = trk->modRange;
	chn->modDelay = trk->modDelay;
}

static inline void Chn_UpdatePorta(fss_channel_t* chn, fss_track_t* trk, int chnId)
{
	chn->manualSweep = 0;
	chn->sweepPitch = trk->sweepPitch;
	chn->sweepCnt = 0;
	if (!(trk->state & TS_PORTABIT))
	{
		chn->sweepLen = 0;
		return;
	}

	int diff = ((int)trk->portaKey - (int)chn->key) << 22;
	chn->sweepPitch += diff >> 16;

	if (trk->portaTime == 0)
	{
		chn->sweepLen = FSS_NoteLengths[chnId];
		chn->manualSweep = 1;
	}else
	{
		int sq_time = (u32)trk->portaTime * (u32)trk->portaTime;
		int abs_sp = chn->sweepPitch;
		abs_sp = abs_sp < 0 ? -abs_sp : abs_sp;
		chn->sweepLen = (abs_sp*sq_time) >> 11;
	}
}

int Note_On(int trkNo, int key, int vel, int len)
{
	fss_track_t* trk = FSS_Tracks + trkNo;
	fss_player_t* ply = trk->ply;

	bool bIsPCM = true;
	const u8* pBnk = ply->pBnk;
	
	fss_channel_t* chn;
	int nCh;

	if (trk->patch > GetBnkInstrCount(pBnk))
		return -1;

	u32 instInfo = GetInstrInBnk(pBnk, trk->patch);
	const u8* pInstData = GetInstData(pBnk, instInfo);
	const SBNKNOTEDEF* pNoteDef = NULL;
	const SWAVINFO* pWavInfo = NULL;
	int fRecord = INST_TYPE(instInfo);
_ReadRecord:
	if (fRecord == 0) return -1;
	else if (fRecord == 1) pNoteDef = (const SBNKNOTEDEF*) pInstData;
	else if (fRecord < 4)
	{
		// PSG
		// fRecord = 2 -> PSG tone, pNoteDef->wavid -> PSG duty
		// fRecord = 3 -> PSG noise
		bIsPCM = false;
		pNoteDef = (const SBNKNOTEDEF*) pInstData;
		if (fRecord == 3)
		{
			nCh = Chn_Alloc(TYPE_NOISE, trk->prio);
			if (nCh < 0) return -1;
			chn = FSS_Channels + nCh;
			chn->reg.CR = SOUND_FORMAT_PSG | SCHANNEL_ENABLE;
		}else
		{
			nCh = Chn_Alloc(TYPE_PSG, trk->prio);
			if (nCh < 0) return -1;
			chn = FSS_Channels + nCh;
			chn->reg.CR = SOUND_FORMAT_PSG | SCHANNEL_ENABLE | SOUND_DUTY(pNoteDef->wavid & 0x7);
		}
		// TODO: figure out what pNoteDef->tnote means for PSG channels
		chn->reg.TIMER = -SOUND_FREQ(440*8); // key #69 (A4)
	}else if (fRecord == 16)
	{
		if (!((pInstData[0] <= key) && (key <= pInstData[1]))) return -1;
		int rn = key - (int)pInstData[0];
		int offset = 2 + rn*(2+sizeof(SBNKNOTEDEF));
		fRecord = pInstData[offset];
		pInstData += offset + 2;
		goto _ReadRecord;
	}else if (fRecord == 17)
	{
		int reg;
		for(reg = 0; reg < 8; reg ++)
			if (key <= pInstData[reg]) break;
		if (reg == 8) return -1;
		
		int offset = 8 + reg*(2+sizeof(SBNKNOTEDEF));
		fRecord = pInstData[offset];
		pInstData += offset + 2;
		goto _ReadRecord;
	}

	if (bIsPCM)
	{
		nCh = Chn_Alloc(TYPE_PCM, trk->prio);
		if (nCh < 0) return -1;
		chn = FSS_Channels + nCh;

		pWavInfo = GetWavInWar(trk->ply->pWar[pNoteDef->warid], pNoteDef->wavid);
		chn->reg.CR = SOUND_FORMAT(pWavInfo->nWaveType & 3) | SOUND_LOOP(pWavInfo->bLoop) | SCHANNEL_ENABLE;
		chn->reg.SOURCE = (u32) GetSample(pWavInfo);
		chn->reg.TIMER = pWavInfo->nTime;
		chn->reg.REPEAT_POINT = pWavInfo->nLoopOffset;
		chn->reg.LENGTH = pWavInfo->nNonLoopLen;
	}

	chn->state = CS_START;
	chn->trackId = trkNo;
	chn->flags = 0;
	chn->prio = trk->prio;
	chn->key = key;
	chn->orgKey = bIsPCM ? pNoteDef->tnote : 69;
	chn->patch = trk->patch;
	chn->velocity = Cnv_Sust(vel);
	chn->pan = (int)pNoteDef->pan - 64;
	chn->modDelayCnt = 0;
	chn->modCounter = 0;
	FSS_NoteLengths[nCh] = len;

	chn->attackLvl = Cnv_Attack(trk->a == 0xFF ? pNoteDef->a : trk->a);
	chn->decayRate = Cnv_Fall(trk->d == 0xFF ? pNoteDef->d : trk->d);
	chn->sustainLvl = trk->s == 0xFF ? pNoteDef->s : trk->s;
	chn->releaseRate = Cnv_Fall(trk->r == 0xFF ? pNoteDef->r : trk->r);

	Chn_UpdateVol(chn, trk);
	Chn_UpdatePan(chn, trk);
	Chn_UpdateTune(chn, trk);
	Chn_UpdateMod(chn, trk);
	Chn_UpdatePorta(chn, trk, nCh);

	trk->portaKey = key;

	return nCh;
}

int Note_On_Tie(int trkNo, int key, int vel)
{
	// Find an existing note
	register int i;
	fss_channel_t* chn;
	for (i = 0; i < 16; i ++)
	{
		chn = FSS_Channels + i;
		if (chn->state > CS_NONE && chn->trackId == trkNo && chn->state != CS_RELEASE)
			break;
	}

	if (i == 16)
		// Can't find note -> create an endless one
		return Note_On(trkNo, key, vel, -1);

	chn->flags = 0;
	chn->prio = FSS_Tracks[trkNo].prio;
	chn->key = key;
	chn->velocity = Cnv_Sust(vel);
	chn->modDelayCnt = 0;
	chn->modCounter = 0;

	fss_track_t* trk = FSS_Tracks + trkNo;
	Chn_UpdateVol(chn, trk);
	//Chn_UpdatePan(chn, trk);
	Chn_UpdateTune(chn, trk);
	Chn_UpdateMod(chn, trk);
	Chn_UpdatePorta(chn, trk, i);

	trk->portaKey = key;
	chn->flags |= F_UPDTMR;
	return i;
}

static inline int getModFlag(int type)
{
	switch(type)
	{
		case 0:  return F_UPDTMR;
		case 1:  return F_UPDVOL;
		case 2:  return F_UPDPAN;
		default: return 0;
	}
}

void Chn_UpdateTracks()
{
	register int i = 0;
	for (i = 0; i < 16; i ++)
	{
		fss_channel_t* chn = FSS_Channels + i;
		int trkn = chn->trackId;
		if (trkn == -1) continue;

		int flags = FSS_TrackUpdateFlags[trkn];
		if (!flags) continue;

		fss_track_t* trk = FSS_Tracks + trkn;
		if (flags & TUF_LEN)
		{
			int st = chn->state;
			if (st > CS_START)
			{
				if (st < CS_RELEASE && !--FSS_NoteLengths[i])
					Chn_Release(chn, i);
				if (chn->manualSweep && chn->sweepCnt < chn->sweepLen)
					chn->sweepCnt ++;
			}
		}
		if (flags & TUF_VOL)
		{
			Chn_UpdateVol(chn, trk);
			chn->flags |= F_UPDVOL;
		}
		if (flags & TUF_PAN)
		{
			Chn_UpdatePan(chn, trk);
			chn->flags |= F_UPDPAN;
		}
		if (flags & TUF_TIMER)
		{
			Chn_UpdateTune(chn, trk);
			chn->flags |= F_UPDTMR;
		}
		if (flags & TUF_MOD)
		{
			int oldType = chn->modType;
			int newType = trk->modType;
			Chn_UpdateMod(chn, trk);
			if (oldType != newType)
				chn->flags |= getModFlag(oldType) | getModFlag(newType);
		}
	}
	memset(FSS_TrackUpdateFlags, 0, sizeof(FSS_TrackUpdateFlags));
}

void Track_ReleaseAllNotes(int handle)
{
	register int i;
	for (i = 0; i < 16; i ++)
	{
		fss_channel_t* chn = FSS_Channels + i;
		if (chn->state > CS_NONE && chn->trackId == handle && chn->state != CS_RELEASE)
			Chn_Release(chn, i);
	}
}

void Track_Run(int handle)
{
	fss_track_t* trk = FSS_Tracks + handle;

	// Indicate "heartbeat" for this track
	FSS_TrackUpdateFlags[handle] |= TUF_LEN;

	// Exit if the track has already ended
	if (trk->state & TS_END) return;

	if (trk->wait)
	{
		trk->wait --;
		if (trk->wait) return;
	}

	const u8** pData = &trk->pos;

	while (!trk->wait)
	{
		int cmd = read8(pData);
		if (cmd < 0x80)
		{
			// Note on
			int key = cmd + trk->transpose;
			int vel = read8(pData);
			int len = readvl_o(pData, trk);
			if (trk->state & TS_NOTEWAIT) trk->wait = len;
			if (!(trk->state & TS_TIEBIT))
				Note_On(handle, key, vel, len);
			else
				Note_On_Tie(handle, key, vel);
		}else switch(cmd)
		{
			//-----------------------------------------------------------------
			// Main commands
			//-----------------------------------------------------------------

			case SSEQ_CMD_REST:
			{
				trk->wait = readvl_o(pData, trk);
				break;
			}

			case SSEQ_CMD_PATCH:
			{
				trk->patch = readvl_o(pData, trk);
				break;
			}

			case SSEQ_CMD_GOTO:
			{
				*pData = trk->ply->pSeqData + read24(pData);
				break;
			}

			case SSEQ_CMD_CALL:
			{
				const byte_t* dest = trk->ply->pSeqData + read24(pData);
				trk->stack[trk->stackPos++] = *pData;
				*pData = dest;
				break;
			}

			case SSEQ_CMD_RET:
			{
				*pData = trk->stack[--trk->stackPos];
				break;
			}

			case SSEQ_CMD_PAN:
			{
				int pan = read8_o(pData, trk) - 64;
				trk->pan = pan;
				FSS_TrackUpdateFlags[handle] |= TUF_PAN;
				break;
			}

			case SSEQ_CMD_VOL:
			{
				int vol = read8_o(pData, trk);
				trk->vol = vol;
				FSS_TrackUpdateFlags[handle] |= TUF_VOL;
				break;
			}

			case SSEQ_CMD_MASTERVOL:
			{
				trk->ply->masterVol = Cnv_Sust(read8_o(pData, trk));
				register int i;
				fss_player_t* ply = trk->ply;
				u8* tIds = ply->trackIds;
				for (i = 0; i < ply->nTracks; i ++)
					FSS_TrackUpdateFlags[*tIds++] |= TUF_VOL;
				break;
			}

			case SSEQ_CMD_PRIO:
			{
				trk->prio = trk->ply->prio + read8(pData);
				// Update here?
				break;
			}

			case SSEQ_CMD_NOTEWAIT:
			{
				trk->state &= ~TS_NOTEWAIT;
				trk->state |= read8(pData) ? TS_NOTEWAIT : 0;
				break;
			}

			case SSEQ_CMD_TIE:
			{
				trk->state &= ~TS_TIEBIT;
				trk->state |= read8(pData) ? TS_TIEBIT : 0;
				Track_ReleaseAllNotes(handle);
				break;
			}

			case SSEQ_CMD_EXPR:
			{
				int expr = read8_o(pData, trk);
				trk->expr = expr;
				FSS_TrackUpdateFlags[handle] |= TUF_VOL;
				break;
			}

			case SSEQ_CMD_TEMPO:
			{
				trk->ply->tempo = read16(pData);
				break;
			}

			case SSEQ_CMD_END:
			{
				trk->state |= TS_END;
				return;
			}

			case SSEQ_CMD_LOOPSTART:
			{
				trk->loopCount[trk->stackPos] = read8_o(pData, trk);
				trk->stack[trk->stackPos ++] = *pData;
				break;
			}

			case SSEQ_CMD_LOOPEND:
			{
				u8* nR = trk->loopCount + trk->stackPos - 1;
				if (!*nR || !--*nR) trk->stackPos --;
				else *pData = trk->stack[trk->stackPos - 1];
				break;
			}

			//-----------------------------------------------------------------
			// Tuning commands
			//-----------------------------------------------------------------

			case SSEQ_CMD_TRANSPOSE:
			{
				trk->transpose = (s8)(u8)read8_o(pData, trk);
				break;
			}

			case SSEQ_CMD_PITCHBEND:
			{
				trk->pitchBend = (s8)(u8)read8_o(pData, trk);
				FSS_TrackUpdateFlags[handle] |= TUF_TIMER;
				break;
			}

			case SSEQ_CMD_PITCHBENDRANGE:
			{
				trk->pitchBendRange = read8(pData);
				FSS_TrackUpdateFlags[handle] |= TUF_TIMER;
				break;
			}

			//-----------------------------------------------------------------
			// Envelope-related commands
			//-----------------------------------------------------------------

			case SSEQ_CMD_ATTACK:
			{
				trk->a = read8_o(pData, trk);
				break;
			}

			case SSEQ_CMD_DECAY:
			{
				trk->d = read8_o(pData, trk);
				break;
			}

			case SSEQ_CMD_SUSTAIN:
			{
				trk->s = read8_o(pData, trk);
				break;
			}

			case SSEQ_CMD_RELEASE:
			{
				trk->r = read8_o(pData, trk);
				break;
			}

			//-----------------------------------------------------------------
			// Portamento-related commands
			//-----------------------------------------------------------------

			case SSEQ_CMD_PORTAKEY:
			{
				trk->portaKey = read8(pData) + trk->transpose;
				trk->state |= TS_PORTABIT;
				// Update here?
				break;
			}

			case SSEQ_CMD_PORTAFLAG:
			{
				trk->state &= ~TS_PORTABIT;
				trk->state |= read8(pData) ? TS_PORTABIT : 0;
				// Update here?
				break;
			}

			case SSEQ_CMD_PORTATIME:
			{
				trk->portaTime = read8_o(pData, trk);
				// Update here?
				break;
			}

			case SSEQ_CMD_SWEEPPITCH:
			{
				trk->sweepPitch = (s16)(u16)read16_o(pData, trk);
				// Update here?
				break;
			}

			//-----------------------------------------------------------------
			// Modulation-related commands
			//-----------------------------------------------------------------

			case SSEQ_CMD_MODDEPTH:
			{
				trk->modDepth = read8_o(pData, trk);
				FSS_TrackUpdateFlags[handle] |= TUF_MOD;
				break;
			}

			case SSEQ_CMD_MODSPEED:
			{
				trk->modSpeed = read8_o(pData, trk);
				FSS_TrackUpdateFlags[handle] |= TUF_MOD;
				break;
			}

			case SSEQ_CMD_MODTYPE:
			{
				trk->modType = read8(pData);
				FSS_TrackUpdateFlags[handle] |= TUF_MOD;
				break;
			}

			case SSEQ_CMD_MODRANGE:
			{
				trk->modRange = read8(pData);
				FSS_TrackUpdateFlags[handle] |= TUF_MOD;
				break;
			}

			case SSEQ_CMD_MODDELAY:
			{
				trk->modDelay = read16_o(pData, trk);
				FSS_TrackUpdateFlags[handle] |= TUF_MOD;
				break;
			}

			//-----------------------------------------------------------------
			// Variable-related commands
			//-----------------------------------------------------------------

			case SSEQ_CMD_RANDOM:
			{
				trk->state |= TS_OVRBIT;
				int min = (s16)(u16)read16(pData);
				int max = (s16)(u16)read16(pData);
				trk->ovrValue = Cnv_Random() % (max-min) + min;
				break;
			}

			case SSEQ_CMD_VAR:
			{
				trk->state |= TS_OVRBIT | TS_OVRVARBIT;
				break;
			}

			case SSEQ_CMD_IF:
			{
				if (trk->state & TS_IFSKIPBIT)
				{
					trk->state &= ~TS_IFSKIPBIT;
					// TODO: skip next command
				}
				break;
			}

			case SSEQ_CMD_PRINTVAR:
			{
				break;
			}

			case SSEQ_CMD_UNSUP2_LO ... SSEQ_CMD_UNSUP2_HI: // TODO
			{
				*pData += 3;
			}
		}
	}
}
