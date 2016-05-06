/*
MIT License

Copyright (c) [2016] [Hakan Soyalp]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

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
