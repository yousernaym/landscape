//****************************************************************************
//Landscape effect file
//****************************************************************************
#include "common.fxi"
#include "water.fxi"

texture wmap; //.a=alphamap(1 on water, 0 outside water), .x=lightmap(currently not used), .yw=depth;
sampler wmapSamp = sampler_state
{
	Texture   = (wmap);
};


//*****************************************************************************
// Land
//*****************************************************************************
static const float texScale = 0.4;
const float4 gdTCScale = float4(0.2, 0.2, 0.4, 0.4) * texScale;
const float4 rsTCScale = float4(0.2, 0.2, 0.2, 0.2) * texScale;
const float4 caustTCScale = float4(0.15, 0.15, 0.055, 0.055) * texScale;
const float4 lstretchTCScale = float4(0, 0, 0.055, 0.055) * texScale;
const float distTexStart = 30;
const float distTexEnd = 200;

const float camDepth;   //0 - 1 (1=surface and above)
const float transpDecr;
const float caustIntensity;  //depending on camera height

const bool bDestAlpha;

texture map;
texture map2;
texture grass;
texture dirt;
texture rock;
texture sand;
texture caust : CAUSTANIM;

sampler mapSamp = sampler_state
{
	Texture   = (map);
};

sampler map2Samp = sampler_state
{
	Texture   = (map2);
};

sampler grassSamp = sampler_state
{
	Texture   = (grass);
};

sampler dirtSamp = sampler_state
{
	Texture   = (dirt);
};

sampler rockSamp = sampler_state
{
	Texture   = (rock);
};

sampler sandSamp = sampler_state
{
	Texture   = (sand);
};

sampler caustSamp = sampler_state
{
	Texture   = (caust);
};

struct LANDVS_INPUT
{
    float3 pos : POSITION;
};

struct AW_LANDVS_OUTPUT
{
    float4 pos : POSITION;
	float4 stretchTC : TEXCOORD0; //.xy = map, .zw = stretched detail
	float4 gdTC : TEXCOORD1; //grass, dirt
	float4 rsTC : TEXCOORD2;  //dirt, sand 
	float3 dft : TEXCOORD4; //x = alpha, y = distTexWeight 0-1, z=fog, w = turb
};

struct UW_LANDVS_OUTPUT
{
    float4 pos : POSITION;
	float4 stretchTC : TEXCOORD0; //.xy = map, .zw = stretched detail
	float4 gdTC : TEXCOORD1; //grass, dirt
	float4 rsTC : TEXCOORD2;  //dirt, sand 
	float4 caustTC : TEXCOORD3; //xy=detail, zw=stretched
	float3 dft : TEXCOORD4; //x = distTexWeight, y=fog, z = turb
};

struct REFL_LANDVS_OUTPUT
{
    float4 pos : POSITION;
	float4 stretchTC : TEXCOORD0; //.xy = map, .zw = stretched detail
	float4 gdTC : TEXCOORD1; //grass, dirt
	float4 rsTC : TEXCOORD2;  //dirt, sand 
	float2 caustTC : TEXCOORD3; //xy=detail
	float2 hf : TEXCOORD4; //x = vertex height, y=fog
};



//-----------------------------------------------------------------------------
// Shaders
//-----------------------------------------------------------------------------

//Vertex shader----------------------------------------------------

AW_LANDVS_OUTPUT aw_landVS(LANDVS_INPUT IN)
{
	AW_LANDVS_OUTPUT OUT;
	   //
	//texture transformations-------
	float4 texCoords = IN.pos.xzxz;
	OUT.stretchTC = texCoords * lstretchTCScale;
	OUT.gdTC = texCoords * gdTCScale;
	OUT.rsTC = texCoords * rsTCScale;
		
	//distance to vertex---------
	//float3 vertDistVec = abs(camPos - IN.pos);
	float vertDist = clamp(distance(camPos, IN.pos), distTexStart, distTexEnd);
	OUT.dft.x = (vertDist - distTexStart) / (distTexEnd - distTexStart);
	
	OUT.dft.z = 0;
	if (camPos.y<0)
	{
		OUT.dft.y = calcFogOtherSide(IN.pos);
		if (IN.pos.y > 0)
			IN.pos = refractVertex(IN.pos, 1);
	}
	else
		OUT.dft.y = calcFog(IN.pos);
		
	//Move to water vertex shader--------
	/*if (bDestAlpha)
	{
		float3 viewVec = normalize(camPos -IN.pos);
		float destAlpha = (1 - dot(float3(0,1,0), viewVec)) + transpDecr;
		OUT.adft.x = min(destAlpha, 1);
	}*/
	//--------------------------
	
	OUT.pos = mul(wvp, float4(IN.pos, 1));
	
	return OUT;
}		

UW_LANDVS_OUTPUT uw_landVS(LANDVS_INPUT IN)
{
	UW_LANDVS_OUTPUT OUT;
	
	//texture transformations-------
	float4 texCoords = IN.pos.xzxz;
	OUT.stretchTC = texCoords * lstretchTCScale;
	OUT.gdTC = texCoords * gdTCScale;
	OUT.rsTC = texCoords * rsTCScale;
	OUT.caustTC = texCoords * caustTCScale;
	
	//distance to vertex---------
	//float3 vertDistVec = abs(camPos - IN.pos);
	float vertDist = clamp(distance(camPos, IN.pos), distTexStart, distTexEnd);
	OUT.dft.x = (vertDist - distTexStart) / (distTexEnd - distTexStart);
	
	OUT.dft.y = 0;
	if (camPos.y>0)
	{
		OUT.dft.z = calcTurbOtherSide(IN.pos);
		if (IN.pos.y < 0)
			IN.pos = refractVertex(IN.pos, -1);
	}
	else
		OUT.dft.z = calcTurb(IN.pos);
	
	OUT.pos = mul(wvp, float4(IN.pos, 1));
	
	return OUT;
}

REFL_LANDVS_OUTPUT refl_aw_landVS(LANDVS_INPUT IN)
{
	REFL_LANDVS_OUTPUT OUT;
	OUT.pos = mul(wvp, float4(IN.pos, 1));

	//texture transformations-------
	float4 texCoords = IN.pos.xzxz;
	OUT.stretchTC = texCoords * lstretchTCScale;
	OUT.gdTC = texCoords * gdTCScale;
	OUT.rsTC = texCoords * rsTCScale;
	OUT.caustTC = OUT.rsTC;		
	OUT.hf.x = IN.pos.y;
	OUT.hf.y = calcFogOtherSide(float3(IN.pos.x, -IN.pos.y, IN.pos.z));
	
	return OUT;
}
REFL_LANDVS_OUTPUT refl_uw_landVS(LANDVS_INPUT IN)
{
	REFL_LANDVS_OUTPUT OUT;
	OUT.pos = mul(wvp, float4(IN.pos, 1));

	//texture transformations-------
	float4 texCoords = IN.pos.xzxz;
	OUT.stretchTC = texCoords * lstretchTCScale;
	OUT.gdTC = texCoords * gdTCScale;
	OUT.rsTC = texCoords * rsTCScale;
	OUT.caustTC = texCoords * caustTCScale;
		
	OUT.hf.x = IN.pos.y;
	OUT.hf.y = calcTurbOtherSide(float3(IN.pos.x, -IN.pos.y, IN.pos.z));
	
	return OUT;
}


//Pixel shader----------------------------------------------------

STDPS_OUTPUT aw_landPS(AW_LANDVS_OUTPUT IN)
{
    STDPS_OUTPUT OUT;
    float4 mapCol = tex2D(mapSamp, IN.stretchTC.xy);
    float4 map2Col = tex2D(map2Samp, IN.stretchTC.xy);
    
    float4 grassCol = tex2D(grassSamp, IN.gdTC.xy);
    float4 dirtCol = tex2D(dirtSamp, IN.gdTC.zw);
    float4 rockCol = tex2D(rockSamp, IN.rsTC.xy);
    float4 sandCol = tex2D(sandSamp, IN.rsTC.zw);
    
    float4 stretchGrassCol = tex2D(grassSamp, IN.stretchTC.zw);
    float4 stretchDirtCol = tex2D(dirtSamp, IN.stretchTC.zw);
    float4 stretchRockCol = tex2D(rockSamp, IN.stretchTC.zw);
       
    grassCol = lerp(grassCol, stretchGrassCol, IN.dft.x);
    dirtCol = lerp(dirtCol, stretchDirtCol, IN.dft.x);
    rockCol = lerp(rockCol, stretchRockCol, IN.dft.x);
    
    float4 current = grassCol * mapCol.b + dirtCol * mapCol.g
				   + rockCol * mapCol.r + sandCol * map2Col.g;
		
	//Lightmap
	current = (current * (mapCol.a + lightAmbient));
	
	//Fog/Turbidness
	current = lerp(current, fogCol, IN.dft.y);
	//current = lerp(current, turbCol, IN.dft.z);
	
	float4 wmapCol = tex2D(wmapSamp, IN.stretchTC.xy);
	//current.a = IN.adft.x * wmapCol.a;
	current.a = wmapCol.a;		
	
	OUT.color = current;
					
	return OUT;
}

STDPS_OUTPUT uw_landPS(UW_LANDVS_OUTPUT IN)
{
    STDPS_OUTPUT OUT;
    float4 mapCol = tex2D(mapSamp, IN.stretchTC.xy);
    float4 map2Col = tex2D(map2Samp, IN.stretchTC.xy);
    
    float4 grassCol = tex2D(grassSamp, IN.gdTC.xy);
    float4 dirtCol = tex2D(dirtSamp, IN.gdTC.zw);
    float4 rockCol = tex2D(rockSamp, IN.rsTC.xy);
    float4 sandCol = tex2D(sandSamp, IN.rsTC.zw);
    
    float4 stretchGrassCol = tex2D(grassSamp, IN.stretchTC.zw);
    float4 stretchDirtCol = tex2D(dirtSamp, IN.stretchTC.zw);
    float4 stretchRockCol = tex2D(rockSamp, IN.stretchTC.zw);
       
    grassCol = lerp(grassCol, stretchGrassCol, IN.dft.x);
    dirtCol = lerp(dirtCol, stretchDirtCol, IN.dft.x);
    rockCol = lerp(rockCol, stretchRockCol, IN.dft.x);
    
    float4 current = grassCol * mapCol.b + dirtCol * mapCol.g
				   + rockCol * mapCol.r + sandCol * map2Col.g;
	//current = sandCol * map2Col.g;
	
	float4 caustCol = tex2D(caustSamp, IN.caustTC.xy);
	float4 stretchCaustCol = tex2D(caustSamp, IN.caustTC.zw);
	caustCol = lerp(stretchCaustCol, caustCol, map2Col.b);
	caustCol *= caustIntensity;
	//caustCol = caustCol + 0.5;
	current += caustCol * map2Col.b;
	//current = lerp(current, caustCol, map2Col.b);
	
	//Lightmap
	current = (current * (mapCol.a + lightAmbient));
	
	//Fog/Turbidness
	//current = lerp(current, fogCol, IN.dft.y);
	current = lerp(current, turbCol, IN.dft.z);
	
	current.a=1;
	OUT.color = current;
		
	return OUT;
}

STDPS_OUTPUT refl_aw_landPS(REFL_LANDVS_OUTPUT IN)
{
    clip(IN.hf.x);
    STDPS_OUTPUT OUT;
    float4 mapCol = tex2D(mapSamp, IN.stretchTC.xy);
    float4 map2Col = tex2D(map2Samp, IN.stretchTC.xy);
        
    float4 grassCol = tex2D(grassSamp, IN.gdTC.xy);
    float4 dirtCol = tex2D(dirtSamp, IN.gdTC.zw);
    float4 rockCol = tex2D(rockSamp, IN.rsTC.xy);
    float4 sandCol = tex2D(sandSamp, IN.rsTC.zw);
    
    float4 current = grassCol * mapCol.b + dirtCol * mapCol.g
				   + rockCol * mapCol.r + sandCol * map2Col.g;
	
	//Lightmap
	current = (current * (mapCol.a + + lightAmbient));
	
	//Fog
	current = lerp(current, fogCol, IN.hf.y);
	
	current.a=1;
	OUT.color = current;
		
	return OUT;
}

STDPS_OUTPUT refl_uw_landPS(REFL_LANDVS_OUTPUT IN)
{
    clip(-IN.hf.x);
    STDPS_OUTPUT OUT;
    float4 mapCol = tex2D(mapSamp, IN.stretchTC.xy);
    float4 map2Col = tex2D(map2Samp, IN.stretchTC.xy);
        
    float4 grassCol = tex2D(grassSamp, IN.gdTC.xy);
    float4 dirtCol = tex2D(dirtSamp, IN.gdTC.zw);
    float4 rockCol = tex2D(rockSamp, IN.rsTC.xy);
    float4 sandCol = tex2D(sandSamp, IN.rsTC.zw);
    
    float4 current = grassCol * mapCol.b + dirtCol * mapCol.g
				   + rockCol * mapCol.r + sandCol * map2Col.g;
	
	float4 caustCol = tex2D(caustSamp, IN.caustTC.xy);
	current += caustCol * map2Col.b;
		
	//Lightmap
	current = (current * (mapCol.a + + lightAmbient));
	
	//Turbidness
	current = lerp(current, turbCol, IN.hf.y);
	
	current.a=1;
	OUT.color = current;
			
	return OUT;
}



//*****************************************************************************
// Water
//*****************************************************************************

//Init once in app---------------------
const float4 wtexScale = float4(0, 0, 0, 0);
const float4 wnmapScale = float4(0.1, 0.1, 0.2, 0.2);
const float4 wnmapScale2 = float4(0.01, 0.01, 0.02, 0.02) *2;
const float4 wnmapScale3 = float4(0.03, 0.03, 0.04, 0.04) *3;
//-------------------------------------

//Update every frame-------------------
const float4x4 reflMat;
const float distBlend;
const float4 waveScrollOffset = float4(0,0,0,0);
const float4 waveScrollOffset2 = float4(0,0,0,0);
const float4 waveScrollOffset3 = float4(0,0,0,0);
//-------------------------------------

texture localRefl;
texture nmap : NMAPANIM;
texture nmap2;
texture nmap3;
texture distTex; 
texture skyRefl;
texture refraction;

sampler localReflSamp = sampler_state
{
	Texture   = (localRefl);
	AddressU = MIRROR;
	AddressV = CLAMP;
};

sampler nmapSamp = sampler_state
{
	Texture   = (nmap);
};

sampler nmap2Samp = sampler_state
{
	Texture   = (nmap2);
};

sampler nmap3Samp = sampler_state
{
	Texture   = (nmap3);
};

sampler distTexSamp = sampler_state
{
   Texture   = (distTex);
};

samplerCUBE skyReflSamp = sampler_state
{
	Texture   = (skyRefl);
};

sampler refractionSamp = sampler_state
{
	Texture   = (refraction);
	AddressU = MIRROR;
	AddressV = CLAMP;
};


// Outputs/Inputs-----------------------------------------

struct WATERVS_INPUT
{
    float3 pos: POSITION;
    float3 normal : NORMAL;
};

struct WATERVS_OUTPUT
{
    float4 pos : POSITION;
	float2 stretchTC : TEXCOORD0;
	//float2 distTexTC: TEXCOORD1;
	float4 nmapTC : TEXCOORD1;
	float4 nmap2TC : TEXCOORD2;
	float4 nmap3TC : TEXCOORD7;
	float3 viewVec : TEXCOORD3; //points from pixel
	float3 normal : TEXCOORD4;
	float4 localReflTC : TEXCOORD5;
	float2 ft : TEXCOORD6;
};

//----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//Helper functions/macros
//-----------------------------------------------------------------------------

float3 calcNormal(in float4 nmapTC, in float4 nmap2TC, in float4 nmap3TC, in float3 inNormal)
{
	//Retrieving current normal -------
	float3 normal = tex2D(nmapSamp, nmapTC.xy);
	normal = (normal - 0.5) * 0.015;
	float3 normalb = tex2D(nmapSamp, nmapTC.zw);
	normal += (normalb - 0.5) * 0.005;
	normal.y *= 100;
	normal.yz = normal.zy*10;
	
	//normal = float3(0,0,0);
	
	float3 normal2 = tex2D(nmap2Samp, nmap2TC.xy);
	normal += (normal2 - 0.5) * 0.65;
	float3 normal2b = tex2D(nmap2Samp, nmap2TC.zw);
	normal += (normal2b - 0.5) * 0.45;
	
	float3 normal3 = tex2D(nmap3Samp, nmap3TC.xy);
	normal += (normal3 - 0.5) * 0.15;
	float3 normal3b = tex2D(nmap3Samp, nmap3TC.zw);
	normal += (normal3b - 0.5) * 0.05;
	
	normal.yz = normal.zy;
	
	normal = normalize(normal + inNormal * 0.35);	
	//------------------------------------
	//blank yta
	//normal = float3(0,1,0);
	//normal = normalize(inNormal);
	return normal;
}
	
//----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Standard shaders
//-----------------------------------------------------------------------------

//Vertex shader----------------------------------------------------

WATERVS_OUTPUT waterVS( WATERVS_INPUT IN)
{
    WATERVS_OUTPUT OUT;

	//Turbidness---------------------------------
	OUT.ft = 0;
	if (camPos.y<0)
		OUT.ft.y = calcTurb(IN.pos);
	else
		OUT.ft.x = calcFog(IN.pos);
			
	
	//Geo transformation
	OUT.pos = mul( wvp, float4(IN.pos, 1) );

	//Tex tranformation
	float4 texCoords;
	texCoords.xyzw = IN.pos.xzxz;
	float4 texCoords1 = texCoords * wtexScale;
	OUT.stretchTC = texCoords1.xy;
	//OUT.distTexTC = texCoords1.zw;
	float4 nmapCoords = texCoords * wnmapScale;
	OUT.nmapTC = nmapCoords + waveScrollOffset;
	float4 nmap2Coords = texCoords * wnmapScale2;
	OUT.nmap2TC = nmap2Coords + waveScrollOffset2;
	float4 nmap3Coords = texCoords * wnmapScale3;
	OUT.nmap3TC = nmap3Coords + waveScrollOffset3;
		
	OUT.viewVec = camPos - IN.pos;
	OUT.normal = IN.normal;
	//OUT.screenCoords = mul( reflMat, float4(IN.pos, 1) );
	OUT.localReflTC = mul( reflMat, float4(IN.pos.x, 0, IN.pos.z, 1) );
			
	return OUT;
}
//------------------------------------------------------------------

// Pixel Shader-----------------------------------------------------

STDPS_OUTPUT caw_waterPS( WATERVS_OUTPUT IN)
{
	STDPS_OUTPUT OUT;

	float3 viewVec = normalize(IN.viewVec);
	
	//float4 wmapCol = tex2D(wmapSamp, IN.stretchTC);
	
	float3 normal = calcNormal(IN.nmapTC, IN.nmap2TC, IN.nmap3TC, IN.normal);
	
	float viewNormalDot = dot(normal, viewVec);
	float fresnel = calcFresnel(1-viewNormalDot);

	float3 viewReflVec = viewNormalDot * 2 * normal - viewVec;	

	//Local reflections texture coords---------------------
	//float z = clamp(camPos.y*5, 0.25, 1);
	float4 wavy = float4(5 * normal.xz, 0, 0);
	float4 localReflTC = IN.localReflTC + wavy;
	//localReflTC.w = localReflTC.z;
	float4 refrTC = IN.localReflTC - wavy;
	
	//localReflTC.y = min(localReflTC.y, 384);
	//localReflTC.xy = clamp(localReflTC.xy, 0, 64);
	//------------------------------------------------

	//Blending ops-------------------------------------
	float4 skyReflCol = texCUBE(skyReflSamp, viewReflVec);
	float4 localReflCol = tex2Dproj(localReflSamp, localReflTC);
	float4 current = lerp(skyReflCol, localReflCol, localReflCol.a);
	float4 refrCol = tex2Dproj(refractionSamp, refrTC);
	current = lerp(refrCol, current, fresnel);
	
	//current = refrCol;
	
	//float4 distTexCol = tex2D(distTexSamp, IN.distTexTC);
	//current = lerp(current, distTexCol, distBlend);
	//--------------------------------------------------
	
	//Fog
	current = lerp(current, fogCol, IN.ft.x);	
	
	//Water transparency
	//current.a = (1 - viewNormalDot);// * wmapCol.a;
	
	//current = localReflCol;
	OUT.color = current;
				
	return OUT;
}

STDPS_OUTPUT cuw_waterPS( WATERVS_OUTPUT IN)
{
    STDPS_OUTPUT OUT;
    const float4 wcol = float4(0.15, 0.2, 0.7, 1);
    
	float3 viewVec = normalize(IN.viewVec);

	float3 normal = -calcNormal(IN.nmapTC, IN.nmap2TC, IN.nmap3TC, IN.normal);
	//blank yta
	//normal = float3(0,-1,0);
	
	//Local reflections texture coords---------------------
	float4 wavy = float4(15 * normal.xz, 0, 0);
	float4 localReflTC = IN.localReflTC - wavy;
	float4 refrTC = IN.localReflTC + wavy;
	
	float viewNormalDot = dot(normal, viewVec);
	float fresnel = calcFresnel((1-viewNormalDot)*2.5);

	//Blending ops------------------------------------------
	float4 current;
	//float4 distTexCol = tex2D(distTexSamp, IN.distTexTC);
	//current = distTexCol * wcol;
	
	float4 refrCol = tex2Dproj(refractionSamp, refrTC);
	float4 localReflCol = tex2Dproj(localReflSamp, localReflTC);
		
	current = lerp(refrCol, localReflCol, fresnel);
		
	//--------------------------------------------------------------
	
	//Turbidness
	current = lerp(current, turbCol, IN.ft.y);	
	
	//water transparency
	//current.a = (1 - viewNormalDot) * 0.5;
				
	OUT.color = current;
	//OUT.color = localReflCol;
		
	return OUT;
}

//------------------------------------------------------------------


//****************************************************************************
//****************************************************************************

//-----------------------------------------------------------------------------
// Techniques
//-----------------------------------------------------------------------------

technique land
{
    pass aw
    {
		Lighting = FALSE;
		
		VertexShader = compile vs_2_0 aw_landVS();
		PixelShader  = compile ps_2_0 aw_landPS();
	}
    pass uw
    {
		Lighting = FALSE;
		
		VertexShader = compile vs_2_0 uw_landVS();
		PixelShader  = compile ps_2_0 uw_landPS();
	}
}

technique refl_land
{
    pass aw
    {
		Lighting = FALSE;
		
		VertexShader = compile vs_2_0 refl_aw_landVS();
		PixelShader  = compile ps_2_0 refl_aw_landPS();
	}
	pass uw
    {
		Lighting = FALSE;
		
		VertexShader = compile vs_2_0 refl_uw_landVS();
		PixelShader  = compile ps_2_0 refl_uw_landPS();
	}
}

technique caw_water
{
    pass pass0
    {
		Lighting = FALSE;
		
		VertexShader = compile vs_2_0 waterVS();
		PixelShader  = compile ps_2_0 caw_waterPS();
	}
}

technique cuw_water
{
    pass pass0
    {
		Lighting = FALSE;
		
		VertexShader = compile vs_2_0 waterVS();
		PixelShader  = compile ps_2_0 cuw_waterPS();
	}
}