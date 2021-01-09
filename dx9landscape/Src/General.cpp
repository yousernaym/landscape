#include "general.h"

//using namespace std;

UINT t, t2, t3;
ULONG frames=0;
ofstream d_file;
string g_errMsg;

void fillBox(float **box, int w1, int h1, int w2, int h2, float v)
{
	for (int x=w1;x<w2;x++)
		for (int y=h1;y<h2;y++)
			box[x][y]=v;
}

void maxmin(float v1, float v2, float v3, float &l, float &m)
{
	if (v1<=v2)
	{
		l=v1;
		if (v3<v1)
			l=v3;
	}
	else
	{
		l=v2;
		if (v3<v2)
			l=v3;
	}
	if (v1>=v2)
	{
		m=v1;
		if (v3>v1)
			m=v3;
	}
	else
	{
		m=v2;
		if (v3>v2)
			m=v3;
	}
}


bool isInsideRect(int x1, int y1, int x2, int y2, int x, int y)
{
	return x>=x1 && x<x1+x2 && y>=y1 && y<y1+y2;
}

void swap(float &a, float &b)
{
	float t=a;
	a=b;
	b=t;
}

void swap(int &a, int &b)
{
	int t=a;
	a=b;
	b=t;
}

float rnd()
{
	return rand()%10000/10000.0f;
}

float rndScale(float chance, int sgn)
{
	//maxChanceTerm is assumed to be positive
	// sgn == 0 means that value returned can be both <1 and >1
	if (sgn < 0)
		return  1 - chance * rnd();
	else if (sgn == 0)
		return 1 - chance + chance * 2 * rnd();
	else
		return 1 + chance * rnd();
}

int sgn(int v)
{
	if (v>0)
		return 1;
	else if (v==0)
		return 0;
	else
		return -1;
}

int sgn(float v)
{
	if (v>0)
		return 1;
	else if (v==0)
		return 0;
	else
		return -1;
}

void calcDistances(float *distOut, int dim, int npoints, const float *points, const float *ref)
{
	float dist=0;
	for (int i=0;i<npoints;i++)
	{
		for (int e=0;e<dim;e++)
		{
			float edist=(float)fabs(points[e+dim*i]-ref[e]);
			dist+=edist*edist;
		}
		distOut[i]=(float)sqrt(dist);
	}
}
		
void interpolate(float &start, float dest, float step)
{
	if (step<0)
		step=-step;
	if (start<dest)
	{
		start+=step;
		if (start>dest)
			start=dest;
	}
	else if (start>dest)
	{
		start-=step;
		if (start<dest)
			start=dest;
	}
}

int getMaxValueIndex(const float *a, int size, int arrStep)
{
	if (size<2)
		return 0;
	float max;
	int mIndex=0;
	
	float a1 = *((float*)((BYTE*)a + arrStep));
	if (a[0]>a1)
		max=a[0];
	else
	{
		max=a1;
		mIndex = 1;
	}
	for (int i=2;i<size;i++)
	{
		float ai = *((float*)((BYTE*)a + i * arrStep));
		if (max<ai)
		{
			max=ai;
			mIndex = i;
		}
	}
	return mIndex;
}

int getMinValueIndex(const float *a, int size, int arrStep)
{
	if (size<2)
		return 0;
	float min;
	int mIndex=0;

	float a1 = *((float*)((BYTE*)a + arrStep));
	if (a[0]<a1)
		min=a[0];
	else
	{
		min=a1;
		mIndex=0;
	}
	for (int i=2;i<size;i++)
	{
		float ai = *((float*)((BYTE*)a + i * arrStep));
		if (min<ai)
		{
			min=ai;
			mIndex=i;
		}
	}
	return mIndex;
}

double getMaxValue(double a, double b)
{
	if (a<b)
		return b;
	else 
		return a;
}

double getMinValue(double a, double b)
{
	if (a>b)
		return b;
	else 
		return a;
}

int getRndIndex(const float *a, int size, int arrStep)
{
	if (size<2)
		return 0;
	float total = 0;
	for (int i=0;i<size;i++)
		total += *((float*)((BYTE*)a + i * arrStep));;
	
	float r = rnd() * total;
	float base=0;
	for (int i=0;i<size;i++)
	{
		float ai = *((float*)((BYTE*)a + i * arrStep));
		if (r >= base && r < ai + base)
			return i;
		base += ai;
	}
	return -1;  //unexpected error
}

string numberToString(int value, UINT digits, char c)
{
	static char cs[100];
	_itoa_s(value, cs, 10);
	string s=cs;
	if (s.length()<digits)
		s.insert(0, digits-s.length(), c);
	return s;
}

double stringToNumber(const string &s)
{
	return atof(s.c_str());
}

template <class T> T &clip(T &number, T low, T high)
{
	if (number>high)
		number=high;
	if (number<low)
		number=low;
	return number;
}

DWORD writeBits(DWORD dest, DWORD src, DWORD mask)
{
	//Replaces the bits (specified by mask) in dest
	//with the corresponding bits in src
	return (dest & ~mask) | (src & mask);
}

int getLs1Pos(DWORD bits)
{
	//Returns the position of the least significant bit with value 1.
	if (!bits)
		return -1;
	UINT i = 0;
	while (!(bits & 1))
	{
		bits >>= 1;
		i++;
	}
	return i;
}

DWORD modulateMask(float f, DWORD mask)
{
    //Multiplies f by mask size, convert to integer,
	//and shift the bits to the least significant mask bit.
	//f is assumed to be in the range [0,1].
	if (!mask)
		return 0;
	int ls1Pos = getLs1Pos(mask);
	DWORD maskSize = mask >> ls1Pos;
	return int(f * maskSize) << ls1Pos;
}

float clampBits(DWORD bits, DWORD mask)
{
	//Clamp bits (specified by mask) to [0,1]
	if (!mask)
		return 0;
	int ls1Pos = getLs1Pos(mask);
	bits = (bits & mask) >> ls1Pos;
	UINT maskSize = mask >> ls1Pos;
	return (float)bits/maskSize;
}

DWORD multiplyBits(float f, DWORD bits, DWORD mask)
{
	//Multiplies f by bits specified by mask, convert result to integer,
	//clip to [0, maskSize] and shift the bits to the least significant mask bit.
	if (!mask)
		return 0;
	int ls1Pos = getLs1Pos(mask);
	bits = DWORD(((bits & mask) >> ls1Pos) * f);
	int maskSize = mask >> ls1Pos;
	clip<DWORD>(bits, 0, maskSize);
	return bits << ls1Pos;
}


string lcase(const string &s)
{
	string s2=s;
	for (UINT i=0;i<s2.size();i++)
	{
		if (s2[i]>='A' && s2[i]<='Z')
		{
			s2[i]-='A';
			s2[i]+='a';
		}
	}
	return s2;
}

string ucase(const string &s)
{
	string s2=s;
	for (UINT i=0;i<s2.size();i++)
	{
		if (s2[i]>='a' && s2[i]<='z')
		{
			s2[i]-='a';
			s2[i]+='A';
		}
	}
	return s2;
}

//Path functions------------
void addBackslash(string &s)
{
	if (s.rfind("\\") != s.size() - 1)
		s += "\\";
}

void removeBackslash(string &s)
{
	int pos;
	if ((pos = s.rfind("\\")) == s.size() - 1)
		s.erase(pos, 1);
}

//Stream functions
void istreamSkipComments(istream &stream, char comment)
{
	stream >> ws;
	while (stream.peek() == comment)
	{
		while(stream.get() != '\n')
			;
		stream >> ws;
	}
}

string istreamPeekWord(istream &stream)
{
	static string tmpstr;
	int pos = (int)stream.tellg();
	stream >> tmpstr;
	stream.seekg(pos);
	return tmpstr;
}

bool istreamSearchWords(istream &stream, const string &words, char comment, bool readPast, bool notFoundMsg)
{
	int startPos = (int)stream.tellg();
	istringstream w(words);
	w >> ws;
	if (w.peek() == EOF)
		return false;

	string s, s2;
	bool found=false;
	while (1)
	{
		if (stream.peek() == EOF)
		{
			if (notFoundMsg)
			{
				string msg = string("Couldn't find the word(s) \"") + words + "\".\n Do you wish to continue?";
				if (IDNO == MessageBox(0, msg.c_str(), "istreamSearchWords", MB_YESNO))
					throw "Exiting application.";
			}
			stream.clear();
			stream.seekg(startPos);
			return false;
		}
		
		istreamSkipComments(stream, '\'');
		s = istreamPeekWord(stream);
		while (w.peek() != EOF)
		{
			w >> s2;
			if (s==s2)
			{
				if (readPast)
					stream >> s;
				return true;
			}
			w >> ws;
		}
		w.clear();
		w.seekg(0);
		stream >> s;
	}
}

char istreamPeek(istream &stream, int steps)
{
	int pos=(int)stream.tellg();
	stream.seekg(steps,ios::cur);
	char c=stream.peek();
	stream.seekg(pos);
	return c;
}


//classes///////////////////////
//Crect
Crect::Crect()
{
}

Crect::Crect(const Crect &r)
{
	 left=r.left; top=r.top; right=r.right; bottom=r.bottom;
}
Crect::Crect(int x1, int y1, int x2, int y2)
{
	left=x1; top=y1; right=x2; bottom=y2;}

Crect::Crect(const POINT &p, int w, int h)
{
	int wh=w/2, hh=h/2;
	left=p.x-wh;
	top=p.y-hh;
	right=p.x+wh;
	bottom=p.y+hh;
}

void Crect::normalize()
{
	if (top>bottom)
		swap(top, bottom);
	if (left>right)
		swap(left, right);
}

void Crect::clip(const Crect &r)
{
	if (left<r.left) left=r.left;
	if (top<r.top) top=r.top;
	if (right>r.right) right=r.right;
	if (bottom>r.bottom) bottom=r.bottom;
}

UINT Crect::width()
{
	return abs(right-left);
}

UINT Crect::height()
{
	return abs(bottom-top);
}
//-----------------------------
//Cpoint
Cpoint::Cpoint()
{
}
Cpoint::Cpoint(const POINT &p)
{
	x=p.x; y=p.y;
}
Cpoint::Cpoint(int initx, int inity)
{
	x=initx; y=inity;
}

void Cpoint::clip(const Crect &r)
{
	if (x<r.left) x=r.left;
	if (y<r.top) y=r.top;
	if (x>r.right) x=r.right;
	if (y>r.bottom) y=r.bottom;
}
//----------------------------------------
//Cfps
Cfps fps;

Cfps::Cfps()
{
	active=true;
	s=1;
	refFps=50;
	QueryPerformanceFrequency(&cps);
	if (!cps.QuadPart)
	{
		cps.QuadPart=1000;
		b_hperformance=false;
	}
	else
		b_hperformance=true;
	time.QuadPart=0;
}

Cfps::~Cfps()
{
}

void Cfps::update()
{
	__int64 time2=time.QuadPart;
	if (b_hperformance)
		QueryPerformanceCounter(&time);
	else
		time.QuadPart=timeGetTime();
	
	if (!time2 || time2>=time.QuadPart)
	{
		s=1;
		fps=-1;
		return;
	}	

	double dif=double(time.QuadPart-time2);   //elapsed counts
		
	fps = double(cps.QuadPart)/dif;
	
	if (!active)
	{
		s=1;
		return;
	}
	static int i;
	const int n=10;
	static float s2[n];
	if (++i==n)
		i=0;
	
	s2[i]=float(refFps/fps);
	s=s2[0];

	for (int i2=1;i2<n;i2++)
		s+=s2[i2];
	s/=n;
	//if (s>5)
	//	s=5;
}

void Cfps::setRefFps(float ref)
{
	refFps=(double)ref;
}

float Cfps::getRefFps()
{
	return (float)refFps;
}

float Cfps::getFps()
{
	return (float)fps;
}

//Template instantiations-------------------
template int &clip(int &, int, int);
template float &clip(float&, float, float);