#include "dxstuff.h"
#include "audiopath.h"

CaudioPath::CaudioPath()
{
	apath=0;
	sbuf3d=0;
	listener=0;
	setPerformance(pdaPerformance);
}

CaudioPath::~CaudioPath()
{
	release();
}

BOOL CaudioPath::init()
{
	if (FAILED(hr = apath->GetObjectInPath(0, DMUS_PATH_PRIMARY_BUFFER, 0, GUID_NULL, 0, IID_IDirectSound3DListener8, (LPVOID*)&listener)))
		return FALSE;
	if (FAILED(hr = apath->GetObjectInPath(DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, 0, GUID_NULL, 0, IID_IDirectSound3DBuffer8, (LPVOID*)&sbuf3d)))
		return FALSE;
	
	return TRUE;
}

void CaudioPath::setPerformance(IDirectMusicPerformance8 *perf)
{
	performance=perf;
}

IDirectMusicPerformance8 *CaudioPath::getPerformance()
{
	return performance;
}

void CaudioPath::setAudioPath(IDirectMusicAudioPath8 *ap)
{
	release();
	apath=ap;
	init();
}

BOOL CaudioPath::createAudioPath(DWORD type, DWORD channels, bool activate)
{
	release();
	if (!performance)
		performance=pdaPerformance;
	if (FAILED(hr=performance->CreateStandardAudioPath(type, channels, activate, &apath)))
		return FALSE;
	return init();
}

CaudioPath::operator IDirectMusicAudioPath*()
{
	return apath;
}

IDirectSound3DBuffer8 *CaudioPath::get3dBuffer()
{
	return sbuf3d;
}

IDirectSound3DListener8 *CaudioPath::getListener()
{
	return listener;
}

BOOL CaudioPath::set()
{
	if (apath)
		if (SUCCEEDED(hr=performance->SetDefaultAudioPath(apath)))
			return TRUE;
	return FALSE;
}

IDirectMusicAudioPath8 *CaudioPath::getAudioPath()
{
	return apath;
}

void CaudioPath::release()
{
	safeRelease(apath);
}