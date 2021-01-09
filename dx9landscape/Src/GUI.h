#ifndef __GUI__
#define __GUI__

#define beStart  0x1 
#define beActive 0x2 
#define beStop   0x4 

class Ccontrol
{
protected:
	CTexture bkgCopy;
	vscreen *container;
	Crect contRect;
	Cpoint pos;
	Cpoint size;
	bool visible;
	void getBkg();
	void restoreBkg();
public:
	Ccontrol();
	Ccontrol(Cpoint &p, Cpoint &s, vscreen *dest);
	~Ccontrol();
	void setContainer(vscreen *dest);
	Cpoint getPos(); 
	Cpoint getSize();
	void setPos(Cpoint &p);
	void setSize(Cpoint &s);
	void setVisible(bool b);
};

class Cbutton : public Ccontrol
{
private:
	CTexture *images[3];
	bool imgUsage[2];
	bool mouseButton;
	bool mousePos;
	int mbutMsg;
	int mposMsg;
	string str;
	Cfont *font;
	COLOR textcol;
	void changePic();
	void drawText();
public:
	Cbutton();
	Cbutton(Cpoint &p, Cpoint &s, vscreen *dest);
	~Cbutton();
	void setPos(Cpoint &p);
	void setSize(Cpoint &s);
	void draw(int pic=-1);
	void listen();
	void loadImage(int i, Cpak *pak, const string &file, const Crect *sr=NULL, const Crect *dr=NULL, DWORD filter=D3DX_DEFAULT, D3DCOLOR colorkey=0);
	int getClickMsg();
	int getPointMsg();
	void setImgUsage(bool iu[]);
	void setSndUsage(bool su[]);
	void resetStatus();
	void setString(const string &s);
	string getString();
	void setFont(Cfont *fnt);
	Cfont *getFont();
	void setTextColor(COLOR c);
	COLOR getTextColor();
	void setVisible(bool v);
};
	
class CWindow : public vscreen
{
public:
	void **controls;
	void listen();
	DWORD getData(DWORD type);
};

class Cspin : public Ccontrol
{
private:
	int delay;
	float spinSize;
public:
	float value;
	float min;
	float max;
	int delay0;
	float step;
	int acc;
	Cbutton arrow1;
	Cbutton arrow2;
	Cbutton result;
	Cspin();
	Cspin::Cspin(Cpoint &p, Cpoint &s, vscreen *dest) ;
	~Cspin();
	void listen();
	void setSize(Cpoint &s);
	void setPos(Cpoint &p);
	void setValue(float v);
	void setVisible(bool v);
	void setFont(Cfont *fnt);
	void draw();
	void setContainer(vscreen *dest);
};

#endif