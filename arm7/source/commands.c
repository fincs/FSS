#include "fss7.h"

static inline u32 cnvType(int x) { return (x & 3) << 29; }
static inline u32 cnvLoop(int x) { return (x & FMT_LOOP) ? SOUND_REPEAT : SOUND_ONE_SHOT; }

static void PrepareCh(fss_channel_t* pCh, int nCh)
{
	memset(pCh, 0, sizeof(fss_channel_t));
	pCh->state = CS_NONE;
	pCh->trackId = -1;

	/*
	pCh->flags = 0;
	pCh->velocity = 0;
	pCh->extPan = 0;
	pCh->extTune = 0;

	pCh->attackLvl = 0;
	*/
	pCh->sustainLvl = 0x7F;
	//pCh->decayRate = 0;
	pCh->releaseRate = 0xFFFF;

	FSS_NoteLengths[nCh] = -1;
}

void Cmd_PlaySmp(msg_playsmp* pArgs)
{
	int prio = pArgs->prio;
	int nCh = Chn_Alloc(TYPE_PCM, prio);
	if (nCh == -1) fifoReturn(fssFifoCh, -1);

	fss_channel_t* pCh = FSS_Channels + nCh;
	PrepareCh(pCh, nCh);

	pCh->prio = prio;
	pCh->pan = (int)pArgs->pan - 64;
	pCh->extAmpl = Cnv_Vol(pArgs->vol);

	pCh->reg.SOURCE = (u32)pArgs->data;
	pCh->reg.TIMER = pArgs->timer;
	pCh->reg.REPEAT_POINT = pArgs->lpoint;
	pCh->reg.LENGTH = pArgs->len;
	pCh->reg.CR = SCHANNEL_ENABLE | cnvType(pArgs->fmt) | cnvLoop(pArgs->fmt);

	pCh->state = CS_START;
	fifoReturn(fssFifoCh, nCh);
}

void Cmd_PlayPsg(msg_playpsg* pArgs)
{
	int prio = pArgs->prio;
	int nCh = Chn_Alloc(TYPE_PSG, prio);
	if (nCh == -1) fifoReturn(fssFifoCh, -1);

	fss_channel_t* pCh = FSS_Channels + nCh;
	PrepareCh(pCh, nCh);

	pCh->prio = prio;
	pCh->pan = (int)pArgs->pan - 64;
	pCh->extAmpl = Cnv_Vol(pArgs->vol);

	pCh->reg.TIMER = pArgs->timer;
	pCh->reg.CR = SCHANNEL_ENABLE | SOUND_FORMAT_PSG | SOUND_DUTY(pArgs->duty);

	pCh->state = CS_START;
	fifoReturn(fssFifoCh, nCh);
}

void Cmd_PlayNoise(msg_playnoise* pArgs)
{
	int prio = pArgs->prio;
	int nCh = Chn_Alloc(TYPE_NOISE, prio);
	if (nCh == -1) fifoReturn(fssFifoCh, -1);

	fss_channel_t* pCh = FSS_Channels + nCh;
	PrepareCh(pCh, nCh);

	pCh->prio = prio;
	pCh->pan = (int)pArgs->pan - 64;
	pCh->extAmpl = Cnv_Vol(pArgs->vol);

	pCh->reg.TIMER = pArgs->timer;
	pCh->reg.CR = SCHANNEL_ENABLE | SOUND_FORMAT_PSG;

	pCh->state = CS_START;
	fifoReturn(fssFifoCh, nCh);
}

void Cmd_ChnIsActive(msg_singlech* pArgs)
{
	fifoReturn(fssFifoCh, FSS_Channels[pArgs->ch].state != CS_NONE);
}

void Cmd_ChnStop(msg_singlech* pArgs)
{
	fss_channel_t* pCh = FSS_Channels + pArgs->ch;
	pCh->prio = 1;
	pCh->state = CS_RELEASE;
	fifoReturn(fssFifoCh, 0);
}

void Cmd_ChnSetParam(msg_chnsetparam* pArgs)
{
	fss_channel_t* pCh = FSS_Channels + pArgs->ch;
	switch (pArgs->tParam)
	{
		case PRM_VOL:
			pCh->extAmpl = Cnv_Vol(pArgs->vParam);
			pCh->flags |= F_UPDVOL;
			break;
		case PRM_PAN:
			pCh->pan = pArgs->vParam - 64;
			pCh->flags |= F_UPDPAN;
			break;
		case PRM_TIMER:
			pCh->reg.TIMER = pArgs->vParam;
			pCh->flags |= F_UPDTMR;
			break;
		case PRM_DUTY:
			pCh->reg.CR &= ~SOUND_DUTY(7);
			pCh->reg.CR |= SOUND_DUTY(pArgs->vParam);
			pCh->flags |= F_UPDPAN; // HACKHACK
			break;
		case PRM_TUNE:
			pCh->extTune = pArgs->vParam;
			pCh->flags |= F_UPDTMR;
			break;
	}
	fifoReturn(fssFifoCh, 0);
}

void Cmd_PlayerAlloc(msg_playeralloc* pArgs)
{
	fifoReturn(fssFifoCh, Player_Alloc(pArgs->prio));
}

void Cmd_PlayerSetup(msg_playersetup* pArgs)
{
	fifoReturn(fssFifoCh, Player_Setup(pArgs->playerId, pArgs->pSeq, pArgs->pBnk, pArgs->pWar));
}

void Cmd_PlayerPlay(msg_singleplayer* pArgs)
{
	Player_Play(pArgs->playerId);
	fifoReturn(fssFifoCh, 0);
}

void Cmd_PlayerSetParam(msg_chnsetparam* pArgs)
{
	fss_player_t* ply = FSS_Players + pArgs->ch;
	int updMask = 0;
	switch (pArgs->tParam)
	{
		case PRM_VOL:
			ply->userVol = Cnv_Vol(pArgs->vParam);
			updMask = TUF_VOL;
			break;
		case PRM_TUNE:
			ply->userPitch = (u16)pArgs->vParam;
			updMask = TUF_TIMER;
			break;
		case PRM_TIMER:
			ply->tempoRate = (u16)pArgs->vParam;
			break;
	}
	if (updMask)
	{
		u8* tIds = ply->trackIds;
		int i;
		for (i = 0; i < ply->nTracks; i ++)
			FSS_TrackUpdateFlags[*tIds++] |= TUF_VOL;
	}
	fifoReturn(fssFifoCh, 0);
}

void Cmd_PlayerRead(fss_plydata_t* data, int param)
{
	fss_player_t* ply = FSS_Players + param;
	data->state = ply->state;
	if (data->state == PS_NONE)
		return;

	data->nTracks = ply->nTracks;
	data->tempo = ply->tempo;
	memcpy(data->trackIds, ply->trackIds, ply->nTracks);
}

void Cmd_TrackRead(fss_trkdata_t* data, int param)
{
	fss_track_t* trk = FSS_Tracks + param;
	data->state = trk->state;
	if (!(data->state & TS_ALLOCBIT))
		return;

	data->number = trk->num;
	data->playerId = trk->ply - FSS_Players;
	data->pos = trk->pos;
	data->patch = trk->patch;
	data->vol = trk->vol;
	data->expr = trk->expr;
	data->pan = trk->pan + 64;
	data->bend = trk->pitchBend;
	data->bendRange = trk->pitchBendRange;
}

void Cmd_ChannelRead(fss_chndata_t* data, int param)
{
	fss_channel_t* chn = FSS_Channels + param;
	data->state = chn->state;
	if (data->state == CS_NONE)
		return;

	data->trackId = chn->trackId;
	data->pan = (chn->reg.CR >> 16) & 0x7F;
	data->key = chn->key;
	data->vol = FSS_ChnVol[param];
	if (data->vol == 0x7FF)
		data->vol = 0;
	data->patch = chn->patch;
}
