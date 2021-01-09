#ifndef __main__
#define __main__

class C3dEntity : public object3d
{
public:
	void update();
};

class CbaseEffect
{
private:
	CbaseEffect();
	virtual ~CbaseEffect()=0;
	virtual void update()=0;
};

class CeffectHandler
{
private:
	CbaseEffect *effect;
public:
	CeffectHandler();
	~CeffectHandler();
	void update();
};

class CmoveWindow : public CbaseEffect
{
    CmoveWindow();
	~CmoveWindow();
	void update();
};

class Csettings
{
public:
	Csettings();
	bool waterWaves;
	bool sky;
	bool skyRefl;
	bool localRefl;
	bool refraction;
	Cd3dInit d3di;
};

class Ccommonfx
{
private:
	LPD3DXEFFECT effect;
	D3DXHANDLE handle_camPos;
public:
	void init();
	void updateParams();
	void release();
};

#endif