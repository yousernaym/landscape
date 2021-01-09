// PrintUsing2.cpp: implementation of the CPrintUsing2 class.
//
//////////////////////////////////////////////////////////////////////

#include "dxstuff.h"
#include "surfaces.h"
#include "PrintUsing2.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPrintUsing2::CPrintUsing2(CPrint *p, puFormat &f)
{
	print=p;
	format=f;
}

CPrintUsing2::~CPrintUsing2()
{

}

void CPrintUsing2::drawText(const string &s)
{
	string s2=s;
	if (!format.dp)
	{
		int fill=format.ld-s.size();
		char c=' ';
		s2.insert((int)0, fill, c);
	}
	print->base->drawText2(s2);
}


CPrintUsing2 &CPrintUsing2::operator<<(const string &right)
{
	drawText(right);
	return *this;
}

CPrintUsing2 &CPrintUsing2::operator<<(char right)
{
	string s;
	s=right;
	drawText(s);
	return *this;
}

CPrintUsing2 &CPrintUsing2::operator<<(int right)
{
	char s[100];
	_itoa_s(right, s, 10);
	drawText(s);
	return *this;
}

CPrintUsing2 &CPrintUsing2::operator<<(float right)
{
	*this<<(double)right;
	return *this;
}

CPrintUsing2 &CPrintUsing2::operator<<(double right)
{
	char s[100];
	sprintf_s(s, "%f", right);
	drawText(s);
	return *this;
}


	
