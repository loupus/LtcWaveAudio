#ifndef _LTCAUDIO_H_
#define _LTCAUDIO_H_

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <time.h>
#include "ltc.h"

#define BLOCK_COUNT 10
#define VFRAME_COUNT 15
static CRITICAL_SECTION waveCriticalSection;
static CRITICAL_SECTION stopltcthread;
static volatile int waveFreeBlockCount;
void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void FreeBlocks(WAVEHDR** blocks);

WAVEHDR** blocks;
HWAVEOUT hOut;
LTCEncoder* encoder;
HANDLE ltcthread;
static volatile int stoppFlag;


LTCEncoder* lGetLtcEncoder()
int lSetCurrentDateTime(SMPTETimecode *pst, LTCEncoder *encoder);
int lSetCurrentTime(SMPTETimecode *pst, LTCEncoder *encoder);
int writeAudioBlock(HWAVEOUT hWaveOut, WAVEHDR* header);
WAVEHDR** AllocateBlocks(int count, int blockdatasize);
int LoadLtcData(WAVEHDR* block, LTCEncoder *encoder, int blockDataSize, int vFrames);
void processltc();
void freeltc();
void tellstop();

#endif  //_LTCAUDIO_H_