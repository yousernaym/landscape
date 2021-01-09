// Surfaces.cpp: implementation of the CGraphics class.
//
//////////////////////////////////////////////////////////////////////

#include "dxstuff.h"
#include "surfaces.h"
#include "printusing2.h"
#include "pixelManip.h"

Cfont* CPrint::font=0;
Cdrawfx CBaseTexture::drawfx;
CalphaBlend CBaseTexture::aBlend;
CscTest CBaseTexture::scTest;
CdcTest CBaseTexture::dcTest;
CdrawMask CBaseTexture::drawMask;

COLOR defCol=0xffffffff;
Cpoint drawPos(0,0);
Cpoint printPos(0,0);
int CSRLIN=1;
int cdepth=4;

CTexture bbuf;

Cmpointer mpointer;

D3DGAMMARAMP CgammaRamp::idGamma;
static CgammaRamp dummy;

//////////////////////////////////////
//Global functions
//////////////////////////////////////
void locateP(int x, int y)
{
	printPos.x=x;
	printPos.y=y;
}

void locateD(int x, int y)
{
	drawPos.x=x;
	drawPos.y=y;
}

bool isInsideScreen(int x, int y)
{
	return x>=0 && x<sw && y>=0 && y<sh;
}

DWORD d3dxColToBits(const D3DXCOLOR &col, D3DFORMAT format)
{
	int bd = getBitDepthFromFormat(format);
	if (bd == 32)
		return (DWORD)col;
	else if (bd == 16)
	{
		D3DXCOLOR c = col;
		if (c.a>1)
			c.a=1;
		if (c.r>1)
			c.r=1;
		if (c.g>1)
			c.g=1;
		if (c.b>1)
			c.b=1;
		if (format==D3DFMT_A4R4G4B4)
			return (int(c.a*15)<<12) + (int(c.r*15)<<8) + (int(c.g*15)<<4) + int(c.b*15);
		else if (format==D3DFMT_R5G6B5)
			return (int(c.r*31)<<11) + (int(c.g*63)<<5) + int(c.b*31);
		else if (format==D3DFMT_A8L8)
			return (int(c.a*255)<<8) + int(c.r*255);
		else 
			return 0;
	}
	else
		return 0;
}

D3DXCOLOR bitsToD3dxCol(DWORD col, D3DFORMAT format)
{
	int bd = getBitDepthFromFormat(format);
	if (bd ==32)
		return D3DXCOLOR(col);
	else if (bd == 16)
	{
		D3DXCOLOR c;
		if (format==D3DFMT_A4R4G4B4)
		{
			c.a=((col & 0xf000)>>12) / 15.0f;
			c.r=((col & 0x0f00)>>8) / 15.0f;
			c.g=((col & 0x00f0)>>4) / 15.0f;
			c.b=(col & 0x000f) / 15.0f;
		}
		else if (format==D3DFMT_R5G6B5)
		{
			c.r=((col>>11) & 0x1f) / 31.0f;
			c.g=((col>>5) & 0x3f) / 63.0f;
			c.b=(col & 0x1f) / 31.0f;
		}
		else if (format==D3DFMT_A8L8)
		{
			c.a=((col>>8) & 0xff) / 255.0f;
			c.r=(col & 0xff) / 255.0f;
		}
		return c;
	}
	else
		return D3DXCOLOR();
}

int getBitDepthFromFormat(D3DFORMAT f)
{
	if (f==D3DFMT_A8R8G8B8 || f==D3DFMT_X8R8G8B8)
		return 32;
	if (f==D3DFMT_A4R4G4B4 || f==D3DFMT_R5G6B5 || f==D3DFMT_A8L8)
		return 16;
	else 
		return 32;
}

fVector getNormalFromColor(D3DXCOLOR col, bool yzSwap)
{
	fVector normal;
	normal.x = col.r - 0.5f;
	if (!yzSwap)
	{
		normal.y = col.g - 0.5f;
		normal.z = col.b - 0.5f;
	}
	else
	{
		normal.z = col.g - 0.5f;
		normal.y = col.b - 0.5f;
	}
	return normal;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPrint::CPrint()
{
}

CPrint::~CPrint()
{
}

CPrintUsing::CPrintUsing()
{
}

CPrintUsing::~CPrintUsing()
{
}

CBaseTexture::CBaseTexture()
{
	pd3dSurface=0;
	pd3dTexture=0;
	mipBias=0;
	bitDepth=32;
}

CBaseTexture::~CBaseTexture()
{
	release();
}

CBaseTexture::CBaseTexture(lpDIRECT3DSURFACE source)
{
	*this=source;
}

CBaseTexture::CBaseTexture(lpDIRECT3DTEXTURE source)
{
	*this=source;
}

CTexture::CTexture()
{
	print.base=this;
	printUsing.print=&print;
	inputl=0;
}

CTexture::~CTexture()
{
}

////////////////////////////////////////

BOOL CBaseTexture::lock(CONST Crect* pRect, DWORD Flags)
{
	D3DLOCKED_RECT lr;
	if (FAILED(hr=pd3dSurface->LockRect(&lr, pRect, Flags)))
		return FALSE;
	else
	{
		pBits = (COLOR*)lr.pBits;
		pBits16= (WORD*)lr.pBits;
		pitch=lr.Pitch*8/bitDepth;
		if (pRect)
		{
			lockWidth=pRect->right-pRect->left;
			lockHeight=pRect->bottom-pRect->top;
		}
		else
		{
			lockWidth=width;
			lockHeight=height;
		}
		return TRUE;
	}
}

BOOL CBaseTexture::unlock()
{
	if (FAILED(hr=pd3dSurface->UnlockRect()))
		return FALSE;
	else
		return TRUE;
}

BOOL CBaseTexture::createTexture(UINT w, UINT h, D3DFORMAT f, UINT levels, D3DPOOL pool, DWORD usage)
{
	release();
	actualWidth=w; actualHeight=h;
	hr=D3DXCheckTextureRequirements(pd3dDevice, (UINT*)&actualWidth, (UINT*)&actualHeight, &levels, NULL, &f, pool);
	if (FAILED(hr))
		throw "Texture requirements check failed.";
	hr=D3DXCreateTexture(pd3dDevice, w, h, levels, usage, f, pool, &pd3dTexture);
	if (FAILED(hr))
		throw "Failed to create texture.";
	*this = pd3dTexture;
	width=w;
	height=h;
	return TRUE;
}

BOOL CBaseTexture::createSurface(UINT w, UINT h, D3DFORMAT f)
{
	release();
	hr=pd3dDevice->CreateOffscreenPlainSurface(w, h, f, D3DPOOL_SCRATCH, &pd3dSurface, NULL);
	if (FAILED(hr))
		throw "Failed to create surface.";
	*this = pd3dSurface;
	return TRUE;
}

void CBaseTexture::createTexture(Cpak *pak, const string &image, UINT levels, D3DPOOL pool, DWORD filter, DWORD mipFilter, D3DCOLOR colorkey, DWORD usage)
{
	release();
	if (!pak)
	{
		hr=D3DXCreateTextureFromFileEx(pd3dDevice, image.c_str(), 0, 0, levels, usage, D3DFMT_UNKNOWN, pool, filter, mipFilter, colorkey, NULL, NULL, &pd3dTexture);
	}
	else
	{
		void *memFile;
		UINT size;
		if (!pak->extractEntry(image, &memFile, &size))
			throw string("Failed to load image ")+image;
		hr=D3DXCreateTextureFromFileInMemoryEx(pd3dDevice, memFile, size, 0, 0, levels, usage, D3DFMT_UNKNOWN, pool, filter, mipFilter, colorkey, NULL, NULL, &pd3dTexture);
		safeDeleteArray(memFile);
	}
	if (FAILED(hr))
		throw string("Failed to load image ")+image;
	
    *this=pd3dTexture;
}
  
void CBaseTexture::loadImage(Cpak *pak, const string &file, const Crect *sr, const Crect *dr, DWORD filter, D3DCOLOR colorkey)
{
	if (!pak)
	{
		hr=D3DXLoadSurfaceFromFile(pd3dSurface, NULL, dr, file.c_str(), sr, filter, colorkey, NULL);
	}
	else
	{
		void *memFile;
		UINT size;
		if (!pak->extractEntry(file, &memFile, &size))
			throw string("Failed to load image ")+file;
		hr=D3DXLoadSurfaceFromFileInMemory(pd3dSurface, NULL, dr, memFile, size, sr, filter, colorkey, NULL);
		safeDeleteArray(memFile);
	}
	if (FAILED(hr))
		throw string("Failed to load image ")+file;
}

BOOL CBaseTexture::saveToFile(const string &file, D3DXIMAGE_FILEFORMAT f, const Crect *r)
{
	if (FAILED(hr=D3DXSaveSurfaceToFile(file.c_str(), f, pd3dSurface, 0, r)))
		return FALSE;
	return TRUE;
}

BOOL CBaseTexture::saveToFile(LPD3DXBUFFER *buf, D3DXIMAGE_FILEFORMAT f, const Crect *r)
{
	if (FAILED(hr=D3DXSaveSurfaceToFileInMemory(buf, f, pd3dSurface, 0, r)))
		return FALSE;
	return TRUE;
}


BOOL CBaseTexture::put(CBaseTexture *dest, const Crect *srcR, const Cpoint *destP, int cycle)
{
	Crect sr;
	Cpoint dp;
	if (srcR==0)
	{
		sr.left=0;
		sr.top=0;
		sr.right=width;
		sr.bottom=height;
	}
	else
	{
		sr.left=srcR->left;
		sr.top=srcR->top;
		sr.right=srcR->right;
		sr.bottom=srcR->bottom;
	}
	if (destP==0)
	{
		dp.x=sr.left;
		dp.y=sr.top;
	}
	else
	{
		dp.x=destP->x;
		dp.y=destP->y;
	}
	
	LONG rwidth=sr.right-sr.left;
	LONG rheight=sr.bottom-sr.top;

	if (dp.x >= dest->width || dp.y >= dest->height || dp.x+rwidth<0 || dp.y+rheight<0)
		return FALSE;
	
	if (dp.x < 0)
	{
		sr.left-=dp.x;
		dp.x=0;
	}
	if (dp.y < 0)
	{
		sr.top-=dp.y;
		dp.y=0;
	}
	
	if (dp.x + rwidth > dest->width)
		sr.right = sr.left + dest->width - dp.x ;
	if (dp.y + rheight > dest->height)
		sr.bottom = sr.top + dest->height - dp.y;

	/*hr=pd3dDevice->CopyRects(pd3dSurface, &sr, 1, dest->pd3dSurface, &dp);
	if (FAILED(hr))
		throw "Couldn't copy surface.";
	return TRUE;*/
	Crect dr(dp.x, dp.y, dp.x+rwidth, dp.y+rheight);
	if (!dest->lock(&dr,0))
		return FALSE;
	if (!lock(&sr, D3DLOCK_READONLY))
	{
		dest->unlock();
		return FALSE;
	}
	int yd=0, ys=0;
	int x, y;
	for (y=0;y<rheight;y++)
	{
		if (bitDepth==32)
		{
			for (x=0;x<rwidth;x++)
				psetx(dest->pBits+x, *(pBits+x));
			dest->pBits+=dest->pitch;
			pBits+=pitch;
		}
		else if (bitDepth==16)
		{
			for (x=0;x<rwidth;x++)
				*(dest->pBits16+x)=*(pBits16+x);
			dest->pBits16+=dest->pitch;
			pBits16+=pitch;
		}
	}
	unlock();
	dest->unlock();
	return TRUE;
}

lpDIRECT3DSURFACE CBaseTexture::getSurface()
{
	return pd3dSurface;
}

lpDIRECT3DTEXTURE CBaseTexture::getTexture()
{
	return pd3dTexture;
}

Cpoint CBaseTexture::getDim()
{
	Cpoint p(width, height);
	return p;
}

Cpoint CBaseTexture::getActualDim()
{
	Cpoint p(actualWidth, actualHeight);
	return p;
}

COLOR *CBaseTexture::getPixelPointer()
{
	return pBits;
}

UINT CBaseTexture::getPitch()
{
	return pitch;
}

void CBaseTexture::setDim(const Cpoint &p)
{
	width=p.x;
	height=p.y;
}

void CBaseTexture::release()
{
	safeRelease(pd3dSurface);
	safeRelease(pd3dTexture);
}

void CBaseTexture::fillRectWithDim(const Crect *inRect, Crect &outRect)
{
	if (inRect==0)
	{
		outRect.left=0;outRect.top=0;outRect.right=width;outRect.bottom=height;
	}
	else
		outRect=*inRect;
}

void CBaseTexture::smooth(int numPasses, bool b_pixel, const Crect *rect, DWORD mask)
{
	SsmoothInfo info(numPasses, b_pixel, mask);
	smooth(&info, 1, rect);
}

void CBaseTexture::smooth(const SsmoothInfo mask[], UINT numMasks, const Crect *rect)
{
	Crect r;
	CBaseTexture tMap;
	fillRectWithDim(rect,r);	
	r.normalize();
	tMap.createSurface(r.width(), r.height(), format);

	int pmax[2] = {0,0};
	int *pixel[2] = {new int[numMasks], new int[numMasks]};
	int nmax[2]={0,0};
	for (UINT i=0;i<numMasks;i++)
	{
		int bp = mask[i].b_pixel ? 1 : 0;
		pixel[bp][pmax[bp]]=i;
		pmax[bp]++;
		if (nmax[bp] < mask[i].numPasses)
			nmax[bp] = mask[i].numPasses;
	}

	for (int bp=0;bp<2;bp++)
	{
		bool b_pixel = bp ? true : false;
		for (int n=0;n<nmax[bp];n++)
		{
			lock(0,D3DLOCK_READONLY);
			tMap.lock(0,0);
			int step;
			if (!b_pixel)
				step=(int)powf(2, float(nmax[bp]-n-1));
			else
				step=1;
			int w=step;
			int h=w;
			for (int y=r.top;y<r.bottom;y+=step)
			{	
				int sy=y-h;
				int ly=y+h;
				if (sy<0)
					sy=height-1;
				if (ly>=r.bottom)
					h=r.bottom-y;
				if (ly>=height)
					ly=0;
				
				for (int x=r.left;x<r.right;x+=step)
				{
					D3DXCOLOR c = bitsToD3dxCol(point(x,y), format);
					DWORD dwordc = d3dxColToBits(c, format);
					
					
						int sx=x-w;
						int lx=x+w;
						if (sx<0)
							sx=width-1;
						if (lx>=r.right)
							w=r.right-x;
						if (lx>=width)
							lx=0;
									
						c = bitsToD3dxCol(point(x,y), format) + bitsToD3dxCol(point(sx,y), format) + bitsToD3dxCol(point(lx,y), format) + bitsToD3dxCol(point(x,sy), format) + bitsToD3dxCol(point(x,ly), format) + bitsToD3dxCol(point(sx,sy), format) + bitsToD3dxCol(point(lx,sy), format) + bitsToD3dxCol(point(lx,ly), format) + bitsToD3dxCol(point(sx,ly), format);
						
						c/=9.0f;
						dwordc = d3dxColToBits(c, format);
					
					for (int i=0;i<pmax[bp];i++)
					{ 
						if (mask[pixel[bp][i]].numPasses >= nmax[bp] - n)
						{
							Cdrawfx dfx(0, 0, 1);
							dfx.drawMask = mask[pixel[bp][i]].mask;;
							setDrawfx(dfx);
							tMap.box(Crect(x, y, x+w-1, y+h-1), dwordc, true);
							setDrawfx(0);
						}
					}
				}
			}
			tMap.unlock();
			unlock();
			tMap.put(this);
		}
	}
	safeDeleteArray(pixel[0]);
	safeDeleteArray(pixel[1]);
}

void CBaseTexture::fillMipLevels(UINT start, UINT end)
{
	if (!pd3dTexture || start>=(UINT)mipLevels || end>(UINT)mipLevels)
		return;
	if (end==0)
		end=mipLevels;
	for (UINT i=start;i<end-1;i++)
	{
		CTexture src;
		CTexture dest;
		getSurfaceLevel(i, src);
		getSurfaceLevel(i+1, dest);
		src.lock(0, D3DLOCK_READONLY);
		dest.lock(0,0);
		int dy=0;
		for (int y=0;y<src.height;y+=2)
		{	
			int ly=y+1;
			if (ly>=src.height)
				ly=src.height-1;
			int dx=0;
			for (int x=0;x<src.width;x+=2)
			{
				int lx=x+1;
				if (lx>=src.width)
					lx=src.width-1;
				
				D3DXCOLOR c;
				/*if (bitDepth==16)
					c=wordToD3dxCol((WORD)src.point(x,y), format) + wordToD3dxCol((WORD)src.point(lx,y), format) + wordToD3dxCol((WORD)src.point(x, ly), format) + wordToD3dxCol((WORD)src.point(lx,ly), format);
				else
					c=(D3DXCOLOR)src.point(x,y)+(D3DXCOLOR)src.point(lx,y) + (D3DXCOLOR)src.point(x, ly) + (D3DXCOLOR)src.point(lx,ly);
				*/
				/*d_file<<"before bitsto..."<<endl;
				d_file<<x<<" "<<y<<endl;
				d_file<<lx<<" "<<ly<<endl;
				d_file<<"width:"<<src.width<<"  height:"<<src.height<<endl;*/
				c=bitsToD3dxCol(src.point(x,y), format) + bitsToD3dxCol(src.point(lx,y), format) + bitsToD3dxCol(src.point(x, ly), format) + bitsToD3dxCol(src.point(lx,ly), format);
				//d_file<<"after bitsto..."<<endl;
				c/=4.0f;
				dest.pset(dx, dy, d3dxColToBits(c, format));
				
				dx++;
			}
			dy++;
		}
		dest.unlock();
		src.unlock();
	}
}

BOOL CBaseTexture::getSurfaceLevel(UINT l, CBaseTexture &destTex)
{
	if (l>=(UINT)mipLevels)
		return FALSE;
	lpDIRECT3DSURFACE surf;
	if (FAILED (hr = pd3dTexture->GetSurfaceLevel(l, &surf)))
		return FALSE;
	destTex=surf;
	//Justera dim
	int div=(int)powf(2,(float)l);
	destTex.setDim(Cpoint(width/div, height/div));
	destTex.bitDepth=bitDepth;
	return TRUE;
}

void CBaseTexture::setMipBias(int bias)
{
	mipBias=(float)bias;
}

float CBaseTexture::getMipBias()
{
	return mipBias;
}

void CBaseTexture::setBitDepth(int bdepth)
{
	bitDepth=bdepth;
}

int CBaseTexture::getBitDepth()
{
	return bitDepth;
}

void CBaseTexture::createHmapFromNmap(const string &nmapFile, bool gAsUp)
{
	//Stores hmap in alpha channel
	CTexture nmap;
	nmap.createTexture(0, nmapFile);
	Cpoint dim = nmap.getDim();
	
	createTexture(dim.x, dim.y, D3DFMT_A8R8G8B8, 0);
	D3DXLoadSurfaceFromSurface(pd3dSurface, 0, 0, nmap.getSurface(), 0, 0, D3DX_DEFAULT, 0);
	
	float **hmap = new float*[dim.x];
	for (int i=0;i<dim.x;i++)
		hmap[i] = new float[dim.y];
	
	hmap[0][0] = 0;
	lock(0,0);
	for (int x=0;x<dim.x-1;x++)
	{
		fVector normal = getNormalFromColor(point(x, 0), !gAsUp);
		hmap[x+1][0] = hmap[x][0] + normal.x / normal.y;
	}
	
	for (int y=0;y<dim.y-1;y++)
	{
		for (int x=0;x<dim.x;x++)
		{
			fVector normal = getNormalFromColor(point(x, y), !gAsUp);
			hmap[x][y+1] = hmap[x][y] + normal.z / normal.y;
		}
	}
	float max = -9999999, min = 9999999;
	for (int y=0;y<dim.y;y++)
	{
		for (int x=0;x<dim.x;x++)
		{
			if (hmap[x][y] > max)
				max = hmap[x][y];
			if (hmap[x][y] < min)
				min = hmap[x][y];
		}
	}
	
	for (int y=0;y<dim.y;y++)
	{
		for (int x=0;x<dim.x;x++)
		{
			DWORD h = BYTE(((hmap[x][y] - min) / (max - min)) * 255); //[0,255]
			h <<= 24;
			DWORD col = point(x,y);
			col = writeBits(col, h, 0xff000000);
			pset(x, y, col);
		}
	}
	unlock();
	for (int i=0;i<dim.x;i++)
		delete[] hmap[i];
	delete[] hmap;
}



//friend---------------------------------------
void setDrawfx(Cdrawfx &dfx)
{
	CBaseTexture::drawfx=dfx;
	CBaseTexture::aBlend.setActive(dfx.aBlendEnable);
	
	static bool dcEnable, scEnable;
	if (dfx.colTestEnable==1)
	{
		scEnable=true;
		dcEnable=false;
	}
	else if (dfx.colTestEnable==2)
	{
		scEnable=false;
		dcEnable=true;
	}
	else
	{
		scEnable=false;
		dcEnable=false;
	}
	if (scEnable)
	{
		CBaseTexture::scTest.setFunc(dfx.testFunc);
		CBaseTexture::scTest.setRef(dfx.testRef);
		CBaseTexture::scTest.setMask(dfx.testMask);
	}
	CBaseTexture::scTest.setActive(scEnable);
	
	if (dcEnable)
	{
		CBaseTexture::dcTest.setFunc(dfx.testFunc);
		CBaseTexture::dcTest.setRef(dfx.testRef);
		CBaseTexture::dcTest.setMask(dfx.testMask);
	}
	CBaseTexture::dcTest.setActive(dcEnable);
	
	CBaseTexture::drawMask.setMask(dfx.drawMask);
	CBaseTexture::drawMask.setActive(dfx.drawMaskEnable);
}

void setDrawfx(int disable)
{
	setDrawfx(Cdrawfx(0,0,0));
}

Cdrawfx getDrawfx()
{
	return CBaseTexture::drawfx;
}

//---------------------------------------
//drawstuff///
void CBaseTexture::stpConvert(const Crect &r, step_2 stp, step_1 &stp2)
{
	if (stp==step1 || stp==stepBoth)
	{
		drawPos.x+=r.left;
		drawPos.y+=r.top;
	}
	else
		locateD(r.left, r.top);		
	if (stp==step2 || stp==stepBoth)
		stp2=step;
	else
		stp2=nostep1;
}	

void CBaseTexture::stpConvert2(Cpoint &p, step_1 stp)
{
	if (stp==step)
	{
		p.x+=drawPos.x;
		p.y+=drawPos.y;
	}
}

BOOL CBaseTexture::point(int x, int y, COLOR &c)
{
	if (isInsideRect(0,0,lockWidth,lockHeight,x,y))
	{
		if (bitDepth==16)
			c=*(pBits16+y*pitch+x);
		else
			c=*(pBits+y*pitch+x);
		return TRUE;
	}
	else
		return FALSE;
}

COLOR CBaseTexture::point(int x, int y)
{
	if (isInsideRect(0,0,lockWidth,lockHeight,x,y))
	{
		if (bitDepth==16)
			return *(pBits16+y*pitch+x);
		else
			return *(pBits+y*pitch+x);
	}
	else
		return 0;
}


void CBaseTexture::pset(int x, int y, COLOR c, step_1 stp)
{
	if (stp==step)
	{
		drawPos.x+=x;
		drawPos.y+=y;
	}
	else
	{
		drawPos.x=x;
		drawPos.y=y;
	}
	if (isInsideRect(0,0,lockWidth,lockHeight,drawPos.x,drawPos.y))
	{
		if (bitDepth == 16)
			*(pBits16 + drawPos.y * pitch + drawPos.x) = (WORD)c;
		else
			*(pBits + drawPos.y * pitch + drawPos.x) = c;
	}
}

void CBaseTexture::line(const Crect &r, COLOR c, step_2 stp)
{
	step_1 stp2;
	stpConvert(r, stp, stp2); 
	line(Cpoint(r.right,r.bottom), c, stp2);
}
void CBaseTexture::line (Cpoint p, COLOR c, step_1 stp)
{
	stpConvert2(p, stp);
	int x0=drawPos.x, y0=drawPos.y;
	locateD(p.x, p.y);
	int yl=p.y-y0;
	
	static int stepy;
	int xl=p.x-x0;
	if (xl<0)
	{
		swap((int&)p.x, x0);
		swap((int&)p.y, y0);
		yl*=-1;
		xl*=-1;
	}
	if (yl<0)
		stepy=-1;
	else
		stepy=1;
	
	//opt stuff
	DWORD yoff=y0*pitch;
	int pitch2=pitch*stepy;
	COLOR *pBits2=pBits+yoff;
	int abs_yl=abs(yl);
	
	int x=x0;
	if (yl && xl) //lutande
	{
		float slope=float(xl)/(float)fabs((double)yl);
		for (int t=0;t<=abs_yl;t++)
		{
			int x2=int(x0 + t * slope);
			for (x=x;x<=x2;x++)
				psetx(pBits2+x, c);
			x--;
			pBits2+=pitch2;
		}
	}
	else if(!yl && xl) // horisontell
	{
		for (int x=x0;x<=p.x;x++)
			psetx(pBits2+x, c);
	}
	else if(yl && !xl) //vertikal
	{
		for (int y=0;y<=abs_yl;y++)
		{
			psetx(pBits2 + x0, c); 
			pBits2+=pitch2;
		}
	}
	else //if(!yl && !xl) en punkt
		psetx(pBits2+x0, c);
}

void CBaseTexture::box (const void *dummy, COLOR c, bool fill, step_2 stp)
{
	Crect r(0, 0, lockWidth, lockHeight);
	box(r, c, fill, stp);
}

void CBaseTexture::box (const Crect &r, COLOR c, bool fill, step_2 stp)
{
	step_1 stp2;
	stpConvert(r, stp, stp2); 
	box(Cpoint(r.right, r.bottom), c, fill, stp2);
}
void CBaseTexture::box(Cpoint p, COLOR c, bool fill, step_1 stp)
{
	stpConvert2(p, stp);
	Crect r(drawPos.x, drawPos.y, p.x, p.y);
	r.normalize();
	if (r.right < 0 || r.bottom < 0 || r.left  >= lockWidth || r.top >=lockHeight)
		return;
	if (r.top<0)
		r.top=0;
	if (r.left<0)
		r.left=0;
	if (r.right>=lockWidth)
		r.right=lockWidth-1;
	if (r.bottom>=lockHeight)
		r.bottom=lockHeight-1;
	if (fill)
	{
		int yoff=r.top*pitch;
		static COLOR *pBits2;
		static WORD *pBits162;
		if (bitDepth==32)
			pBits2=pBits+yoff;
		if (bitDepth==16)
			pBits162=pBits16+yoff;
		for (int y=r.top;y<=r.bottom;y++)
		{
			if (bitDepth==32)
			{
				for (int x=r.left;x<=r.right;x++)
					psetx(pBits2+x,  c);
				pBits2+=pitch;
			}
			if (bitDepth==16)
			{
				for (int x=r.left;x<=r.right;x++)
					*(pBits162+x)=(WORD)c;
				pBits162+=pitch;
			}
		}
	}
	else
	{
		line(Crect(r.left, r.top, r.right, r.top), c);
		line(Cpoint(r.right, r.bottom), c);
		line(Cpoint(r.left, r.bottom), c);
		line(Cpoint(r.left, r.top), c);
	} 
	locateD(p.x, p.y);
}

void CBaseTexture::circle(const Cpoint &p, int r, COLOR c, bool fill, step_1 stp)
{
	float ystep=1.0f/r;
	int y=r;
	float yf=1;
	static int x, x2, y2;
	int vecy, vecx;
	float limit;
	float start=1-ystep/3;
	if (!fill)
	{
		int hx, hx2, vx, vx2;
		int uy, uy2, ny, ny2;
		x=0;
		limit=0.7f-ystep*2;
		for (yf=start;yf>=limit;yf-=ystep)
		{
			x2=int(cos(asin(yf))*r);
			hx=p.x+x;
			hx2=p.x+x2;
			vx=p.x-x;
			vx2=p.x-x2;
			//topp, h,v
			vecy=p.y-y;
			line(Crect(hx, vecy, hx2, vecy), c);
			line(Crect(vx, vecy, vx2, vecy), c);
			//botten, h, v
			vecy=p.y+y;
			line(Crect(hx, vecy, hx2, vecy), c);
			line(Crect(vx, vecy, vx2, vecy), c);
			
			uy=p.y-x;
			uy2=p.y-x2;
			ny=p.y+x;
			ny2=p.y+x2;
			//höger, u, n
			vecx=p.x+y;
			line(Crect(vecx, uy, vecx, uy2), c);
			line(Crect(vecx, ny, vecx, ny2), c);
			//vänster, u, n
			vecx=p.x-y;
			line(Crect(vecx, uy, vecx, uy2), c);
			line(Crect(vecx, ny, vecx, ny2), c);
			
			x=x2;
			y--;
		}
	}
	else
	{
		limit=-ystep*2;
		for (yf=start;yf>=limit;yf-=ystep)
		{
			vecy=p.y-y;
			x=int(cos(asin(yf))*r);
			x2=x+p.x;
			x=p.x-x;
			line(Crect(x, vecy, x2, vecy), c);
			vecy=p.y+y;
			line(Crect(x, vecy, x2, vecy), c);
			y--;
		}
	}
	locateD(p.x, p.y);
}

void CBaseTexture::circlex(const Cpoint &p, int r, COLOR c, bool fill, float start, float end, float aspect, step_1 stp)
{
}

void CBaseTexture::cls()
{
	lock(0,0);
	box(0, 0, 1);//ZeroMemory(pBits, pitch*height*bitDepth/8);
	unlock();
}

void CBaseTexture::psetx(COLOR *dest, COLOR sc)
{
	static COLOR dc;
	if (scTest.isActive())
	{
		if (!doColorTest(scTest, sc))
			return;
	}
	else if(dcTest.isActive())
	{
		if (!doColorTest(dcTest, *dest))
			return;
	}	
	if (aBlend.isActive())
		sc=alphaBlend(*dest, sc);
	if (drawMask.isActive())
		sc=writeBits(*dest, sc, drawMask.getMask());	
	*dest=sc;
}	

void CBaseTexture::operator=(lpDIRECT3DSURFACE srcSurf)
{
	if (!srcSurf)
		return;
	static D3DSURFACE_DESC desc;
	srcSurf->GetDesc(&desc);
	width=actualWidth=desc.Width;
	height=actualHeight=desc.Height;
	format=desc.Format;
	bitDepth=getBitDepthFromFormat(format);
	pd3dSurface=srcSurf;
}

void CBaseTexture::operator=(lpDIRECT3DTEXTURE srcTex)
{
	if (!srcTex)
		return;
	static D3DSURFACE_DESC desc;
	srcTex->GetLevelDesc(0, &desc);
	width=actualWidth=desc.Width;
	height=actualHeight=desc.Height;
	format=desc.Format;
	bitDepth=getBitDepthFromFormat(format);
	mipLevels=pd3dTexture->GetLevelCount();
	hr=pd3dTexture->GetSurfaceLevel(0, &pd3dSurface);
	if (FAILED(hr))
		throw "Failed to get surface from texture.";
	pd3dTexture=srcTex;
	
}

//more drawstuff
void CTexture::drawMB(__int64 mbzoom, int xs, int ys)
{
	static double za, zbi, za0, zbi0, ca, cbi;
	static double scale;
	static int xsize=sw/2;
	static int ysize=sh/2;
	static int i;
	static COLOR c;
	static int limit=2;
	static double axs, ays;
	axs+=(double)xs*scale;
	ays+=(double)ys*scale;
	scale=(double)1/(double)mbzoom;
	static float maxb=0;
	lock(0,0);
	for (int y=-ysize;y<ysize;y++)
	{
		for (int x=-xsize;x<xsize;x++)
		{
			ca=(double)x*scale+axs;
			cbi=(double)y*scale+ays;
			za0=0;
			zbi0=0;
			i=0;
			//za=(ca *ca -cbi * cbi);
			float belopp;
			do
			{
				za=za0*za0 - zbi0*zbi0 + ca;
				zbi=za0*zbi0*2 + cbi;
				za0=za;zbi0=zbi;
				i++;
				belopp=(float)sqrt(za*za+zbi*zbi);
			}while (belopp<limit && i<100);
			if (i>=10)
				c=0;
			else
			{
				//belopp-=2;
				c=COLOR(i*100000+belopp*100);
				c=COLOR(i*2+50+(i*2+50)*256+belopp*256);
				if (maxb<belopp)
					maxb=belopp;
			}
			//c=za;
			pBits[x+sw/2]=c;
		}
		pBits+=pitch;
	}
	unlock();
	locateP(0,0);
	lock(0,0);
	print <<"x: "<<axs<<"         y: "<<ays;
	unlock();
	d_file<<maxb<<endl;
	//scale-=0.01;
}

/////////////////////////////////////
//CTexture

void CTexture::drawText(const string &txt, int x0, int y0, COLOR c, DWORD dtf)
{
	Cfont *fnt=CPrint::font;
	static Cpoint start;
	if (dtf)
	{
		Cpoint size(lockWidth, lockHeight);
		start=fnt->centerText(size, txt);
		if (dtf & centerTextX)
			x0+=start.x;
		if (dtf & centerTextY)
			y0+=start.y;
	}
	fnt->lock(0, D3DLOCK_READONLY);
	COLOR *dest=pBits+x0+y0*pitch;
	COLOR *dest2, *dest3;
	COLOR *src, *src2;
	int x=0,y=0;
	for (UINT i=0;i<txt.size();i++)
	{
		if (txt[i]=='\n')
		{
			x0=0;
			y0+=fnt->maxFontHeight+1;
			dest=pBits+y0*pitch;
			continue;
		}
		if (txt[i]<32 || txt[i]>127) //inte skrivbart
			continue;
		int xsize=fnt->charSize[txt[i]-32].cx;
		if (txt[i]==' ')
		{
			//x0+=xsize;
			//dest+=xsize;
			//continue;
		}
		src2=src=fnt->pBits + (fnt->fTexCoords[txt[i]-32].top) * fnt->pitch + fnt->fTexCoords[txt[i]-32].left;
		dest3=dest2=dest;
		for (y=fnt->maxFontHeight-fnt->charSize[txt[i]-32].cy; y<fnt->maxFontHeight;y++)
		{
			for (x=0;x<xsize;x++)
			{
				int destx=x0+x;
				int desty=y0+y;
				if (destx<lockWidth && desty<lockHeight && destx>=0 && desty>=0)
				{
					if (fnt->b_texture && *src2)
						*dest3 = *src2;
					else if (*src2)
						*dest3 = c;
				}
				dest3++;
				src2++;
			}
			dest2 += pitch;
			dest3=dest2;
			src += fnt->pitch;
			src2=src;
		}
		dest+=xsize;
		x0+=fnt->charSize[txt[i]-32].cx;
	}
	if(x0+x>lockWidth)
	{
		x0=0;
		y0+=fnt->maxFontHeight;
	}
	locateP(x0, y0);
	fnt->unlock();
}

void CTexture::drawText2(const string &s) //Används av CPrint
{
	drawText(s, printPos.x, printPos.y, defCol);
}

BOOL CTexture::createBackBuffer()
{
	release();
	if (FAILED(pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pd3dSurface)))
		return FALSE;
	width=sw;
	height=sh;
	return TRUE;
}

//input functions

bool CTexture::input(int &var)
{
	string var2;
	bool b=input(var2, true);
	var=atoi(var2.c_str());
	return b;

}

bool CTexture::input(double &var)
{
	string var2;
	bool b=input(var2, true);
	var=atof(var2.c_str());
	return b;
}
bool CTexture::input(char &var)
{
	string var2;
	bool b=input(var2);
	var=var2[0];
	return b;
}

bool CTexture::input(string &var, bool number)
{
	static int i=0;
	static bool dot=false;
	static bool init=true;
	static CTexture *inpArea;
	static Cpoint start;
	if (init)
	{
		start=printPos;
		int w=lockWidth-printPos.x, h=print.font->maxFontHeight;
		inpArea = new CTexture;
		inpArea->createTexture(w, h, format, 1, D3DPOOL_SYSTEMMEM);
		if (!inpArea->lock(0,0) || !lock(0,0))
			return true;
		for (int y=0;y<h;y++)
			for (int x=0;x<sw;x++)
				inpArea->pset(x, y, point(x+printPos.x, y+printPos.y));
		unlock();
		inpArea->unlock();
		
	}

	if (i==0)
		var="";
	char c=inkey();
	
	if (number && (dot && c==46 || c<48 || c>57))
		return true; //mer än ett kommatecken eller ej siffra
	if (c>=32 && c<=127 && (i<inputl || inputl==0))
	{
		var+=c;
		i++;
		lock(0, 0);
		print << c;
		unlock();
	}
	else if (c==13) //return
	{
		print << newl;
		i = 0;
		delete inpArea;
		init = true;
		return false;
	}
	else if (c==8 && i>0) //sudda
	{
		i--;
		int w=print.font->charSize[var[i]-32].cx;
		int h=print.font->charSize[var[i]-32].cy;
		if (!inpArea->lock(0,0) || !lock(0,0))
			return true;
		for (int y = printPos.y; y < h + printPos.y; y++)
			for (int x = printPos.x - w; x < printPos.x; x++)
				pset(x, y, inpArea->point(x-start.x, y-printPos.y));
		unlock();
		inpArea->unlock();
		
		var.erase(i, 1);
		printPos.x-=w;
	}
	init = false;
	return true;
}

void CTexture::setInputLength(int length)
{
	inputl=length;
}

//////////////////////////////////////
//CPrint

void setFont(Cfont *name) //friend
{
	CPrint::font=name;
	CSRLIN=1;
}

Cfont *getFont() //friend
{
	return CPrint::font;
}

CPrint &CPrint::operator<<(const string &right)
{
	base->drawText2(right);
	return *this;
}

CPrint &CPrint::operator<<(char right)
{
	if (right==newl)
	{
		locateP(0, printPos.y+font->maxFontHeight+1);
		return *this;
	}
	string s;
	s=right;
	base->drawText2(s);
	return *this;
}

CPrint &CPrint::operator<<(int right)
{
	char s[100];
	_itoa_s(right, s, 10);
	base->drawText2(s);
	return *this;
}

CPrint &CPrint::operator<<(float right)
{
	*this<<(double)right;
	return *this;
}

CPrint &CPrint::operator<<(double right)
{
	char s[100];
	sprintf_s(s, "%f", right);
	base->drawText2(s);
	return *this;
}

//////////////////////////////
//CPrintUsing

CPrintUsing2 CPrintUsing::operator<<(const string &f)
{
	int l=f.size();
	int ld=0,rd=0;
	bool dp=false;
	for (int i=0;i<l;i++)
	{
		if (f[i]=='#' && !dp)
			ld++;
		else if(f[i]=='#' && dp)
			rd++;
		else if(f[i]=='.')
			dp=true;
	}
	puFormat format;
	format.ld=ld;
	format.rd=rd;
	format.dp=dp;
	CPrintUsing2 pu2(print, format);
	return pu2;
}

///////////////////////////////////////////////
//Cmpointer
Cmpointer::Cmpointer()
{
	visible=false;
}

void Cmpointer::init(Cpak *pak, const string &file)
{
	image.release();
	image.createSurface(20, 20, sf);
	image.loadImage(pak, file);
	setDrawfx(dfx_isDestBlack);
	image.lock(0,0);
	image.box(0, 0, true);
	image.unlock();
	setDrawfx(0);
}

void Cmpointer::draw()
{
	if (visible)
	{
		setDrawfx(dfx_isSrcAlpha);
		image.put(&bbuf, 0, &mpos);
		setDrawfx(0);
	}
}

////////////////////////////////
//CgammaRamp
CgammaRamp::CgammaRamp()
{
	for (int i=0;i<256;i++)
	{
		idGamma.red[i]=i*256;
		idGamma.green[i]=i*256;
		idGamma.blue[i]=i*256;
	}
}

void setGammaRamp(float r, float g, float b, bool calibrate)
{
	if (r<0) r=0;
	if (g<0) g=0;
	if (b<0) b=0;

	static D3DGAMMARAMP newGamma;
	DWORD temp;
	for (int i=0;i<256;i++)
	{
		(temp=DWORD(CgammaRamp::idGamma.red[i]*r))>65535 ? newGamma.red[i]=65535 : newGamma.red[i]=(WORD)temp;
		(temp=DWORD(CgammaRamp::idGamma.green[i]*g))>65535 ? newGamma.green[i]=65535 : newGamma.green[i]=(WORD)temp;
		(temp=DWORD(CgammaRamp::idGamma.blue[i]*b))>65535 ? newGamma.blue[i]=65535 : newGamma.blue[i]=(WORD)temp;
	}
	DWORD flags;
	if (calibrate)
		flags=D3DSGR_CALIBRATE;
	else
		flags=D3DSGR_NO_CALIBRATION;
	pd3dDevice->SetGammaRamp(0, flags, &newGamma);
}

void restoreGammaRamp()
{
	pd3dDevice->SetGammaRamp(0, D3DSGR_CALIBRATE, &CgammaRamp::idGamma);
}

