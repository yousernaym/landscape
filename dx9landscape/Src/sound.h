// sound.h: interface for the Csound class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOUND_H__337DA900_4018_475C_ADD3_4AEBF3188D08__INCLUDED_)
#define AFX_SOUND_H__337DA900_4018_475C_ADD3_4AEBF3188D08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Csound  
{
private:
	IDirectMusicPerformance8 *performance;
	IDirectMusicSegment8 *segment;
	CaudioPath *apath;
public:
	Csound();
	virtual ~Csound();
	BOOL play();
	BOOL load(Cpak *pak, const string &file);
	void stop();
	int getLength();
	int getPos();
	void pause();
	void reset();
	void setVolume(int percent);
	void setPerformance(IDirectMusicPerformance8 *perf);
	IDirectMusicPerformance8 *getPerformance();
	void setAudioPath(CaudioPath *ap);
	CaudioPath *getAudioPath();
	void release();
};

#endif // !defined(AFX_SOUND_H__337DA900_4018_475C_ADD3_4AEBF3188D08__INCLUDED_)
