#ifndef __shaders__
#define __shaders__

LPD3DXEFFECT createD3dxEffect(Cpak *pak, const string &file, DWORD flags, const D3DXMACRO *defines = 0, LPD3DXEFFECTPOOL pool = pd3dEffectPool);

const UINT maxConstF=224;
const UINT maxConstI=16;
const UINT maxConstB=16;

struct intVector4
{
	int x, y, z, w;
};

class Cshader
{
protected:
	D3DXVECTOR4 fConstant[maxConstF];
	intVector4 iConstant[maxConstI];
	BOOL bConstant[maxConstB];
	UINT fcount, icount, bcount;
public:
	Cshader();
	virtual ~Cshader();
	BOOL setConstantF(UINT start, float *cdata, UINT count);
	BOOL setConstantI(UINT start, int *cdata, UINT count);
	BOOL setConstantB(UINT start, BOOL *cdata, UINT count);
	ID3DXBuffer *assembleShader(Cpak *pak, const string &file, bool debug);
};

class Cvshader : public Cshader
{
public:
	lpDIRECT3DVERTEXSHADER shader;
		
	Cvshader();
	virtual ~Cvshader();

	BOOL createShader(Cpak *pak, const string &file, bool debug=false);
	BOOL activate(bool b, bool transform);
	void release();
};

class Cpshader : public Cshader
{
public:
	lpDIRECT3DPIXELSHADER shader;

	Cpshader();
	virtual ~Cpshader();

	BOOL createShader(Cpak *pak, const string &file, bool debug=false);
	BOOL activate(bool b);
	void release();
};

#endif
