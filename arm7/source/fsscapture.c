#include "fss7.h"

#define REG_SNDCAPxCNT(n) (*((vu8*)0x04000508+(n)))
#define REG_SNDCAPxDAD(n) (*((vu32*)0x04000510+2*(n)))
#define REG_SNDCAPxLEN(n) (*((vu16*)0x04000514+4*(n)))

void Cap_Setup(msg_capsetup* pArgs)
{
	int x = !!(pArgs->flags & BIT(8));
	int ch = 2*x + 1;
	Chn_Lock(BIT(ch));
	REG_SNDCAPxCNT(x) = pArgs->flags & 0xF;
	REG_SNDCAPxDAD(x) = (u32) pArgs->buffer;
	REG_SNDCAPxLEN(x) = pArgs->bufSizeInWords;
	SCHANNEL_CR(ch) =
		((pArgs->flags & BIT(3)) ? SOUND_FORMAT_8BIT : SOUND_FORMAT_16BIT) |
		((pArgs->flags & BIT(2)) ? SOUND_ONE_SHOT : SOUND_REPEAT);
	SCHANNEL_SOURCE(ch) = (u32) pArgs->buffer;
	SCHANNEL_REPEAT_POINT(ch) = 0;
	SCHANNEL_LENGTH(ch) = pArgs->bufSizeInWords;
	SCHANNEL_TIMER(ch) = -pArgs->timer;
}

void Cap_SetupReplay(msg_caprplsetup* pArgs)
{
	int ch = 2*pArgs->x + 1;
	SCHANNEL_CR(ch) |=
		SOUND_VOL(pArgs->vol & 0x7F) |
		SOUND_VOLDIV(pArgs->vol >> 8) |
		SOUND_PAN(pArgs->pan);
}

void Cap_Start(int x)
{
	if (x & BIT(0))
		REG_SNDCAP0CNT |= BIT(7);
	if (x & BIT(1))
		REG_SNDCAP1CNT |= BIT(7);
}

void Cap_StartReplay(int x)
{
	if (x & BIT(0))
		SCHANNEL_CR(1) |= SCHANNEL_ENABLE;
	if (x & BIT(1))
		SCHANNEL_CR(3) |= SCHANNEL_ENABLE;
}

void Cap_Stop(int x)
{
	if (x & BIT(0))
		REG_SNDCAP0CNT &= ~BIT(7);
	if (x & BIT(1))
		REG_SNDCAP1CNT &= ~BIT(7);

	Chn_Unlock(((x & BIT(0)) << 1) | ((x & BIT(1)) << 2));
}

void Cap_StopReplay(int x)
{
	if (x & BIT(0))
		SCHANNEL_CR(1) &= ~SCHANNEL_ENABLE;
	if (x & BIT(1))
		SCHANNEL_CR(3) &= ~SCHANNEL_ENABLE;
}
