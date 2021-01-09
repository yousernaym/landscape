#include "dxstuff.h"
#include "surfaces.h"
#include "pixelmanip.h"

//dfx-mallar
Cdrawfx dfx_isSrcAlpha(0, 1, 0);
Cdrawfx dfx_isDestAlpha(0, 2, 0);
Cdrawfx dfx_isSrcBlack(0, 1, 0);
Cdrawfx dfx_isDestBlack(0, 2, 0);
Cdrawfx dfx_aBlend(1, 0, 0);

static CinitTemplates dummy;
//global functions//////////////////////////////

bool doColorTest(CcolorTest &test, COLOR c)
{
	if (test.getFunc()==cmp_greaterequal)
	{
		if (((c & test.getMask()) < test.getRef()))
			return false;
	}
	else if (test.getFunc()==cmp_equal)
	{
		if (((c & test.getMask()) != test.getRef()))
			return false;
	}
	return true;
}

COLOR alphaBlend(COLOR dc, COLOR sc)
{
	static int ashift=cdepth*6, rshift=cdepth*4, gshift=cdepth*2;
	static BYTE sa, sr, sg, sb, da, dr, dg, db;
	static int a;
	sa=BYTE((sc & 0xff000000) >> ashift);
	sr=BYTE((sc & 0xff0000) >>rshift);
	sg=BYTE((sc & 0xff00) >>gshift);
	sb=BYTE(sc & 0xff);
	da=BYTE((dc & 0xff000000) >>ashift);
	dr=BYTE((dc & 0xff0000) >>rshift);
	dg=BYTE((dc & 0xff00) >>gshift);
	db=BYTE(dc & 0xff);
			
	a=sa;
				
	da+=sa;
	if (da>0xff)
		da=0xff;
	dr=((a*(sr-dr))>>8)+dr;
	dg=((a*(sg-dg))>>8)+dg;
	db=((a*(sb-db))>>8)+db;
	
	return (da<<24) + (dr<<16) + (dg<<8) + db;
}

//////////////////////////////////////////////////////


CtranspMode::CtranspMode()
{
	active=false;
}

bool CtranspMode::isActive()
{
	return active;
}

//------------------------------------------------

void CalphaBlend::setActive(bool b)
{
	active=b;
	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, b);
}

//----------------------------------------------------

CcolorTest::CcolorTest()
{
	ref=0x01000000;
	mask=0xff000000;
}

COLOR CcolorTest::getRef()
{
	return ref;
}

COLOR CcolorTest::getMask()
{
	return mask;
}

cmpFunc CcolorTest::getFunc()
{
	return func;
}

//----------------------------------------------------

void CscTest::setActive(bool b)
{
	active=b;
	if (mask & 0xff000000)
		pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, b);
	if (b)
		CBaseTexture::dcTest.setActive(false);
}

void CscTest::setMask(COLOR m)
{
	COLOR olda=mask & 0xff000000, newa=m & 0xff000000;
	//om aktiv och alphakanalen öppnas eller stängs
	if (active)
	{
		if (olda && !newa)
			pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		else if (!olda && newa)
			pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	}
	mask=m;
}

void CscTest::setRef(COLOR r)
{
	ref=r;
	pd3dDevice->SetRenderState(D3DRS_ALPHAREF, ref>>24);
}

void CscTest::setFunc(cmpFunc f)
{
	func=f;
	/*if (func==cmp_greaterequal)
		pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	else if (func==cmp_equal)
		pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_EQUAL);*/
	pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, func);

}

//-------------------------------------------

void CdcTest::setActive(bool b)
{
	active=b;
	if (b)
		CBaseTexture::scTest.setActive(false);
}

void CdcTest::setMask(COLOR m)
{
	mask=m;
}
	
void CdcTest::setRef(COLOR r)
{
	ref=r;
}

void CdcTest::setFunc(cmpFunc f)
{
	func=f;
}	

//////////////////////////////////
CinitTemplates::CinitTemplates()
{
		dfx_isSrcAlpha.testRef=0x01000000;
		dfx_isSrcAlpha.testMask=0xff000000;
		dfx_isSrcAlpha.testFunc=cmp_greaterequal;

		dfx_isDestAlpha=dfx_isSrcAlpha;
		dfx_isDestAlpha.colTestEnable=2;

		dfx_isSrcBlack.testRef=0;
		dfx_isSrcBlack.testMask=0x00ffffff;
		dfx_isSrcBlack.testFunc=cmp_equal;

		dfx_isDestBlack=dfx_isSrcBlack;
		dfx_isDestBlack.colTestEnable=2;
}	
