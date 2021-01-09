#include "dxstuff.h"
#include "surfaces.h"
#include "object3d.h"
#include "gui.h"

Ccontrol::Ccontrol()
{
	ZeroMemory(&pos, sizeof(pos));
	ZeroMemory(&size, sizeof(size));
	container=0;
	visible=false;
}

Ccontrol::Ccontrol(Cpoint &p, Cpoint &s, vscreen *dest)
{
	setContainer(dest);
	pos=p;
	size=s;
	bkgCopy.createSurface(size.x, size.y, sf);
	getBkg();
	visible=false;
}

Ccontrol::~Ccontrol()
{
}

/////////private:
void Ccontrol::getBkg()
{
	if (container && visible)
	{
		Crect sr(pos.x, pos.y, pos.x+size.x, pos.y+size.y);
		Cpoint dp(0,0);
		container->put(&bkgCopy, &sr, &dp);
	}
}

void Ccontrol::restoreBkg()
{
	if (container && visible)
		bkgCopy.put(container, 0, &pos);
}
//////////////////////

void Ccontrol::setVisible(bool b)
{
	if (visible && !b)
		restoreBkg();
	if (!visible && b)
	{
		visible=b;
		getBkg();
	}
	visible=b;
}

void Ccontrol::setContainer(vscreen *dest)
{
	container=dest;
	contRect=container->getRect();
}

void Ccontrol::setPos(Cpoint &p)
{
	restoreBkg();
	pos=p;
	getBkg();
}

void Ccontrol::setSize(Cpoint &s)
{
	restoreBkg();
	size=s;
	bkgCopy.release();
	bkgCopy.createSurface(size.x, size.y, sf);
	getBkg();
}

Cpoint Ccontrol::getPos()
 {return pos;}
Cpoint Ccontrol::getSize()
 {return size;}
	
//Cbutton///////////////////
Cbutton::Cbutton()
{ 
	ZeroMemory(imgUsage, sizeof(imgUsage));
	ZeroMemory(images, sizeof(images));
	resetStatus();
	 //sätt font och färg till globala inställningar
	font=::getFont();
	textcol=defCol;
}

Cbutton::Cbutton(Cpoint &p, Cpoint &s, vscreen *dest)  : Ccontrol(p, s, dest)
{
	ZeroMemory(imgUsage, sizeof(imgUsage));
	ZeroMemory(images, sizeof(images));
	resetStatus();
	//sätt font och färg till globala inställningar
	font=::getFont();
	textcol=defCol;
	for (int i=0;i<3;i++)
	{
		images[i] = new CTexture;
		images[i]->createSurface(size.x, size.y, sf);
		images[i]->cls();
	}
}

Cbutton::~Cbutton()
{
	for (int i=0;i<3;i++)
		safeDelete(images[i]); //anropar ~CBaseTexture()
}

//private:
void Cbutton::changePic()
{
	if (!visible)
		return;
	Cpoint p(pos.x+contRect.left, pos.y+contRect.top);
	//återställning av utseende
	if (mposMsg==beStop)
		images[0]->put(container, 0, &p);
	//muspekare flyttas till knapp eller knapp släpps upp
	else if ((mposMsg==beStart || mousePos && mbutMsg==beStop) && imgUsage[0])
		images[1]->put(container, 0, &p);
	//knapp trycks ner
	else if (mousePos && mbutMsg==beStart && imgUsage[1])
		images[2]->put(container, 0, &p);
	drawText();
}

void Cbutton::drawText()
{
	//spara värden
	Cpoint p=printPos;
	Cfont *f=::getFont();
	int row=CSRLIN;
	
	::setFont(font);
	Crect dr(pos.x, pos.y, pos.x+size.x, pos.y+size.y);
	//Cpoint start=font->centerText(size, str);
	container->lock(&dr, 0);
	container->drawText(str, 0, 0, textcol, centerTextXY);
	container->unlock();
	
	//återställ värden
	::setFont(f); 
	printPos=p;
	CSRLIN=row;
}

///////////////////

void Cbutton::setPos(Cpoint &p)
{
	Ccontrol::setPos(p);
	draw();
}

void Cbutton::setSize(Cpoint &s)
{
	Ccontrol::setSize(s);
	for (int i=0;i<=3;i++)
	{
		safeDelete(images[i]);
		images[i] = new CTexture;
		images[i]->createSurface(size.x, size.y, sf);
		images[i]->cls();
	}
}
	
void Cbutton::loadImage(int i, Cpak *pak, const string &file, const Crect *sr, const Crect *dr, DWORD filter, D3DCOLOR colorkey)
{
	if (!images[i])
	{
		images[i] = new CTexture;
		images[i]->createSurface(size.x, size.y, sf);
	}
	images[i]->loadImage(pak, file, sr, dr, filter, colorkey);
}

void Cbutton::draw(int pic)
{
	if (!visible)
		return;
	Cpoint p(pos.x+contRect.left, pos.y+contRect.top);
	if (pic!=-1)
		images[pic]->put(container, 0, &p);
	else
	{
	//standardutseende
	if (!mouseButton && !mousePos)
		images[0]->put(container, 0, &p);
	//muspekare på knapp
	else if (mousePos && !mouseButton && imgUsage[0])
		images[1]->put(container, 0, &p);
	//knapp nertryckt
	else if (mousePos && mouseButton && imgUsage[1])
		images[2]->put(container, 0, &p);
	}
	drawText();
}

void Cbutton::setImgUsage(bool iu[])
{
	for (int i=0;i<=1;i++)
		imgUsage[i]=iu[i];
}
void Cbutton::setSndUsage(bool su[])
{
}

void Cbutton::listen()
{
	Crect r;
	r.left=contRect.left+pos.x; r.top=contRect.top+pos.y;
	r.right=r.left+size.x; r.bottom=r.top+size.y;

	DWORD mbutMsgTemp=mbutMsg, mposMsgTemp=mposMsg;
	if (mpos.x>r.left && mpos.x<r.right &&
		mpos.y>r.top && mpos.y<r.bottom)
		//muspekare på knappen
	{ 
		if (mposMsg == beStart || mposMsg == beActive)
			mposMsg = beActive;
		else  //mposMsg = 0 eller beStop
			mposMsg = beStart;
		mousePos=true;
	}
	else
	{
		if (mposMsg == beStart || mposMsg == beActive) 
			mposMsg = beStop;
		else //mposMsg = 0 eller beStop
			mposMsg=0;
		mousePos=false;
		mbutMsg=0;
	}
			
	if (mstate.rgbButtons[0] & 0x80)   //musknapp nertryckt
	{  
		if (mbutMsg == beStart || mbutMsg == beActive)
			mbutMsg = beActive;
		else //mbutMsg = 0 eller beStop
		{
			if (mousePos) //muspekare på knapp
				mbutMsg = beStart;
		}
		mouseButton=true;
	}
	else
	{
		if (mbutMsg == beStart || mbutMsg == beActive) 
			mbutMsg = beStop;
		else //mbutMsg = 0 eller beStop
			mbutMsg=0; 
		mouseButton=false;
	}
	//rita om knapp vid förändring av tillstånd
	if (mbutMsg != mbutMsgTemp || mposMsg != mposMsgTemp)
		changePic();
}

void Cbutton::resetStatus()
{
	mbutMsg=mposMsg=mouseButton=mousePos=0;
}

int Cbutton::getClickMsg()
{
	return mbutMsg;
}

int Cbutton::getPointMsg()
{
	return mposMsg;
}

void Cbutton::setString(const string &s)
{
	str=s;
	draw(0);
}

string Cbutton::getString()
{
	return str;
}

void Cbutton::setFont(Cfont *fnt)
{
	font=fnt;
}

Cfont *Cbutton::getFont()
{
	return font;
}

void Cbutton::setTextColor(COLOR c)
{
	textcol=c;
}

COLOR Cbutton::getTextColor()
{
	return textcol;
}

void Cbutton::setVisible(bool v)
{
	Ccontrol::setVisible(v);
	draw(0);
}

//Cspin//////////////////
#define spin_delay 300
#define spin_acc 5
Cspin::Cspin()
{
	value=min=0;
	max=100;
	delay=delay0=spin_delay;
	acc=spin_delay;
	spinSize=0.25f;
	step=1;
}
Cspin::Cspin(Cpoint &p, Cpoint &s, vscreen *dest) : Ccontrol(p, s, dest)
{
	value=min=0;
	max=1000;
	delay=delay0=spin_delay;
	acc=spin_acc;
	spinSize=0.25f;
	step=5;
	
	setSize(size);
	setPos(pos);
	setValue(value);
	setContainer(dest);
}

Cspin::~Cspin()
{
}

void Cspin::listen()
{
	static DWORD tid=timeGetTime();
	static DWORD atid=tid;
	arrow1.listen();
	arrow2.listen();
	float v=value;
	if (value<max)
	{
		if (arrow1.getClickMsg()==beStart ||
			arrow1.getClickMsg()==beActive && timeGetTime()-tid>(DWORD)delay)
			value+=step;
		if (value>max) value=max;
	}
	if (value>min)
	{
		if (arrow2.getClickMsg()==beStart ||
			arrow2.getClickMsg()==beActive && timeGetTime()-tid>(DWORD)delay)
			value-=step;
		if (value<min) value=min;
	}
	if (v!=value)
	{
		tid=timeGetTime();
		setValue(value);	
	}
	if (!arrow1.getClickMsg() && !arrow2.getClickMsg())
		delay=delay0;
	else 
		if (timeGetTime()-atid>100)
		{
			atid=timeGetTime();
			delay-=acc;
			if (delay<0) delay=0;
		}
}

void Cspin::setSize(Cpoint &s)
{
	size=s;
	Cpoint spSize(int(size.x*spinSize), size.y/2);
	arrow1.setSize(spSize);
	arrow2.setSize(spSize);
	Cpoint reSize(size.x-spSize.x, size.y);
	result.setSize(reSize);
}
void Cspin::setPos(Cpoint &p)
{
	pos=p;
	Cpoint spPos(int(pos.x+size.x*(1-spinSize)), pos.y);
	arrow1.setPos(spPos);
	spPos.y+=size.y/2;
	arrow2.setPos(spPos);
	result.setPos(pos);
}

void Cspin::setValue(float v)
{
	value=v;
	static char s[50];
	sprintf_s(s, "%g", value);
	result.setString(s);
}

void Cspin::draw()
{
	arrow1.draw();
	arrow2.draw();
	result.draw();
	setValue(value);
}

void Cspin::setVisible(bool v)
{
	arrow1.setVisible(v);
	arrow2.setVisible(v);
	result.setVisible(v);
}
	
void Cspin::setContainer(vscreen *dest)
{
	arrow1.setContainer(dest);
	arrow2.setContainer(dest);
	result.setContainer(dest);
}

void Cspin::setFont(Cfont *fnt)
{
	result.setFont(fnt);
	arrow1.setFont(fnt);
	arrow2.setFont(fnt);
}
