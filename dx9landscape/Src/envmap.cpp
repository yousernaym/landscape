#include "dxstuff.h"
#include "gfxengine.h"

CenvMap envMap;

CenvMap::CenvMap()
{
	cubeMap=0;
	zbuffer=0;
	enabled=false;
}

CenvMap::~CenvMap()
{
	release();
}

BOOL CenvMap::createMap(UINT edgeLength, D3DFORMAT format, D3DFORMAT depthFormat, UINT levels)
{
	safeRelease(cubeMap);
	if (FAILED(hr=pd3dDevice->CreateCubeTexture(edgeLength, levels, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &cubeMap, NULL)))
		return FALSE;
	if(FAILED(hr=pd3dDevice->CreateDepthStencilSurface(edgeLength, edgeLength, depthFormat, D3DMULTISAMPLE_NONE, 0, 0, &zbuffer, 0)))
		return FALSE;
	return TRUE;
}

void CenvMap::setRenderTarget(int i)
{
	face.release();
	lpDIRECT3DSURFACE surface=0;
	if (FAILED(hr=cubeMap->GetCubeMapSurface((D3DCUBEMAP_FACES)i, 0, &surface)))
		throw "Couldn't get cubemapsurface.";
	
	if (FAILED(hr=pd3dDevice->SetRenderTarget(0, surface)))
		throw "Couldn't set cubemap as rendertarget.";

	*((CBaseTexture*)(&face))=surface;
}

void CenvMap::viewTransform(int i, baseObj obj)
{
	if (i==0)  //höger
		obj.setRelAngle(0, pihalf, 0);
	else if (i==1)  //vänster
		obj.setRelAngle(0, -pihalf, 0);
	else if (i==2)  //upp
		obj.setRelAngle(-pihalf, 0, 0);
	else if (i==3)  //ner
		obj.setRelAngle(pihalf, 0, 0);
	else if (i==4)  //framåt
		;
	else if (i==5)  //bakåt
		obj.setRelAngle(0, -pi, 0);
    cam.viewTransform(0, fVector(0,0,1), obj);
}

void CenvMap::beginScene()
{
	pd3dDevice->GetTransform( D3DTS_PROJECTION, &projSave );
	pd3dDevice->GetTransform( D3DTS_VIEW, &viewSave);
	D3DXMATRIX projMat;
	D3DXMatrixPerspectiveFovLH(&projMat, pi/2, 1, nearClip, farClip);
	pd3dDevice->SetTransform(D3DTS_PROJECTION, &projMat);
	
	camSave=cam;
	
	pd3dDevice->GetDepthStencilSurface(&zbufSave);

	if(FAILED(hr=pd3dDevice->SetDepthStencilSurface(zbuffer)))
		throw "Couldn't set depth stencil surface for environment map.";
	
}

void CenvMap::endScene()
{
		if (FAILED(hr=pd3dDevice->SetRenderTarget(0, bbuf.getSurface())))
			throw "Couldn't restore render target";
		if(FAILED(hr=pd3dDevice->SetDepthStencilSurface(zbufSave)))
			throw "Couldn't restore zbuffer";
		safeRelease(zbufSave);
		cam=camSave;
	    pd3dDevice->SetTransform( D3DTS_PROJECTION, &projSave);
		pd3dDevice->SetTransform( D3DTS_VIEW, &viewSave);
}

void CenvMap::enable(bool b)
{
	enabled=b;
	pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, b);
}

bool CenvMap::isEnabled()
{
	return enabled;
}

void CenvMap::release()
{
	face.release();
	safeRelease(cubeMap);
	safeRelease(zbuffer);
}

lpDIRECT3DCUBETEXTURE CenvMap::getMap()
{
	return cubeMap;
}