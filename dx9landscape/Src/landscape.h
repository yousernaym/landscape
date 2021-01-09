extern D3DXMATRIX g_waterReflWorldMat;
extern float waveCalcLimit;

struct landVertex : public xyz_vertex
{
};

struct waterVertex : public xyz_vertex
{
	fVector normal;
	fVector tangent;
	fVector binormal;
};

#define D3DFVF_LAND D3DFVF_XYZ;
#define D3DFVF_WATER D3DFVF_XYZ | D3DFVF_NORMAL;

void copyLandToWater(waterVertex *v1, const landVertex *v2, int index);
void copyWaterToLand(landVertex *v1, const waterVertex *v2, int index);

struct SlandSubset : public SmeshSubset
{
	SmipLevel ***mipLevel;
};

struct Shdif
{
	lpDIRECT3DVERTEXBUFFER vb[2];
	xyz_vertex *data;
	UINT num_vert;
	lpDIRECT3DVERTEXDECLARATION decl;
	int t;
};

//GroundGen classes------------------------------------

class CbaseGroundGen
{
protected:
	bool b_texGen;
	bool b_uwater;
public:
	bool active;
	UINT num_awTypes;
	UINT num_uwTypes;
	UINT numWeights;
	struct Stype
	{
		UINT numIntervals;
		struct Sinterval
		{
			float low;
			float high;
			float lowChance;
			float highChance;
			UINT numAngles;
			UINT numHeights;
			struct SbestCondition
			{
				float low;
				float high;
				float decay;
				float scale;
				//Methods
				SbestCondition()
				{
					low = 0; high = 0; decay = 1; scale = 1;
				}
				float calcWeight(float cond);
				void readStream(istream &stream);
				void writeStream(ostream &stream, int tab);
			} angle[5], height[5];
			float scale;
			float scaleChance;
			float intensity;   //Used only in ground texture generation
			//Methods
			Sinterval()
			{
				low = 0; high = 1; lowChance = 0; highChance = 0; numAngles = 1; numHeights = 1;
				scale = 1; scaleChance = 0.25f, intensity = -1;
			}
			void normalizeCondScales();
		} interval[5];
				
		//output values
		float weight;
		float intensity;

		//Used only in ground texture generation
		UINT numTex;
		struct Stex
		{
			UINT index;  
			DWORD mask;
			//Used only if numTex>1
			  float prob;
			  float blendIntensity;
			Stex()
			{
				index = 0; mask = 0; prob = 1; blendIntensity = 0.5;
			}
		} tex[5];
		//Used only if numTex>1
		float blendProb;  //blanda färger från alla texturer baserat på Stex::intensity
		float mixProb;  //slumpa fram en textur baserat på Stex::prob
		float exclusiveProb;  //som mix men ta även hänsyn till närbelägna pixlar

		//Methods
		Stype()
		{
			numIntervals = 1; numTex = 1;
			blendProb = 1; mixProb = 1; exclusiveProb = 40;
		}
		void calcWeight(float height, float angle);
		void readStream(istream &stream, bool b_texGen);
		void writeStream(ostream &stream, int tab, bool b_texGen);
	}  awType[20], uwType[20], *type;
	
	//Methods
	CbaseGroundGen();
	~CbaseGroundGen();
	void calcWeights(float height, float angle, bool uwater);
	void calcIntensities();
	void readStream(istream &stream);
	void writeStream(ostream &stream);
};

class CgroundGenObjects : public CbaseGroundGen
{
public:
	CgroundGenObjects();
	~CgroundGenObjects();	
	void readStream(istream &stream);
	void writeStream(ostream &stream);
};

class Clandscape;
class CgroundGenTextures : public CbaseGroundGen
{
public:
	float blendProb;  //blanda alla typer baserat på Stype::Sinterval::intensity
	UINT numTex;
	struct Stex
	{
		UINT index;  
		UINT numSmoothMasks;
		SsmoothInfo smoothMask[8];
		Stex()
		{
			index=0;numSmoothMasks=1;
		}
	} tex[5];
	int lightMapIndex;
	WORD lightMapMask;
	int caustMapIndex;
	WORD caustMapMask;

	//Methods
	CgroundGenTextures();
	~CgroundGenTextures();
	BOOL createTextures(Clandscape *land, UINT w, UINT h);
	BOOL lockTextures(Clandscape *land);
	BOOL writePixels(Clandscape *land, int x, int y, int u, int v, int reso, const Clight &light, bool uwater, float minH, float heightFromBottom);
	BOOL unlockTextures(Clandscape *land);
	BOOL smoothTextures(Clandscape *land);
	void normalizeGroundTypePixels(Clandscape *land);
	void readStream(istream &stream);
	void writeStream(ostream &stream);
	bool usesTexIndex(UINT index);
};

class CgroundGen
{
public:
	CgroundGenTextures texGen;
	CgroundGenObjects objGen;
	CgroundGen(bool btex = true, bool bobj = true);
	void calcWeights(float height, float angle, bool uwater);
	void readStream(istream &stream);
	void writeStream(ostream &stream);
	void readFile(Cpak *pak, const string &file);
	void writeFile(const string &file);
};

//Clandscape------------------------------------
class Clandscape : public CbaseMesh
{
    friend class Cwater;
	friend class CgroundGenTextures;
private:
	float **map;
	fVector **n_map;
protected:
	int landx;
	int landz;
	SlandSubset *subset;
	int gridx;
	int gridy;
	int cellx;
	UINT num_ml;
	int celly;
	Shdif hdif;
	fVector maxHeight;
	fVector minHeight;
	D3DXHANDLE fxHandle_caustAnim;
	void updateSubsets();
public:
	bool mlFixed;
	int wlevel;
	bool bReflDraw;
	Clandscape();
	virtual ~Clandscape();
	void createMap(int x, int z, DWORD select=3);
	void zeroNormals();
	void calcMapNormals();
	fVector calcNormal(int x, int z, bool nrm=true);
	float **getMap();
	fVector **getNormalMap();
	BOOL createMeshFromMap(float wrapTex, float clipMarg);
	bool vertexIsVisible(Clandscape *land2, int clipSide, float clipHeight, int x, int y);
	int fillVB(waterVertex *vb, int x1, int y1, int vstep, int s, float wrapTex, bool **&writeMap, Clandscape *land2=0, int clipSide=0, float clipHeight=0);
	int fillIB(USHORT *ib, int vstep, waterVertex *vb, int vbsize, bool fillGaps);
	void findMinHeight(fVector *hpos=0, const Crect *r=0);
	void findMaxHeight(fVector *hpos=0, const Crect *r=0);
	void findMinMaxHeight();
	float getMaxHeight();
	float getMinHeight();
	fVector getMaxHeightPos();
	fVector getMinHeightPos();
	void deleteVB();
	void baseDraw(int sub=-1, int _mip=0, const Crect *rect=0, bool water=false);
	void draw(Cwater *water=0, int sub=-1, int _mip=0, const Crect *rect=0);
	void createSubsets(UINT ns, const Cpoint &cellSize, UINT ml=0);
	void release();
	void deleteMap();
	void deleteSubsets();
	BOOL applySubsetTex(UINT index);
	BOOL applySubsetMtrl(UINT index);
	BOOL setSubsetMtrl(UINT mi, UINT subi);
	BOOL setSubsetTexArr(UINT *ti, UINT numStages, UINT subi);
	BOOL setSubsetTex(UINT ti, UINT tex, UINT subi);

	Cpoint getDim();
	void genGround(int reso, const Clight &light, CTexture *wtex, CgroundGen &groundGen);
	float pointCollision(baseObj &obj, float length, int sign, bool reset, float bouncy);
	float calcPixelLighting(float x, float z, const fVector &lightDir, const Clandscape &sunblock, bool normalTest=true);
	float getHeight(float x, float z) const;
	fVector getNormal(float x, float z);
	void calcHDifs(UINT s, UINT m, UINT gx, UINT gy);
	void setWaterDepth(float depth);
	void snapVertsToWlevel();
	BOOL copyMapToTexture(float scale, CTexture &tex);
	BOOL copyNmapToTexture(CTexture &tex);
	BOOL copyTextureToMap(Cpak *pak, const string &file, float scale);
	BOOL copyTextureToMap(CTexture &tex, float scale);
	BOOL copyTextureToNmap(Cpak *pak, const string &file);
	BOOL copyTextureToNmap(CTexture &tex);
	BOOL createEffect(Cpak *pak, const string &file, DWORD flags, const D3DXMACRO *defines = 0, LPD3DXINCLUDE include = 0, LPD3DXEFFECTPOOL pool = 0);
	void setEffect(LPD3DXEFFECT fx);
	void initEffect();
			
	//hmap gens
	void smooth(int nmax, int imgx, int imgz, BOOL pixel);
	void boxSlump(int imgx, int imgz);
	void imgFault(const Crect *rect, int n, float shift);
	void imgFunc2d(int imgx, int imgz);
	void sealBorders();

	//friends
	friend void createMaps(Clandscape &land, Cwater &water, int x, int z, int reso, const Clight &light, float wlevel, Cpak *pak, const string &groundGenFile, vscreen *output=0);
};

//Cwater------------------------------------
class Cwater : public Clandscape
{
protected:
	D3DXHANDLE fxHandle_nmapAnim;
public:
	float density;
	Clandscape *terrain;
	bool **isShaded;
	Cwater();
	virtual ~Cwater();
	void createMap(int x, int z);
	void wave(float offset, float amp=1);
	float isUnderWater();
	void draw(int sub=-1, int _mip=0, const Crect *rect=0);
	void calcVertexNormals(waterVertex *v, int size, const fVector *lightDir=0);
	BOOL createMeshFromMap(float wrapTex, Clandscape *land2, int clipside, float clipHeight, const fVector &lightDir);
	void genTexture(int reso, const Clight &light);
	BOOL createEffect(Cpak *pak, const string &file, DWORD flags, const D3DXMACRO *defines = 0, LPD3DXINCLUDE include = 0, LPD3DXEFFECTPOOL pool = 0);
	void setEffect(LPD3DXEFFECT fx);
	void initEffect();
	void deleteMap();
	void release();
};

class Csky : public object3d
{
public:
	void draw();
	void create(Cpak *meshPak, const string &meshFile, Cpak *texPak, const string &texFile, Cpak *shaderPak, const string &shaderFile);
};

