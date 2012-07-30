#include "fss7.h"

static int nChans;
static int timer;
static int format;

static void* aBuf;
static int aBufLen; // in WORDS and per channel

void Cmd_StrmSetup(msg_strmsetup* buf)
{
	Strm_Setup(buf->fmt, buf->timer, buf->buf, buf->buflen);
	fifoReturn(fssFifoCh, 0);
}

void Strm_Setup(int _fmt, int _timer, void* buf, int buflen)
{
	nChans = (_fmt >> 2) + 1;
	timer = _timer;
	format = SCHANNEL_ENABLE | SOUND_VOL(0x7F) | ((_fmt & 0x3) << 29) | SOUND_REPEAT;
	aBuf = buf;
	aBufLen = buflen;
}

void Strm_SetStatus(bool bStatus)
{
	word_t mask = BIT(4) | (nChans == 2 ? BIT(5) : 0);
	if (bStatus)
	{
		Chn_Lock(mask);
		int i;

		// Setup sound channels
		for (i = 0; i < nChans; i ++)
		{
			SCHANNEL_CR(4+i) = 0;
			SCHANNEL_TIMER(4+i) = -timer;
			SCHANNEL_SOURCE(4+i) = (u32)aBuf + i*aBufLen*4;
			SCHANNEL_LENGTH(4+i) = aBufLen;
			SCHANNEL_REPEAT_POINT(4+i) = 0;
		}

		// Setup timers
		TIMER_CR(2) = 0;
		TIMER_CR(3) = 0;
		TIMER_DATA(2) = -(timer << 1);
		TIMER_DATA(3) = 0;

		// Fire channels & timers
		{
			int cS = enterCriticalSection();
			int fmt = format; // fast access

			if (nChans == 2)
			{
				SCHANNEL_CR(4) = fmt | SOUND_PAN(0x00);
				SCHANNEL_CR(5) = fmt | SOUND_PAN(0x7F);
			}else
				SCHANNEL_CR(4) = fmt | SOUND_PAN(0x40);

			TIMER_CR(2) = TIMER_ENABLE;
			TIMER_CR(3) = TIMER_ENABLE | TIMER_CASCADE;

			leaveCriticalSection(cS);
		}

	}else
	{
		TIMER_CR(2) = 0;
		TIMER_CR(3) = 0;
		SCHANNEL_CR(4) = 0;
		if (nChans == 2)
			SCHANNEL_CR(5) = 0;
		Chn_Unlock(mask);
	}
}

int Strm_GetPos()
{
	return TIMER_DATA(3);
}
