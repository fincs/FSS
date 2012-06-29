#include "fss7.h"

int fssFifoCh;

static void fssDatamsgHandler(int nBytes, void* user_data);
static void fssAddressHandler(void* address, void* user_data);

int arm7_main(int fifoCh)
{
	fssFifoCh = fifoCh;
	Snd_Init();
	coopFifoSetDatamsgHandler(fifoCh, fssDatamsgHandler, NULL);
	fifoSetAddressHandler(fifoCh, fssAddressHandler, NULL);
	return 0;
}

void arm7_fini()
{
	coopFifoSetDatamsgHandler(fssFifoCh, NULL, NULL);
	fifoSetAddressHandler(fssFifoCh, NULL, NULL);
	Snd_Deinit();
}

void fssDatamsgHandler(int nBytes, void* user_data)
{
	int rBuf[(nBytes + 3)>>2];
	fifoGetDatamsg(fssFifoCh, nBytes, (u8*)rBuf);

	switch (rBuf[0])
	{
		case FSSFIFO_INIT:
			FSS_SharedWork = ((msg_init*)rBuf)->sharedWork;
			fifoReturn(fssFifoCh, 0);
			break;
		case FSSFIFO_PLAYSMP:
			Cmd_PlaySmp((msg_playsmp*) rBuf);
			break;
		case FSSFIFO_PLAYPSG:
			Cmd_PlayPsg((msg_playpsg*) rBuf);
			break;
		case FSSFIFO_PLAYNOISE:
			Cmd_PlayNoise((msg_playnoise*) rBuf);
			break;
		case FSSFIFO_CHNISACTIVE:
			Cmd_ChnIsActive((msg_singlech*) rBuf);
			break;
		case FSSFIFO_CHNSTOP:
			Cmd_ChnStop((msg_singlech*) rBuf);
			break;
		case FSSFIFO_CHNSETPARAM:
			Cmd_ChnSetParam((msg_chnsetparam*) rBuf);
			break;
		case FSSFIFO_PLAYERALLOC:
			Cmd_PlayerAlloc((msg_playeralloc*) rBuf);
			break;
		case FSSFIFO_PLAYERSETUP:
			Cmd_PlayerSetup((msg_playersetup*) rBuf);
			break;
		case FSSFIFO_PLAYERPLAY:
			Cmd_PlayerPlay((msg_singleplayer*) rBuf);
			break;
		case FSSFIFO_PLAYERSTOP:
		{
			msg_genericWithParam* msg = (msg_genericWithParam*) rBuf;
			Player_Stop(msg->which, msg->param);
			fifoReturn(fssFifoCh, 0);
			break;
		}
		case FSSFIFO_PLAYERSETPAUSE:
		{
			msg_genericWithParam* msg = (msg_genericWithParam*) rBuf;
			Player_SetPause(msg->which, msg->param);
			fifoReturn(fssFifoCh, 0);
			break;
		}
		case FSSFIFO_PLAYERFREE:
			Player_Free(((msg_generic*) rBuf)->which);
			fifoReturn(fssFifoCh, 0);
			break;
		case FSSFIFO_SETOUTPFLAGS:
			Snd_SetOutputFlags(((msg_generic*) rBuf)->which);
			fifoReturn(fssFifoCh, 0);
			break;
		case FSSFIFO_LOCKCHANNELS:
		{
			int which = ((msg_lockchns*) rBuf)->mask;
			if (which & BIT(16))
				Chn_Lock(which);
			else
				Chn_Unlock(which);
			fifoReturn(fssFifoCh, 0);
		}
		case FSSFIFO_CAPSETUP:
			Cap_Setup((msg_capsetup*) rBuf);
			fifoReturn(fssFifoCh, 0);
			break;
		case FSSFIFO_CAPREPLAYSETUP:
			Cap_SetupReplay((msg_caprplsetup*) rBuf);
			fifoReturn(fssFifoCh, 0);
			break;
		case FSSFIFO_CAPSTART:
			Cap_Start(((msg_generic*) rBuf)->which);
			fifoReturn(fssFifoCh, 0);
			break;
		case FSSFIFO_CAPREPLAYSTART:
			Cap_StartReplay(((msg_generic*) rBuf)->which);
			fifoReturn(fssFifoCh, 0);
			break;
		case FSSFIFO_CAPSTOP:
			Cap_Stop(((msg_generic*) rBuf)->which);
			fifoReturn(fssFifoCh, 0);
			break;
		case FSSFIFO_CAPREPLAYSTOP:
			Cap_StopReplay(((msg_generic*) rBuf)->which);
			fifoReturn(fssFifoCh, 0);
			break;
	}
}

void fssAddressHandler(void* rBuf, void* user_data)
{
	switch (*(int*)rBuf)
	{
		case FSSFIFO_PLAYERREAD:
			Cmd_PlayerRead((msg_ptrwithparam*) rBuf);
			break;
		case FSSFIFO_TRACKREAD:
			Cmd_TrackRead((msg_ptrwithparam*) rBuf);
			break;
		case FSSFIFO_CHANNELREAD:
			Cmd_ChannelRead((msg_ptrwithparam*) rBuf);
			break;
	}
}
