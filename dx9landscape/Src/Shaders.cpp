#include "dxstuff.h"
#include "gfxengine.h"

//Effect files--------------
LPD3DXEFFECT createD3dxEffect(Cpak *pak, const string &file, DWORD flags, const D3DXMACRO *defines, LPD3DXEFFECTPOOL pool)
{
	if (!pool)
	{
		if (!pd3dEffectPool)
		{
			if(FAILED(hr=D3DXCreateEffectPool(&pd3dEffectPool)))
				throw "Couldn't create effect pool";
		}
		pool = pd3dEffectPool;
	}
	LPD3DXEFFECT effect;
	if (d3dDebug)
		flags |= D3DXSHADER_DEBUG;
	LPD3DXBUFFER errMsg = 0;
	if (!pak)
		hr = D3DXCreateEffectFromFile(pd3dDevice, file.c_str(), defines, 0, flags, pool, &effect, &errMsg);
	else
	{
		void *data;
		UINT size;
		if (!pak->extractEntry(file, &data, &size))
			return FALSE;
		hr = D3DXCreateEffect(pd3dDevice, data, size, defines, 0, flags, pool, &effect, &errMsg);
		safeDeleteArray(data);
	}
	if (errMsg)
	{
		g_errMsg = (char*)errMsg->GetBufferPointer();
		d_file << endl << g_errMsg << endl;
		safeRelease(errMsg);
	}
	if (FAILED(hr))
		return 0;
	return effect;
}
//---------------------------------------------------


Cshader::Cshader()
{
	ZeroMemory(this, sizeof(*this));
}

Cshader::~Cshader()
{
}

BOOL Cshader::setConstantF(UINT start, float *cdata, UINT count)
{
	UINT end=count+start;
	if (end>maxConstF || start>=maxConstF)
		return FALSE;
	for (UINT i=start, j=0; i<end; i++, j+=4)
	{
		fConstant[i].x=cdata[j];
		fConstant[i].y=cdata[j+1];
		fConstant[i].z=cdata[j+2];
		fConstant[i].w=cdata[j+3];
	}

	if (end>fcount)
		fcount=end;
	return TRUE;
}

BOOL Cshader::setConstantI(UINT start, int *cdata, UINT count)
{
	UINT end=count+start;
	if (end>maxConstI || start>=maxConstI)
		return FALSE;
	for (UINT i=start, j=0; i<end; i++, j+=4)
	{
		iConstant[i].x=cdata[j];
		iConstant[i].y=cdata[j+1];
		iConstant[i].z=cdata[j+2];
		iConstant[i].w=cdata[j+3];
	}
	if (end>icount)
		icount=end;
	return TRUE;
}

BOOL Cshader::setConstantB(UINT start, BOOL*cdata, UINT count)
{
	UINT end=start+count;
	if (end>maxConstB || start>=maxConstB)
		return FALSE;
	for (UINT i=start, j=0; i<end; i++, j++)
		bConstant[i]=cdata[j];
	if (end>bcount)
		bcount=end;
	return TRUE;
}

ID3DXBuffer *Cshader::assembleShader(Cpak *pak, const string &file, bool debug)
{
	ID3DXBuffer *source=0;
	ID3DXBuffer *errmsg=0;
   	
	DWORD flags=0;
	if (debug)
		flags=D3DXSHADER_DEBUG;

	if (!pak)
		hr = D3DXAssembleShaderFromFile(file.c_str(), NULL, NULL, flags, &source, &errmsg);
	else
	{
		void *data;
		UINT size;
		if (!pak->extractEntry(file, &data, &size))
			return 0;
		hr = D3DXAssembleShader((LPCSTR)data, size, NULL, NULL, flags, &source, &errmsg);
		safeDeleteArray(data);
	}
	if (errmsg)
	{
		g_errMsg = (char*)errmsg->GetBufferPointer();
		d_file << endl << g_errMsg << endl;
		safeRelease(errmsg);
	}
	if(FAILED(hr))
		return 0;
	
	return source;
}

//Cvshader
Cvshader::Cvshader()
{
	ZeroMemory(this, sizeof(*this));
}

Cvshader::~Cvshader()
{
	release();
}


BOOL Cvshader::createShader(Cpak *pak, const string &file, bool debug)
{
	ID3DXBuffer *source = assembleShader(pak, file, debug);
	if (!source)
		return FALSE;
	safeRelease(shader);
	hr=pd3dDevice->CreateVertexShader((DWORD*)source->GetBufferPointer(),&shader);
	safeRelease(source);
	
	if (FAILED(hr))
		return FALSE;
	
	return TRUE;
}

BOOL Cvshader::activate(bool b, bool transform)
{
	if (!shader || !b)
	{
		pd3dDevice->SetVertexShader(0);
		return FALSE;
	}
		
	UINT fstart=0;
	if (transform)
	{
		D3DXMATRIX m;
		m=createTransformation();
		D3DXMatrixTranspose(&m, &m);
		if (FAILED(hr=pd3dDevice->SetVertexShaderConstantF(0, (float*)&m, 4)))
			return FALSE;
		fstart=4;
	}
		
	//float constants
	if (fcount>fstart)
		if (FAILED(hr=pd3dDevice->SetVertexShaderConstantF(fstart, (float*)&fConstant[fstart], fcount-fstart)))
			return FALSE;
	
	//int constants
	if (icount>0)
		if (FAILED(hr=pd3dDevice->SetVertexShaderConstantI(0, (int*)iConstant, icount)))
			return FALSE;
	
	//bool constants
	if (bcount>0)
		if (FAILED(hr=pd3dDevice->SetVertexShaderConstantB(0, (BOOL*)bConstant, bcount)))
			return FALSE;
	
	if(FAILED(hr=pd3dDevice->SetVertexShader(shader)))
		return FALSE;
	return TRUE;
}

void Cvshader::release()
{
	safeRelease(shader);
}


//Cpshader
Cpshader::Cpshader()
{
	ZeroMemory(this, sizeof(*this));
}

Cpshader::~Cpshader()
{
	release();
}

BOOL Cpshader::createShader(Cpak *pak, const string &file, bool debug)
{
	ID3DXBuffer *source = assembleShader(pak, file, debug);
	if (!source)
		return FALSE;
	safeRelease(shader);
	hr=pd3dDevice->CreatePixelShader((DWORD*)source->GetBufferPointer(),&shader);
	safeRelease(source);
	
	if (FAILED(hr))
		return FALSE;
	
	return TRUE;
}
BOOL Cpshader::activate(bool b)
{
	if (!shader || !b)
	{
		pd3dDevice->SetPixelShader(0);
		return FALSE;
	}
		
	if (fcount>0)
		if (FAILED(hr=pd3dDevice->SetPixelShaderConstantF(0, (float*)fConstant, fcount)))
			return FALSE;
	
	if (icount>0)
		if (FAILED(hr=pd3dDevice->SetPixelShaderConstantI(0, (int*)iConstant, icount)))
			return FALSE;
	
	if (bcount>0)
		if (FAILED(hr=pd3dDevice->SetPixelShaderConstantB(0, (BOOL*)bConstant, bcount)))
			return FALSE;
	
	if(FAILED(hr=pd3dDevice->SetPixelShader(shader)))
		return FALSE;
	return TRUE;
}

void Cpshader::release()
{
	safeRelease(shader);
}
