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



#include "ltcaudio.h"


LTCEncoder* lGetLtcEncoder()
{
	LTCEncoder *encoder;
	double fps = 25;
	double sample_rate = 48000;
	encoder = ltc_encoder_create(sample_rate, fps, fps == 25 ? LTC_TV_625_50 : LTC_TV_525_60, LTC_USE_DATE);
	return encoder;
}

int lSetCurrentDateTime(SMPTETimecode *pst, LTCEncoder *pencoder)
{

	time_t t;
	struct tm * tmp;
	t = time(NULL);
	tmp = localtime(&t);
	char sene[4] = { 0 };
	char sene2[3] = { 0 };
	sprintf(sene, "%d", tmp->tm_year);
	memcpy(sene2, &sene[1], 2);
	const char timezone[6] = "+0200";
	strcpy(pst->timezone, timezone);
	pst->years = atoi(sene2);
	pst->months = tmp->tm_mon;
	pst->days = tmp->tm_mday;
	pst->hours = tmp->tm_hour;
	pst->mins = tmp->tm_min;
	pst->secs = tmp->tm_sec;
	pst->frame = 0;
	ltc_encoder_set_timecode(pencoder, pst);
	return 0;
}

int lSetCurrentTime(SMPTETimecode *pst, LTCEncoder *pencoder)
{
	time_t t;
	struct tm * tmp;
	t = time(NULL);
	tmp = localtime(&t);
	pst->hours = tmp->tm_hour;
	pst->mins = tmp->tm_min;
	pst->secs = tmp->tm_sec;
	pst->frame = 0;
	ltc_encoder_set_timecode(pencoder, pst);
	char * simdik = malloc(sizeof(char)* 100);
	strftime(simdik, strlen(simdik), "%H:%M:%S", tmp);
	printf("%s\n", simdik);
	return 0;
}

int writeAudioBlock(HWAVEOUT hWaveOut, WAVEHDR* header)
{

	LPTSTR hata = malloc(sizeof(char)* 100);
	MMRESULT back = NULL;
	if (header->dwFlags & WHDR_PREPARED)
	{
		back = waveOutUnprepareHeader(hWaveOut, header, sizeof(WAVEHDR));
	}
	back = waveOutPrepareHeader(hWaveOut, header, sizeof(WAVEHDR));
	back = waveOutWrite(hWaveOut, header, sizeof(WAVEHDR));
	if (back != MMSYSERR_NOERROR)
	{	
		waveOutGetErrorText(back, hata, 100);
		printf(hata);
		free(hata);
		return 1;
	}

	EnterCriticalSection(&waveCriticalSection);
	waveFreeBlockCount--;
	LeaveCriticalSection(&waveCriticalSection);

	if (back == MMSYSERR_NOERROR)
		return 0;

}

WAVEHDR** AllocateBlocks(int count, int blockdatasize)
{
	WAVEHDR** blocks = NULL;
	WAVEHDR* block = NULL;
	blocks = calloc(count, sizeof(WAVEHDR*));
	for (int i = 0; i < count; i++)
	{
		blocks[i] = (WAVEHDR*)calloc(1, sizeof(WAVEHDR));
		blocks[i]->lpData = (LPSTR)calloc(blockdatasize, sizeof(char));
	}
	return blocks;
}

int LoadLtcData(WAVEHDR* block, LTCEncoder *pencoder, int blockDataSize, int vFrames)
{
	ltcsnd_sample_t *buf;
	memset(block->lpData, 0, blockDataSize);
	int len;
	for (int i = 0; i < vFrames; i++)
	{
		ltc_encoder_encode_frame(pencoder);
		buf = ltc_encoder_get_bufptr(pencoder, &len, 1);		
		memcpy(block->lpData + (len * i), buf, len);
		ltc_encoder_inc_timecode(pencoder);
	}
	
	block->dwBufferLength = len * vFrames;
	block->dwUser = 0;
	return 0;
}

void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{	
	if (uMsg != WOM_DONE) return;
	int* freeBlockCounter = (int*)dwInstance;
	EnterCriticalSection(&waveCriticalSection);
	(*freeBlockCounter)++;
	if (stoppFlag)
	{
		printf("freeblocks : %d\n", (*freeBlockCounter));
	}

	LeaveCriticalSection(&waveCriticalSection);
}

void FreeBlocks(WAVEHDR** pblocks)
{
	for (int i = 0; i < BLOCK_COUNT; i++)
	{
		if (pblocks[i]->lpData)free(pblocks[i]->lpData);
		if (pblocks[i])free(pblocks[i]);		
	}
	free(pblocks);
}

void processltc()
{
	WAVEHDR temp;
	WAVEFORMATEX wfx;
	SMPTETimecode st;
	clock_t cl;

	stoppFlag = 0;
	InitializeCriticalSection(&waveCriticalSection);
	InitializeCriticalSection(&stopltcthread);


	encoder = lGetLtcEncoder();
	if (!encoder) return;
	
	int lbuffsize = 0;
	int blockdatasize = 0;
	lbuffsize = ltc_encoder_get_buffersize(encoder);
	blockdatasize = lbuffsize  * VFRAME_COUNT;

	blocks = AllocateBlocks(BLOCK_COUNT, blockdatasize);


	wfx.nSamplesPerSec = 48000;
	wfx.wBitsPerSample = 8;
	wfx.nChannels = 1;
	wfx.cbSize = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) / 8 ;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;


	if (blocks == NULL) return;

	if (waveOutOpen(&hOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)&waveFreeBlockCount, CALLBACK_FUNCTION | WAVE_ALLOWSYNC) != MMSYSERR_NOERROR) {
		fprintf(stderr, "unable to open WAVE_MAPPER device\n");
		return;
	}

	waveFreeBlockCount = BLOCK_COUNT;
	int hangiblock = 0;
	lSetCurrentDateTime(&st, encoder);
	while (stoppFlag == 0)
	{
		if (waveFreeBlockCount > 0)
		{
			LoadLtcData(blocks[hangiblock], encoder, blockdatasize, VFRAME_COUNT);
			writeAudioBlock(hOut, blocks[hangiblock]);
			hangiblock++;
			hangiblock %= BLOCK_COUNT;
		}
		while (waveFreeBlockCount < 5) Sleep(10);
	}
	
	for (int i = 0; i < BLOCK_COUNT; i++)
	{
		if (blocks[i]->dwFlags & WHDR_PREPARED)
		{
			while (waveOutUnprepareHeader(hOut, blocks[i], sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) Sleep(100);
		}
	}

	printf("end of ltc process loop\n");

}

void tellstop()
{
	EnterCriticalSection(&stopltcthread);
	stoppFlag = 1;
	LeaveCriticalSection(&stopltcthread);
}

void freeltc()
{
	if (encoder) ltc_encoder_free(encoder);
	if (hOut) waveOutClose(hOut);
	DeleteCriticalSection(&waveCriticalSection);
	DeleteCriticalSection(&stopltcthread);
	if(blocks)FreeBlocks(blocks);
}

DWORD WINAPI fnltcthread(void* data) {
	processltc();
	return 0;
}


void doltc()
{
	ltcthread = CreateThread(NULL, 0, fnltcthread, NULL, 0, NULL);
}

void stopltc()
{
	tellstop();
	WaitForSingleObject(ltcthread, INFINITE);
	freeltc();
}

int main(int argc, char* argv[])
{

	int secim = 0;
	while (1)
	{
		printf("to start ltcaudio press 1, to stop press 2, 9 for exit\n");
		scanf(" %d", &secim);
		switch (secim)
		{
		case 1:
			doltc();
			break;
		case 2:
			stopltc();
			break;
		case 9:
			return 0;
		default:
			printf("hadi len\n");
		}
	}
	system("pause");
	return 0;
}
	
