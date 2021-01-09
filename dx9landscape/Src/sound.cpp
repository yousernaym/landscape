// sound.cpp: implementation of the Csound class.
//
//////////////////////////////////////////////////////////////////////

#include "dxstuff.h"
#include "audiopath.h"
#include "sound.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Csound::Csound()
{
	segment=0;
	performance=0;
	apath=0;
}

Csound::~Csound()
{
	release();
}

BOOL Csound::play()
{
	if (!performance)
		setPerformance(pdaPerformance);
	if (segment)
	{
		if (FAILED(performance->PlaySegmentEx(segment, 0, 0, DMUS_SEGF_NOINVALIDATE | DMUS_SEGF_SECONDARY, 0, 0, 0, (IDirectMusicAudioPath*)*apath)))
			return FALSE;
		return TRUE;
	}
	else
		return FALSE;
   
 }

BOOL Csound::load(Cpak *pak, const string &file)
{
	release();
	if (!pak)
	{
		static char cfile[MAX_PATH];
		strcpy(cfile, file.c_str());
		WCHAR wstrFileName[MAX_PATH];
		MultiByteToWideChar( CP_ACP, 0, cfile, -1, wstrFileName, MAX_PATH);
		if (FAILED(hr=pdaLoader->LoadObjectFromFile(CLSID_DirectMusicSegment, IID_IDirectMusicSegment8, wstrFileName, (LPVOID*) &segment)))
			return FALSE;
	}
	else
	{
		return FALSE;
	}

	if (!performance)
		performance=pdaPerformance;
	if (FAILED(hr=segment->Download(performance)))
		return FALSE;
	return TRUE;
}

void Csound::stop()
{
	performance->Stop(segment, 0, 0, 0);
}

void Csound::setPerformance(IDirectMusicPerformance8 *perf)
{
	if (segment)
	{
		if (performance)
			segment->Unload(performance);
		segment->Download(perf);
	}
	performance=perf;
}

IDirectMusicPerformance8 *Csound::getPerformance()
{
	return performance;
}

void Csound::setAudioPath(CaudioPath *ap)
{
	apath=ap;
}

CaudioPath *Csound::getAudioPath()
{
	return apath;
}
	

void Csound::release()
{
	if (segment)
		segment->Unload(performance);
	safeRelease(segment);
}