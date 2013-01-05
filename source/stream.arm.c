#include "fss_private.h"

static fss_stream_t strmData;
static void* strmUserData;
static bool strmStatus;
static void* strmBuffer;
static int strmBufSize;
static word_t strmSmpMask;
static int strmLastTimer;
static int strmPos;
static int strmWorkPos;
static int strmTodo;

static inline void strmResetVars()
{
	strmPos = 0;
	strmWorkPos = 0;
	strmTodo = 0;
	strmLastTimer = 0;
}

static inline int strmCallback(void* outBuf, int reqSamples)
{
	return strmData.callback(strmUserData, outBuf, &strmData, reqSamples);
}

void deinterleave16(s16* inBuf, s16* leftBuf, int bytes, s16* rightBuf);
void deinterleave8(s8* inBuf, s8* leftBuf, int bytes, s8* rightBuf);

static inline void copySamples(int smpSizeShift, void* inBuf, int bufSamples, int byteCount)
{
	if (strmSmpMask) // Deinterleave
	{
		if (smpSizeShift)
			deinterleave16(inBuf, (s16*)strmBuffer + strmPos, byteCount, (s16*)strmBuffer + bufSamples + strmPos);
		else
			deinterleave8(inBuf, (s8*)strmBuffer + strmPos, byteCount, (s8*)strmBuffer + bufSamples + strmPos);
	}else // Copy
	{
		// Either only channel or left channel
		int q = strmPos << smpSizeShift;
		memcpy((char*)strmBuffer + q, inBuf, byteCount);

		if (strmData.format & BIT(2)) // Stereo
		{
			// Right channel
			int p = strmData.bufSampleCount << smpSizeShift;
			memcpy((char*)strmBuffer + p + q, (char*)inBuf + p, byteCount);
		}
	}
}

static inline int FSS_StreamRead(int reqSamples)
{
	if (!reqSamples)
		return 0;

	int format = strmData.format;
	int smpSizeShift = format & BIT(0);
	int workSmpSizeShift = smpSizeShift;
	if (strmData.format & BIT(3))
		workSmpSizeShift ++;
	int bufSamples = strmData.bufSampleCount;
	reqSamples += strmTodo;
	if (reqSamples >= bufSamples)
		reqSamples = bufSamples;
	strmTodo = 0;
	void* workBuf = (char*)strmBuffer + (strmBufSize >> 1);
#ifdef DEBUG
	printf("reqSmp %d", reqSamples);
#endif
	int orgReqSamples = reqSamples;
	while (reqSamples > strmSmpMask)
	{
		int nsamples = strmCallback((char*)workBuf + (strmWorkPos << workSmpSizeShift), reqSamples - strmWorkPos);
#ifdef DEBUG
		printf(" - %d{%d}", nsamples, strmPos);
#endif
		if (nsamples < 0)
			return orgReqSamples - reqSamples;
		if (nsamples == 0)
		{
			// Nothing can be done about this - the callback REFUSES to decode this little :D
			// So just save how many samples we NEED to decode on the next call.
			strmTodo = reqSamples - strmWorkPos;
			break;
		}
		strmWorkPos = nsamples & strmSmpMask;
		nsamples &= ~strmSmpMask;
		reqSamples -= nsamples;
		void* excessSamples = (char*)workBuf + (nsamples << workSmpSizeShift);

		void* copyPos = workBuf;
		{
			int strmEndPos = strmPos + nsamples;
			if (strmEndPos > bufSamples)
			{
				nsamples = bufSamples - strmPos;
				copySamples(smpSizeShift, copyPos, bufSamples, nsamples << workSmpSizeShift);

				copyPos = (char*)copyPos + (nsamples << workSmpSizeShift);
				strmPos = 0;
				nsamples = strmEndPos - bufSamples;
			}
		}

		copySamples(smpSizeShift, copyPos, bufSamples, nsamples << workSmpSizeShift);
		strmPos += nsamples;
		if (strmPos == bufSamples) // the >= condition can't happen because it's handled above
			strmPos = 0;

		if (strmWorkPos)
			// Copy excess samples to beginning of buf
			memcpy(workBuf, excessSamples, strmWorkPos << workSmpSizeShift);
	}
#ifdef DEBUG
	printf("\n");
#endif
	DC_FlushRange(strmBuffer, strmBufSize >> 1);
	FeOS_DrainWriteBuffer();
	return orgReqSamples - reqSamples;
}

#define SHIFTER(x) (!!(x))
bool FSS_StreamSetup(const fss_stream_t* pStream, void* userData)
{
	// Do not allow setting a stream up if there's already one
	if (strmBuffer)
		return false;

	int smpCount = pStream->bufSampleCount;
	int format = pStream->format;
	int timer = pStream->timer;
	strmSmpMask = (format & BIT(3)) ? 3 : 0; // stereo interleaved sources NEED word-alignment
	// Round the buffer size obeying the sample mask, or at least word-aligning it
	smpCount &= strmSmpMask ? (~strmSmpMask) : (format & BIT(0)) ? (~1) : (~3);
	strmBufSize = smpCount << SHIFTER(format & BIT(0));
	int bufSizeWords = strmBufSize >> 2;
	strmBufSize <<= SHIFTER(format & BIT(2)) + 1; // Stereo and work buf.

	strmUserData = userData;
	strmData.timer = timer;
	strmData.format = format;
	strmData.bufSampleCount = smpCount;
	strmData.callback = pStream->callback;
	strmStatus = false;

	strmBuffer = malloc(strmBufSize);
	if (!strmBuffer)
		return false;

	FSS_RawStreamSetup(strmBuffer, bufSizeWords, timer, format &~ BIT(3));
	strmResetVars();

	return true;
}

bool FSS_StreamExists()
{
	return strmBuffer != 0;
}

int FSS_StreamMain()
{
	if (!strmStatus) return 0;

	int curTimer = FSS_RawStreamGetPos() &~ strmSmpMask; // word-align timer if necessary
	int diff = (curTimer - strmLastTimer) & 0xFFFF;
	strmLastTimer = curTimer;

	if (!diff) return 0; // nothing to do

	// We need to update!
	return FSS_StreamRead(diff);
}

int FSS_StreamSetStatus(bool bActive)
{
	int rc = 1;

	if (!strmBuffer) return -1;
	if (strmStatus == bActive) return rc; // nothing to do

	if (!bActive)
		strmResetVars();
	else
	{
		// Prefill
		rc = FSS_StreamRead(strmData.bufSampleCount);
		if (rc <= 0)
			return rc; // FAIL
	}

	FSS_RawStreamSetStatus(bActive);
	strmStatus = bActive;
	return rc;
}

bool FSS_StreamGetStatus()
{
	return strmBuffer && strmStatus;
}

void FSS_StreamFree()
{
	if (strmBuffer)
	{
		if (strmStatus)
			FSS_StreamSetStatus(false);
		free(strmBuffer);
		strmBuffer = NULL;
	}
}
