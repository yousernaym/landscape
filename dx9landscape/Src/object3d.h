#ifndef __object3d__
#define __object3d__

enum {mtrl_diffuse=0x1, mtrl_ambient=0x10, mtrl_specular=0x100, mtrl_emmisive=0x1000};

extern D3DXMATRIX texProjMatrix;

float interHeights(float fx, float fy, float height1, float height2, float height3);
float interHeights(float *dist, float height1, float height2, float height3);
fVector interNormals(float fx, float fy, const fVector &normal1, const fVector &normal2, const fVector &normal3);

class CTexture;
class vscreen;
class Cvshader;
class Cpshader;

struct SmipLevel
{
	lpDIRECT3DINDEXBUFFER pib;
	lpDIRECT3DVERTEXBUFFER pvb;
	UINT num_prim;
	UINT num_vert;
	float tag;
};

struct SmeshSubset
{
	UINT *texIndex;
	UINT num_ti;
	UINT mtrlIndex;
	SmipLevel *mipLevel;
	UINT num_ml;
	D3DPRIMITIVETYPE primType;
};


class baseObj
{
protected:
	fVector scale;
	void _brake(fVector &vector, float friction, bool x, bool y, bool z);
public:
	fVector pos;
	float maxSpeed;
	float acc;
	fVector velocity;
	float friction;
	float brakeFriction;
	fVector a;
	float maxRotSpeed;
	float rotAcc;
	fVector rotVelocity;
	float rotFriction;
	float rotBrakeFriction;
	fVector lookDir;
	fVector upDir;
	fVector leftDir;
	D3DXMATRIX localFrame;
	D3DXMATRIX worldMat;
	float rotSpeed;
	float speed;
	bool active;
	
	baseObj();
	void move(const fVector &destVel);
	void rotate(const fVector &destVel);
	void brake(bool x, bool y, bool z);
	void rotBrake(bool x, bool y, bool z);
	void setRelPos(const fVector &position);
	void setRelPos(float x, float y, float z);
	void setPos(const fVector &pos);
	void setPos(float x, float y, float z);
	void setRelAngle(const fVector &angle);
	void setRelAngle(float x, float y, float z);
	void setAngle(const fVector &angle);
	void setAngle(float x, float y, float z);
	void setLocalFrame(const D3DXMATRIX &lf);
	void update(bool b_velocity=true);
	void setMaxSpeed(float s);
	void setMaxRotSpeed(float s);
	void worldTransform();
	void activate(bool b);
};

class CbaseMesh : public baseObj
{
protected:
	CTexture *texture;
	UINT num_mtrl;
	UINT num_tex;
	SmeshSubset *subset;
	UINT num_ss;
	bool mmEnabled;
	bool mmAlwaysDraw;
	LPD3DXMESH pd3dxMesh;
	bool visible;
	bool drawTexture;
	DWORD fvf;
	DWORD fvfSize;
	lpDIRECT3DVERTEXDECLARATION vdecl;
	int mip;
	int *noDelTex;
	UINT numNoDelTexs;
	//shaders
	Cvshader **vshader;
	Cpshader **pshader;
	LPD3DXEFFECT effect;
	D3DXHANDLE fxHandle_wvp;
public:
	D3DMATERIAL *material;
	bool b_envMap;	
	CbaseMesh();
	virtual ~CbaseMesh();
	
	BOOL loadXfile(Cpak *pak, const string &file);
	LPD3DXMESH getD3dxmesh();
	void show(bool b);
	void scaling(float x, float y, float z);
	
	void setMeshData(const SmeshSubset *md, UINT n);
	BOOL setMtrl(const D3DMATERIAL &mtrl, UINT index); //false if invalid index
	void setMtrl(const D3DMATERIAL *mtrl, UINT n);
	BOOL setMtrl(const D3DXCOLOR &col, UINT mtrlProperty, UINT index);
	BOOL setMtrl(const D3DXCOLOR &col, UINT index);
	BOOL zeroMtrl(UINT index);
	BOOL setMtrlSP(float power, UINT index);
	
	BOOL setTex(CTexture &tex, UINT index);
	BOOL createTex(Cpak *pak, const string &file, UINT index);
	CTexture *getTex(UINT index);

	void drawSubset(UINT index, UINT _mip);
	void draw(int _mip);
	
	BOOL applyMtrl(UINT index);
	BOOL applyTex(UINT index, UINT stage);
	BOOL applySubsetMtrl(UINT index);
	BOOL applySubsetTex(UINT index);
	BOOL setSubsetMtrl(UINT mi, UINT subi);
	BOOL setSubsetTexArr(UINT *ti, UINT numStages, UINT subi);
	BOOL setSubsetTex(UINT ti, UINT tex, UINT subi);
		
	void createMtrls(UINT n);
	void createTexs(UINT n);
	void createSubsets(UINT n);
	void deleteMtrls();
	void deleteTexs();
	void deleteSubsets();
	void releaseD3dxMesh();
	void releaseVertexDecl();
	void release();
	
	void enableMip(bool b);
	void alwaysDrawMip(bool b);
	void enableTextures(bool b);
	void setFVF(DWORD _fvf);
	DWORD getFVF();
	void setNoDelTexs(const int arr[], UINT numTexs);
	//shaders
	void activateShaders(bool v, bool p);
	void setVertexShader(Cvshader **vs);
	void setPixelShader(Cpshader **ps);
	Cvshader *getVertexShader();
	Cpshader *getPixelShader();
	BOOL createEffect(Cpak *pak, const string &file, DWORD flags, const D3DXMACRO *defines = 0, LPD3DXEFFECTPOOL pool = 0);
	LPD3DXEFFECT getEffect();
	void setEffect(LPD3DXEFFECT fx);
	void initEffect();
	UINT beginEffect(DWORD flags=0, bool tranform = true);
	BOOL endEffect();
	void setVertexDecl(lpDIRECT3DVERTEXDECLARATION _vdecl);
	lpDIRECT3DVERTEXDECLARATION getVertexDecl();
	BOOL createVertexDecl(D3DVERTEXELEMENT *velement);
};

class object3d : public CbaseMesh
{
public:
	object3d();
	virtual ~object3d();
	void deleteSubsets();
	void release();
};


class camera : public baseObj
{
public:
	camera();
	bool uw;
	void viewTransform(const fVector &posOffset, const fVector &angleOffset, const baseObj &object=baseObj());
};

class Clight : public baseObj , public D3DLIGHT
{
private:
	static int numLights;
	int index;
	bool enabled;
public:
	Clight();
	virtual ~Clight();
	void enable(bool b);
	bool isEnabled();
	void set();
	void setAllComps(const D3DXCOLOR &c);
};

class Cbillboard : public CbaseMesh , public CTexture, public D3DMATERIAL
{
protected:
	lpDIRECT3DVERTEXBUFFER pvb;
	UINT num_prim;
	D3DPRIMITIVETYPE primType;
public:
	Cbillboard();
	~Cbillboard();
	void draw();
	void operator=(const CTexture &right);
	void release();
};

class vscreen : public Cbillboard
{
private:
	Crect rect;
	fVector posOffset;
	fVector aOffset;
public:
	vscreen();
	~vscreen();
	void setRect(Crect *r=0);
	Crect getRect();
	void set2dPos(int x, int y);
	void set3dOffest(const fVector *posoff, const fVector *aoff);
	BOOL createTexture(UINT w, UINT h, D3DFORMAT f, UINT levels, D3DPOOL pool, DWORD usage=0);
	void createTexture(Cpak *pak, const string &image);
	void draw();
	void operator=(const CTexture &right);
};

class Cplane : public D3DXVECTOR4
{
public:
	Cplane();
	Cplane(float a, float b, float c, float d);
	Cplane(const fVector &normal, float offset);
	Cplane transform(const D3DXMATRIX &mat, bool invTransp = false);
};

extern camera cam;

#endif