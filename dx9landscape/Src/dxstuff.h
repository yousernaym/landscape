#ifndef __dxStuff__
#define __dxStuff__

#define DIRECTINPUT_VERSION 0x0800
//#define D3D_DEBUG_INFO

#include "winstuff.h"
#include "general.h"
#include <basetsd.h>
#include <d3dx9.h>
#include <dxerr.h>
#include <dinput.h>
//#include <dmusici.h>

extern HRESULT hr;

//Identifiers----------
typedef LPDIRECT3DTEXTURE9 lpDIRECT3DTEXTURE;
typedef LPDIRECT3DCUBETEXTURE9 lpDIRECT3DCUBETEXTURE;
typedef LPDIRECT3DSURFACE9 lpDIRECT3DSURFACE;
typedef LPDIRECT3DVERTEXBUFFER9 lpDIRECT3DVERTEXBUFFER;
typedef LPDIRECT3DINDEXBUFFER9 lpDIRECT3DINDEXBUFFER;
typedef D3DMATERIAL9 D3DMATERIAL;
typedef D3DLIGHT9 D3DLIGHT;
typedef LPDIRECT3DVERTEXSHADER9 lpDIRECT3DVERTEXSHADER;
typedef LPDIRECT3DVERTEXDECLARATION9 lpDIRECT3DVERTEXDECLARATION;
typedef D3DVERTEXELEMENT9 D3DVERTEXELEMENT;
typedef LPDIRECT3DPIXELSHADER9 lpDIRECT3DPIXELSHADER;

//dinput
typedef LPDIRECTINPUTDEVICE8 lpDIRECTINPUTDEVICE;

////////////
//Direct3d//
////////////

extern LPDIRECT3D9 pd3d;
extern LPDIRECT3DDEVICE9 pd3dDevice;
extern D3DCAPS9 d3dCaps;
extern LPDIRECT3DSWAPCHAIN9 pd3dSwapChain;
extern LPD3DXEFFECTPOOL pd3dEffectPool; 

const float scr_ratio=4.0f/3.0f;

extern D3DXMATRIX g_viewMat;
extern D3DXMATRIX g_projMat;

extern float nearClip;
extern float farClip;
extern D3DXCOLOR g_bc;
extern int sw, sh;
extern int hsw, hsh;
extern D3DFORMAT sf;
extern D3DXMATRIX g_identityMat;
extern bool d3dDebug;

class fVector : public D3DXVECTOR3
{
public:
	fVector();
	fVector(float value);
	fVector(float x, float y, float z) : D3DXVECTOR3(x, y, z) {}
	fVector(const D3DVECTOR &v) : D3DXVECTOR3(v) {}
	bool isZero() const;
	float getValue() const;
	void normalize();
	float dot(const fVector &v) const;
	fVector cross(const fVector &v) const;
	void interpolate(const fVector &dest, const fVector &step);
	void fVector::reflect(const fVector &normal);
	fVector operator/(const fVector &arg2) const;
	friend fVector operator/(float arg1, const fVector &arg2);
};

struct xyz_vertex
{
	fVector pos;
};

struct std3dVertex : public xyz_vertex
{
	fVector normal;
	float tu, tv;
};

struct xyz_tex1_vertex : public xyz_vertex
{
	float tu, tv;
};

class Cd3dInit : public D3DPRESENT_PARAMETERS
{
public:
	D3DDEVTYPE deviceType;
	DWORD behaviorFlags;
	Cd3dInit();
};


#define D3DFVF_STD3D (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)
#define D3DFVF_XYZ_TEX1 (D3DFVF_XYZ | D3DFVF_TEX1)

void initD3d(Cd3dInit &d3dInit);
void cleanD3d();
void toggleFullscreen(BOOL full);
lpDIRECT3DVERTEXBUFFER createVB(const void *vert, UINT size, DWORD fvf, D3DPOOL pool=D3DPOOL_DEFAULT, DWORD usage=0);
lpDIRECT3DINDEXBUFFER createIB(const void *indices, UINT size, D3DFORMAT format, D3DPOOL pool=D3DPOOL_DEFAULT, DWORD usage=0);
void screenTransform();

fVector vecMultMat(fVector &v, D3DXMATRIX &m);
void rotate(fVector &d, fVector &s, fVector &a);
void setDefaultTSS();
int findVertexWithPos(const xyz_vertex *v, UINT voffset, UINT size, float x, float y, float z);
D3DXMATRIX createTransformation(DWORD flags=0x111 );
D3DXMATRIX createObliqueProjMat(const D3DXVECTOR4& clipPlane, const D3DXMATRIX &projMat, float planeScale = 0);
void ClipProjectionMatrix(D3DXMATRIX &matView, D3DXMATRIX &matProj,
                                             D3DXPLANE & clip_plane);
///////////////
//DirectInput//
//////////////
extern LPDIRECTINPUT8 pdi;

extern lpDIRECTINPUTDEVICE pdiKeyb;
extern char kstate[256];
extern DIDEVICEOBJECTDATA kdata;

extern lpDIRECTINPUTDEVICE pdiMouse;
extern DIMOUSESTATE mstate;
extern DIDEVICEOBJECTDATA mdata;
extern Cpoint mpos;
extern DIDEVCAPS diMouseCaps;

const int maxJoys=10;
const int joyAxisRange = 1000;
extern UINT numJoys;
extern UINT defJoy;

class Cjoy
{
private:
	DIPROPDWORD dipdwDevice;
public:
	lpDIRECTINPUTDEVICE device;
	DIJOYSTATE state;
	DIDEVICEOBJECTDATA data;
	DIDEVCAPS caps;
	bool enabled;
	Cjoy()
	{
		device=0;
		dipdwDevice.diph.dwSize       = sizeof(DIPROPDWORD);
	    dipdwDevice.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	    dipdwDevice.diph.dwObj        = 0;
	    dipdwDevice.diph.dwHow        = DIPH_DEVICE;
		caps.dwSize=sizeof(DIDEVCAPS);
		enabled=false;
	}
	HRESULT setDeviceProp(REFGUID type, DWORD value)
	{
		dipdwDevice.dwData=value;
		hr=device->SetProperty(type, &dipdwDevice.diph);
		return hr;
	}
	HRESULT getDeviceProp(REFGUID type, DWORD &value)
	{
		hr=device->GetProperty(type, &dipdwDevice.diph);
		value=dipdwDevice.dwData;
		return hr;
	}
	HRESULT getCapabilities()
	{
		if (FAILED(hr = device->GetCapabilities(&caps)))
			enabled=false;
		return hr;
	}
	
	BOOL butdown(UINT button)
	{
		return state.rgbButtons[button] & 0x80;
	}
};
extern Cjoy joy[maxJoys];

#define kbufSize 16
#define mbufSize 16
enum {inp_mouse=1, inp_keyb=2, inp_joy=4};

void initDI();
void cleanDI();
BOOL updateDevices(DWORD type=-1);
BOOL getDeviceData(lpDIRECTINPUTDEVICE device, DIDEVICEOBJECTDATA *data, DWORD &nelements);
BOOL getDeviceState(lpDIRECTINPUTDEVICE device, void *state, int size);

UCHAR inkey();
static USHORT scan2asc(DWORD scan);
BOOL keydown(DWORD key);
BOOL keypress(DWORD key);
BOOL keyup(DWORD key);
bool mbutdown(int button);
bool mbutpress(int button);
bool mbutup(int button);
void showMouse(bool b);

////////////////
//Direct Audio
////////////////

//extern IDirectMusicLoader8 *pdaLoader;
//extern IDirectMusicPerformance8 *pdaPerformance;
//
//void initDA();
//void cleanDA();
/////////////////////////////////////////////

void initDx(Cd3dInit &d3dInit);
void cleanDx();

#endif