#pragma once

void Snd_Init();
void Snd_Deinit();

void Snd_SetOutputFlags(int flags);

int Cnv_Attack(int x);
int Cnv_Fall(int x);
int Cnv_Sust(int x); // quadratic volume curve
int Cnv_Vol(int x); // linear volume curve

enum { TYPE_PCM, TYPE_PSG, TYPE_NOISE };
int Chn_Alloc(int type, int prio);

void Chn_Lock(u32 mask);
void Chn_Unlock(u32 mask);

void Chn_Kill(fss_channel_t* pCh, int nCh);
void Chn_Release(fss_channel_t* pCh, int nCh);

u16 Timer_Adjust(u16 basetmr, int pitch);

int Cnv_Sine(int arg);

void Cmd_PlaySmp(msg_playsmp* pArgs);
void Cmd_PlayPsg(msg_playpsg* pArgs);
void Cmd_PlayNoise(msg_playnoise* pArgs);

void Cmd_ChnIsActive(msg_singlech* pArgs);
void Cmd_ChnStop(msg_singlech* pArgs);
void Cmd_ChnSetParam(msg_chnsetparam* pArgs);

int Player_Alloc(int prio);
void Player_Free(int handle);
void Player_Run(int handle);

bool Player_Setup(int handle, const byte_t* pSeq, const byte_t* pBnk, const byte_t* const pWar[4]);
void Player_Play(int handle);
void Player_Stop(int handle, bool bKillSound);
void Player_SetPause(int handle, bool bPause);

int Track_Alloc();
void Track_Free(int handle);
void Track_ReleaseAllNotes(int handle);

void Chn_UpdateTracks();

void Cmd_PlayerAlloc(msg_playeralloc* pArgs);
void Cmd_PlayerSetup(msg_playersetup* pArgs);
void Cmd_PlayerPlay(msg_singleplayer* pArgs);
void Cmd_PlayerSetParam(msg_chnsetparam* pArgs);

void Cmd_PlayerRead(fss_plydata_t* data, int param);
void Cmd_TrackRead(fss_trkdata_t* data, int param);
void Cmd_ChannelRead(fss_chndata_t* data, int param);

enum { WUF_PLAYERS = BIT(0), WUF_TRACKS = BIT(1), WUF_CHANNELS = BIT(2) };
extern int FSS_WorkUpdFlags;

void Cmd_MicStart(msg_micstart* data);
void Cmd_MicGetPos();
void Cmd_MicStop();

void Cmd_StrmSetup(msg_strmsetup* buf);
void Strm_Setup(int _fmt, int _timer, void* buf, int buflen);
void Strm_SetStatus(bool bStatus);
int Strm_GetPos();
