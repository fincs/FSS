#include "fss7.h"

volatile fss_sharedwork_t* FSS_SharedWork;
fss_channel_t FSS_Channels[16];
u16 FSS_ChnVol[16];
int FSS_NoteLengths[16];
fss_player_t FSS_Players[FSS_PLAYERCOUNT];
fss_track_t FSS_Tracks[FSS_MAXTRACKS];
byte_t FSS_TrackUpdateFlags[FSS_MAXTRACKS];
u16 FSS_ChnLockMask;
int FSS_WorkUpdFlags;

// Adapted from VGMTrans

int Cnv_Attack(int attk)
{
	static const u8 lut[] =
	{
		0x00, 0x01, 0x05, 0x0E, 0x1A, 0x26, 0x33, 0x3F, 0x49, 0x54,
		0x5C, 0x64, 0x6D, 0x74, 0x7B, 0x7F, 0x84, 0x89, 0x8F
	};
	
	return (attk >= 0x6D) ? lut[0x7F-attk] : (0xFF-attk);
}

int Cnv_Fall(int fall)
{
	if      (fall == 0x7F) return 0xFFFF;
	else if (fall == 0x7E) return 0x3C00;
	else if (fall < 0x32)  return ((fall<<1)+1) & 0xFFFF;
	else                   return (0x1E00/(0x7E - fall)) & 0xFFFF;
}

int Cnv_Sust(int sust)
{
	static const s16 lut[] =
	{
		-32768, -722, -721, -651, -601, -562, -530, -503,
		-480, -460, -442, -425, -410, -396, -383, -371,
		-360, -349, -339, -330, -321, -313, -305, -297,
		-289, -282, -276, -269, -263, -257, -251, -245,
		-239, -234, -229, -224, -219, -214, -210, -205,
		-201, -196, -192, -188, -184, -180, -176, -173,
		-169, -165, -162, -158, -155, -152, -149, -145,
		-142, -139, -136, -133, -130, -127, -125, -122,
		-119, -116, -114, -111, -109, -106, -103, -101,
		-99, -96, -94, -91, -89, -87, -85, -82,
		-80, -78, -76, -74, -72, -70, -68, -66,
		-64, -62, -60, -58, -56, -54, -52, -50,
		-49, -47, -45, -43, -42, -40, -38, -36,
		-35, -33, -31, -30, -28, -27, -25, -23,
		-22, -20, -19, -17, -16, -14, -13, -11,
		-10, -8, -7, -6, -4, -3, -1, 0
	};

	return lut[sust];
}

int Cnv_Sine(int arg)
{
	static const int lut_size = 32;
	static const s8 lut[] =
	{
		0, 6, 12, 19, 25, 31, 37, 43, 49, 54, 60, 65, 71, 76, 81, 85, 90, 94,
		98, 102, 106, 109, 112, 115, 117, 120, 122, 123, 125, 126, 126, 127, 127
	};

	if (arg < 1*lut_size) return  lut[arg];
	if (arg < 2*lut_size) return  lut[2*lut_size - arg];
	if (arg < 3*lut_size) return -lut[arg - 2*lut_size];
	/*else*/              return -lut[4*lut_size - arg];
}
