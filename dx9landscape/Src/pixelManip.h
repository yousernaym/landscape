#ifndef __pixelmanip__
#define __pixelmanip__

class CtranspMode
{
protected:
	bool active;
public:
	CtranspMode();
	bool isActive();
};

class CalphaBlend : public CtranspMode
{
public:
	void setActive(bool b);
};

class CcolorTest : public CtranspMode
{
protected:
	COLOR ref;
	COLOR mask;
	cmpFunc func;
public:
	CcolorTest();
	COLOR getRef();
	COLOR getMask();
	cmpFunc getFunc();
};

class CscTest : public CcolorTest
{
public:
	void setActive(bool b);
	void setMask(COLOR m);
	void setRef(COLOR r);
	void setFunc(cmpFunc f);
};

class CdcTest : public CcolorTest
{
public:
	void setActive(bool b);
	void setMask(COLOR m);
	void setRef(COLOR r);
	void setFunc(cmpFunc f);
};

//////////////////////////////////////////////
class CdrawMask
{
private:
	bool active;
	COLOR mask;
public:
	CdrawMask() {active=false; mask=0xffffffff;}
	void setActive(bool b) {active=b;}
	bool isActive() {return active;}
	void setMask(COLOR m)  {mask=m;}
	COLOR getMask() {return mask;}
};

////////////////////////////////////////////////////
class Cdrawfx
{
public:
	bool aBlendEnable;
	int colTestEnable;
	cmpFunc testFunc;
	COLOR testRef;
	COLOR testMask;
	bool drawMaskEnable;
	COLOR drawMask;
	Cdrawfx(){ ZeroMemory(this, sizeof(*this)); }
	Cdrawfx(bool blend, int test, bool mask)
	{
		aBlendEnable=blend;colTestEnable=test;drawMaskEnable=mask;
	}
};
/////////////////////////////////////////
class CinitTemplates
{
public:
	CinitTemplates();
};

bool doColorTest(CcolorTest &test, COLOR c);
COLOR alphaBlend(COLOR dc, COLOR sc);
COLOR setColorBits(COLOR c, COLOR mask, COLOR cbits);

//dfx-mallar
extern Cdrawfx dfx_isSrcAlpha;
extern Cdrawfx dfx_isDestAlpha;
extern Cdrawfx dfx_isDestBlack;
extern Cdrawfx dfx_aBlend;

#endif
