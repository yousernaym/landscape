// PrintUsing2.h: interface for the CPrintUsing2 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PRINTUSING2_H__F9D90B86_6DE2_4830_9CA5_BF76AD75D232__INCLUDED_)
#define AFX_PRINTUSING2_H__F9D90B86_6DE2_4830_9CA5_BF76AD75D232__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

struct puFormat
{
	int ld;
	int rd;
	bool dp;
};

class CPrintUsing2 //: public CPrint
{
private:
	CPrint *print;
	puFormat format;
public:
	CPrintUsing2(CPrint *p, puFormat &f);
	virtual ~CPrintUsing2();
	void drawText(const string &s);
	CPrintUsing2 &operator<<(const string &right);
	CPrintUsing2 &operator<<(char right);
	CPrintUsing2 &operator<<(int right);
	CPrintUsing2 &operator<<(float right);
	CPrintUsing2 &operator<<(double right);
};

#endif // !defined(AFX_PRINTUSING2_H__F9D90B86_6DE2_4830_9CA5_BF76AD75D232__INCLUDED_)
