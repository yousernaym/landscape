//****************************************************************************
//
//****************************************************************************
#include "common.fxi"
#include "water.fxi"

//Functions------------------------------------------------------------
float3 refractSkyVertex(in float3 vPos)
{
	float3 offsetVec = (float3(0,1,0) - vPos);
	
	return vPos + offsetVec * 0.35;
}

//---------------------------------------------------------------------

texture tex;

sampler texSamp = sampler_state
{
	Texture = (tex);
};

struct VS_INPUT
{
    float3 pos : POSITION;
	float2 t0 : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 pos : POSITION;
	float2 t0 : TEXCOORD0;
	float2 ta : COLOR;  //turbidness, alpha
};


//-----------------------------------------------------------------------------
// Shaders
//-----------------------------------------------------------------------------

//Vertex shader----------------------------------------------------

VS_OUTPUT VS(VS_INPUT IN)
{
	VS_OUTPUT OUT;
	
	OUT.ta.x =0;
	if (camPos.y<0)
	{
		OUT.ta.x = calcTurb(camPos+IN.pos);
		IN.pos = refractSkyVertex(IN.pos);
	}
			
	
	OUT.pos = mul(wvp, float4(IN.pos, 1));
	OUT.t0 = IN.t0;

	OUT.ta.y = min(abs(IN.pos.y) * 4, 1);
	
	return OUT;
}		

//Pixel shader----------------------------------------------------

STDPS_OUTPUT PS(VS_OUTPUT IN)
{
    STDPS_OUTPUT OUT;
	OUT.color = tex2D(texSamp, IN.t0);
	OUT.color = lerp(OUT.color, turbCol, IN.ta.x);
	OUT.color.a = IN.ta.y;
					
	return OUT;
}


//****************************************************************************
//****************************************************************************
//-----------------------------------------------------------------------------
// Techniques
//-----------------------------------------------------------------------------

technique srd
{
    pass pass0
    {
		Lighting = FALSE;
		AlphaBlendEnable = TRUE;
		
		VertexShader = compile vs_2_0 VS();
		PixelShader  = compile ps_2_0 PS();
	}
}

