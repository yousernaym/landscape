#include "dxStuff.h"

//Direct3d------------------------
LPDIRECT3D9 pd3d=0;
LPDIRECT3DDEVICE9 pd3dDevice=0;
D3DCAPS9 d3dCaps;
LPDIRECT3DSWAPCHAIN9 pd3dSwapChain=0;
LPD3DXEFFECTPOOL pd3dEffectPool = 0;

D3DXMATRIX g_viewMat;
D3DXMATRIX g_projMat;

float nearClip=0.1f;
float farClip=200;
D3DXCOLOR g_bc(1,1,1,1);
int sw=1024,sh=768;
int hsw=sw/2, hsh=sh/2;
D3DFORMAT sf=D3DFMT_A8R8G8B8;
D3DXMATRIX g_identityMat;
bool d3dDebug=false;

//DirectInput-----------------
LPDIRECTINPUT8 pdi=0;

//keyboard
lpDIRECTINPUTDEVICE pdiKeyb=0;
char kstate[256];
DIDEVICEOBJECTDATA kdata;

//mouse
lpDIRECTINPUTDEVICE pdiMouse=0;
DIMOUSESTATE mstate;
DIDEVICEOBJECTDATA mdata;
Cpoint mpos(0,0);
DIDEVCAPS diMouseCaps;

//joystick
Cjoy joy[maxJoys];
UINT numJoys=0;
UINT defJoy=0;
BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext);
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext);

//int inputl=0;

//DirectAudio-------------------------
//IDirectMusicLoader8 *pdaLoader = 0;
//IDirectMusicPerformance8 *pdaPerformance = 0;

HRESULT hr;

//////////////////////////////////////////
//Direct Graphics
/////////////////////////////////////////
Cd3dInit::Cd3dInit()
{
	ZeroMemory(this, sizeof(*this));
	BackBufferWidth=1024;
	BackBufferHeight=768;
	BackBufferFormat = D3DFMT_A8R8G8B8;
	EnableAutoDepthStencil = TRUE;
	AutoDepthStencilFormat = D3DFMT_D24X8;
	BackBufferCount=2;
	Windowed=true;
	MultiSampleType=D3DMULTISAMPLE_NONE;
	SwapEffect = D3DSWAPEFFECT_DISCARD;
	PresentationInterval=D3DPRESENT_INTERVAL_ONE;
	SwapEffect = D3DSWAPEFFECT_DISCARD;
	Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	behaviorFlags=D3DCREATE_MIXED_VERTEXPROCESSING;
	deviceType=D3DDEVTYPE_HAL;
}

void initD3d(Cd3dInit &d3dInit)
{
	D3DXMatrixIdentity(&g_identityMat);
	sw=d3dInit.BackBufferWidth;
	sh=d3dInit.BackBufferHeight;
	sf=d3dInit.BackBufferFormat;
	hsw=sw/2;
	hsh=sh/2;
	if(NULL == (pd3d = Direct3DCreate9( D3D_SDK_VERSION ) ) )
		throw "D3d object couldn't be created.";

	if(FAILED(hr=pd3d->CheckDeviceMultiSampleType( D3DADAPTER_DEFAULT, d3dInit.deviceType , d3dInit.BackBufferFormat, d3dInit.Windowed, d3dInit.MultiSampleType, NULL) ) )
		throw "Multisample type not supported.";
			
	if (!d3dInit.Windowed)
		d3dInit.FullScreen_RefreshRateInHz=D3DPRESENT_RATE_DEFAULT;
	
	if( FAILED(hr = pd3d->CreateDevice( D3DADAPTER_DEFAULT, d3dInit.deviceType, hwnd, d3dInit.behaviorFlags, &d3dInit, &pd3dDevice ) ) )
		if( FAILED(hr = pd3d->CreateDevice( D3DADAPTER_DEFAULT, d3dInit.deviceType, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dInit, &pd3dDevice ) ) )
			throw "D3d device couldn't be created.";

	pd3dDevice->GetSwapChain(0, &pd3dSwapChain);
	ZeroMemory(&d3dCaps, sizeof(d3dCaps));
	if (FAILED(hr=pd3dDevice->GetDeviceCaps(&d3dCaps)))
		throw "Failed to read capabilities of the graphics device.";
	
	if (d3dInit.MultiSampleType > 0)
		pd3dDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);

	pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_SRCALPHA);
	pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVSRCALPHA);
	pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pd3dDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_COLORVALUE(0.3f,0.3f,0.3f,1));
	//pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
	pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		
	for (UINT i=0;i<d3dCaps.MaxTextureBlendStages;i++)
	{
		if (d3dCaps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY)
		{
			pd3dDevice->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
			pd3dDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, d3dCaps.MaxAnisotropy);
		}
		else
			pd3dDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		//pd3dDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_GAUSSIANQUAD);
		//pd3dDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_PYRAMIDALQUAD);
		
		pd3dDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		pd3dDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	}
	
	setDefaultTSS();
	
	//pd3dDevice->SetRenderState(D3DRS_WRAP1, D3DWRAPCOORD_0|D3DWRAPCOORD_1|D3DWRAPCOORD_2);
}

void cleanD3d()
{
    safeRelease(pd3dEffectPool);
	safeRelease(pd3dSwapChain);
	safeRelease(pd3dDevice);
    safeRelease(pd3d);
}

void toggleFullscreen(BOOL full)
{
	if (!pd3dSwapChain)
		return;
	D3DPRESENT_PARAMETERS param;;
	pd3dSwapChain->GetPresentParameters(&param);
	if (param.Windowed == full)
	{
		param.Windowed = !full;
		if (full)
			param.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
		else
			param.FullScreen_RefreshRateInHz = 0;
		pd3dDevice->Reset(&param);
	}
}

lpDIRECT3DVERTEXBUFFER createVB(const void *vert, UINT size, DWORD fvf, D3DPOOL pool, DWORD usage)
{
	lpDIRECT3DVERTEXBUFFER pvb=0;
	if (pool == D3DPOOL_DEFAULT)
		usage |= D3DUSAGE_WRITEONLY;
	if (FAILED(hr = pd3dDevice->CreateVertexBuffer(size, usage, fvf, pool, &pvb, NULL)))
		throw "CreateVertexBuffer failed.";
		
	void *destVertices;
	if (FAILED(hr = pvb->Lock(0, size, (void**)&destVertices, 0)))
		throw "Couldn't lock vertex buffer.";
	
	memcpy(destVertices, vert, size);

	pvb->Unlock();
	return pvb;
}

lpDIRECT3DINDEXBUFFER createIB(const void *indices, UINT size, D3DFORMAT format, D3DPOOL pool, DWORD usage)
{
	lpDIRECT3DINDEXBUFFER pib;
	if (pool == D3DPOOL_DEFAULT)
		usage |= D3DUSAGE_WRITEONLY;
	if (FAILED(hr = pd3dDevice->CreateIndexBuffer(size, usage, format, pool, &pib, NULL)))
	{
		d_file<<"CreateIndexBuffer("<<size<<", 0, "<<format<<", "<<pool<<", "<<&pib<<") failed.";
		throw "CreateIndexBuffer failed.";
	}
	
	void *destIndices;
	if (FAILED(hr = pib->Lock(0, size, (void**)&destIndices, 0)))
		throw "Couldn't lock index buffer.";
	
	memcpy(destIndices, indices, size);
	
	pib->Unlock();
	
	return pib;	
}

void screenTransform()
{
	D3DXMatrixPerspectiveFovLH(&g_projMat, pi/4, scr_ratio, nearClip, farClip);
	//projMat._11 = .1f;
	//projMat._22 = .1f;
	pd3dDevice->SetTransform(D3DTS_PROJECTION, &g_projMat);
}

fVector vecMultMat(fVector &v, D3DXMATRIX &m)
{
	fVector vec;
	vec.x = v.x * m._11 + v.y * m._21 + v.z * m._31;
	vec.y = v.x * m._12 + v.y * m._22 + v.z * m._32;
	vec.z=  v.x * m._13 + v.y * m._23 + v.z * m._33;
	return vec;
}

void rotate(fVector &d, fVector &s, fVector &a)
{
	fVector s2;
	fVector s3;
		
	s2.x = (float)(s.x * cos(a.z) + s.y * sin(a.z));
	s2.y = (float)(s.y * cos(a.z) - s.x * sin(a.z));
	s2.z = s.z;
	
	s3.y = (float)(s2.y * cos(a.x) - s2.z * sin(a.x));
	s3.z = (float)(s2.y * sin(a.x) + s2.z * cos(a.x));
	s3.x = s2.x;
	
	d.z = (float)(s3.z * cos(a.y) - s3.x * sin(a.y));
	d.x = (float)(s3.z * sin(a.y) + s3.x * cos(a.y));
	d.y = s3.y;
}

void setDefaultTSS()
{
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

int findVertexWithPos(const xyz_vertex *v, UINT voffset, UINT size, float x, float y, float z)
{
	for (UINT i=voffset;i<size;i++)
	{
		if (v[i].pos.x==x && v[i].pos.y==y && v[i].pos.z==z)
			return i;
	}
    return -1;
}

D3DXMATRIX createTransformation(DWORD flags)
{
		static D3DXMATRIX m, m2, m3;
		if (!flags)
		{
			D3DXMatrixIdentity(&m);
			return m;
		}
		if (flags & 0x100)
			pd3dDevice->GetTransform(D3DTS_WORLD, &m);
		if (flags & 0x010)
			pd3dDevice->GetTransform(D3DTS_VIEW, &m2);
		if (flags & 0x001)
			pd3dDevice->GetTransform(D3DTS_PROJECTION, &m3);
	
        if (flags==0x111)
		{
			m=m*m2*m3;
			return m;
		}
		else if (flags & 0x100)
		{
			if (flags & 0x010)
				return m*m2;
			else if (flags & 0x001)
				return m*m3;
			else
				return m;
		}
		else if (flags & 0x010)
		{
			if (flags & 0x001)
				return m2*m3;
			else
				return m2;
		}
		else  //flags==0x001
			return m3;
}

D3DXMATRIX createObliqueProjMat(const D3DXVECTOR4& clipPlane, const D3DXMATRIX &projMat, float planeScale)
{
    D3DXMATRIX matrix;
    D3DXVECTOR4 q;

    // Calculate the clip-space corner point opposite the clipping plane
    // as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
    // transform it into camera space by multiplying it
    // by the inverse of the projection matrix
    
    q.x = (float)sgn(clipPlane.x) / projMat._11;
    q.y = (float)sgn(clipPlane.y) / projMat._22;
    q.z = 1.0F;
    q.w = (1.0F - projMat._33) / projMat._43;
    
    // Calculate the scaled plane vector
	D3DXVECTOR4 c;
	if (!planeScale)
		c = clipPlane / D3DXVec4Dot(&clipPlane, &q);
	else
		c = clipPlane * planeScale;


	/*D3DXVECTOR4 projClipPlane = clipPlane;
	projClipPlane /= abs(projClipPlane.z); // normalize such that depth is not scaled
    projClipPlane.w = projClipPlane.z-1;

    if(projClipPlane.z < 0)
    projClipPlane *= -1;

	c=projClipPlane;*/

	// Replace the third column of the projection matrix
    matrix = projMat;
	matrix._13 = c.x;
    matrix._23 = c.y;
    matrix._33 = c.z;
    matrix._43 = c.w;
	
	return matrix;
}

void ClipProjectionMatrix(D3DXMATRIX &matView, D3DXMATRIX &matProj,
                                             D3DXPLANE &clip_plane)
{
    D3DXMATRIX matClipProj;

    
    D3DXMATRIX WorldToProjection;
    
    
    WorldToProjection = matView * matProj;

    // m_clip_plane is plane definition (world space)
    D3DXPlaneNormalize(&clip_plane, &clip_plane);

    D3DXMatrixInverse(&WorldToProjection, NULL, &WorldToProjection);
    D3DXMatrixTranspose(&WorldToProjection, &WorldToProjection);


    D3DXVECTOR4 clipPlane(clip_plane.a, clip_plane.b, clip_plane.c, clip_plane.d);
    D3DXVECTOR4 projClipPlane;

    // transform clip plane into projection space
    D3DXVec4Transform(&projClipPlane, &clipPlane, &WorldToProjection);
    D3DXMatrixIdentity(&matClipProj);


    if (projClipPlane.w == 0)  // or less than a really small value
    {
        // plane is perpendicular to the near plane
        pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj);
        return;
    }

    if (projClipPlane.w > 0)
    {
        // flip plane to point away from eye
  /*      D3DXVECTOR4 clipPlane(-clip_plane.a, -clip_plane.b, -clip_plane.c, -clip_plane.d);

        // transform clip plane into projection space
        D3DXVec4Transform(&projClipPlane, &clipPlane, &WorldToProjection);
	*/
	
		projClipPlane /= abs(projClipPlane.z); // normalize such that depth is not scaled
		projClipPlane.w -= 1;

		if(projClipPlane.z < 0)
		projClipPlane *= -1;
    }


	//projClipPlane *= 10;

    // put projection space clip plane in Z column
    matClipProj(0, 2) = projClipPlane.x;
    matClipProj(1, 2) = projClipPlane.y;
    matClipProj(2, 2) = projClipPlane.z;
    matClipProj(3, 2) = projClipPlane.w;

    // multiply into projection matrix
    D3DXMATRIX projClipMatrix = matProj *matClipProj;

    pd3dDevice->SetTransform( D3DTS_PROJECTION, &projClipMatrix);



}


///////////////////////////////////////////////////////
//Direct Input
///////////////////////////////////////////////////////

void initDI()
{
	if (FAILED(hr = DirectInput8Create(hinst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pdi, NULL))) 
		throw "Couldn't create DirectInput object.";
	
	//keyboard
	if (FAILED(hr = pdi->CreateDevice(GUID_SysKeyboard, &pdiKeyb, NULL)))
		throw "Couldn't create keyboard device.";
	
	if (FAILED(hr = pdiKeyb->SetDataFormat(&c_dfDIKeyboard)))
		throw "Couldn't set keyboard data format.";

	if (FAILED(hr = pdiKeyb->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
		throw "Attempt to set keyboard cooperativelevel failed.";
 
	DIPROPDWORD dipdw;

    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj        = 0;
    dipdw.diph.dwHow        = DIPH_DEVICE;
    dipdw.dwData            = kbufSize; 

	if( FAILED( hr = pdiKeyb->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
		throw "Couldn't set keyboard buffersize.";

	pdiKeyb->Acquire();
		
	//mouse
	if (FAILED(hr = pdi->CreateDevice(GUID_SysMouse, &pdiMouse, NULL)))
		throw "Couldn't create mouse device.";
	
	if (FAILED(hr = pdiMouse->SetDataFormat(&c_dfDIMouse)))
		throw "Couldn't set mouse data format.";

	if (FAILED(hr = pdiMouse->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
		throw "Attempt to set mouse cooperativelevel failed.";
	
	dipdw.dwData = mbufSize; 
	if( FAILED( hr = pdiMouse->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
		throw "Couldn't set mouse buffersize.";

	diMouseCaps.dwSize=sizeof(DIDEVCAPS);
	if (FAILED(pdiMouse->GetCapabilities(&diMouseCaps)))
		throw "Couldn't get mouse capabilities.";
	
	pdiMouse->Acquire();

	//joysticks
	//Enumerate joysticks
	if (FAILED(hr=pdi->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, 0, DIEDFL_ATTACHEDONLY)))
		throw "EnumDevices failed.";
	for (UINT i=0;i<numJoys;i++)
	{
		if (FAILED(hr = joy[i].device->SetDataFormat(&c_dfDIJoystick)))
			throw "Couldn't set joystick data format.";
		
		if (FAILED(hr = joy[i].device->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
			throw "Attempt to set joystick cooperativelevel failed.";

		if (FAILED(hr = joy[i].device->EnumObjects(EnumAxesCallback, &i, DIDFT_AXIS)))
			throw "Failed to enumerate joystick objects.";
		joy[i].setDeviceProp(DIPROP_DEADZONE, 2000);
		joy[i].setDeviceProp(DIPROP_SATURATION, 9000);
		joy[i].device->Acquire();
	}
}

BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext )
{
    // Obtain an interface to the enumerated joystick.
    hr = pdi->CreateDevice( pdidInstance->guidInstance, &joy[numJoys].device, NULL );
    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if( SUCCEEDED(hr) ) 
	{
		joy[numJoys].enabled=true;
		numJoys++;
	}
	return DIENUM_CONTINUE;
}

BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext )
{
    int *i=(int*)pContext;
	DIPROPRANGE diprg; 
    diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
    diprg.diph.dwHow        = DIPH_BYID; 
    diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
    diprg.lMin              = -joyAxisRange; 
    diprg.lMax              = +joyAxisRange; 
    
	// Set the range for the axis
	if( FAILED( joy[*i].device->SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
		return DIENUM_STOP;
	return DIENUM_CONTINUE;
}
  
BOOL updateDevices(DWORD type)
{
	static DWORD nelements;
	//mouse
	if (type & inp_mouse)
	{
		do
		{
			nelements=1;
			if (!getDeviceData(pdiMouse, &mdata, nelements))
				return FALSE;
		} while (mdata.dwOfs && (mdata.dwOfs<DIMOUSE_BUTTON0 ||
			mdata.dwOfs>DIMOUSE_BUTTON3));
	
		if (!getDeviceState(pdiMouse, &mstate, sizeof(DIMOUSESTATE)))
			return FALSE;
		mpos.x+=mstate.lX;
		mpos.y+=mstate.lY;
		if (mpos.x<0) mpos.x=0;
		if (mpos.x>=sw) mpos.x=sw-1;
		if (mpos.y<0) mpos.y=0;
		if (mpos.y>=sh) mpos.y=sh-1;
	}
	//keyboard
	if (type & inp_keyb)
	{
		nelements=1;
		if (!getDeviceData(pdiKeyb, &kdata, nelements))
			return FALSE;
		if (!getDeviceState(pdiKeyb, &kstate, sizeof(kstate)))
			return FALSE;
	}
	//joystick
	if (type & inp_joy)
	{
		for (UINT i=0;i<numJoys;i++)
		{
			if (joy[i].enabled)
			{
				joy[i].device->Poll();
				if (!getDeviceState(joy[i].device, &joy[i].state, sizeof(DIJOYSTATE)))
					return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL getDeviceState(lpDIRECTINPUTDEVICE device, void *state, int size)
{
	if (FAILED(hr = device->GetDeviceState(size, LPVOID(state))))
	{
		if (hr==DIERR_INPUTLOST)
			device->Acquire();
		else
			return FALSE;
	}
	return TRUE;
}

BOOL getDeviceData(lpDIRECTINPUTDEVICE device, DIDEVICEOBJECTDATA *data, DWORD &nelements)
{
	if (!data)
		return FALSE;
	if (FAILED(hr = device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), data, &nelements, 0)))
	{
		if (hr==DIERR_INPUTLOST)
			device->Acquire();
		else
			return FALSE;
	}
	if (!nelements)
	{
		data->dwOfs=0;
		data->dwData=0;
	}
	return TRUE;
}

//keyboard functions///////////////////
UCHAR inkey()
{
	UCHAR c;
	if (kdata.dwData & 0x80)
	{
		c=(UCHAR)scan2asc(kdata.dwOfs);
	}
	else
		c='\0';
	return c;
}

static USHORT scan2asc(DWORD scan)
{
	static HKL layout = GetKeyboardLayout(0);
	static UCHAR state[256];
	static USHORT result[10];
	if (!GetKeyboardState(state))
		return 0;
	UINT vk = MapVirtualKeyEx(scan, 1, layout);
	int dum = ToAsciiEx(vk, scan, state, result, 0, layout);
	return result[0];
}
//////////////////////////////////
void cleanDI()
{
	if (pdi)
	{
		if(pdiKeyb)
		{
			pdiKeyb->Unacquire();
			pdiKeyb->Release();
			pdiKeyb=0;
		}
		if(pdiMouse)
		{
			pdiMouse->Unacquire();
			pdiMouse->Release();
			pdiMouse=0;
		}
		for (UINT i=0;i<numJoys;i++)
		{
			joy[i].device->Unacquire();
			joy[i].device->Release();
			joy[i].device=0;
			numJoys=0;
		}
		pdi->Release();
		pdi=0;
	}
}
 
BOOL keydown(DWORD key)
{
	return (kstate[key] & 0x80);
}


BOOL keypress(DWORD key)
{
	return (key==kdata.dwOfs && kdata.dwData & 0x80);
}

BOOL keyup(DWORD key)
{
	return (key==kdata.dwOfs && !(kdata.dwData & 0x80));
}

bool mbutdown(int button)
{
	if (button==-1)
	{
		for (int i=0;i<4;i++)
			if (mstate.rgbButtons[i] & 0x80)
				return true;
		return false;
	}
	if (mstate.rgbButtons[button] & 0x80)
		return true;
	else
		return false;
}

bool mbutpress(int button)
{
	if (button==-1)
	{
		for (int i=0;i<4;i++)
			if (button==mdata.dwOfs && mdata.dwData & 0x80)
				return true;
		return false;
	}
	return button==mdata.dwOfs && mdata.dwData & 0x80;
}

bool mbutup(int button)
{
	if (button==-1)
	{
		for (int i=0;i<4;i++)
			if (button==mdata.dwOfs && !(mdata.dwData & 0x80))
				return true;
		return false;
	}
	return button==mdata.dwOfs && !(mdata.dwData & 0x80);
}

//////////////////////////////////////////////
//Direct Audio
/////////////////////////////////////////////

//void initDA()
//{
//	CoInitialize(0);
//    CoCreateInstance(CLSID_DirectMusicLoader, NULL, CLSCTX_INPROC, IID_IDirectMusicLoader8, (void**)&pdaLoader);
//    CoCreateInstance(CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC, IID_IDirectMusicPerformance8, (void**)&pdaPerformance);
//	if (FAILED(pdaPerformance->InitAudio(0, 0, hwnd, DMUS_APATH_DYNAMIC_STEREO, 64, DMUS_AUDIOF_ALL, 0)))
//		throw "Failed to initialize audio";
//
//	//set search directory
//	char strPath[MAX_PATH];
//    GetCurrentDirectory(MAX_PATH, strPath);
//	WCHAR wstrSearchPath[MAX_PATH];
//    MultiByteToWideChar( CP_ACP, 0, strPath, -1, wstrSearchPath, MAX_PATH );
//	if (FAILED(pdaLoader->SetSearchDirectory(GUID_DirectMusicAllTypes, wstrSearchPath, FALSE)))
//		throw"Failed to set audio searchpath";
//}
//
//void cleanDA()
//{
//	if (pdaPerformance)
//	{
//		pdaPerformance->Stop(0, 0, 0, 0);
//		pdaPerformance->CloseDown();
//	}
//	safeRelease(pdaLoader);
//    safeRelease(pdaPerformance);
//    CoUninitialize();
//}
        

/////////////////////////////////////////////
void initDx(Cd3dInit &d3dInit)
{
	initD3d(d3dInit);
	initDI();
//	initDA();
}

void cleanDx()
{
	cleanD3d();
	cleanDI();
	//cleanDA();
}

//general stuff
fVector::fVector()
{
}

fVector::fVector(float value)
{
	x=y=z=value;
}

bool fVector::isZero() const
{
	return x==0 && y==0 && z==0;
}

float fVector::getValue() const
{
	return (float)sqrt(x*x+y*y+z*z);
}

void fVector::normalize() 
{
	if (isZero())
		return;
	float v=getValue();
	x/=v;y/=v;z/=v;
}

float fVector::dot(const fVector &v) const
{
	return x*v.x+y*v.y+z*v.z;
}

fVector fVector::cross(const fVector &v) const
{
	fVector result;
	D3DXVec3Cross(&result, this, &v);
	return result;
}

void fVector::interpolate(const fVector &dest, const fVector &step)
{
	::interpolate(x, dest.x, step.x);
	::interpolate(y, dest.y, step.y);
	::interpolate(z, dest.z, step.z);
}

void fVector::reflect(const fVector &normal)
{
	float nlength=dot(normal)*-2;  //'-' pga att *this ska vara riktad mot ytan
	if (nlength<0)
		return;
	*this=normal*nlength+(*this);
}

fVector fVector::operator/(const fVector &arg2) const
{
	return fVector(x/arg2.x, y/arg2.y, z/arg2.z);
}

fVector operator/(float arg1, const fVector &arg2)
{
	return fVector (arg1/arg2.x, arg1/arg2.y, arg1/arg2.z);
}
