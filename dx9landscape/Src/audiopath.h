#ifndef __audiopath__
#define __audiopath__

class CaudioPath
{
private:
	IDirectMusicPerformance8 *performance;
	IDirectMusicAudioPath8 *apath;
	IDirectSound3DBuffer8 *sbuf3d;
	IDirectSound3DListener8 *listener;
	BOOL init();
public:
	CaudioPath();
	~CaudioPath();
	void setPerformance(IDirectMusicPerformance8 *perf);
	IDirectMusicPerformance8 *getPerformance();
	void setAudioPath(IDirectMusicAudioPath8 *ap);
	BOOL createAudioPath(DWORD type, DWORD channels, bool activate=true);
	operator IDirectMusicAudioPath8*();
	IDirectSound3DBuffer8 *get3dBuffer();
	IDirectSound3DListener8 *getListener();
	BOOL set();
	IDirectMusicAudioPath8 *getAudioPath();
	void release();
};

#endif