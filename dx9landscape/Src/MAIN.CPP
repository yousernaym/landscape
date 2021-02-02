#include "gameengine.h"
#include "main.h"

//#define __DEPLOY__

//D3d objects----------------------
static C3dEntity object;
static object3d musicObj;
static Clandscape landscape;
static Cwater water;

static vscreen os;
static Cbillboard os2;

//static Csound bkgmusic;
//static CaudioPath apStd3d;
//static CaudioPath apStdStereo;
//static IDirectSound3DBuffer8 *sbuf3d;
//static IDirectSound3DListener8 *slis;

static Cvshader land_vsh;
static Cpshader land_psh;
static Cpshader landRefl_psh;
static Cvshader water_vsh;
static Cpshader water_psh;
static Cpshader waterUW_psh;
static Cvshader sky_vsh;
static Cpshader sky_psh;
static Ccommonfx commonfx;
static LPD3DXEFFECT lscapefx;
static LPD3DXEFFECT skyfx;

//static Cvshader *pland_vsh=&land_vsh;
//static Cpshader *pland_psh=&land_psh;
//static Cvshader *pwater_vsh=&water_vsh;
//static Cpshader *pwater_psh=&water_psh;
//static Cvshader *psky_vsh=&sky_vsh;;
//static Cpshader *psky_psh=&sky_psh;;
//--------------------------------------

//Misc variables----------------------
static int geomip=0;
static float scale=1;
static Csky sky;
static UINT lsize=256;
static UINT subsize=64;
static D3DXCOLOR skycol(0xff28546f);
//static D3DXCOLOR skycol(0xff359cd1);
static D3DXCOLOR uwcol(0xff001535);
static float fogDensity=0.00000f;
static Cfont stdfnt;
static Clight sun;
static bool showfps=false;
static Csettings settings;
static int lReflTexIndex = 0;
static int refractionTexIndex = 4;
//----------------------------------------

//App Data---------------------------------
#ifdef __DEPLOY__
	static string workingDir = "";
#else
	static string workingDir = "..\\dependencies\\runtime\\";
#endif
static string dataPath = workingDir;
static string shaderPath = "Shaders\\";

//Paths in pak file
static string gfxPath = "gfx\\";
static string audioPath = "audio\\";

Cpak dataPak;
Cpak* shaderPak = &dataPak;
//----------------------------------------------

void setFog();
void render();
void updateEnvMap();
void createLandscape(Clandscape &landscape, Cwater &water, UINT xsize, UINT zsize, const Clight &light);
void setShaderConstants();
void drawLocalReflections();
void drawRefraction();

void initApp()
{
	g_bc=skycol;
	srand(timeGetTime());
	
	settings.d3di.BackBufferWidth=1280;
	settings.d3di.BackBufferHeight=1024;
		
	initDx(settings.d3di);

	if (!(d3dCaps.SrcBlendCaps & D3DPBLENDCAPS_DESTALPHA)  || !(d3dCaps.DestBlendCaps & D3DPBLENDCAPS_INVDESTALPHA))
		throw "Hardware doesn't support destination alpha blends.";
	
	if (!dataPak.load(dataPath + "data.dat"))
		throw (string)"Couldn't load " + workingDir + "\\data.dat";
    	
	nearClip=0.05f;
	farClip=1000;
	screenTransform();
			
	Cfont initFnt;
	if (!initFnt.create(27, "copperplate gothic bold", 0, 0, &dataPak, gfxPath+"arctic.bmp"))
		throw "Couldn't create font.";
	setFont(&initFnt);
		
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
		
	bbuf.createBackBuffer();
	
	os.setMaxSpeed(0.25f*scale);
	os.setRect(&Crect(0,0,30,30));
	os.createTexture(30, 30, sf, 1, D3DPOOL_MANAGED);
	if (os.lock(0,0))
	{
		os.box(0, 0, true);
		os.unlock();
	}
	//os2.setRect();
	os2.createTexture(sw, sh, sf, 1, D3DPOOL_MANAGED);
	if (os2.lock(0,0))
	{
		os2.box(0, 0, true);
		os2.unlock();
	}

	fVector dir(-1, -0.3f, 0.5f);
	dir.normalize();
	sun.Direction=dir;
	sun.Type=D3DLIGHT_DIRECTIONAL;
	sun.setAllComps(D3DXCOLOR(1, 1, 1, 1));
	sun.set();
	sun.enable(true);
	
	//setFog();
	
	cam.setMaxSpeed((float)0.01*scale);
	cam.setPos(140,10.5,83);
		
	object.setPos(cam.pos.x,cam.pos.y,cam.pos.z);
	object.setAngle(0,0,0);
	object.setMaxSpeed(0.50);
	//object.acc*=2;
	//object.friction*=2.5;
	object.maxRotSpeed *= 1.1f;
	object.rotAcc*=1.5;
	object.rotBrakeFriction*=1.3f;
	object.rotFriction*=100.5f;
	//object.brakeFriction = 0;
	//object.friction=0;
	object.show(false);
		
	//if (!sky_vsh.createShader(shaderPak, shaderPath+"sky.vsh", d3dDebug))
	//	throw "Couldn't create sky vertex shader";
	//if (!sky_psh.createShader(shaderPak, shaderPath+"sky.psh", d3dDebug))
	//	throw "Couldn't create sky pixel shader";
	//sky.setVertexShader(&psky_vsh);
	//sky.setPixelShader(&psky_psh);
	sky.create(&dataPak, gfxPath + "sphere.x", &dataPak, gfxPath + "hemispherical.jpg", shaderPak, shaderPath + "sky.fxo");
		
	if (!envMap.createMap(1024, sf, D3DFMT_D16, 1))
		throw "Couldn't create environment map.";
	envMap.enable(true);

	createLandscape(landscape, water, lsize, lsize, sun);
	
	/*apStd3d.createAudioPath(DMUS_APATH_DYNAMIC_3D, 1);
	apStdStereo.createAudioPath(DMUS_APATH_DYNAMIC_STEREO, 1);
	sbuf3d=apStd3d.get3dBuffer();
	slis=apStd3d.getListener();*/
	//slis->SetRolloffFactor(1, DS3D_IMMEDIATE);

	if (!musicObj.loadXfile(&dataPak, gfxPath + "triplane.x"))
		throw "Couldn't load triplane.x.";
	fVector min=landscape.getMinHeightPos();
	musicObj.setPos(min.x, 2, min.z);
	landscape.pointCollision(musicObj, 8.0f, 1, true, 1);
	musicObj.scaling(0.3f,0.3f,0.3f);
	musicObj.update(false);

	stdfnt.create(12, "arial", 0, 0);
	stdfnt.b_texture=true;
	
	if (stdfnt.lock(0,0))
	{
		Cdrawfx fx=dfx_isDestAlpha;
		fx.drawMask=0xffffff;
		fx.drawMaskEnable=true;
		setDrawfx(fx);
		stdfnt.box(0, 0xffffff, true);
	
		Cdrawfx fx2=dfx_isDestBlack;
		fx2.testMask=0xffffffff;
		setDrawfx(fx2);
		stdfnt.box(0, 0x70000000, true);
		stdfnt.unlock();
		setDrawfx(0);
	}
	setFont(&stdfnt);

	waveCalcLimit=50;
	fps.setRefFps(50);
	fps.update();

	commonfx.init();
	
	dataPak.close();
	
	//fps.active=false;
}

bool main()
{	
	commonfx.updateParams();
	screenTransform();
	fps.update();
	//fps.s=0.1;
	if (keydown(DIK_M))
		musicObj.setPos(object.pos);
	fVector pos=object.pos;
	//slis->SetPosition(pos.x, pos.y, pos.z, DS3D_DEFERRED);
	fVector lookDir=object.lookDir;
	fVector upDir=object.upDir;
	//slis->SetOrientation(lookDir.x, lookDir.y, lookDir.z, upDir.x, upDir.y, upDir.z, DS3D_DEFERRED);
	fVector vel=object.velocity;
	//slis->SetVelocity(vel.x, vel.y, vel.z, DS3D_DEFERRED);
	//slis->SetDopplerFactor(8, DS3D_DEFERRED);
	//slis->CommitDeferredSettings();
	/*if (keypress(DIK_SPACE))
	{
		bkgmusic.play();
	}*/
	/*if (keypress(DIK_Z))
	{
		DWORD dwBuffer=0;
		IDirectSound3DBuffer8 *sb3d;
		IDirectMusicAudioPath8 *apath=apStd3d.getAudioPath();
		while (SUCCEEDED(apath->GetObjectInPath(DMUS_PCHANNEL_ALL, DMUS_PATH_BUFFER, dwBuffer, GUID_NULL, 0, IID_IDirectSound3DBuffer8, (void**)&sb3d)))
		{
			dwBuffer++;
			sb3d->Release();
		}
		d_file<<"3d buffers: "<<dwBuffer<<" ";
	}*/
	
	static float w=0;
	w+=0.1f;
	if (w>6.282) w=0;
	if (keypress(DIK_ESCAPE))
	{
		PostQuitMessage(0);
		return 0;
	}
	for (UINT joy=0;joy<numJoys;joy++)
	{
		if (keypress(DIK_1+joy))
			defJoy=joy;
	}
	/*for (int lod=0;lod<10;lod++)
	{
		if (keypress(DIK_1+lod-1))
		{
			int sign=1;
			if (keydown(DIK_NUMPADMINUS))
				sign=-1;
			CTexture *tex=landscape.getTex(0);
			tex->setMipBias(lod*sign);
			geomip=lod;
		}
	}*/
	
	bool uw=cam.uw;
	float depth=water.isUnderWater();
	if (depth<0)
	{
		depth=1.0f+depth/20;
		g_bc=uwcol;//*depth;
		//setGammaRamp(depth, depth, depth, 0);
//		pwater_psh=&waterUW_psh;
		lscapefx->SetTechnique("cuw_water");
	}
	else if (uw)
	{
		g_bc=skycol;
		//restoreGammaRamp();
//		pwater_psh=&water_psh;
		lscapefx->SetTechnique("caw_water");
	}
	
	if (fps.getFps()>10 && settings.waterWaves)
	{
		water.wave(w, 0.5f);
	}
		
	object.update();
	float cspeed=landscape.pointCollision(object, 1.0f, 1, true, 1);
		
	cam.viewTransform(fVector(0,0,0), fVector(0,0,1), object);
	//pd3dDevice->SetTransform(D3DTS_VIEW, &object.worldMat);
		
	//shader constants----------
		//land_vsh-----------
		D3DXVECTOR4 c8=cam.pos;
		//float camDepth;
		/*if (cam.pos.y>0)
			camDepth=1;
		else
		{
			camDepth=40+cam.pos.y;
			if (camDepth<0)
				camDepth=0;
			else
				camDepth/=40.0f;
		}
		c8.w=camDepth;*/
//		pland_vsh->setConstantF(8, c8, 1);
		//lscapefx->SetValue("camPos", &cam.pos, 12);

		//land_psh
		
		float distanceBlend=cam.pos.y/480.0f;
		distanceBlend=0.6f-distanceBlend;
		clip<float>(distanceBlend, 0, 0.6f);
		lscapefx->SetFloat("caustIntensity", distanceBlend);
		
		//waterVS/PS------------
		//VS
		D3DXMATRIX m=createTransformation(0x011);
		m=water.worldMat*m*texProjMatrix;
		D3DXMatrixTranspose(&m, &m);
		lscapefx->SetMatrix("reflMat", &m);
		
		float globalScrollSpeed = frames * 0.007f;
		D3DXVECTOR4 scrollOffset(globalScrollSpeed * -0.05f, globalScrollSpeed * 0.1f, globalScrollSpeed * 0.05f, globalScrollSpeed * -0.1f);
		lscapefx->SetVector("waveScrollOffset", &scrollOffset);
		D3DXVECTOR4 scrollOffset2(globalScrollSpeed * 0.15f, globalScrollSpeed * 0.09f, globalScrollSpeed * -0.075f, globalScrollSpeed * -0.15f);
		lscapefx->SetVector("waveScrollOffset2", &scrollOffset2);
		//D3DXVECTOR4 scrollOffset3(globalScrollSpeed * 0.35f, globalScrollSpeed * 0.55f, globalScrollSpeed * -0.15f, globalScrollSpeed * -0.25f);
		D3DXVECTOR4 scrollOffset3(globalScrollSpeed * 0.6f, globalScrollSpeed * 0.10f, globalScrollSpeed * -0.06f, globalScrollSpeed * -0.12f);
		lscapefx->SetVector("waveScrollOffset3", &scrollOffset3);
				
		//PS
		distanceBlend=cam.pos.y/120.0f;
		clip<float>(distanceBlend, 0, 1);
		lscapefx->SetFloat("distBlend", distanceBlend);
		
		updateEnvMap();
		render();
	//---------------------------

	if (keypress(DIK_F))
		showfps=!showfps;
	if (keypress(DIK_T))  //time-based movement?
		fps.active=!fps.active;

	if (frames % 30==0 && showfps)
	{
		locateP(0, 0);
		os.lock(0, 0);
		os.box(0,0,1);
		os.print<<(int)fps.getFps();
		os.unlock();
	}
	//d_file << fps.getFps() << "\n";
	
}

void setFog()
{
	pd3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
	pd3dDevice->SetRenderState(D3DRS_FOGCOLOR, g_bc);
	//pd3dDevice->SetRenderState(D3DRS_FOGCOLOR, uwcol);
	pd3dDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_EXP);
	pd3dDevice->SetRenderState(D3DRS_FOGSTART, *(DWORD*)(&nearClip));
	float fogend=farClip/5;
	pd3dDevice->SetRenderState(D3DRS_FOGEND, *(DWORD*)(&fogend));
	pd3dDevice->SetRenderState(D3DRS_FOGDENSITY, *(DWORD*)(&fogDensity));
	pd3dDevice->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
}
	
void render()
{
	if (FAILED(hr=pd3dDevice->BeginScene()))
		throw "beginscene failed.";
	
	pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, g_bc, 1, 0);
	if (!cam.uw && settings.sky) 
		sky.draw();
	
	drawLocalReflections();
	drawRefraction();
		
	//Dest blend stuff
	const float destBlendStartHeight=100;
	BOOL  b_destAlpha=false;
	if (cam.pos.y>destBlendStartHeight)
		b_destAlpha=true;
//	pland_vsh->setConstantB(2, &b_destAlpha, 1);
	
	//Calculate decrease of transp
	float y = cam.pos.y - destBlendStartHeight;
	if (y < 0) y = 0;
	const float transpChangeSpeed = 0.005f;
	y *= transpChangeSpeed;
	//pland_vsh->setConstantF(10, &y, 1);
	
	if (!cam.uw)
		landscape.draw(0, 0, geomip);
	else
		landscape.draw(&water, 1, geomip);
	
	if (b_destAlpha)
	{
		pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTALPHA);
		pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVDESTALPHA);
		pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	}
		
	//pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	water.draw(-1, geomip);
	//pd3dDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);	
	
	if (b_destAlpha)
	{
		pd3dDevice->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_SRCALPHA);
		pd3dDevice->SetRenderState(D3DRS_DESTBLEND , D3DBLEND_INVSRCALPHA);
		pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
		pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	}
		
	//setDrawfx(dfx_aBlend);
	
	//setDrawfx(dfx_isSrcAlpha);
	//os2.draw();
	
	setDrawfx(0);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	object.draw(0);
	pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    
	musicObj.draw(0);
	
	pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	
	if (showfps)
		os.draw();

	if (FAILED(hr=pd3dDevice->EndScene()))
		throw"endscene failed";
}

void updateEnvMap()
{
	static bool init=false;
	if (!envMap.isEnabled() || init)
		return;
	init=true;
	bool faces[6]={1,1,1,1,1,1};
	envMap.beginScene();
	
	for (int i=0;i<6;i++)
	{
		if (!faces[i])
			continue;
		envMap.setRenderTarget(i);
		
		if (FAILED(hr=pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, g_bc, 1, 0)))
			throw "Couldn't clear cubemap.";
		if (FAILED(hr=pd3dDevice->BeginScene()))
			throw"envmap beginscene failed";
		
		envMap.viewTransform(i, baseObj());

		if (settings.skyRefl)
			sky.draw();
				
		if (FAILED(hr=pd3dDevice->EndScene()))
			throw"envmap endscene failed";
	}
	envMap.endScene();
}


void createLandscape(Clandscape &landscape, Cwater &water, UINT xsize, UINT zsize, const Clight &light)
{
	//Effect init
	if (!(lscapefx = createD3dxEffect(shaderPak, shaderPath+"landscape.fxo", 0)))
		throw "Couldn't create landscape effect";

	landscape.setEffect(lscapefx);
	water.setEffect(lscapefx);
	
	float wlevel=27.0f*scale;
	landscape.createTexs(9);
	landscape.createMtrls(1);
	water.terrain=&landscape;
	water.createTexs(135);
	water.createMtrls(1);
	
	landscape.setMtrl(D3DXCOLOR(0.99f,0.99f,0.99f,1), mtrl_diffuse, 0);
	landscape.setMtrl(D3DXCOLOR(0.4f,0.4f,0.4f,1), mtrl_ambient, 0);
	static vscreen intro;
	
	createMaps(landscape, water, xsize, zsize, 1, light, wlevel, &dataPak, gfxPath + "ground.gen", &intro);
	
	intro.release();
	
	landscape.createSubsets(2, Cpoint(subsize, subsize));
	landscape.createMeshFromMap(0,2);
	
	landscape.setSubsetMtrl(0, 0);
	landscape.setSubsetMtrl(0, 1);
		
	UINT ti[]={0, 1, 2, 3, 4, 5, 6, 7, 8};
	landscape.setSubsetTexArr(ti, 9, 0);
	landscape.setSubsetTexArr(ti, 9, 1);

	landscape.setTex(*water.getTex(1), 2);
	
	CTexture *lmap = landscape.getTex(0);
	lscapefx->SetTexture("map", lmap->getTexture());
	CTexture *lmap2 = landscape.getTex(1);
	lscapefx->SetTexture("map2", lmap2->getTexture());

	CTexture *dtex1=landscape.getTex(3);
	dtex1->createTexture(&dataPak, gfxPath + "grass.jpg");
	
	setDrawfx(dfx_aBlend);
	dtex1->lock(0, 0);
	dtex1->box(0, 0x99003300, 1);
	dtex1->unlock();
	setDrawfx(0);
	dtex1->fillMipLevels(0,0);
	dtex1->setMipBias(0);
	 CTexture dtex1_1;
	 dtex1->getSurfaceLevel(6, dtex1_1);
     dtex1_1.smooth(2, false);
     dtex1->fillMipLevels(6,0);
	 lscapefx->SetTexture("grass", dtex1->getTexture());
	
	CTexture *dtex2=landscape.getTex(4);
	dtex2->createTexture(&dataPak, gfxPath + "dirt.jpg");
	//dtex2->setMipBias(0);
	lscapefx->SetTexture("dirt", dtex2->getTexture());
	 	
	CTexture *dtex3=landscape.getTex(5);
	dtex3->createTexture(&dataPak, gfxPath + "rock.jpg");
	dtex3->setMipBias(0); 
	CTexture dtex3_1;
	 //dtex3->getSurfaceLevel(6, dtex3_1);
	 //dtex3_1.smooth(1, true);
	 //dtex3->fillMipLevels(6,0);
	lscapefx->SetTexture("rock", dtex3->getTexture());

	CTexture *dtex5=landscape.getTex(6);
	dtex5->createTexture(&dataPak, gfxPath + "sand.jpg");
	lscapefx->SetTexture("sand", dtex5->getTexture());

	CTexture *dtex6=landscape.getTex(7);
	dtex6->createTexture(&dataPak, gfxPath + "rock2.jpg");

	int lnodeltex[]={2,8};
	landscape.setNoDelTexs(lnodeltex, 2);
    	 
	
	//Shader init-----------------------------
	/*if (!land_vsh.createShader(shaderPak, shaderPath+"land.vsh", d3dDebug))
		throw "Couldn't create land vertex shader";
	if (!land_psh.createShader(shaderPak, shaderPath+"land.psh", d3dDebug))
		throw "Couldn't create land pixel shader.";
	if (!landRefl_psh.createShader(shaderPak, shaderPath+"land_refl.psh", d3dDebug))
		throw "Couldn't create land reflection pixel shader.";*/
	
	//landscape.setVertexShader(&pland_vsh);
	//landscape.setPixelShader(&pland_psh);
	
	//---------------------------------
	landscape.update();
	//landscape.enableTextures(false);
	//landscape.show(false);
	//landscape.enableMip(false);
	//landscape.mlFixed=true;
		
	//Caustics + normal map-------------
	const int caustStartIndex = 7;
	const int caustNumFrames = 64;
	const int caustFileIndexBase = 1;
	for (int i=caustStartIndex; i < caustNumFrames + caustStartIndex; i++)
	{
		//ladda caust texture
		CTexture *wtex=water.getTex(i);
		string file=gfxPath + "caustics\\caust_"+numberToString(i-caustStartIndex+caustFileIndexBase, 3, '0')+".jpg";
		wtex->createTexture(&dataPak, file);
		wtex->smooth(1, true);
	}
	for (int i=caustNumFrames + caustStartIndex;i<caustNumFrames*2 + caustStartIndex;i++)
	{
		bool create=false;
		if (create)
		{
			//skapa normal map från caust
			int tsize=256;
			CTexture wtex;
			wtex.createTexture(tsize, tsize, D3DFMT_X8R8G8B8, 1, D3DPOOL_SYSTEMMEM);
			(*water.getTex(i-64)).put(&wtex);
			
			wtex.smooth(3, false);

			bool saveNmap =false;
			if (saveNmap)
			{
				string s="data\\caustics\\nmap source\\ncaust_"+numberToString(i-caustNumFrames - caustStartIndex + caustFileIndexBase, 3, '0')+".bmp";
				if (!wtex.saveToFile(s, D3DXIFF_BMP))
				{
					string s2="Couldn't save normal map source "+s;
					throw s;
				}
			}

			Clandscape caust;
			caust.createMap(tsize,tsize);
			//caust.copyTextureToMap(wtex, 0.015f);
			caust.copyTextureToMap(wtex, 1);
			caust.calcMapNormals();
			
			//copy the generated normal map to water tex array
			CTexture &wtex2=*water.getTex(i);  
			caust.copyNmapToTexture(wtex2);
			
			string file="data\\waves\\nmap_"+numberToString(i- caustNumFrames - caustStartIndex + caustFileIndexBase, 3, '0')+".bmp";
			if (!wtex2.saveToFile(file, D3DXIFF_BMP))
			{
				string s="Couldn't save normal map "+file;
				throw s;
			}
		}
		else
		{
			//ladda normal map
			CTexture &wtex2=*water.getTex(i);
			string file=gfxPath + "waves\\nmap_"+numberToString(i - caustNumFrames - caustStartIndex + caustFileIndexBase, 3, '0')+".jpg";
			wtex2.createTexture(&dataPak, file);
		}
		//wtex2.setMipBias(1);
	}
	//----------------------------------------------
		
	water.createSubsets(1, Cpoint(subsize, subsize), 0);
	
	D3DXCOLOR wdiff(skycol);
	wdiff.a=0.7f;
	water.setMtrl(wdiff, mtrl_diffuse, 0);
	D3DXCOLOR wamb(uwcol);
	wamb*=0.4f;
	water.setMtrl(wamb, mtrl_ambient, 0);
	water.setMtrl(D3DXCOLOR(0.95f,0.7f, 0.3f,1),mtrl_specular, 0);
    water.setMtrlSP(50, 0);
	
	water.setPos(0,0,0);
	
	//Local reflection texture
	CTexture *localRefl=water.getTex(lReflTexIndex);
	localRefl->createTexture(512, 512, sf, 1, D3DPOOL_DEFAULT, D3DUSAGE_RENDERTARGET);
	lscapefx->SetTexture("localRefl", localRefl->getTexture());

	//Sky reflection texture
	lscapefx->SetTexture("skyRefl", envMap.getMap());	
	
	CTexture *wmap = water.getTex(1);
	lscapefx->SetTexture("wmap", wmap->getTexture());
		
	//Texture at index 2 is created in Cwater::genTexture

	//Distance texture
	CTexture *wdTex=water.getTex(3);
	wdTex->createTexture(&dataPak, gfxPath + "water.jpg");
	setDrawfx(dfx_aBlend);
	wdTex->lock();
	wdTex->box(0, 0x23000030, true);
	wdTex->unlock();
	setDrawfx(0);
	wdTex->fillMipLevels();
	lscapefx->SetTexture("distTex", wdTex->getTexture());

	//Refraction texture
	CTexture *refraction = water.getTex(refractionTexIndex);
	refraction->createTexture(512, 512, D3DFMT_A8R8G8B8, 1, D3DPOOL_DEFAULT, D3DUSAGE_RENDERTARGET);
	lscapefx->SetTexture("refraction", refraction->getTexture());
	
	CTexture *nmap2 = water.getTex(5);
	nmap2->createTexture(&dataPak, gfxPath + "waves\\nmap2.jpg");
	lscapefx->SetTexture("nmap2", nmap2->getTexture());
	CTexture *nmap3 = water.getTex(6);
	nmap3->createTexture(&dataPak, gfxPath + "waves\\nmap3.jpg");
	lscapefx->SetTexture("nmap3", nmap3->getTexture());

	water.setSubsetMtrl(0, 0);
	UINT ti2[]={0, 1, 2, 3};
	water.setSubsetTexArr(ti2, 4, 0);
	int wnodeltex[]={2};
	water.setNoDelTexs(wnodeltex, 1);
	
	water.createMeshFromMap(0, &landscape, 1, 0.6f, light.Direction);
	
	water.b_envMap=true;

	D3DVERTEXELEMENT ve[]={
		{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		{0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
		{0, 36, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
		D3DDECL_END()
	};
	water.createVertexDecl(ve);

	water.update();
	//water.show(false);
	//water.enableTextures(false);

	setShaderConstants();
		//pland_psh=&landRefl_psh;
		//pwater_psh=&waterUW_psh;
	//setShaderConstants();
		//pland_vsh=&land_vsh;
		//pland_psh=&land_psh;
		//pwater_psh=&water_psh;
}

//effects------------
BOOL closeWindow(bool start, const vscreen &vscr, int num_i, const fVector &posOff, const fVector &aOff)
{
	static int i=-1;
	if (start)
		i=0;
	if (i==-1)
		return TRUE;
	return FALSE;
}
//----------------

void setShaderConstants()
{
	//land_vsh--------------------
	Cpoint ldim=landscape.getDim();
	float g_scalex=0.4f;
	float g_scalez=0.4f;

	static D3DXVECTOR4 scaleVec(g_scalex * 0.2f, g_scalez * 0.2f, g_scalex * 0.2f, g_scalez * 0.2f);
	static D3DXVECTOR4 scaleVec2(g_scalex * 0.4f, g_scalez * 0.4f, g_scalex * 0.15f, g_scalez * 0.15f);
	static D3DXVECTOR4 scaleVec3(g_scalex * 0.055f, g_scalez * 0.055f, 1.0f/ldim.x, 1.0f/ldim.y);
	static D3DXVECTOR4 scaleVec4(0.01f, 0.01f, 0, 0);
	
	/*pland_vsh->setConstantF(4, scaleVec, 1);
	pland_vsh->setConstantF(5, scaleVec2, 1);
	pland_vsh->setConstantF(6, scaleVec3, 1);
	pland_vsh->setConstantF(11, scaleVec4, 1);*/
	static D3DXVECTOR4 stretchVec(1.0f/ldim.x, 1.0f/ldim.y, g_scalex * 0.055f, g_scalez * 0.055f);
	lscapefx->SetVector("lstretchTCScale", &stretchVec);
	
	D3DXVECTOR4 misc(0, landscape.getMaxHeight(), 0.8f, 0);
//	pland_vsh->setConstantF(7, misc, 1);
		
	lscapefx->SetVector("turbCol", (D3DXVECTOR4*)&uwcol);

	BOOL b=true;
	//pland_vsh->setConstantB(1, &b, 1);
	//---------------------------------------------*/
	//waterVS
	ldim=water.getDim();
	g_scalex=1.0f;
	g_scalez=1.0f;
	static D3DXVECTOR4 wscaleVec(1.0f/ldim.x, 1.0f/ldim.y, g_scalex * 0.02f, g_scalez * 0.02f);
	float cscale=1.0f;
	static D3DXVECTOR4 wscaleVec2(g_scalex * cscale * 0.03f, g_scalez * cscale * 0.03f, g_scalex * cscale * 0.09f, g_scalez * cscale * 0.09f);  //caustics 1 + 2
	//pwater_vsh->setConstantF(4, wscaleVec, 1);
	//pwater_vsh->setConstantF(7, wscaleVec2, 1);
	lscapefx->SetValue("wtexScale", &wscaleVec, 8);
}

void drawLocalReflections()
{
	static bool init=false;
	if ((!settings.localRefl) && init)
		return;
	
	CTexture *localRefl=water.getTex(lReflTexIndex);
	
	//Clear local reflection texture(only once if local reflections are disabled)
	pd3dDevice->SetRenderTarget(0, localRefl->getSurface());
	pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET, 0x00ffffff & uwcol, 1, 0);
	init=true;
	if (!settings.localRefl)
	{
		pd3dDevice->SetRenderTarget(0, bbuf.getSurface());
		return;
	}

	landscape.bReflDraw=true;
	int subset = 0;
	if (!cam.uw)
	{
		//ej speglat----------------------------------
		D3DXMATRIX projMat;
		D3DXMATRIX viewMat;
		pd3dDevice->GetTransform(D3DTS_VIEW, &viewMat);
		pd3dDevice->GetTransform(D3DTS_PROJECTION, &projMat);
		D3DXPLANE d3dxPlane(0,-1,0,2);
		if (!keydown(DIK_O))
			ClipProjectionMatrix(viewMat, projMat, d3dxPlane);
		if (cam.pos.y<100)
			landscape.draw(&water, 0, geomip+1);   //rättvänt
		pd3dDevice->SetTransform(D3DTS_PROJECTION, &projMat);

		//3d objects above water
		D3DXMATRIX mat = musicObj.worldMat;
		musicObj.worldMat *= g_waterReflWorldMat;
		musicObj.draw(0);
		musicObj.worldMat = mat;
	}
	else
	{
		subset = 1;
	}
	
	//speglat----------------------------
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	landscape.scaling(1,-1,1);
	landscape.update(false);
		
	landscape.draw(&water, subset, geomip+1);   //uppochner
	
	landscape.bReflDraw = false;
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	landscape.scaling(1,1,1);
	landscape.update(false);
	
	//--------------------------------------------------
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	pd3dDevice->SetRenderTarget(0, bbuf.getSurface());
	pd3dDevice->Clear(0, 0, D3DCLEAR_ZBUFFER, g_bc, 1, 0);
	screenTransform();
}

void drawRefraction()
{
	static bool init=false;
	if (!settings.refraction && init)
		return;
	CTexture *refraction=water.getTex(refractionTexIndex);
	
	//Clear local reflection texture(only once if local reflections are disabled)
	pd3dDevice->SetRenderTarget(0, refraction->getSurface());
	pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET, 0x00ffffff & uwcol, 1, 0);
	init=true;
	if (!settings.refraction)
	{
		pd3dDevice->SetRenderTarget(0, bbuf.getSurface());
		return;
	}

	if (!cam.uw)  //Rita saker under vattnet
	{
		landscape.draw(&water, 1);
	}
	else           //Rita saker ovanför vattnet
	{
		sky.draw();
		landscape.draw(0, 0);
		musicObj.draw(0);
	}

	pd3dDevice->SetRenderTarget(0, bbuf.getSurface());
	pd3dDevice->Clear(0, 0, D3DCLEAR_ZBUFFER, g_bc, 1, 0);
}


//--------------------------------
//Classes
//--------------------------------

//-------------------------
//C3dEntity
void C3dEntity::update()
{
	static fVector a;
	static fVector pos;
	static fVector a2;
	static fVector pos2;
	a2=a;
	pos2=pos;
	
	a.y=a.x=a.z=0;
	
	if (mbutdown(0))
		a.z=1;
	if (mbutdown(1))
		a.z+=-1;

	a.y += (joy[defJoy].state.lX) * 0.001f;
	a.x += -(joy[defJoy].state.lY) * 0.001f;
	a.z += -(joy[defJoy].state.lRz) * 0.001f;
	pos = (float)joy[defJoy].state.rglSlider[0] * lookDir * -0.001f;

	if (keydown(DIK_A))
		pos-=leftDir;
	
	if (keydown(DIK_D))
		pos+=leftDir;

	if (keydown(DIK_W))
		pos += lookDir;
	
	if (keydown(DIK_S))
		pos -= lookDir;
	
	if (keydown(DIK_LSHIFT) || keydown(DIK_RSHIFT))
		pos += upDir;
	
	if (keydown(DIK_LCONTROL) || keydown(DIK_RCONTROL))
		pos -= upDir;
	
	float v=a.getValue();
	if (v>1)
		a/=v;
	
	//a.y+=(float)mstate.lX/10.0f;
	//a.x+=(float)mstate.lY/10.0f;
	const int mrotn = 2;
	static int mroti;
	static fVector mrot[mrotn];
	mrot[mroti].x=mstate.lY/200.0f;
	mrot[mroti].y=mstate.lX/200.0f;
	fVector mrotSum = mrot[0];
	for (int i=1;i<mrotn;i++)
		mrotSum += mrot[i];
	mroti++;
	if (mroti >= mrotn)
		mroti = 0;
	setRelAngle(mrotSum/(float)mrotn);
	
	static float rotspeed=1;
	static float rbf=rotBrakeFriction;
	static float racc=rotAcc;
	
	float rs=rotVelocity.getValue();
	rs/=maxRotSpeed;
	if (rs>1)
	{
		if (rotspeed<rs)
			rotspeed=rs;
		rotBrakeFriction=rotspeed*rbf;
	}
	else
	{
		rotspeed=1;
		rotBrakeFriction=rbf;
	}

	v=pos.getValue();
	if (v>1)
		pos/=v;
	
	move(pos);
	brake(!pos.x, !pos.y, !pos.z);
	rotate(a);
	rotBrake(!a.x, !a.y, !a.z);
	bool bx=fabs(a.x)<fabs(rotVelocity.x) || sgn(a.x)!=sgn(rotVelocity.x);
	bool by=fabs(a.y)<fabs(rotVelocity.y) || sgn(a.y)!=sgn(rotVelocity.y);
	bool bz=fabs(a.z)<fabs(rotVelocity.z) || sgn(a.z)!=sgn(rotVelocity.z);
	//rotBrake(bx, by, bz);
	
	baseObj::update();
}

void closeApp()
{
	landscape.release();
	water.release();
	sky.release();
	musicObj.release();
	object.release();
	
	os.release();
	os2.release();
	
	/*bkgmusic.release();
	apStd3d.release();
	apStdStereo.release();*/
	//sbuf3d;
	//slis;

	land_vsh.release();
	land_psh.release();
	landRefl_psh.release();
	water_vsh.release();
	water_psh.release();
	waterUW_psh.release();
	sky_vsh.release();
	
	commonfx.release();
	for (int i=0;i<16;i++)
	{
		if (pd3dDevice)
			pd3dDevice->SetTexture(i, 0);
	}
	cleanDx();
}

//----------------------------------------------
//Csettings
Csettings::Csettings()
{
	sky=true;
	skyRefl=true;
	localRefl=true;
	waterWaves=true;
	refraction=true;
	
	/*sky=false;
	skyRefl=false;
	localRefl=false;
	waterWaves=false;*/
}

void Ccommonfx::init()
{
	if (!(effect = createD3dxEffect(&dataPak, "shaders\\common.fxo", 0)))
		throw "Couldn't create commonfx";

	if (!(handle_camPos = effect->GetParameterByName(0, "camPos")))
		throw "Couldn't create commonfx camPos handle";
	effect->SetValue("lightDir", &sun.Direction, 12);
}

void Ccommonfx::updateParams()
{
	effect->SetValue(handle_camPos, &cam.pos, 12);
}

void Ccommonfx::release()
{
	safeRelease(effect);
}