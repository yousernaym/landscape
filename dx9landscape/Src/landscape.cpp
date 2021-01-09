#include "dxstuff.h"
#include "gfxengine.h"

D3DXMATRIX g_waterReflWorldMat(1, 0, 0, 0,
							  0,-1, 0, 0,
							  0, 0, 1, 0,
							  0, 0, 0, 1);
float waveCalcLimit=50;

void copyLandToWater(waterVertex *v1, const landVertex *v2, int index)
{
    for (int i=0;i<index;i++)
	{
		v1[i].pos=v2[i].pos;
	}
}

void copyWaterToLand(landVertex *v1, const waterVertex *v2, int index)
{
	for (int i=0;i<index;i++)
	{
		v1[i].pos=v2[i].pos;
	}
}

///////////////////////////////
//Construction/Destruction

CbaseGroundGen::CbaseGroundGen()
{
	num_awTypes=2;
	num_uwTypes=2;
	active = true;
}

CbaseGroundGen::~CbaseGroundGen()
{
}

CgroundGenObjects::CgroundGenObjects()
{
	b_texGen = false;
}

CgroundGenObjects::~CgroundGenObjects()
{
}

CgroundGenTextures::CgroundGenTextures()
{
	blendProb = 0.2f;
	b_texGen = true;
	numTex = 1;
	caustMapIndex = 0;
	caustMapMask = 0;
	lightMapIndex = 0;
	lightMapMask = 0;
}

CgroundGenTextures::~CgroundGenTextures()
{
}

CgroundGen::CgroundGen(bool btex, bool bobj)
{
	texGen.active = btex;
	objGen.active = bobj;
}	

Clandscape::Clandscape()
{
	//ZeroMemory(this, sizeof(*this));
	mlFixed=false;
	wlevel=0;
	fvf=D3DFVF_LAND;
	fvfSize=sizeof(landVertex);
	mmAlwaysDraw=true;
	mip=0;
	map=0;
	n_map=0;
	subset=0;
	for (int i=0;i<2;i++)
		hdif.vb[i]=0;
	hdif.decl=0;
	bReflDraw = false;
	fxHandle_caustAnim = 0;
}

Clandscape::~Clandscape()
{
	release();
}

Cwater::Cwater()
{
	fvf=0;
	fvfSize=sizeof(waterVertex);
	density=0.08f;
	isShaded=0;
	fxHandle_nmapAnim = 0;
}

Cwater::~Cwater()
{
	release();
}

///////////////////////////////////////////////
//Ground generation

//CbaseGroundGen-----------------------
void CbaseGroundGen::calcWeights(float height, float angle, bool uwater)
{
    if (!active)
		return;
	if (uwater)
	{
		type = uwType;
		numWeights = num_uwTypes;
	}
	else
	{
		type = awType;
		numWeights = num_awTypes;
	}

	for (unsigned i=0;i<numWeights;i++)
		type[i].calcWeight(height, angle);
	
	calcIntensities();
}

void CbaseGroundGen::Stype::calcWeight(float height, float angle)
{
	weight = 0;
	intensity = 0;
	float condScaleSum = 0;
	for (unsigned i=0;i<numIntervals;i++)
	{
		if (height < (interval[i].low * rndScale(interval[i].lowChance)) || height > (interval[i].high * rndScale(interval[i].highChance)))
			continue;

		for (unsigned j=0;j<interval[i].numAngles;j++)
		{
			weight += interval[i].angle[j].calcWeight(angle);
			condScaleSum += interval[i].angle[j].scale;
		}
		for (unsigned j=0;j<interval[i].numHeights;j++)
		{
			weight += interval[i].height[j].calcWeight(height);
			condScaleSum += interval[i].height[j].scale;
		}

		/*for (unsigned j=0;j<interval[i].numAngles;j++)
			interval[i].angle[j].scale /= condScaleSum;
		for (unsigned j=0;j<interval[i].numHeights;j++)
			interval[i].height[j].scale /= condScaleSum;*/
		
		if (intensity >= 0 && interval[i].intensity >= 0)
			intensity += interval[i].intensity;
		else
			intensity = -1;

		weight *= interval[i].scale * rndScale(interval[i].scaleChance) / condScaleSum;
		
		break; //point is in interval so don't check more intervals to avoid overlaps
	}
}

float CbaseGroundGen::Stype::Sinterval::SbestCondition::calcWeight(float cond)
{
	float optCond;
	if (cond < low)
		optCond = low;
	else if (cond > high)
		optCond = high;
	else
		return scale;  //in optimal interval, return highest possible weight

	float weight = (1 - (float)fabs(optCond-cond) * decay) * scale;
	if (weight < 0)
		weight = 0;
	return weight;
}

void CbaseGroundGen::calcIntensities()
{
	int max = getMaxValueIndex(&type[0].weight, numWeights, sizeof(Stype));
	float intensity = type[max].intensity;
	if (intensity == -1)
		type[max].intensity = type[max].weight;
	else
	{
		//maxtypens intensitet = summan av de övriga tyngderna * intensity
		//de övriga intensiteterna = respektive weight * (1 - intensity)
		float weightSum = 0;
		for (unsigned i=0; i<numWeights; i++)
		{
			if (i != max)
			{
				weightSum += type[i].weight;
				type[i].intensity = type[i].weight * (1 - intensity);
			}
		}
		type[max].intensity = weightSum * intensity;
	}

	//Normalize intensities
	float intensitySum = 0;
	for (unsigned i=0; i<numWeights; i++)
		intensitySum += type[i].intensity;

	for (unsigned i=0; i<numWeights; i++)
		type[i].intensity/= intensitySum;
}

void CbaseGroundGen::readStream(istream &stream)
{
	if (!active)
		return;
	istreamSearchWords(stream, "num_awTypes", '\'');
	stream >> num_awTypes;
	for (UINT i=0;i<num_awTypes;i++)
		awType[i].readStream(stream, b_texGen);
	istreamSearchWords(stream, "num_uwTypes", '\'');
	stream >> num_uwTypes;
	for (UINT i=0;i<num_uwTypes;i++)
		uwType[i].readStream(stream, b_texGen);
}

void CbaseGroundGen::Stype::readStream(istream &stream, bool b_texGen)
{
	istreamSearchWords(stream, "numIntervals", '\'');
	stream >> numIntervals;
	for (UINT i=0;i<numIntervals;i++)
	{
		istreamSearchWords(stream, "low", '\'');
		stream >> interval[i].low;
		istreamSearchWords(stream, "high", '\'');
		stream >> interval[i].high;
		istreamSearchWords(stream, "lowChance", '\'');
		stream >> interval[i].lowChance;
		istreamSearchWords(stream, "highChance", '\'');
		stream >> interval[i].highChance;
				
		istreamSearchWords(stream, "numAngles", '\'');
		stream >> interval[i].numAngles;
		istreamSearchWords(stream, "numHeights", '\'');
		stream >> interval[i].numHeights;
		
			for (UINT j=0;j<interval[i].numAngles;j++)
				interval[i].angle[j].readStream(stream);
			for (UINT j=0;j<interval[i].numHeights;j++)
				interval[i].angle[j].readStream(stream);
		
		istreamSearchWords(stream, "scale", '\'');
		stream >> interval[i].scale;
		istreamSearchWords(stream, "scaleChance", '\'');
		stream >> interval[i].scaleChance;
		if (b_texGen)
		{
			istreamSearchWords(stream, "intensity", '\'');
			stream >> interval[i].intensity;
		}

		interval[i].normalizeCondScales();
	}	
		
	if (b_texGen)
	{
		istreamSearchWords(stream, "numTex", '\'');
		stream >> numTex;
		for (UINT i=0;i<numTex;i++)
		{
			istreamSearchWords(stream, "index", '\'');
			stream >> tex[i].index;
			istreamSearchWords(stream, "mask", '\'');
			stream >> setbase(16) >> tex[i].mask >> setbase(10);
			istreamSearchWords(stream, "prob", '\'');
			stream >> tex[i].prob;
			istreamSearchWords(stream, "blendIntensity", '\'');
			stream >> tex[i].blendIntensity;
		}
		istreamSearchWords(stream, "blendProb", '\'');
		stream >> blendProb;
		istreamSearchWords(stream, "mixProb", '\'');
		stream >> mixProb;
		istreamSearchWords(stream, "exclusiveProb", '\'');
		stream >> exclusiveProb;
	}
}

void CbaseGroundGen::Stype::Sinterval::SbestCondition::readStream(istream &stream)
{
	istreamSearchWords(stream, "low", '\'');
	stream >> low;
	istreamSearchWords(stream, "high", '\'');
	stream >> high;
	istreamSearchWords(stream, "decay", '\'');
	stream >> decay;
	istreamSearchWords(stream, "scale", '\'');
	stream >> scale;
}

void CbaseGroundGen::Stype::Sinterval::normalizeCondScales()
{
	float scaleSum = 0;
	for (unsigned j=0; j<numAngles; j++)
		scaleSum += angle[j].scale;
	for (unsigned j=0; j<numHeights; j++)
		scaleSum += height[j].scale;
	
	for (unsigned j=0; j<numAngles; j++)
		angle[j].scale /= scaleSum;
	for (unsigned j=0; j<numHeights; j++)
		height[j].scale /= scaleSum;
}

void CbaseGroundGen::writeStream(ostream &stream)
{
	if (!active)
		return;
		
	stream << "'--------------------------------------------"<<endl;
	stream << "'Above water:" << endl;
		
	int tab=1;
	stream << string(tab, '\t') << "num_awTypes " << num_awTypes << endl;
	for (UINT i=0;i<num_awTypes;i++)
	{
		stream << string(tab, '\t') << "'--------------------------------------------" << endl;
		stream << string(tab, '\t') << "'awType "<< i << endl;
		awType[i].writeStream(stream, tab + 1, b_texGen);
	}
	
	stream << "'--------------------------------------------" << endl;
	stream << "'Under water:" << endl;
	
	stream << string(tab, '\t') << "num_uwTypes " << num_uwTypes << endl;
	for (UINT i=0;i<num_uwTypes;i++)
	{
		stream << string(tab, '\t') << "'--------------------------------------------" << endl;
		stream << string(tab, '\t') << "'uwType "<< i << endl;
		uwType[i].writeStream(stream, tab + 1, b_texGen);
	}
}

void CbaseGroundGen::Stype::writeStream(ostream &stream, int tab, bool b_texGen)
{
	//stream << string(tab, '\t') << "'Height intervals:" << endl;
	stream << string(tab, '\t') << "numIntervals " << numIntervals << endl;
	int tab2 = tab + 1;
	for (UINT i=0;i<numIntervals;i++)
	{
		stream << string(tab, '\t') << "'Interval " << i << endl;
		stream << string(tab2, '\t') << "low " << interval[i].low << endl;
		stream << string(tab2, '\t') << "high " << interval[i].high << endl;
		stream << string(tab2, '\t') << "lowChance " << interval[i].lowChance << endl;
		stream << string(tab2, '\t') << "highChance " << interval[i].highChance << endl;
				
		stream << string(tab2, '\t') << "'Optimal conditions:" << endl;
		int tab3 = tab2 + 1;
		stream << string(tab3, '\t') << "numAngles " << interval[i].numAngles << endl;
		stream << string(tab3, '\t') << "numHeights " << interval[i].numHeights << endl;
		
			for (UINT j=0;j<interval[i].numAngles;j++)
			{
				stream << string(tab3, '\t') << "'Angle condition " << j << endl;
				interval[i].angle[j].writeStream(stream, tab3+1);
			}
			for (UINT j=0;j<interval[i].numHeights;j++)
			{
				stream << string(tab3, '\t') << "'Height condition " << j << endl;
				interval[i].angle[j].writeStream(stream, tab3+1);
			}
		stream << string(tab2, '\t') << "scale " << interval[i].scale << endl;
		stream << string(tab2, '\t') << "scaleChance " << interval[i].scaleChance << endl;
		if (b_texGen)
			stream << string(tab2, '\t') << "intensity " << interval[i].intensity << endl;
	}	
		
	if (b_texGen)
	{
		stream << string(tab, '\t') << "'Texture information:" << endl;
		int tab2 = tab + 1;
		stream << string(tab2, '\t') << "numTex " << numTex << endl;
		int tab3 = tab2 + 1;
		for (UINT i=0;i<numTex;i++)
		{
			stream << string(tab2, '\t') << "'Texture_" << i << endl;
			stream << string(tab3, '\t') << "index " << tex[i].index << endl;
			stream << string(tab3, '\t') << setbase(16) << "mask " << tex[i].mask << setbase(10) << endl;
			stream << string(tab3, '\t') << "prob " << tex[i].prob << endl;
			stream << string(tab3, '\t') << "blendIntensity " << tex[i].blendIntensity << endl;
		}
		stream << string(tab2, '\t') << "blendProb " << blendProb << endl;
		stream << string(tab2, '\t') << "mixProb " << mixProb << endl;
		stream << string(tab2, '\t') << "exclusiveProb " << exclusiveProb << endl;
	}
}

void CbaseGroundGen::Stype::Sinterval::SbestCondition::writeStream(ostream &stream, int tab)
{
	stream << string(tab, '\t') << "low " << low << endl;
	stream << string(tab, '\t') << "high " << high << endl;
	stream << string(tab, '\t') << "decay " << decay << endl;
	stream << string(tab, '\t') << "scale " << scale << endl;
}
		
//CgroundGenObjects-----------------------
void CgroundGenObjects::readStream(istream &stream)
{
	CbaseGroundGen::readStream(stream);
}

void CgroundGenObjects::writeStream(ostream &stream)
{
	stream << "'Ground object generation info" << endl;
	stream << "'--------------------------------" << endl;
	stream << endl;
	CbaseGroundGen::writeStream(stream);
}

//CgroundGenTextures-----------------------
BOOL CgroundGenTextures::createTextures(Clandscape *land, UINT w, UINT h)
{
	for (unsigned i=0;i<numTex;i++)
		land->texture[tex[i].index].createTexture(w, h, D3DFMT_A4R4G4B4, 0, D3DPOOL_MANAGED);
	return TRUE;
}

BOOL CgroundGenTextures::lockTextures(Clandscape *land)
{
	for (unsigned i=0;i<numTex;i++)
		land->texture[tex[i].index].lock();
	return TRUE;
}

BOOL CgroundGenTextures::writePixels(Clandscape *land, int x, int y, int u, int v, int reso, const Clight &light, bool uwater, float minH, float heightFromBottom)
{
	int maxValueIndex = getMaxValueIndex(&type[0].weight, numWeights, sizeof(Stype));
			
	WORD **pixel = new WORD*[numTex];
//	DWORD *offset = new DWORD[numTex];	
	DWORD *pitch = new DWORD[numTex];
	int x2 = x * reso + u;
	int y2 = y * reso + v;
	for (unsigned i=0;i<numTex;i++)
	{
		pitch[i] = land->texture[tex[i].index].getPitch();
		DWORD offset = x2 + y2 * pitch[i];
		pixel[i] = (WORD*)land->texture[tex[i].index].getPixelPointer() + offset;
		*pixel[i]=0;
	}
    
	//Blend if blendTypes == true && intensity != -1
	bool blendTypes = rnd() < blendProb;
	
	for (unsigned w = 0;w < numWeights; w++)
	{
        float intensity = type[w].intensity;
		/*if (!blendTypes)
		{
			if (w == maxValueIndex)
			{
				if (type[w].intensity >= 0)
					intensity = type[w].intensity;
			}
			else 
				continue;
		}*/
		int mode = getRndIndex(&type[w].blendProb, 3);
		if (mode == 0) //blend
		{
			for (unsigned t = 0; t < type[w].numTex; t++)
			{
				int gIndex = type[w].tex[t].index;  //global texture list index
				*pixel[gIndex] |= (WORD)modulateMask(type[w].tex[t].blendIntensity * intensity, type[w].tex[t].mask);
			}
		}
		else if (mode == 1 || mode == 2) //mix/exclusive
		{
			int wtIndex;
			if (type[w].numTex > 1)
			{
				float *probArr = new float[type[w].numTex];
				for (unsigned t = 0; t < type[w].numTex; t++)
				{
					probArr[t] = type[w].tex[t].prob;
					if (mode == 2) //exclusive
					{
						//modify probability depending on neighboring pixels
						float probScale = 1;
						UINT gIndex = type[w].tex[t].index;
						DWORD mask = type[w].tex[t].mask;

						if (y > 0 && (*(pixel[gIndex] - pitch[gIndex]) & mask))
							probScale += 1;
						if (x > 0 && (*(pixel[gIndex] - 1) & mask))
							probScale += 1;
						if (x > 0 && y > 0 && (*(pixel[gIndex] - 1 - pitch[gIndex]) & mask) )
							probScale += 1;
						
						probArr[t] *= probScale;
					}
				}
				wtIndex = getRndIndex(probArr, type[w].numTex);  //weight texture list index
				delete[] probArr;
			}
			else
				wtIndex = 0;

			*pixel[type[w].tex[wtIndex].index] |= (WORD)modulateMask(intensity, type[w].tex[wtIndex].mask);
		}
	}
		
	/*if (wtex)
	{
		WORD wtexCol=(WORD)wtex->point(x, z);
		wtexCol = (wtexCol & 0xf000) >> 12;
				
		*pixel[waterMapIndex] |= wtexCol << getLs1Pos(waterMapMask);
	}*/
	
	//set shadow for pixel
	float lum;
	if (x<land->landx-1 && y<land->landz-1)
		lum=land->calcPixelLighting((float)x + (float)u / reso, (float)y + (float)v / reso, *((fVector*)&light.Direction), *land);
	else
		lum=1;
		
	if (lum && uwater) //caustics
	{
		float limit=-minH-16.0f;
		if (heightFromBottom>=limit && heightFromBottom < fabs(minH)-1)
		{
			int styrka=int(heightFromBottom-limit);
			styrka=(int)getMaxValue(styrka, 3);
			styrka=(int)getMinValue(styrka, 13);
			*pixel[caustMapIndex] |= int((styrka)/1) << getLs1Pos(caustMapMask);
		}
	}
																
	WORD c = (WORD)modulateMask(lum, lightMapMask);
	*pixel[lightMapIndex] |= c;

	delete[] pixel;
	//delete[] offset;
	delete[] pitch;
	
	return TRUE;
}

BOOL CgroundGenTextures::unlockTextures(Clandscape *land)
{
	for (unsigned i=0;i<numTex;i++)
		land->texture[tex[i].index].unlock();
	return TRUE;
}

BOOL CgroundGenTextures::smoothTextures(Clandscape *land)
{
	for (UINT t = 0; t < numTex; t++)
	{
		UINT index = tex[t].index;
		if (index >= land->num_tex)
			continue;
		if (tex[t].numSmoothMasks)
			land->texture[index].smooth(tex[t].smoothMask, tex[t].numSmoothMasks);
	}
	//texture[1].smooth(2, false, 0, 0xfff);
	//texture[7].smooth(2, true);
	return TRUE;
}

void CgroundGenTextures::normalizeGroundTypePixels(Clandscape *land)
{
	const int numGtt = 5;
	Cpoint dim = land->texture[awType[0].tex[0].index].getDim();
	static struct SgroundTypeTex
	{
		bool b_used;
		UINT numMasks;
        DWORD mask[4];
	} groundTypeTex[numGtt];
	for (unsigned i=0; i<num_awTypes; i++)
	{
		for (unsigned j=0; j<type[j].numTex; j++)
		{
			UINT index = type[i].tex[j].index;
			bool b_dup = false;
			for (unsigned m=0; m<groundTypeTex[index].numMasks; m++)
			{
                if (groundTypeTex[index].mask[m] == type[i].tex[j].mask)
				{
					b_dup = true;
					break;
				}
			}
			if (b_dup)
				continue;
			groundTypeTex[index].b_used = true;
			groundTypeTex[index].mask[groundTypeTex[index].numMasks++] = type[i].tex[j].mask;
			land->texture[tex[i].index].lock(0,0);
		}
	}
			
    for (int y=0; y<dim.y; y++)
	{
		for (int x=0; x<dim.x; x++)
		{
			float sum = 0;
			//Calculate sum
			for (unsigned i=0; i<numGtt; i++)
			{
				if (!groundTypeTex[i].b_used)
					continue;
				WORD color = (WORD)land->texture[tex[i].index].point(x,y);
				for (unsigned m=0; m<groundTypeTex[i].numMasks; m++)
					sum += clampBits(color, groundTypeTex[i].mask[m]);
			}
			//Divide by sum
			for (unsigned i=0; i<numGtt; i++)
			{
				if (!groundTypeTex[i].b_used)
					continue;
				WORD color = (WORD)land->texture[tex[i].index].point(x,y);
				sum = 1 / sum;
				WORD color2=0;
				WORD maskSum=0;
				for (unsigned m=0; m<groundTypeTex[i].numMasks; m++)
				{
					color2 |= (WORD)multiplyBits(sum, color, groundTypeTex[i].mask[m]);
					maskSum |= groundTypeTex[i].mask[m];
				}
				color = (WORD)writeBits(color, color2, maskSum);
				land->texture[tex[i].index].pset(x, y, color);
			}
		}
	}
	for (unsigned i=0; i<numGtt; i++)
	{
		if (!groundTypeTex[i].b_used)
			continue;
		land->texture[tex[i].index].unlock();
	}
}

void CgroundGenTextures::writeStream(ostream &stream)
{
	stream << "'Ground texture generation info" << endl;
	stream << "'-------------------------------" << endl;
	stream << endl;
	stream << "\'Blend mode: use weight as intensity\n";
	stream << "\'Not blend mode: use weight as intensity only if intensity == -1,\n";
	stream << "\t\t\'and only write intensity for the highest weight\n";
	stream << "\t\t\'intensity will be normalized a second time after smoothing\n";
	stream << "blendProb " << blendProb << endl;
	CbaseGroundGen::writeStream(stream);
	stream << endl;
	
	stream <<"'Texture list:" << endl;
	stream << "'----------------------------" << endl;
	stream << "numTex " << numTex << endl;
	for (unsigned i=0;i<numTex;i++)
	{
		stream << "'Texture_" << i << ":" << endl;
		int tab=1;
		stream << string(tab, '\t') << "index " << tex[i].index << endl;
		stream << string(tab, '\t') << "'smoothInfo" << endl;
		int tab2=2;
		stream << string(tab2, '\t') << "numSmoothMasks " << tex[i].numSmoothMasks << endl;
		for (UINT j=0;j<tex[i].numSmoothMasks;j++)
		{
			stream << string(tab2, '\t') << "'mask_" << i << ":" << endl;
			int tab3=3;
			stream << string(tab3, '\t') << setbase(16) << "mask " << (WORD)tex[i].smoothMask[j].mask << setbase(10) << endl;
			stream << string(tab3, '\t') << "numPasses " << tex[i].smoothMask[j].numPasses << endl;
			stream << string(tab3, '\t') << "b_pixel " << (tex[i].smoothMask[j].b_pixel ? "true" : "false") << endl;
		}
	}
	stream << "lightMapIndex " << lightMapIndex << endl;
	stream << setbase(16) << "lightMapMask " << lightMapMask << setbase(10) << endl;
	stream << "caustMapIndex " << caustMapIndex << endl;
	stream << setbase(16) << "caustMapMask " << caustMapMask << setbase(10) << endl;
	stream << endl;
}

void CgroundGenTextures::readStream(istream &stream)
{
	istreamSearchWords(stream, "blendProb", '\'');
	stream >> blendProb;
	
	CbaseGroundGen::readStream(stream);
	
	istreamSearchWords(stream, "numTex", '\'');
	stream >> numTex;
	for (UINT i=0;i<numTex;i++)
	{
		istreamSearchWords(stream, "index", '\'');
		stream >> tex[i].index;
		istreamSearchWords(stream, "numSmoothMasks", '\'');
		stream >> tex[i].numSmoothMasks;
		for (UINT j=0;j<tex[i].numSmoothMasks;j++)
		{
			istreamSearchWords(stream, "mask", '\'');
			stream >> setbase(16) >> tex[i].smoothMask[j].mask >> setbase(10);
			istreamSearchWords(stream, "numPasses", '\'');
			stream >> tex[i].smoothMask[j].numPasses;
			istreamSearchWords(stream, "b_pixel", '\'');
			string ps;
			stream >> ps;
			tex[i].smoothMask[j].b_pixel = (ps == "true" ? true : false);
		}
	}
	istreamSearchWords(stream, "lightMapIndex", '\'');
	stream >> lightMapIndex;
	istreamSearchWords(stream, "lightMapMask", '\'');
	stream >> setbase(16) >> lightMapMask >> setbase(10);
	istreamSearchWords(stream, "caustMapIndex", '\'');
	stream >> caustMapIndex;
	istreamSearchWords(stream, "caustMapMask", '\'');
	stream >> setbase(16) >> caustMapMask >> setbase(10);
}

//CgroundGen-----------------------------
void CgroundGen::calcWeights(float height, float angle, bool uwater)
{
	texGen.calcWeights(height, angle, uwater);
	objGen.calcWeights(height, angle, uwater);
}

void CgroundGen::readStream(istream &stream)
{
	texGen.readStream(stream);
	objGen.readStream(stream);
}

void CgroundGen::writeStream(ostream &stream)
{
	texGen.writeStream(stream);
	objGen.writeStream(stream);
}

void CgroundGen::writeFile(const string &file)
{
	ofstream stream(file.c_str());
	writeStream(stream);
}

void CgroundGen::readFile(Cpak *pak, const string &file)
{
	if (!pak)
	{
		ifstream stream(file.c_str());
		readStream(stream);
	}
	else
	{
		char *s=0;
		UINT size;
		pak->extractEntry(file, (void**)&s, &size);
		istringstream stream;
		stream.str(s);
		readStream(stream);
		delete s;
	}
}

/////////////////////////////////////////////////////
//Clandscape//////////////////////////

//protected
void Clandscape::updateSubsets()
{
	for (UINT i=0;i<num_ss;i++)
	{
		safeDeleteArray(subset[i].texIndex)
		subset[i].texIndex=CbaseMesh::subset[i].texIndex;
		subset[i].num_ti=CbaseMesh::subset[i].num_ti;
		subset[i].mtrlIndex=CbaseMesh::subset[i].mtrlIndex;
		subset[i].num_ml=CbaseMesh::subset[i].num_ml;
		subset[i].primType=CbaseMesh::subset[i].primType;
	}
}

void Clandscape::createMap(int x, int z, DWORD select)
{
	deleteMap();
	landx=x;
	landz=z;
	if (select & 1) map=new float*[landx];
	if (select & 2) n_map=new fVector*[landx];
	for (int i=0;i<landx;i++)
	{
		if (select & 1) map[i]=new float[landz];
		if (select & 2) n_map[i]=new fVector[landz];
	}
	for (int i=0;i<landz;i++)
	{
		for (int j=0;j<landx;j++)
		{
			if (select & 1) ZeroMemory(&map[i][j],sizeof(float)); 
			if (select & 2) ZeroMemory(&n_map[i][j],sizeof(fVector)); 
		}
	}
}
void Clandscape::draw(Cwater *water, int sub, int _mip, const Crect *rect)
{
	pd3dDevice->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	pd3dDevice->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 2);
	pd3dDevice->SetTextureStageState(3, D3DTSS_TEXCOORDINDEX, 3);
	pd3dDevice->SetTextureStageState(4, D3DTSS_TEXCOORDINDEX, 4);
	pd3dDevice->SetTextureStageState(5, D3DTSS_TEXCOORDINDEX, 5);
	pd3dDevice->SetTextureStageState(6, D3DTSS_TEXCOORDINDEX, 6);
	pd3dDevice->SetTextureStageState(7, D3DTSS_TEXCOORDINDEX, 7);
	
	int i=0;
	if (effect)
	{
		if (bReflDraw)
			effect->SetTechnique("refl_land");
		else
			effect->SetTechnique("land");
	}
	
	if (water)
	{
		//texture[8]=*water->getTex(((timeGetTime()/100) % 64)+4);
		static UINT caustClock;
		caustClock++;
		if (effect && fxHandle_caustAnim)
		{
			CTexture *ctex = water->getTex(((int)(caustClock / 5.f) % 64)+7);
			effect->SetTexture(fxHandle_caustAnim, ctex->getTexture());
		}
		if (!effect)
			texture[8]=*water->getTex(((caustClock/5) % 64)+5);
	}
		
	if (FAILED(hr=pd3dDevice->SetStreamSource(0, hdif.vb[0], 0, sizeof(xyz_vertex))))
		throw "Failed to set hdif as stream source";
	pd3dDevice->SetFVF(D3DFVF_XYZ);
	if (FAILED(hr=pd3dDevice->SetSoftwareVertexProcessing(true)))
		throw "Failed to set software processing";
	worldTransform();
	if (FAILED(hr=pd3dDevice->ProcessVertices(0, 0, hdif.num_vert*2, hdif.vb[1], NULL, 0)))
		throw "Failed to process hdif vertices";
	if (FAILED(hr=pd3dDevice->SetSoftwareVertexProcessing(false)))
		if (!d3dDebug)
			throw "Failed to set hardware processing";
	
	if (FAILED(hr=hdif.vb[1]->Lock(0, 0, (VOID**)&hdif.data, D3DLOCK_READONLY)))
		throw "Couldn't lock hdif.vb[1] buffer";
		
	baseDraw(sub, _mip, rect);

	
	if (FAILED(hr=hdif.vb[1]->Unlock()))
		throw "Couldn't unlock; hdif.vb[1] buffer";

}

void Clandscape::baseDraw(int sub, int _mip, const Crect *rect, bool water)
{
	if (!visible)
		return;
	if (sub>=(int)num_ss)
		return;
	
	if (FAILED(hr=pd3dDevice->SetFVF(fvf)))
		throw "Couldn't set fvf";
	if (!fvf)
		if (FAILED(hr=pd3dDevice->SetVertexDeclaration(vdecl)))
			throw "Couldn't set vertex declaration";
					
	mip=_mip;
	
	int s, s2;
	//Bestäm subset-----
	if (sub<0)
	{
		s=0;
		s2=num_ss;
	}
	else
	{
		s=sub;
		s2=sub+1;
	}
	//--------------------------	
	Cpoint camGridPos((int)cam.pos.x/cellx, (int)cam.pos.z/celly);
	float sqrtOfCamHeight=(float)sqrt(cam.pos.y);
	float powOfCamHeight=(float)pow(cam.pos.y, 2);
	int view=(int)powOfCamHeight*2+10;
	Crect r(camGridPos.x-1-view, camGridPos.y-1-view, camGridPos.x+view+2, camGridPos.y+view+2);
	if (rect)
		r=*rect;
	r.normalize();
	r.clip(Crect(0, 0, gridx, gridy));
	locateP(500, 0);
	//D3DXHANDLE tech=0;
	UINT numPasses = beginEffect();
	if (!numPasses)
		throw "Couldn't begin landscape effect";
	for (int i=s;i<s2;i++)
	{
		Cpoint p=camGridPos;
		p.clip(Crect(0, 0, gridx-1, gridy-1));
				
		if (!effect || 1)
		{
			applySubsetMtrl(i);
			applySubsetTex(i);
		}
		if (effect)
		{
			if (FAILED(hr=effect->BeginPass(i)))
				throw "Couldn't begin effect pass";
		}
		for (int y=r.top;y<r.bottom;y++)
		{
			for (int x=r.left;x<r.right;x++)
			{
				if (!mlFixed)
				{
					if (!water)  //ställ in rätt miplevel
					{
						mip=0;
						int offset = (x + (y * gridx)) * num_ml ;
						float t;
						do
						{
							fVector pos = hdif.data[offset + mip].pos;
							fVector pos2 = hdif.data[offset + mip + hdif.num_vert].pos;
							t = (float)getMaxValue( fabs(pos.x/pos.z - pos2.x/pos2.z), fabs(pos.y/pos.z - pos2.y/pos2.z) );
							//t = abs(int(pos.y - pos2.y));
							mip++;
						} while (t<0.0016 && (UINT)mip < num_ml);	
						mip+=_mip;
						if (mip<0)
							mip=0;
						if ((UINT)mip>=num_ml)
							mip=num_ml-1;
						//else
						//	mip-=2*(num_ml-mip)/num_ml;  //öka stabiliteten
					}
					else  //water mip
					{
						if (x>=p.x-1 && x<=p.x+1 && y>=p.y-1 && y<=p.y+1 && cam.pos.y<waveCalcLimit)
							mip=0;
						else
							mip=num_ml-1;
					}
				}
				
				if (mip>=(int)num_ml)
					mip=num_ml-1;


				if (!subset[i].mipLevel[mip][x][y].pib)
					continue;
					
				if (FAILED(hr=pd3dDevice->SetStreamSource(0, subset[i].mipLevel[mip][x][y].pvb, 0, fvfSize)))
					throw "Couldn't set stream source.";
				if (FAILED(hr=pd3dDevice->SetIndices(subset[i].mipLevel[mip][x][y].pib)))
					throw"Couldn't set indices.";
				if (FAILED(hr = pd3dDevice->DrawIndexedPrimitive(subset[i].primType, 0, 0, subset[i].mipLevel[mip][x][y].num_vert, 0, subset[i].mipLevel[mip][x][y].num_prim)))
					throw "DrawIndexedPrimitive failed.";
					
				//pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
			}
		}
		if (effect)
			if (FAILED(hr=effect->EndPass()))
				throw "Couldn't end effect pass";
	}
	if (!endEffect())
		throw "Couldn't end landscape effect.";
}

void Clandscape::calcMapNormals()
{
	for (int z=0;z<landz;z++)
		for (int x=0;x<landx;x++)
			n_map[x][z]=calcNormal(x, z);
}

fVector Clandscape::calcNormal(int x, int z, bool nrm)
{
	//static fVector normal[6];
	//static fVector vector[6];
	if (x<0 || x>=landx || z<0 || z>=landz)
		return fVector(0, 0, 0);
	
	int x1=x-1;int x2=x+1;int z1=z-1;int z2=z+1;
	if (x1<0) x1=landx-1; if (x2>=landx) x2=0;
	if (z1<0) z1=landz-1; if (z2>=landz) z2=0;

	/*vector[0]=fVector(-1, map[x1][z]-map[x][z], 0);
	vector[1]=fVector(0, map[x][z1]-map[x][z], -1);
	//vector[2]=fVector(1, map[x2][z1]-map[x][z], -1);
	vector[2]=fVector(1, map[x2][z]-map[x][z], 0);
	vector[3]=fVector(0, map[x][z2]-map[x][z], 1);
	//vector[5]=fVector(-1, map[x1][z2]-map[x][z], +1);*/
	float y1 = map[x1][z] - map[x][z];
	float y2 = map[x][z1] - map[x][z];
	float y3 = map[x2][z] - map[x][z];
	float y4 = map[x][z2] - map[x][z];

//	Cross products
	fVector norm(y1, 4, y2);
	norm.x -= y3; norm.z += y2;
	norm.x -= y3; norm.z -= y4;
	norm.x += y1; norm.z -= y4;
	if (nrm)
		D3DXVec3Normalize(&norm, &norm);
	return norm;
	
	/*for (int n=0;n<4;n++)
	{
		static int n2;
		if (n==3)
			n2=0;
		else 
			n2=n+1;
		D3DXVec3Cross(&normal[n], &vector[n2], &vector[n]);
		//normal[n] = fVector(-vector[n].y, 1, vector[n2].y); //cross product
		if (n>0)
			normal[0]+=normal[n];
	}
	if (nrm)
		D3DXVec3Normalize(&normal[0], &normal[0]);
	return normal[0];*/
}


float **Clandscape::getMap()
{
	return map;
}

fVector **Clandscape::getNormalMap()
{
	return n_map;
}

BOOL Clandscape::createMeshFromMap(float wrapTex, float clipMarg)
{
	if (!subset)
		return FALSE;
			
	D3DVERTEXELEMENT velem[]={ {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
																D3DDECL_END()};
	if (FAILED(hr=pd3dDevice->CreateVertexDeclaration(velem, &hdif.decl)))
		throw "Couldn't create hdif vertex declaration.";
	
	hdif.num_vert = num_ml * gridx  * gridy;
	for (int i=0;i<2;i++)
		if (FAILED(hr=pd3dDevice->CreateVertexBuffer(sizeof(xyz_vertex) * hdif.num_vert * 2, D3DUSAGE_SOFTWAREPROCESSING, D3DFVF_XYZ, D3DPOOL_SYSTEMMEM, &hdif.vb[i], 0)))
			throw"Failed to creat hdif vertex buffer.";
	hdif.vb[0]->Lock(0,0,(VOID**)&hdif.data,0);	
	    	
	//Fyller i subsetsen
	UINT vbsize=(cellx+2)*(celly+2);
	waterVertex *vb=new waterVertex[vbsize];
	landVertex *vb2=new landVertex[vbsize];
	UINT ibsize=(cellx)*(celly)*6;
	USHORT *ib=new USHORT[ibsize];
	bool **writeMap=0;


	for (UINT s=0;s<num_ss;s++)
	{
		//Fyller i en cell i taget
		for (int gy=0;gy<gridy;gy++)
		{
			for (int gx=0;gx<gridx;gx++)
			{
				//Cellens miplevels
				for (UINT m=0;m<subset[s].num_ml;m++)
				{	
					int vstep=(int)pow(2.0f, (int)m);
					//Miplevelns vertices
					
					float marg = clipMarg;
					int side = -1;
					if (s==0)
					{
						marg = -clipMarg;
						side = 1;
					}
					int vbi=fillVB(vb, gx*cellx, gy*celly, vstep, s, wrapTex, writeMap, 0, side, marg);
					
					int ibi=0;
					if (vbi>=3)
						ibi=fillIB(ib, vstep, vb, vbi, true);
					//Skapa vertex- och indexbuffertar
					if (ibi)
					{
						copyWaterToLand(vb2, vb, vbsize);
						subset[s].mipLevel[m][gx][gy].num_vert=vbi;
						subset[s].mipLevel[m][gx][gy].pvb=createVB(vb2, vbi*fvfSize, fvf, D3DPOOL_MANAGED);
						calcHDifs(s, m, gx, gy);
						subset[s].primType=D3DPT_TRIANGLELIST; 
						subset[s].mipLevel[m][gx][gy].num_prim=ibi/3;
						subset[s].mipLevel[m][gx][gy].pib=createIB(ib, ibi*sizeof(USHORT), D3DFMT_INDEX16);
					}
					else
					{
						subset[s].mipLevel[m][gx][gy].pvb=0;
						subset[s].mipLevel[m][gx][gy].pib=0;
					}
				}
			}
		}
	}

	for (int i=0;i<landx;i++)
		safeDeleteArray(writeMap[i]);
	safeDeleteArray(writeMap);
	
	safeDeleteArray(vb);
	safeDeleteArray(vb2);
	safeDeleteArray(ib);
	hdif.vb[0]->Unlock();
	return TRUE;
}

bool Clandscape::vertexIsVisible(Clandscape *land2, int clipSide, float clipMarg, int x, int z)
{
	//räkna ut koordinater i andra landskapet
	int tx=int(x+pos.x-land2->pos.x),tz=int(z+pos.z-land2->pos.z);
	//är koordinaterna utanför gränserna?
	if (!(tx>=land2->landx || tz>=land2->landz || tx<0 || tz<0))
		//är det här landskapet(vattnet) högre(pm>0) eller lägre(pm<0) än det andra landskapet?
		if (!((pos.y+clipMarg)*clipSide<(land2->map[tx][tz]+land2->pos.y)*clipSide))
			return true;
	return false;
}

int Clandscape::fillVB(waterVertex *vb, int x1, int y1, int vstep, int s, float wrapTex, bool **&writeMap, Clandscape *land2, int clipSide, float clipMarg)
{
	int vbi=0;
	Cpoint tdim=texture[0].getDim();
	Cpoint tdim2=texture[0].getActualDim();
	float tumax=(float)tdim.x/tdim2.x;
	float tvmax=(float)tdim.y/tdim2.y;
	tumax/=landx;
	tvmax/=landz;
	int x2=x1+cellx;//+vstep;
	int y2=y1+celly;//+vstep;
	
	if (!writeMap)
	{
		writeMap=new bool*[landx];
		for (int i=0;i<landx;i++)
			writeMap[i]=new bool[landz];
	}
	for (int i=0;i<landx;i++)
		for (int j=0;j<landz;j++)
			writeMap[i][j]=false;

	for (int y=y1;y<=y2;y+=vstep)
	{
		if (y>landz-vstep)
			continue;
		int yhigh = y + vstep;
		if (yhigh == landz)
			yhigh--;
		int ylow = y - vstep;
		if (ylow < y1)
			ylow = y1;
		for (int x=x1;x<=x2;x+=vstep)
		{
			if (x>landx-vstep)
				continue;
			int xhigh = x + vstep;
			if (xhigh == landx)
				xhigh--;
			int xlow = x - vstep;
			if (xlow < x1)
				xlow = x1;

	
			bool write=false;
			if (land2 && vstep<32) 
			{
				//klipp bort övertäckta vertices
				//for (int xx=x;xx<=vstep+x;xx+=vstep)
				//	for (int yy=y;yy<=vstep+y;yy+=vstep)
						if (vertexIsVisible(land2, clipSide, clipMarg, x, y))
							write=true;
			}
			else if (!land2)
			{
				if (map[x][y] * clipSide > clipMarg * clipSide)
					write = true;
			}
			else
			{
				write=true;
			}
			if (write)
			{
				writeMap[x][y]=true;
				writeMap[xhigh][y]=true;
				writeMap[x][yhigh]=true;
				
				writeMap[xlow][yhigh]=true;
				writeMap[xlow][y]=true;
				writeMap[x][ylow]=true;
				writeMap[xhigh][ylow]=true;

				writeMap[xhigh][yhigh]=true;
				writeMap[xlow][ylow]=true;

				//writeMap[xh][yh]=true;
			}
		}
	}
	for (int y=y1;y<=y2;y+=vstep)
	{
		if (y>landz)
			continue;
		int _y=y;
		if (y==landz)
			_y--;

		for (int x=x1;x<=x2;x+=vstep)
		{
			if (x>landx)
				continue;
			int _x=x;
			if (x==landx)
				_x--;
			if (writeMap[_x][_y])
			{
				//Fyller i vertexbufferten
				
				//ta bort sprickor (endast på land)
				int xp=0, yp=0;
				if (!land2)
				{
					if (_x==x2) xp=1;
					if (_y==y2) yp=1;
				}

				float h;
				if (!map)
					h=0;
				else
					h=map[_x+xp][_y+yp];
				
				vb[vbi].pos=fVector(float(_x+xp), h, float(_y+yp));
				vbi++;
			}
		}
	}
	return vbi;
}

int Clandscape::fillIB(USHORT *ib, int vstep, waterVertex *vb, int vbsize, bool fillGaps)
{
	int ibi=0;
	for (int i=0;i<vbsize-1;i++)
	{
		//Fyll i index om det finns vertices till höger och nertill----
		fVector pos=vb[i].pos;
		if (pos.x==landx-1)
			continue;
		if (pos.z==landz-1)
			break;
		
		float x2=pos.x+vstep;
		if (x2==(float)landx)
			x2=float(landx-1);
		if (0==int(x2)%cellx && fillGaps)
			x2++;
		
		float z2=pos.z+vstep;
		if (z2==landz)
			z2=float(landz-1);
		if (0==int(z2)%celly && fillGaps)
			z2++;
		
		//höger
		if (vb[i+1].pos.x != x2 || vb[i+1].pos.z != pos.z)
			continue;
		int j=i+2;
		//ner
		while (j<vbsize-1 && (vb[j].pos.x != pos.x || vb[j].pos.z != z2))
			j++;
		if (j==vbsize-1)
			continue;
		
		//nere till höger
		if (vb[j+1].pos.x != x2 || vb[j+1].pos.z != z2)
			continue;
		//---------------------------------------------------
		
		ib[ibi]=i;
		ib[ibi+1]=i+1;
		ib[ibi+2]=j;
		ibi+=3;

		ib[ibi]=i+1;
		ib[ibi+1]=j+1;
		ib[ibi+2]=j;
		ibi+=3;
	}
	return ibi;
}

void Clandscape::findMinHeight(fVector *hpos, const Crect *r)
{
	fVector hp;
	hp.y=99999;
	Crect rect;
	if (!r)
		rect=Crect(0,0,landx, landz);
	else
	{
		rect=*r;
		rect.clip(Crect(0,0,landx, landz));
	}
	
	for (int x=rect.left;x<rect.right;x++)
		for (int z=rect.top;z<rect.bottom;z++)
			if (hp.y>map[x][z])
			{
				hp.y=map[x][z];
				hp.x=(float)x;
				hp.z=(float)z;
			}
	if (hpos)  //return result
		*hpos=hp;
	else  //store result in internal struct
		minHeight=hp;
}

void Clandscape::findMaxHeight(fVector *hpos, const Crect *r)
{
	fVector hp;
	hp.y=-99999;
	Crect rect;
	if (!r)
		rect=Crect(0,0,landx, landz);
	else
	{
		rect=*r;
		rect.clip(Crect(0,0,landx, landz));
	}
	
	for (int x=rect.left;x<rect.right;x++)
		for (int z=rect.top;z<rect.bottom;z++)
			if (hp.y<map[x][z])
			{
				hp.y=map[x][z];
				hp.x=(float)x;
				hp.z=(float)z;
			}
	if (hpos)  //return result
		*hpos=hp;
	else  //store result in internal struct
		maxHeight=hp;
}

void Clandscape::findMinMaxHeight()
{
	maxHeight.y=-99999;
	minHeight.y=99999;
	for (int x=0;x<landx;x++)
		for (int z=0;z<landz;z++)
		{
			if (maxHeight.y<map[x][z])
			{
				maxHeight.y=map[x][z];
				maxHeight.x=(float)x;
				maxHeight.z=(float)z;
			}
			if (minHeight.y>map[x][z])
			{
				minHeight.y=map[x][z];
				minHeight.x=(float)x;
				minHeight.z=(float)z;
			}
		}
}

float Clandscape::getMaxHeight()
{
	return maxHeight.y;
}

float Clandscape::getMinHeight()
{
	return minHeight.y;
}

fVector Clandscape::getMaxHeightPos()
{
	return maxHeight;
}

fVector Clandscape::getMinHeightPos()
{
	return minHeight;
}

void Clandscape::createSubsets(UINT ns, const Cpoint &cellSize, UINT ml)
{
	deleteSubsets();
	cellx=cellSize.x;
	celly=cellSize.y;
	gridx=landx/cellx;
	gridy=landz/celly;
	//räkna ut antal miplevels
	if (!ml)
	{
		int lx=cellx, lz=celly;
		while (lx>1 && lz>1)
		{
			lx/=2;lz/=2;
			ml++;
		}
		ml++;
	}
	
	num_ss=ns;
	num_ml=ml;
	subset=new SlandSubset[num_ss];
	for (UINT i=0;i<num_ss;i++)
	{
		subset[i].mtrlIndex=0;
		subset[i].texIndex=0;
		subset[i].num_ml=ml;
		subset[i].mipLevel=new SmipLevel**[ml];
		for (UINT m=0;m<ml;m++)
		{
			subset[i].mipLevel[m]=new SmipLevel*[gridx];
			for (int x=0;x<gridx;x++)
			{
				subset[i].mipLevel[m][x]=new SmipLevel[gridy];
				for (int y=0;y<gridy;y++)
				{
					subset[i].mipLevel[m][x][y].pib=0;
					subset[i].mipLevel[m][x][y].pvb=0;
				}
				

			}
		}
	}
}

void Clandscape::release()
{
	CbaseMesh::release();
	deleteSubsets();
	deleteMap();
	for (int i=0;i<2;i++)
		safeRelease(hdif.vb[i]);
	safeRelease(hdif.decl);
}

void Clandscape::deleteMap()
{
	if (map)
	{
		for (int i=0;i<landx;i++)
			safeDeleteArray(map[i]);
		safeDeleteArray(map);
	}
	if (n_map)
	{
		for (int i=0;i<landx;i++)
			safeDeleteArray(n_map[i]);
		safeDeleteArray(n_map);
	}
}
void Clandscape::deleteSubsets()
{
	if (subset)
	{
		for (UINT i=0;i<num_ss;i++)
		{
			safeDeleteArray(subset[i].texIndex);
			if (subset[i].mipLevel)
			{
				for (UINT j=0;j<subset[i].num_ml;j++)
				{
					for (int y=0;y<gridy;y++)
					{
						for (int x=0;x<gridx;x++)
						{
							safeRelease(subset[i].mipLevel[j][x][y].pvb);
							safeRelease(subset[i].mipLevel[j][x][y].pib);
						}
					}
				}	
				safeDeleteArray(subset[i].mipLevel);
			}
		}
		safeDeleteArray(subset);
	}
}

BOOL Clandscape::applySubsetMtrl(UINT index)
{
	if (index>=num_ss)
		return FALSE;
	applyMtrl(subset[index].mtrlIndex);
	return TRUE;
}

BOOL Clandscape::applySubsetTex(UINT index)
{
	if (index>=num_ss)
		return FALSE;
	for (UINT stage=0;stage<subset[index].num_ti;stage++) 
	{
		UINT i=subset[index].texIndex[stage];
		applyTex(i, stage);
	}
	
	return TRUE;
}

BOOL Clandscape::setSubsetMtrl(UINT mi, UINT subi)
{
	if (subi>=num_ss || mi>=num_mtrl)
		return FALSE;
	subset[subi].mtrlIndex=mi;
	return TRUE;
}

BOOL Clandscape::setSubsetTexArr(UINT *ti, UINT numStages, UINT subi)
{
	if (subi>=num_ss)
		return FALSE;
	safeDeleteArray(subset[subi].texIndex);
	subset[subi].texIndex=new UINT[numStages];
	subset[subi].num_ti=numStages;
	for (UINT i=0;i<numStages;i++)
	{
		if (ti[i]>=num_tex)
			continue;
		subset[subi].texIndex[i]=ti[i];
	}
	return TRUE;
}

BOOL Clandscape::setSubsetTex(UINT ti, UINT tex, UINT subi)
{
	if (subi>=num_ss || tex>=num_tex || ti>=subset[subi].num_ti)
		return FALSE;
	subset[subi].texIndex[ti]=tex;
	return TRUE;
}

Cpoint Clandscape::getDim()
{
	return Cpoint(landx, landz);
}

void Clandscape::genGround(int reso, const Clight &light, CTexture *wtex, CgroundGen &groundGen)
{
	D3DCOLOR ambcol;
	pd3dDevice->GetRenderState(D3DRS_AMBIENT, (DWORD*)&ambcol);
	D3DXCOLOR ambient=ambcol;
	
	groundGen.texGen.createTextures(this, landx*reso, landz*reso);	
	
	/*for (unsigned i=0;i<groundGen.texGen.numTex;i++)
	{
		int ltIndex = groundGen.texGen.tex[i].index;  //Landscape texture list index
		texture[ltIndex].createTexture(landx, landz, D3DFMT_A4R4G4B4, 0, D3DPOOL_MANAGED);
		texture[ltIndex].lock(0,0);
		WORD *texBits[i] = texture[ltIndex].getPixelPointer();
		UINT texPitch[i] = texture[ltIndex].getPitch();
	}*/
		
	groundGen.texGen.lockTextures(this);
	if (wtex)
		wtex->lock(0,0);
	
	
	static float greenAngle=1;
	static float brownAngle=0.5f;
	static float greyAngle=0.0f;
	static float snowAngle=1;
	static float sandAngle=1;

	static float greenHeight=0;
	static float brownHeight=1;
	static float greyHeight=0.5f;
	static float snowHeight=1;

	float minH=minHeight.y;
	float maxH=maxHeight.y;
	
	float fstep=1.0f/reso;
	
	for (int z=0;z<landz;z++)
	{
		//COLOR *pBits2=pBits;
		int resoz=z==landz-1 ? 1 : reso;
		for (int x=0;x<landx;x++)
		{
			float fy=0;
			int resox=x==landx-1 ? 1 : reso;
			for (int v=0;v<resoz;v++)
			{
				float fx=0;
				for (int u=0;u<resox;u++)
				{
					static fVector normal;
					static float height;
					if (x==landx-1 || z==landz-1)
					{
						normal=fVector(0,0,0);
						height=0;
					}
					else
					{
						if (fy<1-fx)
						{
							normal=interNormals(fx, fy, n_map[x][z], n_map[x+1][z], n_map[x][z+1]);
							height=interHeights(fx, fy, map[x][z], map[x+1][z], map[x][z+1]);
						}
						else
						{
							float fx2=1-fx;
							float fy2=1-fy;
							normal=interNormals(fx2, fy2, n_map[x+1][z+1], n_map[x][z+1], n_map[x+1][z]);
							height=interHeights(fx2, fy2, map[x+1][z+1], map[x][z+1], map[x+1][z]);
						}
					}
					float angle;
					if (normal.x==0 && normal.z==0)
						angle=1;
					else
					{
						fVector normal2(normal); //xz-projection of normal
						normal2.y=0;
						normal2.normalize();
						angle=(float)acos(normal.dot(normal2))/pihalf;
					}
					
					float greenposs=0;
					float brownposs=0;
					float greyposs=0;
					float snowposs=0;
					float sandposs=0;
										
					bool uwater=false;
					if (height<0)
						uwater=true;
					
					float heightNoClamp=height;
					float heightFromBottom=height-minH;

					float beachHeight=rnd()*0.5f+0.1f;
					bool ubeach = height<beachHeight;
					
					if (ubeach)
						height=heightFromBottom/(beachHeight-minH);  //0=botten, 1=beachheight
					else
						height=(height-beachHeight)/maxH;   //0=beachheight, 1=maxHeight
					
					groundGen.calcWeights(height, angle, ubeach);
										
					groundGen.texGen.writePixels(this, x, z, u, v, reso, light, uwater, minH, heightFromBottom);
					
									
					fx+=fstep;
				}
				fy+=fstep;
			}
		}
	}
	groundGen.texGen.unlockTextures(this);
	if (wtex)
		wtex->unlock();
	
	groundGen.texGen.smoothTextures(this);
	
	groundGen.texGen.normalizeGroundTypePixels(this);

	for (unsigned i=0; i<groundGen.texGen.numTex; i++)
		texture[groundGen.texGen.tex[i].index].fillMipLevels();
}

float Clandscape::pointCollision(baseObj &obj, float length, int sign, bool reset, float bouncy)
{
	float fx1=obj.pos.x-pos.x;
	float fz1=obj.pos.z-pos.z;
	if ((int)fx1<0 || (int)fz1<0 || (int)fx1>=landx-1 || (int)fz1>=landz-1)
		return 0;
	float height;
	height=getHeight(fx1, fz1)+pos.y;
	
	if (obj.pos.y*sign-length<height*sign)
	{
		fVector normal=getNormal(fx1, fz1);
		if (reset)
		{
			obj.pos.y=height+length*sign*1.1f;
			fVector vec=obj.velocity;
			float v=vec.getValue();
			vec.normalize();
			vec.reflect(normal);
			obj.velocity=vec*v*bouncy;
		}
		return float(fabs(normal.dot(obj.velocity))); //returnera hastigheten mot ytan
	}
	else
		return 0;
}

float Clandscape::calcPixelLighting(float x, float z, const fVector &lightDir, const Clandscape &sunblock, bool normalTest)
{
	float height=getHeight(x, z)+pos.y;
	fVector point(x, height, z);

	float max;
	if (fabs(lightDir.x)<fabs(lightDir.z))
		max=(float)fabs(lightDir.z);
	else if (lightDir.x)
		max=(float)fabs(lightDir.x);
	else //zenit
		return 1;
	fVector ldir=lightDir/max;

	while(isInsideRect(0, 0, landx-2, landz-2, (int)point.x, (int)point.z))
	{
		point-=ldir;
		//är landskapet högre än solstrålen?
		if (sunblock.getHeight(point.x, point.z)+sunblock.pos.y>point.y)
			return 0;
	}
	if (!normalTest)
		return 1;
	//Punkten ligger inte i skugga, beräkna vinkeln från solen
	fVector normal=getNormal(x, z);
	float shadow=-normal.dot(lightDir);
	if (shadow<0)
		return 0;
	shadow=acosf(shadow)/pihalf;
	return 1-shadow;
}

float Clandscape::getHeight(float x, float z) const
{
	float height;
	
	float fx=x-(int)x;
	float fz=z-(int)z;
	
	int x1=(int)x;
    int z1=(int)z;
	
	int x2=x1+1;
	int z2=z1+1;
	
	if (x1>=landx || z1>=landz || !map)
		return 0;
	if (x2>=landx || z2>=landz)
		return map[x1][z1];

	//float ref[2]={fx, fz};
	//float dist[3];
	if (fz<1-fx)
	{
		//float points[6]={0,0, 1,0, 0,1};
		//calcDistances(dist, 2, 3, points, ref);
		height=interHeights(fx, fz, map[x1][z1], map[x2][z1], map[x1][z2]);
	}
	else
	{
		//float points[6]={1,0, 1,1, 0,1};
		//calcDistances(dist, 2, 3, points, ref);
		fx=1-fx;fz=1-fz;
		height=interHeights(fx, fz, map[x2][z2], map[x1][z2], map[x2][z1]);
		
	}
	return height;
}

fVector Clandscape::getNormal(float x, float z)
{
	fVector normal;
	float fx=x-(int)x;
	float fz=z-(int)z;
	int x1=(int)x;
    int z1=(int)z;
	int x2=x1+1;
	int z2=z1+1;
	
	if (x1>=landx || z1>=landz || !n_map)
		return 0;
	if (x2>=landx || z2>=landz)
		return n_map[x1][z1];
	
	if (fz<1-fx)
		normal=interNormals(fx, fz, n_map[x1][z1], n_map[x2][z1], n_map[x1][z2]);
	else
	{
		fx=1-fx;fz=1-fz;
		normal=interNormals(fx, fz, n_map[x2][z2], n_map[x1][z2], n_map[x2][z1]);
	}
	return normal;
}

void Clandscape::calcHDifs(UINT s, UINT m, UINT gx, UINT gy)
{
	if (s>num_ss || m>subset[s].num_ml || gx>(UINT)gridx || gy>(UINT)gridy)
		return;

	int voffset=0;
	int hdifDataOffset = (gx + gy * gridx) * num_ml + m;
	int hdifDataOffset2 = hdifDataOffset + hdif.num_vert;

	SmipLevel &mipLevel=subset[s].mipLevel[m][gx][gy];
	
	landVertex *vertices;

	mipLevel.pvb->Lock(0, 0, (void**)&vertices, D3DLOCK_READONLY);
	
	float max=0, max_x=0, max_y=0, max_y2=0, max_z=0;
	int step=(int)pow(2.0f, (int)m);
	
	for (UINT y=gy*celly ; y<(gy+1)*celly ; y+=step)
	{
        if (y+step>=(UINT)landz || y-step<0)
			continue;
		for (UINT x=gx*cellx ; x<(gx+1)*cellx ; x+=step)
		{
			if (x+step>=(UINT)landx || x-step<0)
				continue;
			//interpolate
			float h2;
			int modStep=step*2;
			if (x%modStep && y%modStep==0)
				h2=(map[x+step][y]+map[x-step][y])/2;
			else if (x%modStep==0 && y%modStep)
				h2=(map[x][y+step]+map[x][y-step])/2;
			else if (x%modStep && y%modStep)
				h2=(map[x-step][y+step]+map[x+step][y-step])/2;
			else
				h2=map[x][y];
			
			float dif=(float)fabs(map[x][y]-h2);
            if (max<dif)
			{				
				max_x=(float)x;
				max_y = map[x][y];
				max_z = (float)y;
				max_y2=h2;
				max=dif;
			}

			//int vi=findVertexWithPos(vertices, voffset, mipLevel.num_vert, (float)x, map[x][y], (float)y);
			//voffset++;
			//if (vi>=0)
			//	vertices[vi].tween=dif;
		}
	}
	hdif.data[hdifDataOffset].pos.x = hdif.data[hdifDataOffset2].pos.x = max_x;
	hdif.data[hdifDataOffset].pos.y = max_y;
	hdif.data[hdifDataOffset].pos.z = hdif.data[hdifDataOffset2].pos.z = max_z;
				
	hdif.data[hdifDataOffset2].pos.y=max_y2;
				
	mipLevel.pvb->Unlock();
	//mipLevel.tag=max;
}

void Clandscape::setWaterDepth(float depth)
{
	wlevel=(int)depth;
	float dif=-minHeight.y-depth;
	if (dif==0)
		return;
	for (int z=0;z<landz;z++)
		for (int x=0;x<landx;x++)
			map[x][z]+=dif;
	maxHeight.y+=dif;
	minHeight.y+=dif;
}

void Clandscape::snapVertsToWlevel()
{
	for (int j=0;j<landz-1;j++)
	{
		for (int i=0;i<landx-1;i++)
		{
			float *height[3] = {&map[i][j], &map[i+1][j], &map[i][j+1]};
			float aDist=0, bDist=0;
			
			if (*height[0] * *height[1] < 0 || *height[0] * *height[2] < 0)
			{
				for (int i=0; i<3; i++)
				{
					if (*height[i] > 0)
						aDist += *height[i];
					else
						bDist -= *height[i];
				}
				for (int i=0; i<3; i++)
				{
					if (aDist >= bDist && *height[i] < 0 || aDist <= bDist && *height[i] > 0)
						*height[i]=0;
				}
			}


			height[0] = &map[i+1][j+1];
			if (*height[0] * *height[1] < 0 || *height[0] * *height[2] < 0)
			{
				for (int i=0; i<3; i++)
				{
					if (*height[i] > 0)
						aDist += *height[i];
					else
						bDist -= *height[i];
				}
				for (int i=0; i<3; i++)
				{
					if (aDist >= bDist && *height[i] < 0 || aDist <= bDist && *height[i] > 0)
						*height[i]=0;
				}
			}



		}
	}
}



BOOL Clandscape::copyMapToTexture(float scale, CTexture &tex)
{
	if (!tex.createTexture(landx, landz, D3DFMT_X8R8G8B8, 0, D3DPOOL_MANAGED))
		return FALSE;
	if (tex.lock(0,0))
	{
		for (int j=0;j<landz;j++)
		{
			for (int i=0;i<landx;i++)
			{
				COLOR c = BYTE(map[i][j] * scale);
				c|=(c<<8) | (c<<16);
				tex.pset(i, j, c);
			}
		}
		tex.unlock();
	}
	else
		return FALSE;
	return TRUE;
}

BOOL Clandscape::copyNmapToTexture(CTexture &tex)
{
	if (!tex.createTexture(landx, landz, D3DFMT_X8R8G8B8, 0, D3DPOOL_MANAGED))
		return FALSE;
	if (tex.lock(0,0))
	{
		for (int j=0;j<landz;j++)
		{
			for (int i=0;i<landx;i++)
			{
				COLOR c = (BYTE((n_map[i][j].x+1)*127) << 16) + (BYTE((n_map[i][j].y+1)*127) << 8) + BYTE((n_map[i][j].x+1)*127);
				tex.pset(i, j, c);
			}
		}
		tex.unlock();
	}
	else
		return FALSE;
	return TRUE;
}

BOOL Clandscape::copyTextureToMap(Cpak *pak, const string &file, float scale)
{
	CTexture hmap;
	if (!hmap.createSurface(landx, landz, sf))
		return FALSE;
	hmap.loadImage(pak, file);
	return copyTextureToMap(hmap, scale);
}

BOOL Clandscape::copyTextureToMap(CTexture &tex, float scale)
{
	if (tex.lock(0, D3DLOCK_READONLY))
	{
		for (int j=0;j<landz;j++)
			for (int i=0;i<landx;i++)
				map[i][j]=float(0xff & tex.point(i,j))*scale;
		tex.unlock();
	}
	else
		return FALSE;
	return TRUE;
}

BOOL Clandscape::copyTextureToNmap(Cpak *pak, const string &file)
{
	CTexture nmap;
	if (!nmap.createSurface(landx, landz, sf))
		return FALSE;
	nmap.loadImage(pak, file);
	return copyTextureToNmap(nmap);
}

BOOL Clandscape::copyTextureToNmap(CTexture &tex)
{
	if (tex.lock(0, D3DLOCK_READONLY))
	{
		for (int j=0;j<landz;j++)
		{		
			for (int i=0;i<landx;i++)
			{
				COLOR c = tex.point(i,j);
				n_map[i][j].x = ((c & 0xff0000) >> 16) / 127.0f - 1;
				n_map[i][j].x = ((c & 0xff00) >> 8) / 127.0f - 1;
				n_map[i][j].x = (c & 0xff) / 127.0f - 1;
			}
		}		
		tex.unlock();
	}
	else
		return FALSE;
	return TRUE;
}

BOOL Clandscape::createEffect(Cpak *pak, const string &file, DWORD flags, const D3DXMACRO *defines, LPD3DXINCLUDE include, LPD3DXEFFECTPOOL pool)
{
	if (!CbaseMesh::createEffect(pak, file, flags, defines, pool))
		return FALSE;
	initEffect();
	return TRUE;
}

void Clandscape::setEffect(LPD3DXEFFECT fx)
{
	CbaseMesh::setEffect(fx);
	initEffect();
}

void Clandscape::initEffect()
{
	if (effect)
	{
		fxHandle_caustAnim = effect->GetParameterBySemantic(0, "CAUSTANIM");
		CbaseMesh::initEffect();
	}
}


//hmap gens

void Clandscape::boxSlump(int imgx, int imgz)
{
	int x=0, z=0, h=0, w=0;
    BOOL b_width=TRUE, b_height=TRUE;

	float y=0;
	while (b_height)
	{	
		while(b_width)
		{
			y=(float)(rand()%20);
			w = rand() % 10;
			h = rand() % 10;
			w=h=1;
			float s=(float)imgx/5;
			float xf, zf;
			xf=(float)(x-(float)imgx/2)/s; zf=(float)(z-(float)imgz/2)/s;
			y+=float(sin(xf/30)*sin(zf/30)*20);
			//if (((xf*xf-1)*(xf*xf-1)+zf*zf)==0) y=0;
			
			if (w + x >= imgx) {w=imgx-x-1;b_width=FALSE;}
			if (h + z >= imgz) {h=imgz-z-1;b_height=FALSE;}
			fillBox(map, x, z, x+w, z+h, y);
			x+=w;
		}
		z+=h;
		x=0; b_width=TRUE;
		
	}
}

void Clandscape::smooth(int nmax, int imgx, int imgz, BOOL pixel)
{
	//float max=getMaxHeight();
	//float min=getMinHeight();
	int x=0, z=0, h=0, w=0;
    BOOL b_width=TRUE, b_height=TRUE;
	float **tMap;
	tMap = new float *[imgx];
	for (x=0;x<imgx;x++)
		tMap[x] = new float [imgz];

	for (int n=0;n<nmax;n++)
	{
		x=0,z=0;
		b_height=TRUE;
		while (b_height)
		{	
			w=nmax-n; 
			if (pixel) w=1;
			h=w;
			int sz=z-h;
			int lz=z+h;
			if (lz>=imgz) lz=imgz-1;
			if (sz<0) sz=0;
			if (h+z>=imgz) {h=imgz-z;b_height=FALSE;}
			b_width=TRUE;
			while(b_width)
			{
				int sx=x-w;
				int lx=x+w;
				if (lx>=imgx) lx=imgx-1;
				if (sx<0) sx=0;
				if (w+x>=imgx) {w=imgx-x;b_width=FALSE;}
				
				float d=9;
				float y = (map[x][z] + map[sx][z] + map[lx][z] + map[x][sz] + map[x][lz] + map[sx][sz] + map[lx][sz] + map[lx][lz] + map[sx][lz])/d;
				fillBox(tMap, x, z, x+w, z+h, y);
				x+=w;
			}
			z+=h;
			x=0;
		}
	for (x=0;x<imgx;x++)
		for (z=0;z<imgz;z++)
			map[x][z]=tMap[x][z];
	}
	for (x=0;x<imgx;x++)
		safeDeleteArray(tMap[x]);
	safeDeleteArray(tMap);
}

void Clandscape::imgFault(const Crect *rect, int n, float shift)
{
	Crect r;
	if (rect==0)
	{
		r.left=0;r.top=0;r.right=landx;r.bottom=landz;
	}
	else
	{
		r=*rect;
		r.normalize();
	}
	for (int i=0;i<n;i++)
	{
		float a=float(rand()%179-89);
		a*=pi/180;
		float k=(float)tan(a);
		int mx=rand()%(r.right-r.left)+r.left;
		int mz=rand()%(r.bottom-r.top)+r.top;
				
		float pm=float(rand()%2);
		if (pm==0) pm=-1;
		for (int x=r.left;x<r.right;x++)
			for(int z=r.top;z<r.bottom;z++)
			{
				if (z*pm<(k*(x-mx)+mz)*pm)
					//map[x][z]+=5/(map[x][z]+1);
					map[x][z]+=shift;
				else
					map[x][z]-=shift;
			}
	}
}

void Clandscape::imgFunc2d(int imgx, int imgz)
{
	for (int x=0;x<imgx;x++)
		for (int z=0;z<imgz;z++)
				map[x][z]=float(pow((x-imgx/2)/30.0f, 1) * pow((z-imgz/2)/30.0f, 1));
}

void Clandscape::sealBorders()
{
	int i=0,j=0;
	for (i=0; i<landx;i++)
		map[i][j]=0;
	j=landz-1;
	for (i=0; i<landx;i++)
		map[i][j]=0;
	
	i=0;
	for (j=0; j<landz;j++)
		map[i][j]=0;
	i=landx-1;
	for (j=0; j<landz;j++)
		map[i][j]=0;
}

//-----------------------
//friends
void createMaps(Clandscape &land, Cwater &water, int x, int z, int reso, const Clight &light, float wlevel, Cpak *pak, const string &groundGenFile, vscreen *output)
{
	output->setRect();
	output->createTexture(sw, sh, sf, 1, D3DPOOL_MANAGED);
	output->cls();
	
	Cbutton label(Cpoint(0,0), Cpoint(sw, sh), output);
	label.setVisible(true);
		
	UINT headerSize=sizeof(x)+sizeof(z);
	UINT pitch = z*4;
		
	FILE *landFile;
	if ((fopen_s(&landFile, "landscape", "rb")))
	{
		land.createMap(x, z);
		water.createMap(x, z);
		if (!fopen_s(&landFile, "heightmap.bmp", "rb"))
		{
			fclose(landFile);
			land.copyTextureToMap(0, "heightmap.bmp", 0.4f);
		}						
		else
		{
			label.setString("Generating heightmap...");
			pd3dDevice->BeginScene();
			output->draw();
			pd3dDevice->EndScene();
			pd3dDevice->Present(0,0,0,0);
			
			//land.imgFault(0, int(x*1.5f), 1.5f);
			land.imgFault(0, int(x*1.5f), .85f);
			//land.imgFunc2d(x, z);
			
			land.smooth(3, x, z, false);
			land.smooth(1, x, z, true);
		}
		land.findMinMaxHeight();
		land.setWaterDepth(wlevel);
		//land.snapVertsToWlevel();
		land.calcMapNormals();
		
		label.setString("Generating textures...");
		pd3dDevice->BeginScene();
		output->draw();
		pd3dDevice->EndScene();
		pd3dDevice->Present(0,0,0,0);
		
		water.genTexture(reso, light);
		CTexture *wtex=water.getTex(1);
		
		CgroundGen groundGen(1, 0);
		groundGen.readFile(pak, groundGenFile);
		d_file<<"before genground"<<endl;
		land.genGround(reso, light, wtex, groundGen);
		d_file<<"aftergenground";		
		UINT size = x * pitch + headerSize;
		BYTE *terrain = new BYTE[size];
		memcpy(terrain, &x, sizeof(x));
		memcpy(terrain + sizeof(x), &z, sizeof(z));

        for (int i=0;i<x;i++)
			memcpy(terrain + headerSize + i * pitch, land.map[i], pitch);

		Cpak pak;
		pak.init("landscape");
		pak.addEntry(terrain, size, "terrain");
			
        UINT numTex = groundGen.texGen.numTex;
		pak.addEntry(&numTex, 4, "numTex");
		LPD3DXBUFFER *map = new LPD3DXBUFFER[numTex];
		for (unsigned t=0; t<numTex; t++)
		{
			int t2=t;
			//if (t2==2) t2=7;
			//t2 = groundGen.texGen.tex[t].index;
			string filename="map"+numberToString(t)+".dds";
			land.texture[t2].saveToFile(&map[t], D3DXIFF_DDS);
			pak.addEntry(map[t]->GetBufferPointer(), map[t]->GetBufferSize(), filename);
		}
		LPD3DXBUFFER wmap;
		water.texture[1].saveToFile(&wmap, D3DXIFF_DDS);
		pak.addEntry(wmap->GetBufferPointer(), wmap->GetBufferSize(), "wmap.dds");
		
		pak.writePakFile(9);

		for (unsigned i=0;i<numTex;i++)
			safeRelease(map[i]);
		safeRelease(wmap);
		safeDeleteArray(map);
	}
	else //terrain existerar
	{
		Cpak pak;
		fclose(landFile);
		pak.load("landscape");
        BYTE *terrain;
		UINT size;
		if (!pak.extractEntry("terrain", (void**)&terrain, &size))
			throw "Couldn't read terrain data";

		memcpy(&x, terrain, sizeof(x));
		memcpy(&z, terrain + sizeof(x), sizeof(z));

		land.createMap(x, z);
		water.createMap(x, z);
        for (int i=0;i<x;i++)
			memcpy(land.map[i], terrain + headerSize + i * pitch, pitch);
		safeDeleteArray(terrain);
		
		UINT *numTex=0;
		pak.extractEntry("numTex", (void**)&numTex, &size);
		for (unsigned t=0;t<*numTex;t++)
		{
			int t2=t;
			//if (t2==2) t2=7;
			string filename="map"+numberToString(t)+".dds";
			land.texture[t2].createTexture(&pak, filename);
			
		}
		safeDelete(numTex);
		water.texture[1].createTexture(&pak, "wmap.dds");

		land.calcMapNormals();
		land.findMinMaxHeight();
		pak.close();
	}
	
	label.setString("Initializing scene...");
	pd3dDevice->BeginScene();
	output->draw();
	pd3dDevice->EndScene();
	pd3dDevice->Present(0,0,0,0);
}

////////////////////////////////////////////
//Cwater////////////////////////////////////

void Cwater::createMap(int x, int z)
{
	Clandscape::createMap(x, z, 1);
	deleteMap();
	landx=x;
	landz=z;
	isShaded = new bool*[landx];
	for (int i=0;i<landx;i++)
		isShaded[i]=new bool[landz];
}

void Cwater::wave(float offset, float amp)
{
	if (!visible)
		return;
	float a=(float)fabs(cam.pos.y/waveCalcLimit);
    a=min (1, a);
	a=1-a;
	if (!a)
		return;
	amp*=a;

	Cpoint camGridPos((int)cam.pos.x/cellx, (int)cam.pos.z/celly);
	Crect r(camGridPos.x-1, camGridPos.y-1, camGridPos.x+2, camGridPos.y+2);
	r.clip(Crect(0, 0, gridx, gridy));
	waterVertex *pVertices;
	for (int gy=r.top;gy<r.bottom;gy++)
	{
		for (int gx=r.left;gx<r.right;gx++)
		{
			for (UINT s=0;s<num_ss;s++)
			{
				lpDIRECT3DVERTEXBUFFER pvb=subset[s].mipLevel[0][gx][gy].pvb;
				if (!pvb)
					continue;
								
				int num_vert=subset[s].mipLevel[0][gx][gy].num_vert;
				if (FAILED(hr = pvb->Lock(0, fvfSize*num_vert, (void**)&pVertices, 0)))
				{
					if (hr == D3DERR_WASSTILLDRAWING)
						continue;
					else
						throw "Couldn't lock vertex buffer.";
				}
				
				for (int i=0; i<num_vert; i++)
				{
					int posx = (int)pVertices[i].pos.x;
					int posz = (int)pVertices[i].pos.z;

					
					//platta ut vågorna i kanterna
					if (gx==r.left && posx <= gx * cellx ||  gx==r.right-1 && posx >= (gx+1) * cellx || gy==r.top && posz <= gy * celly || gy==r.bottom-1 && posz >= (gy+1) * celly) 
					{
						pVertices[i].pos.y=0;
						map[posx][posz]=0;
						continue;
					}
					float y=(float)sin(-offset+posx*5)*0.1f;
					int x=posx-landx/2;
					int z=posz-landz/2;
					y += (float)sin(-offset+sqrt(float(x*x + z*z))*9)*0.1f;
					y *= amp;
					map[posx][posz] = pVertices[i].pos.y = y;
					
					/*posx++;
					if (posx>(gx+1)*cellx)
					{
						posx = gx * cellx;
						posz++;
						if (posz>(gy+1)*celly)
						{
							posz = gy * celly;
							pvb->Unlock();
							throw "sdtrgi";
						}

					}*/
				}
				calcVertexNormals(pVertices, num_vert);
				pvb->Unlock();
			}
		}
	}
}


float Cwater::isUnderWater()
{
	fVector opos=cam.pos;
	opos-=pos;
	if (opos.x<0 || opos.x>=landx || opos.z<0 || opos.z>=landz)
		return 0;
	
	float depth=opos.y;
	if (depth>=0)
		cam.uw=false;
	else
		cam.uw=true;
	return depth;
}

void Cwater::draw(int sub, int _mip, const Crect *rect)
{
	if (!visible)
		return;
	if (effect)
	{
		if (cam.uw)
			effect->SetTechnique("cuw_water");
		else
			effect->SetTechnique("caw_water");
	}
	static UINT tclock;
	tclock++;
	//texture[2]=texture[((timeGetTime()/64) % 32)+35];
	//tclock=0;
	if (effect && fxHandle_nmapAnim)
		effect->SetTexture(fxHandle_nmapAnim, texture[((tclock/2) % 64)+71].getTexture());
	//texture[2]=texture[39];

	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	if (!effect)
	{
		texture[2]=texture[((tclock/2) % 64)+68];
		if (b_envMap)
		{
			hr=pd3dDevice->SetTexture(4, envMap.getMap());
			pd3dDevice->SetSamplerState(4, D3DSAMP_MIPMAPLODBIAS, 0); 
		}
		
		pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
		pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
			
		setDrawfx(dfx_aBlend);
	}
		
	
	baseDraw(sub, _mip, rect, true);

	if (!effect)
	{
		setDrawfx(0);
		pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	}
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
}

void Cwater::calcVertexNormals(waterVertex *vb, int size, const fVector *lightDir)
{
	for (int i=0; i<size; i++)
	{
		vb[i].normal = calcNormal((int)vb[i].pos.x, (int)vb[i].pos.z, false);
		/*if (normal.y > 0.99999f )
		{
			vb[i].tangent = fVector(1,0,0);
			vb[i].normal = normal;
			vb[i].binormal = fVector(0,0,1);
			continue;
		}
		fVector xzproj = normal;
		xzproj.y=0;
		fVector rotAxis = fVector(xzproj.z, 0, -xzproj.x);  //normal x xzproj
		float angle = (float)acos(normal.dot(fVector(0,1,0)));
		D3DXMATRIX tspace;
		D3DXMatrixRotationAxis(&tspace, &rotAxis, angle);

		//vb[i].normal = normal;
		vb[i].tangent = fVector(tspace._11, tspace._12, tspace._13);
		vb[i].normal = fVector(tspace._21, tspace._22, tspace._23);
		vb[i].binormal = fVector(tspace._31, tspace._32, tspace._33);*/
	}
}

BOOL Cwater::createMeshFromMap(float wrapTex, Clandscape *land2, int clipSide, float clipMarg, const fVector &lightDir)
{
	if (!subset)
		return FALSE;
	//Fyller i subsetsen
	UINT vbsize=(cellx+2)*(celly+2);
	waterVertex *vb=new waterVertex[vbsize];
	UINT ibsize=cellx*celly*6;
	USHORT *ib=new USHORT[ibsize];
	bool **writeMap=0;

	for (UINT s=0;s<num_ss;s++)
	{
		//Fyller i en cell i taget
		for (int gy=0;gy<gridy;gy++)
		{
			for (int gx=0;gx<gridx;gx++)
			{
				//Cellens miplevels
				for (UINT m=0;m<subset[s].num_ml;m++)
				{	
					int vstep=(int)pow(2.0f, (int)m);
					//Miplevelns vertices
					int vbi=fillVB(vb, gx*cellx, gy*celly, vstep, s, wrapTex, writeMap, land2, clipSide, clipMarg);
					int ibi=0;
					if (vbi>=3)
						ibi=fillIB(ib, vstep, vb, vbi, false);
					//Skapa vertex- och indexbuffertar
					if (ibi)
					{
						subset[s].mipLevel[m][gx][gy].num_vert=vbi;
						if (mip==0)
							subset[s].mipLevel[0][gx][gy].tag=terrain->subset[s].mipLevel[0][gx][gy].tag;
						calcVertexNormals(vb, vbi, &lightDir);
						subset[s].mipLevel[m][gx][gy].pvb=createVB(vb, vbi*fvfSize, fvf, D3DPOOL_MANAGED);
						subset[s].primType=D3DPT_TRIANGLELIST; 
						subset[s].mipLevel[m][gx][gy].num_prim=ibi/3;
						subset[s].mipLevel[m][gx][gy].pib=createIB(ib, ibi*sizeof(USHORT), D3DFMT_INDEX16);
					}
					else
					{
						subset[s].mipLevel[m][gx][gy].pvb=0;
						subset[s].mipLevel[m][gx][gy].pib=0;
					}
				}
			}
		}
	}
	for (int i=0;i<landx;i++)
		safeDeleteArray(writeMap[i]);
	safeDeleteArray(writeMap);
	
	safeDeleteArray(vb);
	safeDeleteArray(ib);
	return TRUE;
}

void Cwater::genTexture(int reso, const Clight &light)
{
	D3DCOLOR ambcol;
	pd3dDevice->GetRenderState(D3DRS_AMBIENT, (DWORD*)&ambcol);
	D3DXCOLOR ambient=ambcol;
	ambient*=2;
	
	texture[1].createTexture(landx, landz, D3DFMT_A4R4G4B4, 0, D3DPOOL_MANAGED);
	texture[1].lock(0,0);
	WORD *lmapBits=(WORD*)texture[1].getPixelPointer();
	UINT lmapPitch=texture[1].getPitch();
	
	float fstep=1.0f/reso;
	for (int z=0;z<landz;z++)
	{
		//COLOR *pBits2=pBits;
		int resoz=z==landz-1 ? 1 : reso;
		for (int x=0;x<landx;x++)
		{
			float fy=0;
			int resox=x==landx-1 ? 1 : reso;
			for (int v=0;v<resoz;v++)
			{
				float fx=0;
				for (int u=0;u<resox;u++)
				{
					//är vattnet under land?
					float a=1; //(float)sqrt(fabs(terrain->map[x][z]+terrain->pos.y-pos.y)/2.0f);
					if (terrain->map[x][z]+terrain->pos.y-pos.y>0.4f)
						a=0;
					if (a>1)
						a=1;
					
					//är vattnet skuggat?
					int lum=0xfff;
					if (!calcPixelLighting((float)x, (float)z, light.Direction, *terrain , false))
						lum=0;
					
					WORD c=(int(a*15)<<12) | lum;
					*(lmapBits+reso*(x+z*lmapPitch)+u+v*lmapPitch)=c;
					//*(lmapBits+reso*(x+z*lmapPitch)+u+v*lmapPitch)=0xffff;
					fx+=fstep;
				}
				fy+=fstep;
			}
		}
	}
	texture[1].unlock();
	//texture[1].smooth(1, true);
	texture[1].fillMipLevels();
	texture[1].setMipBias(2);
}

BOOL Cwater::createEffect(Cpak *pak, const string &file, DWORD flags, const D3DXMACRO *defines, LPD3DXINCLUDE include, LPD3DXEFFECTPOOL pool)
{
	if (!CbaseMesh::createEffect(pak, file, flags, defines, pool))
		return FALSE;
	initEffect();
	return TRUE;
}

void Cwater::setEffect(LPD3DXEFFECT fx)
{
	CbaseMesh::setEffect(fx);
	initEffect();
}

void Cwater::initEffect()
{
	if (effect)
	{
		fxHandle_nmapAnim = effect->GetParameterBySemantic(0, "nmapAnim");
		CbaseMesh::initEffect();
	}
}

void Cwater::deleteMap()
{
	if (isShaded)
	{
		for (int i=0;i<landx;i++)
			safeDeleteArray(isShaded[i]);
		safeDeleteArray(isShaded);
	}
}

void Cwater::release()
{
	deleteMap();
	effect=0;
	Clandscape::release();
};



/////////////////////////////////
//Csky

void Csky::draw()
{
	setDrawfx(dfx_aBlend);
	baseObj bobj;
	//bobj.setLocalFrame(object.localFrame);
	//cam.viewTransform(fVector(0,0,0), fVector(0,0,1), bobj);
	static D3DXMATRIX vmat, vmatBackup;
	pd3dDevice->GetTransform(D3DTS_VIEW, &vmatBackup);
	D3DXQUATERNION rotQuat;
	D3DXQuaternionRotationMatrix(&rotQuat, &vmatBackup); 
	D3DXMatrixRotationQuaternion(&vmat, &rotQuat);
	//vmat._14=vmat._24=vmat._34=0;
	//vmat._41=vmat._42=vmat._43=0;
	//vmat._44=1;
	pd3dDevice->SetTransform(D3DTS_VIEW, &vmat);

	pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pd3dDevice->SetRenderState(D3DRS_WRAP0, D3DWRAPCOORD_0);
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	
	object3d::draw(0);
		
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	pd3dDevice->SetRenderState(D3DRS_WRAP0, 0);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	
	pd3dDevice->SetTransform(D3DTS_VIEW, &vmatBackup);
	//cam.viewTransform(fVector(0,0,0), fVector(0,0,1), object);
	setDrawfx(0);
}

void Csky::create(Cpak *meshPak, const string &meshFile, Cpak *texPak, const string &texFile, Cpak *shaderPak, const string &shaderFile)
{
	if (!createEffect(shaderPak, shaderFile, 0))
		throw "Couldn't create sky effect.";

	setPos(0,-0.2f,0);
	setAngle(0,0,0);
	loadXfile(meshPak, meshFile.c_str());
	//sky.enableTextures(false);
	xyz_tex1_vertex *vdata;
	if (FAILED(pd3dxMesh->LockVertexBuffer(0, (void**)&vdata)))
		throw "Failed to lock sky vertex buffer.";
	float minr=9;
	int nvert=pd3dxMesh->GetNumVertices();
	
	for (int i=0;i<nvert;i++)
	{
		static float scale=1;
		float x=vdata[i].pos.x/scale;;
		float y=vdata[i].pos.z/scale;
		float z=vdata[i].pos.y/scale;
		vdata[i].pos.y=y;
		vdata[i].pos.z=z;
		
		float r=(float)sqrt(x*x+z*z);
	
		if (!r)
		{
			vdata[i].tu=0.5f;
			if (y>0)
				vdata[i].tv=0.02f;
			else
				vdata[i].tv=1.98f;
			continue;
		}
		
		if (r<minr) minr=r;
		 
		float tu=(float)acos(x/r);
		if (z<0)
			tu=pi2-tu;
		tu/=pi2;
		vdata[i].tu=tu;

		vdata[i].tv=1-y;

		if (r<0.07f)
		{
			vdata[i].tv=0.03f;
		}


	}
	if (FAILED(pd3dxMesh->UnlockVertexBuffer()))
		throw "Failed to unlock sky vertex buffer.";
	texture[0].createTexture(texPak, texFile, 1);
	if (effect)
		effect->SetTexture("tex", texture[0].getTexture());
}


/////////////////////////////////////////////////////////////
//Cchunk/////////////////////////////////////////////////////
/*
//static init
CquadChunk::vbNumMips = 6;
CquadChunk::firstVBNumMips = 6;

void CvbChunk::CvbChunk(Cquad &_quad)
{
	int mip = calcNumMips(abs(_quad.point[1].x - _quad.point[0].x));
	CvbChunk(_quad, true, false, mip);
}

//private---------------------------
void CvbChunk::CvbChunk(Cquad &_quad, int mip, bool _b_firstVB, bool _b_lastVB) : CquadChunk(_quad, mip);
{
	b_lastVB = _b_lastVB;
	b_firstVB = _b_firstVB;
	generateVB(0);
	fillMips(0);
}

void CvbChunk::generate(bool b_freeMem)
{
	calcDrawMip();
	int desiredGenMip = drawMip - genMipBuf;
	if (genMip < desiredGenMip && b_freeMem) 
		deleteMip(desiredGenMip - 1);
	else if(genMip > desiredGenMip)  //fill mips from desiredGenMip or 0
	{
		int ml = desiredGenMip >=0 ? desiredGenMip : 0;
		generateVB(ml);
		fillMips(ml);
	}
	if(desiredGenMip < 0)  //new vb nodes possibly needed, create quads, !(starting with the one that share this chunk with the vb)
	{
		vb[0]->Lock(NOOVERWRITE);
		//quad.point[0] = (*pVert).pos;
		//quad.point[1] = pVert[vbSide].pos;
		int bottomOffset = vbSide * vbSide;
		//quad.point[2] = pVert[bottomOffset].pos;
		//quad.point[3] = pVert[bottomOffset + vbSide].pos;

		generateChunks(b_firstVB ? firstVBNumMips: vbNumMips , desiredGenMip, pVert[offset], vbSide, bottomOffset, b_freeMem);
	}
}

void CvbChunk::calcDrawMip()
{
}

void CvbChunk::generateVB(int mip)
{
	genMip = mip;
	ërrorDist = -1;
	int startMip = mipLevel - b_firstVB ? firstVBNumMips : vbNumMips;
	int actualGenMip = startMip + mip;
	int step = 1 >> actualGenMip;
	
	int numLines = rand() % 
	for (int l = 0; l < numLines; l++)
	vb[mip]->Lock();
	//generate lines and heights
	vb[mip]->Unlock();
}

void CvbChunk::fillMips(int mip)
{
}



void CquadChunk::generateChunks(int vbCountdown, int desiredGenMip, landVertex *pVert, int vbRightOffset, int vbBottomOffset, bool b_freeMem)
{
	int offsetStepx = rightOffset / 2;
	int offsetStepy = (bottomOffset << 2) + offsetStepx;
	vbCountDown--;
	for (int i=0; i<4; i++)
	{
		int freeMem = b_freeMem;
		if (!chunk[i])
		{
			int offset = 0;
			if (i == 1)
				offset = offsetStepx; '
			else if(i == 2)
				offset = offsetStepy;
			else if (i == 3)
				offset = offsetStepx + offsetStepy;

			Cquad q;
			q.point[0] = (*pVert).pos;
			q.point[1] = pVert[offsetStepx].pos;
			q.point[2] = pVert[offsetStepy].pos;
			q.point[3] = pVert[offsetStepx + offsetStepy].pos;

			//Calculate mip distance for current quad
			
			int mip = mipLevel - 1;
			if (vbCountdown > 0)
				chunk[i] = new CquadChunk(q, mip);
			else
			{
				bool lastvb = mip < vbFreq ? true : false;
				chunk[i] = new CvbChunk(q, mipLevel, false, lastvb);
			}
			//freeMem = false;  // New branch created, so there's nothing to free(except if a new vb chunk is created)
	
		}
		if (vbCountdown > 0)   //quadChunk
			chunk[i]->generate(vbCountdown, desiredGenMip, pVert, offsetStepx, offsetStepy, freeMem);
		else					//vbChunk
			chunk[i]->generate(freeMem);

	}
}

void CquadChunk::CquadChunk(const Cquad &_quad, int mip)
{
	quad = _quad;
	mipLevel = mip;
	errorDist = calcErrorDist();
	
	for (int i=0; i<4; i++)
		chunk[i] = 0;
}

void CquadChunk::generate(int vbCountdown, int desiredGenMip, landVertex *pVert, int vbRightOffset, int vbBottomOffset, bool b_freeMem)
{
	camDist = getCamDist();
	if (desiredGenMip++ < 0 || camDist < errorDist)
		generateChunks(vbCountDown, desiredGenMip, pVert[offset], vbRightOffset, vbBottomOffset, b_freeMem);
	else if (b_freeMem)
	{
		for (int i=0; i<4; i++)
			safeDelete(chunk[i]);
	}
				
}

void getCamDist()
{
	//if (cam.pos.x < midPoint.x)
	//{
	//	if (cam.pos.z < midPoint.y)

}
//----------------------------------------

void Cchunk::process(bool sort, bool bFrustumCull, bool bReflFrustumCull)
{
	if (bFrustumCull)
		visible = frustumCull(AABB);
	if (bReflFrustumCull)
		reflVisible = frustumCull(reflAABB);

	if (!visible && !reflVisible)
		return;
	
	if (chunk)
	{
		if (visible == 2)
		{
			mipError = checkMipError(0);
			if (mipError < mipErrorThreshold[0])
			{
				if (aw)
					addQuadToVB(pdvb[0]);
				if (uw)
					addQuadToVB(pdvb[1]);
				visible = 0;
				return;
			}
		}
		if (sort)
			sort(chunk);
		for (int i=0; i<4; i++)
		{
			chunk[i]->process(sort, visible, reflVisible);
		}
	}
	else
	{
		mipLevel = numMipLevels;
		while ((checkMipError(--mipLevel) > mipErrorThreshold)
			;
		if (mipLevel > dvbThreshold)
		{
			if (aw)
				addChunkToVB(pdvb[0], subset[0][mipLevel]);
			if (uw)
				addChunkToVB(pdvb[1], subset[1][mipLevel]);
		}
	}
}

void Cchunk::draw(bool bRefl)
{
	if (!bRefl)
	{
		if (!aw && subsetIndex == 0 || !uw && subsetIndex == 1 || !visible)
			return;
	}
	else if (!reflVisible)
		return;
		
	if (chunk)
	{
		for (int i=0; i<4; i++)
			chunk[i]->draw(bRefl);
	}
	else
	{
		if (mipLevel < dvbThreshold)
		{
			pd3dDevice->SetStreamSource(
			pd3dDevice->SetIndices(
			pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, subset[subsetIndex][mipLevel].numVert, 0, subset[subsetIndex][mipLevel].numPrim);
		}
	}
}

int frustumCull(AABB[])
{
	int visiblePoints = 0;
	for (int i=0;i<8;i++)
	{
		point = AABB[i] * viewProjMat;
		if (point.isInFrustum())
			visiblePoints++;
	}
	if (!visiblePoints)
		return 0;
	else if (visiblePoints == 8)
		return 2;
	else
		return 1;
}



*/