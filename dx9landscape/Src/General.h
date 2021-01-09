#ifndef __general__
#define __general__

#include <windows.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <mmsystem.h>
#include <tchar.h>
#include <string>
#include <math.h>

using namespace std;
//typedef std::string string;
//typedef std::ofstream ofstream;

#define _CRT_SECURE_NO_DEPRECATE

#define safeDelete(p)       { if(p) { delete (p);     (p)=0; } }
#define safeDeleteArray(p) { if(p) { delete[] (p);   (p)=0; } }
#define safeRelease(p)      { if(p) { (p)->Release(); (p)=0; } }

const float pi=3.141f;
const float pi2=pi*2;
const float pihalf=pi/2;

enum {cmp_never=1, cmp_less, cmp_equal, cmp_lessequal, cmp_greater, cmp_notequal, cmp_greaterequal, cmp_always};
typedef int cmpFunc;

extern UINT t,t2,t3;
extern ULONG frames;
extern ofstream d_file;
extern string g_errMsg;

//////////////////////////////////////////
//global functions

void fillBox(float **box, int w1, int h1, int w2, int h2, float v);
void maxmin(float v1, float v2, float v3, float &l, float &m);
bool isInsideRect(int x1, int y1, int x2, int y2, int x, int y);
void swap(float &a, float &b);
void swap(int &a, int &b);
float rnd();
float rndScale(float chance, int sgn = 0);
int sgn(int v);
int sgn(float v);
void calcDistances(float *distOut, int dim, int npoints, const float *points, const float *ref);
void interpolate(float &start, float dest, float step);
int getMaxValueIndex(const float *a, int size, int arrStep = sizeof(float));
int getMinValueIndex(const float *a, int size, int arrStep = sizeof(float));
double getMaxValue(double a, double b);
double getMinValue(double a, double b);
int getRndIndex(const float *a, int size, int arrStep = sizeof(float));
double stringToNumber(const string &s);
string numberToString(int value, UINT digits=0, char c=' ');
template <class T> T &clip(T &number, T low, T high);
DWORD writeBits(DWORD dest, DWORD src, DWORD mask);
int getLs1Pos(DWORD var);
DWORD modulateMask(float f, DWORD mask);
float clampBits(DWORD bits, DWORD mask);
DWORD multiplyBits(float f, DWORD bits, DWORD mask);
string lcase(const string &s);
string ucase(const string &s);

//Path functions
void addBackslash(string &s);
void removeBackslash(string &s);

//Stream functions
void istreamSkipComments(istream &stream, char comment);
string istreamPeekWord(istream &stream);
bool istreamSearchWords(istream &stream, const string &words, char comment, bool readPast = true, bool notFoundMsg = true);
char istreamPeek(istream &stream, int steps);


///////////////////////////////////////////////////////////////
//classes
class Crect : public RECT
{
public:
	Crect();
	Crect(const Crect &r);
	Crect(int x1, int y1, int x2, int y2);
	Crect(const POINT &p, int w, int h);
	void normalize();
	void clip(const Crect &r);
	UINT width();
	UINT height();
};

class Cpoint : public POINT
{ 
public:
	Cpoint();
	Cpoint(const POINT &p);
	Cpoint(int initx, int inity);
	void clip(const Crect &r);
};

class Cfps
{
private:
	LARGE_INTEGER time;
	LARGE_INTEGER cps;
	double fps;
	double refFps;
	bool b_hperformance;
public:
	float s;
	bool active;
	Cfps();
	~Cfps();
	void update();
	void setRefFps(float ref);
	float getRefFps();
	float getFps();
};
extern Cfps fps;


#include "pak.h"
#endif