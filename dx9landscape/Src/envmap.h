#ifndef __envMap__
#define __envMap__

class CenvMap
{
private:
	lpDIRECT3DCUBETEXTURE cubeMap;
	lpDIRECT3DSURFACE zbuffer;
	lpDIRECT3DSURFACE zbufSave;
	CTexture face;
	camera camSave;
	D3DMATRIX projSave;
	D3DMATRIX viewSave;

	bool enabled;
public:
	CenvMap();
	~CenvMap();
	BOOL createMap(UINT edgeLength, D3DFORMAT format, D3DFORMAT depthFormat, UINT levels);
	void setRenderTarget(int i);
	void viewTransform(int i, baseObj obj);
	void beginScene();
	void endScene();
	void enable(bool b);
	bool isEnabled();
	void release();
	lpDIRECT3DCUBETEXTURE getMap();
};
extern CenvMap envMap;	

#endif