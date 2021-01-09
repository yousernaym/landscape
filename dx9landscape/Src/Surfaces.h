// Surfaces.h
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAPHICS_H__08AA5920_1A70_4B95_9CF0_E9CEE7F34120__INCLUDED_)
#define AFX_GRAPHICS_H__08AA5920_1A70_4B95_9CF0_E9CEE7F34120__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef DWORD COLOR;

const char newl='\n';

extern COLOR defCol;
extern int CSRLIN;
extern int cdepth;
extern Cpoint drawPos;
extern Cpoint printPos;

void locateP(int x, int y);
void locateD(int x, int y);
bool isInsideScreen(int x, int y);
DWORD d3dxColToBits(const D3DXCOLOR &col, D3DFORMAT format);
D3DXCOLOR bitsToD3dxCol(DWORD col, D3DFORMAT format);
int getBitDepthFromFormat(D3DFORMAT f);
fVector getNormalFromColor(D3DXCOLOR col, bool yzSwap);

enum step_1 {nostep1, step};
enum step_2 {nostep2, step1, step2, stepBoth};

enum transpMode{tr_disable, tr_alphablend, tr_dctest, tr_sctest};

enum drawTextFlags {centerTextX=1, centerTextY, centerTextXY};

struct SsmoothInfo
{
	int numPasses;
	bool b_pixel;
	DWORD mask;
	SsmoothInfo()
	{
		numPasses = 1; b_pixel = true; mask = 0xffffffff;
	}
	SsmoothInfo(int np, bool bp, DWORD m)
	{
		numPasses = np; b_pixel = bp; mask = m;
	}
};

class Cdrawfx;
class CTexture;
class Cfont;
class CalphaBlend;
class CscTest;
class CdcTest;
class CdrawMask;
class vscreen;

class CPrint
{
friend class CTexture;
friend class CPrintUsing2;
private:
	static Cfont *font;
	CTexture *base;
public:
	CPrint();
	~CPrint();
	CPrint &operator<<(const string &right);
	CPrint &operator<<(char right);
	CPrint &operator<<(int right);
	CPrint &operator<<(float right);
	CPrint &operator<<(double right);
	friend void setFont(Cfont *name);
	friend Cfont *getFont();
	//friend bool input(string &var, bool number=false);
};

class CPrintUsing
{
friend class CTexture;
private:
	CPrint *print;
public:
	CPrintUsing();
	~CPrintUsing();
	CPrintUsing2 operator<<(const string &f);
};

////////////////////////////////////////////////
// Texture classes /////////////////////////////

class CBaseTexture
{
friend class CscTest;
friend class CdcTest;
friend class CinitDfxTemplates;
protected:
	lpDIRECT3DSURFACE pd3dSurface;
	lpDIRECT3DTEXTURE pd3dTexture;
	int width, height, lockWidth, lockHeight, actualWidth, actualHeight;
	D3DFORMAT format;
	int mipLevels;
	float mipBias;
	COLOR *pBits;
	WORD *pBits16;
	UINT pitch;
	int bitDepth;
	    	
	//pixelmanipulation
	static Cdrawfx drawfx;
	static CalphaBlend aBlend;
	static CscTest scTest;
	static CdcTest dcTest;
	static CdrawMask drawMask;

	void stpConvert(const Crect &r, step_2 stp2, step_1 &stp1);
	void stpConvert2(Cpoint &p, step_1 stp);
public: 
	CBaseTexture();
	CBaseTexture(lpDIRECT3DSURFACE source);
	CBaseTexture(lpDIRECT3DTEXTURE source);
	~CBaseTexture();
	BOOL lock(CONST Crect* pRect=0, DWORD Flags=0);
	BOOL unlock();
	BOOL createTexture(UINT w, UINT h, D3DFORMAT f, UINT levels=1, D3DPOOL pool=D3DPOOL_MANAGED, DWORD usage = 0);
	BOOL createSurface(UINT w, UINT h, D3DFORMAT f);
	void createTexture(Cpak *pak, const string &image, UINT levels = 0, D3DPOOL pool=D3DPOOL_MANAGED, DWORD filter = D3DX_DEFAULT, DWORD mipFilter = D3DX_DEFAULT, D3DCOLOR colorkey = 0, DWORD usage = 0);
	void loadImage(Cpak *pak, const string &file, const Crect *sr=NULL, const Crect *dr=NULL, DWORD filter=D3DX_DEFAULT, D3DCOLOR colorkey=0);
	BOOL saveToFile(const string &file, D3DXIMAGE_FILEFORMAT f, const Crect *r=0);
	BOOL saveToFile(LPD3DXBUFFER *buf, D3DXIMAGE_FILEFORMAT f, const Crect *r=0);
	BOOL put(CBaseTexture *dest, const Crect *srcR=0, const Cpoint *destP=0, int cycle=0);
	lpDIRECT3DSURFACE getSurface();
	lpDIRECT3DTEXTURE getTexture();
	Cpoint getDim();
	Cpoint getActualDim();
	UINT getPitch();
	COLOR *getPixelPointer();
	void setDim(const Cpoint &p);
	void release();
	void fillRectWithDim(const Crect *inRect, Crect &outRect);
	void smooth(int numPasses, bool b_pixel, const Crect *rect = 0, DWORD mask = 0xffff);
	void smooth(const SsmoothInfo info[], UINT numMasks, const Crect *rect = 0);
	void fillMipLevels(UINT start=0, UINT end=0);
	BOOL getSurfaceLevel(UINT l, CBaseTexture &destTex);
	void setMipBias(int bias);
	float getMipBias();
	void setBitDepth(int bdepth);
	int getBitDepth();
	void createHmapFromNmap(const string &nmapFile, bool gAsUp);
	friend void setDrawfx(Cdrawfx &dfx);
	friend void setDrawfx(int disable);
	friend Cdrawfx getDrawfx();
	void operator=(lpDIRECT3DSURFACE srcSurf);
	void operator=(lpDIRECT3DTEXTURE srcTex);
	
//drawstuff
	void line (const Crect &r, COLOR c, step_2 stp = nostep2);
	void line (const Cpoint p, COLOR c, step_1 stp = nostep1);
	void box (const void *dummy, COLOR c, bool fill, step_2 stp = nostep2);
	void box (const Crect &r, COLOR c, bool fill, step_2 stp = nostep2);
	void box (Cpoint p, COLOR c, bool fill, step_1 stp = nostep1);
	void circle(const Cpoint &p, int r, COLOR c, bool fill, step_1 stp=nostep1);
	void circlex(const Cpoint &p, int r, COLOR c, bool fill, float start, float end, float aspect, step_1 stp=nostep1);
	void pset (int x, int y, COLOR c, step_1 stp= nostep1);
	void paint (int x, int y, COLOR c, step_1 stp = nostep1);
	BOOL point (int x, int y, COLOR &c);
	COLOR point(int x, int y);
	void cls();
	void psetx(COLOR *dest, COLOR sc);
};

//CTexture
class CTexture : public CBaseTexture
{
friend class CPrint;
friend class CPrintUsing2;
protected:
	int inputl;
	void drawText2(const string &s);
public: 
	CTexture();
	CTexture(lpDIRECT3DSURFACE source) : CBaseTexture(source) {}
	CTexture(lpDIRECT3DTEXTURE source) : CBaseTexture(source) {}
	~CTexture();
	BOOL createBackBuffer();
	
	bool input(int &var);
	bool input(double &var);
	bool input(char &var);
	bool input(string &var, bool number=false);
	void setInputLength(int length);

//more drawstuff
	void drawMB(__int64 mbzoom, int xs, int ys);

//printstuff
	CPrint print;
	CPrintUsing printUsing;
	void drawText(const string &txt, int x, int y, COLOR c=defCol, DWORD dtf=0);
};

class Cfont : public CBaseTexture
{
friend class CTexture;
friend class CPrint;
private:
	Crect fTexCoords[128];
	SIZE charSize[128];
	int maxFontHeight;
public:
	bool b_texture;
	Cfont();
	virtual ~Cfont();
	BOOL create(UINT fontHeight, const string &type, int weight, bool bItalic, Cpak *pak=0, const string &image="");
	Cpoint getSize(const string &str);
	Cpoint centerText(const Cpoint &size, const string &str);
	//friend bool input(string &var, bool number);
};

class Cmpointer
{
public:
	CTexture image;
	bool visible;
	Cmpointer();
	void init(Cpak *pak, const string &file="mpointer.bmp");
	void draw();
};

class CgammaRamp
{
private:
	static D3DGAMMARAMP idGamma;
public:
	CgammaRamp();
	friend void setGammaRamp(float r, float g, float b, bool calibrate);
	friend void restoreGammaRamp();
};

extern CTexture bbuf;
extern Cmpointer mpointer;

#endif // !defined(AFX_GRAPHICS_H__08AA5920_1A70_4B95_9CF0_E9CEE7F34120__INCLUDED_)
