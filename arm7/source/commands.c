#include "fss7.h"

static inline u32 cnvType(int x) { return (x & 3) << 29; }
static inline u32 cnvLoop(int x) { return (x & FMT_LOOP) ? SOUND_REPEAT : SOUND_ONE_SHOT; }
static inline u16 cnvPrio(int x) { return (x << 8) | 0x80; }

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

	FSS_ChnVol[nCh] = 0;
	FSS_NoteLengths[nCh] = -1;
}

void Cmd_PlaySmp(msg_playsmp* pArgs)
{
	int prio = cnvPrio(pArgs->prio);
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
	int prio = cnvPrio(pArgs->prio);
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
	int prio = cnvPrio(pArgs->prio);
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

void Cmd_PlayerRead(msg_ptrwithparam* pArgs)
{
	fss_plydata_t* data = (fss_plydata_t*) pArgs->ptr;
	fss_player_t* ply = FSS_Players + pArgs->param;
	data->state = ply->state;
	if (data->state == PS_NONE)
		return;
	data->nTracks = ply->nTracks;
	data->tempo = ply->tempo;
	memcpy(data->trackIds, ply->trackIds, FSS_TRACKCOUNT);
	fifoReturn(fssFifoCh, 0);
}

void Cmd_TrackRead(msg_ptrwithparam* pArgs)
{
	fss_trkdata_t* data = (fss_trkdata_t*) pArgs->ptr;
	fss_track_t* trk = FSS_Tracks + pArgs->param;
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
	fifoReturn(fssFifoCh, 0);
}

void Cmd_ChannelRead(msg_ptrwithparam* pArgs)
{
	fss_chndata_t* data = (fss_chndata_t*) pArgs->ptr;
	fss_channel_t* chn = FSS_Channels + pArgs->param;
	data->state = chn->state;
	if (data->state == CS_NONE)
		return;
	data->trackId = chn->trackId;
	data->pan = chn->pan + 64;
	data->key = chn->key;
	data->vol = FSS_ChnVol[pArgs->param];
	data->patch = chn->patch;
	fifoReturn(fssFifoCh, 0);
}
