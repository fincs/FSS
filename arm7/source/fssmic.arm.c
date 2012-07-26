#include "fss7.h"

static void* micBuffer;

#define BUF8  ((s8*) micBuffer)
#define BUF16 ((s16*)micBuffer)

int micPos = -1;
int micLength;
bool micOneShot;

static void _micTimer12();
static void _micTimer8();

void Cmd_MicStart(msg_micstart* data)
{
	micOn();
	micBuffer = data->buffer;
	micPos = 0;
	micLength = data->length;
	int fmt = data->fmt;
	micOneShot = (fmt & FMT_LOOP) ? false : true;
	//fmt &= ~FMT_LOOP;

	timerStart(0, ClockDivider_1, -(int)data->timer, fmt & FMT_16BIT ? _micTimer12 : _micTimer8);
	fifoReturn(fssFifoCh, 0);
}

static void _micStop()
{
	timerStop(0);
	micOff();
	micBuffer = NULL;
	micPos = -1;
	micLength = 0;
	micOneShot = false;
}

void Cmd_MicStop()
{
	_micStop();
	fifoReturn(fssFifoCh, 0);
}

void Cmd_MicGetPos()
{
	fifoReturn(fssFifoCh, micPos);
}

// This would be easier if C had templates...
#define MIC_TIMER(funcName, bufName, micFuncName, shift, flip) \
	void funcName() \
	{ \
		int cS = enterCriticalSection(); \
		bufName[micPos++] = (micFuncName() << (shift)) ^ (flip); \
		if (micPos == micLength) \
		{ \
			if (micOneShot) _micStop(); \
			else micPos = 0; \
		} \
		leaveCriticalSection(cS); \
	}

MIC_TIMER(_micTimer12, BUF16, micReadData12, 4, 0x8000);
MIC_TIMER(_micTimer8,  BUF8,  micReadData8,  0, 0x80);

//---------------------------------------------------------------------------------
// Turn on the Microphone Amp. Code based on neimod's example.
//---------------------------------------------------------------------------------
void micSetAmp(u8 control, u8 gain) {
//---------------------------------------------------------------------------------
	SerialWaitBusy();
	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz | SPI_CONTINUOUS;
	REG_SPIDATA = PM_AMP_OFFSET;

	SerialWaitBusy();

	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz;
	REG_SPIDATA = control;

	SerialWaitBusy();

	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz | SPI_CONTINUOUS;
	REG_SPIDATA = PM_GAIN_OFFSET;

	SerialWaitBusy();

	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz;
	REG_SPIDATA = gain;
}

//---------------------------------------------------------------------------------
// Read a byte from the microphone. Code based on neimod's example.
//---------------------------------------------------------------------------------
u8 micReadData8() {
//---------------------------------------------------------------------------------
	static u16 result, result2;

	SerialWaitBusy();

	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_MICROPHONE | SPI_BAUD_2MHz | SPI_CONTINUOUS;
	REG_SPIDATA = 0xEC;  // Touchscreen command format for AUX

	SerialWaitBusy();

	REG_SPIDATA = 0x00;

	SerialWaitBusy();

	result = REG_SPIDATA;
  	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_TOUCH | SPI_BAUD_2MHz;
	REG_SPIDATA = 0x00;

	SerialWaitBusy();

	result2 = REG_SPIDATA;

	return (((result & 0x7F) << 1) | ((result2>>7)&1));
}

//---------------------------------------------------------------------------------
// Read a short from the microphone. Code based on neimod's example.
//---------------------------------------------------------------------------------
u16 micReadData12() {
//---------------------------------------------------------------------------------
	static u16 result, result2;

	SerialWaitBusy();

	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_MICROPHONE | SPI_BAUD_2MHz | SPI_CONTINUOUS;
	REG_SPIDATA = 0xE4;  // Touchscreen command format for AUX, 12bit

	SerialWaitBusy();

	REG_SPIDATA = 0x00;

	SerialWaitBusy();

	result = REG_SPIDATA;
  	REG_SPICNT = SPI_ENABLE | SPI_DEVICE_TOUCH | SPI_BAUD_2MHz;
	REG_SPIDATA = 0x00;

	SerialWaitBusy();

	result2 = REG_SPIDATA;

	return (((result & 0x7F) << 5) | ((result2>>3)&0x1F));
}
