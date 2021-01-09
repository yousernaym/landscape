#include "dxstuff.h"
#include "gfxengine.h"

camera cam;
int Clight::numLights=0;
static fVector dirx(1,0,0);
static fVector diry(0,1,0);
static fVector dirz(0,0,1);

D3DXMATRIX texProjMatrix( 0.5f, 0   , 0   ,  0,
						  0   ,-0.5f, 0   ,  0,
						  0   , 0   , 0.5f,  0,
						  0.5f, 0.5f, 0.5f,  1);


//Global functions
fVector interNormals(float fx, float fy, const fVector &normal1, const fVector &normal2, const fVector &normal3)
{
	fVector normal=(1-fx)*normal1+fx*normal2;
	normal=(1-fy)*normal+fy*normal3;
	normal.normalize();
	return normal;
}

float interHeights(float fx, float fy, float height1, float height2, float height3)
{
	//float height=(1-fx)*height1+fx*height2;
	//height=(1-fy)*height+fy*height3;
	height2-=height1;height3-=height1;
	float height=fx*height2 + fy*height3 + height1;
	return height;
}	

float interHeights(float *dist, float height1, float height2, float height3)
{
	float height=dist[0]*height1 + dist[1]*height2 + dist[2]*height3;
	float totalDist=dist[0]+dist[1]+dist[2];
	height/=totalDist;
	return height;
}	


///////////////////////////////
//Construction/Destruction
		
baseObj::baseObj()
{
	pos=a=velocity=rotVelocity=fVector(0,0,0);
	maxSpeed=1;
	acc=maxSpeed/30.0f;
	friction=acc/10;
	brakeFriction=acc*2;
	maxRotSpeed=pi/180.0f;
	rotAcc=maxRotSpeed/30.0f;
	rotFriction=rotAcc/10;
	rotBrakeFriction=rotAcc*2;

	D3DXMatrixIdentity(&localFrame);
	scale.x=scale.y=scale.z=1;
	
	active=true;
	update();
}

CbaseMesh::CbaseMesh()
{
	//ZeroMemory(this, sizeof(*this));
	material=0;
	texture=0;
	num_mtrl=0;
	num_tex=0;
	pd3dxMesh=0;
	visible=true;
	drawTexture=true;
	mmEnabled=true;
	mmAlwaysDraw=false;
	subset=0;
	num_ss=0;
	b_envMap=false;
	fvf=D3DFVF_STD3D;
	fvfSize=sizeof(std3dVertex);
	vdecl=0;
	numNoDelTexs = 0;
	vshader=0;
	pshader=0;
	effect=0;
	fxHandle_wvp = 0;
}

CbaseMesh::~CbaseMesh()
{
}

camera::camera()
{
}

object3d::object3d()
{
}

object3d::~object3d()
{
	release();
}

Clight::Clight()
{
	ZeroMemory(this, sizeof(*this));
	index=numLights++;
	enabled=false;
}

Clight::~Clight()
{
}


//////////////////////////////////////////////////////
//BaseObj
	
void baseObj::move(const fVector &destVel)
{
	fVector vec;
	/*if (destVel.isZero())
	{
		vec=velocity;
		vec.normalize();
	}
	else
		vec=destVel;*/
	fVector dest=destVel*maxSpeed;
	vec=dest-velocity;
	vec.normalize();
	//fps.s=1;
	velocity.interpolate(dest, vec*acc*fps.s);
}

void baseObj::rotate(const fVector &destVel)
{
	//fps.s=1;
	rotVelocity.interpolate(maxRotSpeed*destVel, destVel*rotAcc*fps.s);
	a=rotVelocity;
}

void baseObj::_brake(fVector &vector, float friction, bool x, bool y, bool z)
{
	fVector bf(friction*x, friction*y, friction*z);
	fVector nvec=vector;
	nvec.normalize();
	bf.x*=nvec.x;
	bf.y*=nvec.y;
	bf.z*=nvec.z;
	//fps.s=1;
	vector.interpolate(fVector(0,0,0), bf*fps.s);
}

void baseObj::brake(bool x, bool y, bool z)
{
	_brake(velocity, brakeFriction, x, y, z);
}


void baseObj::rotBrake(bool x, bool y, bool z)
{
	_brake(rotVelocity, rotBrakeFriction, x, y, z);
}

void baseObj::setPos(const fVector &pos)
{
	setPos(pos.x, pos.y, pos.z);
}

void baseObj::setPos(float x, float y, float z)
{
	pos.x=x;
	pos.y=y;
	pos.z=z;
	update(false);
}

void baseObj::setRelPos(const fVector &position)
{
	setRelPos(position.x,position.y,position.z);
}

void baseObj::setRelPos(float x, float y, float z)
{
	pos.x+=x;
	pos.y+=y;
	pos.z+=z;
	update(false);
}

void baseObj::setRelAngle(const fVector &angle)
{
	setRelAngle(angle.x, angle.y, angle.z);
}

void baseObj::setRelAngle(float x, float y, float z)
{
	a.x=x;
	a.y=y;
	a.z=z;
	update(false);
}

void baseObj::setAngle(const fVector &angle)
{
	setAngle(angle.x, angle.y, angle.z);
}

void baseObj::setAngle(float x, float y, float z)
{
	localFrame=g_identityMat;
	setRelAngle(x, y, z);
}

void baseObj::setLocalFrame(const D3DXMATRIX &lf)
{
	localFrame=lf;
	update(false);
}

void baseObj::update(bool b_velocity)
{
	if (!active)
		return;
	if (b_velocity)
	{
		pos+=velocity*fps.s;
		a=rotVelocity*fps.s;
	}
	
	static D3DXMATRIX rotMat;
	static D3DXMATRIX transMat;
	static D3DXMATRIX scaleMat;
	
	D3DXMatrixScaling(&worldMat, scale.x, scale.y, scale.z);
	
	/*
	if (a.x)
	{
		D3DXMatrixRotationX(&rotMat, a.x);
		localFrame=rotMat*localFrame;
	}
	if (a.y)
	{
		D3DXMatrixRotationY(&rotMat, a.y);
		localFrame=rotMat*localFrame;
	}
	if (a.z)
	{
		D3DXMatrixRotationZ(&rotMat, a.z);
		localFrame=rotMat*localFrame;
	}*/
	D3DXMatrixRotationYawPitchRoll(&rotMat, a.y, a.x, a.z);
	localFrame = rotMat * localFrame;

	worldMat=worldMat*localFrame;

	D3DXMatrixTranslation(&transMat, pos.x, pos.y, pos.z);
	worldMat=worldMat*transMat;
	
	//worldMat._41=pos.x;worldMat._42=pos.y;worldMat._43=pos.z;worldMat._44=1;
	
	a.x=a.y=a.z=0;
	if (b_velocity)
	{
		fVector dest(0,0,0);
		velocity.interpolate(dest, friction*velocity*fps.s);
		rotVelocity.interpolate(dest, rotFriction*rotVelocity*fps.s);
	}
	
	leftDir=fVector(localFrame._11, localFrame._12, localFrame._13);
	upDir=fVector(localFrame._21, localFrame._22, localFrame._23);
	lookDir=fVector(localFrame._31, localFrame._32, localFrame._33);
}

void baseObj::setMaxSpeed(float s)
{
	float k=s/maxSpeed;
	maxSpeed=s;
	acc*=k;
	friction*=k;
	brakeFriction*=k;
}

void baseObj::setMaxRotSpeed(float s)
{
	float k=s/maxRotSpeed;
	maxRotSpeed=s;
	rotAcc*=k;
	rotFriction*=k;
	rotBrakeFriction*=k;
}

void baseObj::worldTransform()
{
	pd3dDevice->SetTransform(D3DTS_WORLD, &worldMat);
}

void baseObj::activate(bool b)
{
	active=b;
}

/////////////////////////////////////////////////////
//CbaseMesh

void setMeshData(const SmeshSubset *md, UINT n)
{
}

BOOL CbaseMesh::setMtrl(const D3DMATERIAL &mtrl, UINT index)
{
	if (index>=num_mtrl)
		return FALSE;
	material[index]=mtrl;
	return TRUE;
}

void CbaseMesh::setMtrl(const D3DMATERIAL *mtrl, UINT n)
{
	deleteMtrls();
	for (UINT i=0;i<n;i++)
		material[i]=mtrl[i];
	num_mtrl=n;
}

BOOL CbaseMesh::setMtrl(const D3DXCOLOR &col, UINT mtrlProperty, UINT index)
{
	if (index>=num_mtrl)
		return FALSE;
	if (mtrlProperty & mtrl_diffuse)
		material[index].Diffuse=col;
	if (mtrlProperty & mtrl_ambient)
		material[index].Ambient=col;
	if (mtrlProperty & mtrl_specular)
		material[index].Specular=col;
	if (mtrlProperty & mtrl_emmisive)
		material[index].Emissive=col;
	return TRUE;
}

BOOL CbaseMesh::setMtrl(const D3DXCOLOR &col, UINT index)
{
	if (index>=num_mtrl)
		return FALSE;
	material[index].Diffuse=material[index].Ambient=material[index].Specular=material[index].Emissive=col;
	return TRUE;
}

BOOL CbaseMesh::zeroMtrl(UINT index)
{
	if (index>=num_mtrl)
		return FALSE;
	ZeroMemory(&material[index], sizeof(D3DMATERIAL));
	return TRUE;
}

BOOL CbaseMesh::setMtrlSP(float power, UINT index)
{
	if (index>=num_mtrl)
		return FALSE;
	material[index].Power=power;
	return TRUE;
}

BOOL CbaseMesh::setTex(CTexture &tex, UINT index)
{
	if (index>=num_tex)
		return FALSE;
	texture[index]=tex;
	return TRUE;
}

BOOL CbaseMesh::createTex(Cpak *pak, const string &file, UINT index)
{
	if (index>=num_tex)
		return FALSE;
	texture[index].createTexture(pak, file);
	return TRUE;
}

CTexture *CbaseMesh::getTex(UINT index)
{
	if (index>=num_tex)
		return 0;
	return &texture[index];
}


BOOL CbaseMesh::applyMtrl(UINT index)
{
	if (index>=num_mtrl)
		return FALSE;
	pd3dDevice->SetMaterial(&material[index]);
	return TRUE;
}

BOOL CbaseMesh::applyTex(UINT index, UINT stage)
{
	if (index>=num_tex)
		return FALSE;
	if (drawTexture)
	{
		if (FAILED(hr=pd3dDevice->SetTexture(stage, texture[index].getTexture())))
			d_file<<"Couldn't set texture "<<index<<" at stage "<<stage;
		//pd3dDevice->SetTexture(stage, 0);
		float bias=texture[index].getMipBias();
		pd3dDevice->SetSamplerState(stage, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)(&bias))); 
	}
	else
		pd3dDevice->SetTexture(stage, 0);
	return TRUE;
}

BOOL CbaseMesh::applySubsetMtrl(UINT index)
{
	if (index>=num_ss)
		return FALSE;
	applyMtrl(subset[index].mtrlIndex);
	return TRUE;
}

BOOL CbaseMesh::applySubsetTex(UINT index)
{
	if (index>=num_ss)
		return FALSE;
	for (UINT stage=0;stage<subset[index].num_ti;stage++) 
	{
		UINT i=subset[index].texIndex[stage];
		applyTex(i, stage);
	}
	return TRUE;
}

BOOL CbaseMesh::setSubsetMtrl(UINT mi, UINT subi)
{
	if (subi>=num_ss || mi>=num_mtrl)
		return FALSE;
	subset[subi].mtrlIndex=mi;
	return TRUE;
}

BOOL CbaseMesh::setSubsetTexArr(UINT *ti, UINT numStages, UINT subi)
{
	if (subi>=num_ss)
		return FALSE;
	safeDeleteArray(subset[subi].texIndex);
	subset[subi].texIndex=new UINT[numStages];
	subset[subi].num_ti=numStages;
	for (UINT i=0;i<numStages;i++)
	{
		if (ti[i]>num_tex)
			continue;
		subset[subi].texIndex[i]=ti[i];
	}
	return TRUE;
}

BOOL CbaseMesh::setSubsetTex(UINT ti, UINT tex, UINT subi)
{
	if (subi>=num_ss || tex>=num_tex || ti>=subset[subi].num_ti)
		return FALSE;
	subset[subi].texIndex[ti]=tex;
	return TRUE;
}


void CbaseMesh::createMtrls(UINT n)
{
	deleteMtrls();
	material=new D3DMATERIAL[n];
	num_mtrl=n;
	for (UINT i=0;i<num_mtrl;i++)
		ZeroMemory(&material[i], sizeof(D3DMATERIAL));
}
	
void CbaseMesh::createTexs(UINT n)
{
	deleteTexs();
	texture=new CTexture[n];
	num_tex=n;
}

void CbaseMesh::draw(int _mip)
{
	if (!visible)
		return;
	UINT numPasses = beginEffect();
	if (!numPasses)
		throw "EffectBegin failed";
	
	for (UINT p = 0; p<numPasses; p++)
	{
		if (effect)
			if (FAILED(hr=effect->BeginPass(p)))
				throw string("Couldn't begin effect pass") + numberToString(p);
		for (UINT i=0;i<num_ss;i++)
		{
			if (pd3dxMesh)
			{
				if (!effect)
				{
					applyMtrl(i);
					applyTex(i, 0);
				}
				if (FAILED(hr = pd3dxMesh->DrawSubset(i)))
					throw "Couldn't draw mesh subset.";
			}
		}
		if (effect)
			if (FAILED(effect->EndPass()))
				throw string("Couldn't end effect pass") + numberToString(p);;
	}
	if (!endEffect())
		throw "EffectEnd failed";
}

void CbaseMesh::drawSubset(UINT index, UINT _mip)
{
	if (pd3dxMesh)
		pd3dxMesh->DrawSubset(index);
}

BOOL CbaseMesh::loadXfile(Cpak *pak, const string &file)
{
	LPD3DXBUFFER pd3dxMtrlBuf;
	if (!pak)
	{
		hr = D3DXLoadMeshFromX(file.c_str(), D3DXMESH_SYSTEMMEM, pd3dDevice, NULL, &pd3dxMtrlBuf, NULL, (ULONG*)(&num_mtrl), &pd3dxMesh);
	}
	else
	{
		void *data=0;
		UINT size;
		if (!pak->extractEntry(file, &data, &size))
			return FALSE;
		hr = D3DXLoadMeshFromXInMemory(data, size, D3DXMESH_SYSTEMMEM, pd3dDevice, NULL, &pd3dxMtrlBuf, NULL, (ULONG*)(&num_mtrl), &pd3dxMesh);
		safeDeleteArray(data);
	}
	if (FAILED(hr))
		return FALSE;

	num_ss=num_tex=num_mtrl;
	D3DXMATERIAL *d3dxMtrl=(D3DXMATERIAL*)pd3dxMtrlBuf->GetBufferPointer();
	
	material = new D3DMATERIAL[num_mtrl];
	texture = new CTexture[num_mtrl];
	
	for (DWORD i=0;i<num_mtrl;i++)
	{
		material[i]=d3dxMtrl[i].MatD3D;
		material[i].Ambient=material[i].Diffuse;
		if (d3dxMtrl[i].pTextureFilename)
			texture[i].createTexture(pak, d3dxMtrl[i].pTextureFilename);
			
	}
	pd3dxMtrlBuf->Release();
	return TRUE;
}

LPD3DXMESH CbaseMesh::getD3dxmesh()
{
	return pd3dxMesh;
}

void CbaseMesh::show(bool b)
{
	visible=b;
}

void CbaseMesh::scaling(float x, float y, float z)
{
	scale.x=x;
	scale.y=y;
	scale.z=z;
}

void CbaseMesh::deleteMtrls()
{
	safeDeleteArray(material);
}

void CbaseMesh::deleteTexs()
{
	if (texture)
	{
		//change pointers without deleting what they were pointing to
		for (unsigned i=0;i<numNoDelTexs;i++)
			texture[noDelTex[i]]=CTexture();
	
		safeDeleteArray(texture);
	}
}

void CbaseMesh::releaseD3dxMesh()
{
	safeRelease(pd3dxMesh);
}

void CbaseMesh::releaseVertexDecl()
{
	safeRelease(vdecl);
}

void CbaseMesh::release()
{
	deleteMtrls();
	deleteTexs();
	releaseD3dxMesh();
	releaseVertexDecl();
	safeRelease(effect);
}

void CbaseMesh::enableMip(bool b)
{
	mmEnabled=b;
}

void CbaseMesh::alwaysDrawMip(bool b)
{
	mmAlwaysDraw=b;
}

void CbaseMesh::enableTextures(bool b)
{
	drawTexture=b;
}

void CbaseMesh::setFVF(DWORD _fvf)
{
	fvf=_fvf;
}

DWORD CbaseMesh::getFVF()
{
	return fvf;
}

void CbaseMesh::setNoDelTexs(const int arr[], UINT numTexs)
{
	safeDeleteArray(noDelTex);
	numNoDelTexs = numTexs;
	if (!numTexs)
		return;
	noDelTex = new int[numTexs];
	memcpy(noDelTex, arr, numTexs*sizeof(int));
}

//shaders

void CbaseMesh::activateShaders(bool v, bool p)
{
	if (vshader && *vshader)
		(*vshader)->activate(v, true);
	else
		pd3dDevice->SetVertexShader(0);
	
	if (pshader && *pshader)
		(*pshader)->activate(p);
	else
		pd3dDevice->SetPixelShader(0);
}

void CbaseMesh::setVertexShader(Cvshader **vs)
{
	vshader=vs;
}

void CbaseMesh::setPixelShader(Cpshader **ps)
{
	pshader=ps;
}

Cvshader *CbaseMesh::getVertexShader()
{
	return *vshader;
}

Cpshader *CbaseMesh::getPixelShader()
{
	return *pshader;
}

BOOL CbaseMesh::createEffect(Cpak *pak, const string &file, DWORD flags, const D3DXMACRO *defines, LPD3DXEFFECTPOOL pool)
{
	safeRelease(effect);
	if (!(effect = ::createD3dxEffect(pak, file, flags, defines, pool)))
		return FALSE;
	initEffect();
	return TRUE;
}

LPD3DXEFFECT CbaseMesh::getEffect()
{
	return effect;
}

void CbaseMesh::setEffect(LPD3DXEFFECT fx)
{
	safeRelease(effect);
	effect=fx;
	initEffect();
}

void CbaseMesh::initEffect()
{
	if (effect)
		fxHandle_wvp = effect->GetParameterBySemantic(0, "WVP");
}

UINT CbaseMesh::beginEffect(DWORD flags, bool transform)
{
	worldTransform();
	UINT passes;
	if (effect)
	{
		if (transform)
		{
			D3DXMATRIX m;
			m=createTransformation();
			D3DXMatrixTranspose(&m, &m);
			if (FAILED(hr=effect->SetMatrix(fxHandle_wvp, &m)))
				return FALSE;
		}
		hr = effect->Begin(&passes, flags);
	}
	else
	{
		activateShaders(1,1);
		passes=1;
	}
	return passes;
}

BOOL CbaseMesh::endEffect()
{
	if (effect)
	{
		hr = effect->End();
		return (SUCCEEDED(hr));
	}
	else
		activateShaders(0,0);
	return TRUE;
}

void CbaseMesh::setVertexDecl(lpDIRECT3DVERTEXDECLARATION _vdecl)
{
	vdecl=_vdecl;
}

lpDIRECT3DVERTEXDECLARATION CbaseMesh::getVertexDecl()
{
	return vdecl;
}

BOOL CbaseMesh::createVertexDecl(D3DVERTEXELEMENT *velement)
{
	releaseVertexDecl();
	if (FAILED(hr=pd3dDevice->CreateVertexDeclaration(velement, &vdecl)))
		return FALSE;
	return TRUE;
}

/////////////////////////////////
//object3d
void object3d::deleteSubsets()
{
	if (subset)
	{
		for (UINT i=0;i<num_ss;i++)
		{
			safeDeleteArray(subset[i].texIndex);
			if (subset[i].mipLevel)
			{
				for (UINT j=0;j<subset[i].num_ml;j++)
				{
					safeRelease(subset[i].mipLevel[j].pvb);
					safeRelease(subset[i].mipLevel[j].pib);
				}	
				safeDeleteArray(subset[i].mipLevel);
			}
		}
		safeDeleteArray(subset);
	}
}

void object3d::release()
{
	CbaseMesh::release();
	deleteSubsets();
}

///////////////////////////////////////////////////
//camera

void camera::viewTransform(const fVector &posOffset, const fVector &angleOffset, const baseObj &object)
{
	static D3DXMATRIX viewMat;
	
	*(baseObj*)this=object;
	pos+=posOffset.x*leftDir;
	pos+=posOffset.y*upDir;
	pos+=posOffset.z*lookDir;
	update(false);
//worldMat
	a=angleOffset.x*leftDir;
	a+=angleOffset.y*upDir;
	a+=angleOffset.z*lookDir;
	D3DXMatrixLookAtLH(&viewMat, &pos, &(pos+lookDir), &upDir);
	pd3dDevice->SetTransform(D3DTS_VIEW, &viewMat);
}

//////////////////////////////////////////////////////////
//Clight
void Clight::enable(bool b)
{
	enabled=b;
	pd3dDevice->LightEnable(index, b);
}

bool Clight::isEnabled()
{
	return enabled;
}

void Clight::set()
{
	pd3dDevice->SetLight(index, this);
}

void Clight::setAllComps(const D3DXCOLOR &c)
{
	Diffuse=Ambient=Specular=c;
}

///////////////////////////
//Cbillboard
Cbillboard::Cbillboard()
{
	Specular=Emissive=D3DXCOLOR(0,0,0,0);;
	Power=0;
	num_prim=2;
	primType=D3DPT_TRIANGLESTRIP;
	pvb=0;
	Ambient=Diffuse=D3DXCOLOR(1,1,1,1);
}

Cbillboard::~Cbillboard()
{
	safeRelease(pvb);
}

void Cbillboard::draw()
{
	worldTransform();
	pd3dDevice->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, *((LPDWORD)(&mipBias)));
	pd3dDevice->SetStreamSource(0, pvb, 0, sizeof(xyz_tex1_vertex));
	pd3dDevice->SetFVF(D3DFVF_XYZ_TEX1);
	pd3dDevice->SetTexture(0, pd3dTexture);
	pd3dDevice->SetMaterial(this);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	activateShaders(1,1);

	if (FAILED(hr = pd3dDevice->DrawPrimitive(primType, 0, num_prim)))
		throw "Failed to render surface.";
	
	activateShaders(0,0);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	pd3dDevice->SetTexture(0, 0);
}

void Cbillboard::operator=(const CTexture &right)
{
	CTexture::release();
	*(CTexture*)this=right;
}

void Cbillboard::release()
{
	CTexture::release();
}

////////////////////////////////////
//vscreen

vscreen::vscreen()
{
	setPos(0,0,1.8105f);
	posOffset.x=posOffset.y=posOffset.z=0;
}

vscreen::~vscreen()
{
}

void vscreen::setRect(Crect *r)
{
	if (r==0)
	{
		r=new Crect;
		r->left=0; r->top=0;
		r->right=sw; r->bottom=sh;
		rect=*r;
		delete r;
	}
	else
		rect=*r;
	float x1 = (float)rect.left / hsw - 1.0f,  y1 = (float)rect.top / hsh - 1.0f, x2 = (float)rect.right / hsw - 1.0f, y2 = (float)rect.bottom / hsh-1.0f;
	
	
	y1*=-3.0f/4;
	y2*=-3.0f/4;
	
	xyz_tex1_vertex vert[4];
	
	vert[0].pos = fVector(x1, y2, 0); //v n
	vert[1].pos = fVector(x1, y1, 0); //v u
	vert[2].pos = fVector(x2, y2, 0); //h n
	vert[3].pos = fVector(x2, y1, 0); //h u
	
	if (pvb) //keep the tex coords
	{
		xyz_tex1_vertex *oldVert;
		if (SUCCEEDED(pvb->Lock(0, sizeof(xyz_tex1_vertex)*4, (void**)&oldVert, D3DLOCK_READONLY)))
		{
			for (int i=0;i<4;i++)
			{
				vert[i].tu=oldVert[i].tu;
				vert[i].tv=oldVert[i].tv;
			}
			pvb->Unlock();
		}
	}

	safeRelease(pvb);

	pvb=createVB(vert, 4*sizeof(xyz_tex1_vertex), D3DFVF_XYZ_TEX1);
}

Crect vscreen::getRect()
{
	return rect;
}

void vscreen::set2dPos(int x, int y)
{
	int w=rect.width();
	int h=rect.height();
	rect.left=x;
	rect.right=rect.left+w;
	rect.top=y;
	rect.bottom=rect.top+h;
	setRect(&rect);
}

void vscreen::set3dOffest(const fVector *posoff, const fVector *aoff)
{
	if (posoff)
		posOffset=*posoff;
	if (aoff)
		aOffset=*aoff;
}

BOOL vscreen::createTexture(UINT w, UINT h, D3DFORMAT f, UINT levels, D3DPOOL pool, DWORD usage)
{
	if (!CBaseTexture::createTexture(w, h, f, levels, pool, usage))
		return FALSE;
	
	float wk=(float)w/actualWidth;
	float hk=(float)h/actualHeight;
	//hk=1;
	xyz_tex1_vertex *vert;
	
	if (SUCCEEDED(pvb->Lock(0, 0, (void**)&vert, 0)))
	{
		vert[0].tu=0;
		vert[0].tv=hk;
		vert[1].tu=0;
		vert[1].tv=0;
		vert[2].tu=wk;
		vert[2].tv=hk;
		vert[3].tu=wk;
		vert[3].tv=0;
		pvb->Unlock();
	}
	else
		return FALSE;
	return TRUE;
}
	
void vscreen::createTexture(Cpak *pak, const string &image)
{
	CBaseTexture::createTexture(pak, image);
	
	UINT w2=sw, h2=sh;
	UINT levels=1;
	D3DPOOL pool=D3DPOOL_MANAGED;
	D3DFORMAT f=sf;
	hr=D3DXCheckTextureRequirements(pd3dDevice, &w2, &h2, &levels, NULL, &f, pool);
	float wk=(float)sw/w2;
	float hk=(float)sh/h2;
	//hk=1;
	xyz_tex1_vertex *vert;
	
	if (SUCCEEDED(pvb->Lock(0, 0, (void**)&vert, 0)))
	{
		vert[0].tu=0;
		vert[0].tv=hk;
		vert[1].tu=0;
		vert[1].tv=0;
		vert[2].tu=wk;
		vert[2].tv=hk;
		vert[3].tu=wk;
		vert[3].tv=0;
		pvb->Unlock();
	}
}

void vscreen::draw()
{
	static D3DXMATRIX viewMat;
	static D3DXMATRIX tempMat;
	D3DXMatrixIdentity(&viewMat);
	pd3dDevice->GetTransform(D3DTS_VIEW, &tempMat);
	pd3dDevice->SetTransform(D3DTS_VIEW, &viewMat);
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	Cbillboard::draw();
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	pd3dDevice->SetTransform(D3DTS_VIEW, &tempMat);
}

void vscreen::operator=(const CTexture &right)
{
	CTexture::release();
	*(CTexture*)this=right;
}

///////////////////////////////////////////
//Cplane

Cplane::Cplane(const fVector &normal, float offset)
{
	x=normal.x;
	y=normal.y;
	z=normal.z;
	w=offset;
}

Cplane::Cplane(float a, float b, float c, float d)
{
	x=a;
	y=b;
	z=c;
	w=d;
}

Cplane Cplane::transform(const D3DXMATRIX &mat, bool invTransp)
{
	fVector normal(x,y,z);
	fVector point(normal * w);
		
	D3DXMATRIX rotMat = mat;
	rotMat._41 = rotMat._42 = rotMat._43 = rotMat._14 = rotMat._24 = rotMat._34 = 0;
	rotMat._44 = 1;

	D3DXVec3TransformCoord(&normal, &normal, &rotMat);
	normal.normalize();
	
	D3DXVec3TransformCoord(&point, &point, &mat);
	
	return Cplane(normal, normal.dot(point)*-1);
}

