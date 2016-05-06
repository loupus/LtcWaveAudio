# LtcWaveAudio
Ltc Timecode Playback With MME WaveOut
Microsoft MultiMedia Extensions ile Ltc Timecode 

We're going to develop a simple C project with Visual Studio 2013, that can playback wave data loaded with SMTPE timecode information. For this project we are going to use microsoft multimedia extensions to send timecode information through sound card. Why soundcard? Some professional timecode generators use it, and we will be able to send timecode info to their broadcast clocks. For this project we used Robin Gareus libltc on github (https://github.com/x42/libltc). Also, we follow the buffering logic of David Overton's sample on planetsourcecode for streaming wave packets to audio device . ( https://www.planet-source-code.com/vb/scripts/ShowCode.asp?txtCodeId=4422&lngWId=3). 

Microsoft Visual Studio 2013 ile basit bir C projesi yazacağız. Amacımız wave paketlerinin içine SMTPE timecode bilgisi koymak ve sürekli olarak ses kartını göndermek. Projemizde  microsoft multimedia extensions kütüphanesini kullancağız. Neden timecode bilgisini ses cihazını gönderiyoruz? Çünkü profesyonel cihazlar öyle yapıyor, biz de onların saatlerine timecode gönderebilmek için bu yolu seçtik. Linear/Logitudinal Time Code (LTC) kısmında Robin Gareus'un kütüphanesini kullanarak yazacağız. Detaylı bilgi için bakınız : https://github.com/x42/libltc . Waveout arayüzünde ise David Overton'un buffer mantığını kullanacağız. https://www.planet-source-code.com/vb/scripts/ShowCode.asp?txtCodeId=4422&lngWId=3 adresinden detaylı bilgi alabilirsiniz. 

1- Open an empty Visual C++ project. Visual Studio is a C++ ide but gives support to C to some extend. Visual Studio 2013 supports C89. We will be creating C files in accordance with C89 and will disregard crt secure warnings that has put forward by microsoft for the developers to be safe with code. Tell linker that we will use winmm.lib and add _CRT_SECURE_NO_WARNINGS to preprocessor definitions. 
1- Boş bir C++ projesi açın. Visual Studio C++ geliştirme ortamıdır ancak C'ye de destek vermektedir. Visual Studio 2013 C89 desteği vermektedir. Biz de bunu kullanacağız. Linker'a winmm.lib kütüphanesini gösterin ve preprocessor kısmında  _CRT_SECURE_NO_WARNINGS tanımını ekleyin.
 
Linker --> Input --> Additional Dependencies --> Edit --> winmm.lib
 
C/C++  --> PreProcessor --> Preprocessor Definitions --> Edit --> _CRT_SECURE_NO_WARNINGS

2- For ltc, you have to download ltclib from https://github.com/x42/libltc. Go Download ZIP  and add decoder.c,  decoder.h,  encoder.c,  encoder.h, ltc.c, ltc.h and timecode.c files to your project. You can also compile libltc and use as library, however in this project we prefer to use code pages.
2- Ltc için https://github.com/x42/libltc adresinden libltc'i indirin ve decoder.c,  decoder.h,  encoder.c,  encoder.h, ltc.c, ltc.h ve timecode.c dosylarını projenize ekleyin. İsterseniz libltc kütüphanesini derleyip öyle de kullanabilirsiniz. Ancak bu projede kod sayfalarını eklemeyi tercih ettik. 

3-Create a new header file, like ltcaudio.h and a new c file like ltcaudio.c. 
3-Yeni bir header ve c dosyası oluşturun. Biz ltcaudio.h ve ltcaudio.c isimlerini vermeyi tercih ettik. 
 
4-Include headers, mmsystem.h is the header file that will let us use mme library, ltc.h is libltc header file.

4-Header dosyalarını ekleyin. mmsystem.h wave için ltc.h libltc kullanımı için ekliyoruz.


Very briefly, we will make a queue of wave blocks and send them to the audio device while decrementing waveFreeBlockCount. As soon as audio device interrupts and playbacks them we will increment the freeblock count so that we can queue more blocks. We will be using 48kHz, 8bit mono for audio and 25fps for ltc. Simple unit "frame" means different in audio and video. Ltc is for video but we will use it in audio. So for 25 fps ltc, single frame is 48000 / 25 = 1920 bayts. This is size of our single ltc frame. Single block will contain 15 frames, so, single block will size at least 1920*15 = 28000 bayts. This is 4 times bigger then wave nBlockAlign (48000 * 1 channel / 8 = 6000),  the least multiple of it. In queue we will have 10 blocks. To modify waveFreeBlockCount variable in different threads we will be using critical session and we will define a callback function waveOutProc  to know the time when a block is played. 
Buradaki mantığımız basitçe şöyle olacak. waveFreeBlockCount adındaki değişkenimiz ile boş block sayımızı tutacağız. blokları doldurup ses cihazına gönderdikçe bu sayıyı düşürecek ve ses cihazı blokları okudukça bu sayıyı artıracağız. Sayı blok sayısına eşit oldukça yeni blok göndermeyeceğiz. Bir blokta 15 ltc frame bilgisi olacak. Ltc frame'i bir video frame gibi düşünün. Saniyede 25 frame olacak şekilde ayarladığımızda saniyede 48000 ses örneği veren bir ses akışında 48000/25 = 1920 bayt bir ltc frame data büyüklüğü olacak. Bundan 15 tanesi ile bir block oluşturduğumuzda aynı zamanda wave block align'ın en düşük kat sayısını da yakalamış olacağız. Toplamda 10 tane blok boşaldıkça yeni data ile dolacak ve yeniden ses cihazına gönderilecek.
5- We will allocate memory for our blocks. For this, we will ask ltcencoder to give us a single ltc frame's size. Then we will multipy it with VFRAME_COUNT so that we find a single block's data size.
5-Öncelikle bloklarımız için hafızadan yer tutacağız. Ancak bunun için tek bir ltc frame'in ne büyüklükte olduğunu ltcencoder'a soracağız. 

	int lbuffsize = 0;
	int blockdatasize = 0;
	lbuffsize = ltc_encoder_get_buffersize(encoder);
	blockdatasize = lbuffsize  * VFRAME_COUNT;

6-We will encode smpte data frame by frame. We will use a pointer to encoder's internal buffer and copy data from it to our block data up until a single block is filled up.
6- Smpte datasını frame olarak encode edeceğiz. Encode işleminden sonra encoder'ın iç buffer'ına bir pointer ile ulaşıp ondaki datayı bloğumuzun alanına yazacağız. Bir döngüde bir blok doldurmak için bunu VFRAME_COUNT kadar yapacağız. 

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

	
ltc_encoder_inc_timecode(pencoder); 

this line will increment the timecode for encoder so we dont need to give time info for encoder everytime. 
bu satır encoder'da timecode'u bir ileri sürecek, bu şekilde her seferinde encoder'a zaman bilgisi vermek durumunda kalmayacağız.

7- 

void CALLBACK waveOutProc(
HWAVEOUT hWaveOut, 
UINT uMsg, 
DWORD dwInstance, 
DWORD dwParam1, 
DWORD dwParam2)
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

In our callback function whenever a block is played, we will increment freeBlockCounter by 1. To do this we will pass this variable as a parameter to the callback function.
Callback fonksiyonumuzda bir blok okunduğunda freeBlockCounter'ı 1 artıracağız. Bunu yapabilmek için bu değişkeni callback fonksiyonun parametresi olarak geçeceğiz.

8-

	if (waveOutOpen(&hOut, WAVE_MAPPER, &wfx, (DWORD_PTR)waveOutProc, (DWORD_PTR)&waveFreeBlockCount, CALLBACK_FUNCTION | WAVE_ALLOWSYNC) != MMSYSERR_NOERROR) {
		fprintf(stderr, "unable to open WAVE_MAPPER device\n");
		return;
	}
	
While opening waveout interface we tell that waveOutProc is our callback function and waveFreeBlockCount is its parameter. And we alsa say that we will use a callback function. There are many ways to open the interface you can visit msdn for waveOutOpen function. 
WaveOut arayüzünü açarken waveOutProc callback fonksiyonunu kullanacağımızı ve waveFreeBlockCount ise onun parametresi olduğunu ifade ediyoruz. Waveout arayüzünü kullanmanın birden fazla yolu var, ilgili msdn sayfasından waveOutOpen özellikleri incelenebilir.

9-

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
	
Before we enter the loop, we set waveFreeBlockCount to the BLOCK_COUNT. We get current time and give ltc encoder its start point. Until we set stopflag and we have some free blocks we will load them with ltc data and write to audio interface.

Döngüye girmeden önce waveFreeBlockCount sayısını block_count'a eşitleyelim. O anki zamanı alıp ltc encoder'a başlangıç zamanını verelim. Döngümüzü stoppFlag set edilene kadar devam edecek şekilde ayarlıyoruz. Döngüye girince öncelikle boş bir blok var mı diye kontrol ediyoruz ve sonunda ses cihazına en az 5 blok okuyana kadar fırsat veriyoruz.

10- To stop streaming all we have to do is setting stop flag and releasing sources that we have borrowed from heap.
10- Akışı durdurmak için tüm yapmamız gereken stop flag'ini set etmek ve heap'dan ödünç aldığımız hafıza alanını temizlemek.


stoppFlag = 1;

	for (int i = 0; i < BLOCK_COUNT; i++)
	{
		if (pblocks[i]->lpData)free(pblocks[i]->lpData);
		if (pblocks[i])free(pblocks[i]);		
	}
	free(pblocks);

