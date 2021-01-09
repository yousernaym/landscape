// Font.cpp: implementation of the Cfont class.
//
//////////////////////////////////////////////////////////////////////

#include "dxstuff.h"
#include "surfaces.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Cfont::Cfont()
{
	b_texture=false;	
}

Cfont::~Cfont()
{
}

//////////////////////////////////////////////////

BOOL Cfont::create(UINT fontHeight, const string &type, int weight, bool bItalic, Cpak *pak, const string &image)
{
	// Large fonts need larger textures
    if( fontHeight >= 40 )
        width = height = 1024;
    else if( fontHeight >= 20 )
        width = height = 512;
    else
        width  = height = 256;

	this->createTexture(width, height, sf, 1, D3DPOOL_MANAGED);

	// Prepare to create a bitmap
    DWORD*      pBitmapBits;
    BITMAPINFO bmi;
    ZeroMemory( &bmi.bmiHeader,  sizeof(BITMAPINFOHEADER) );
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       =  (int)width;
    bmi.bmiHeader.biHeight      = -(int)height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biBitCount    = 32;

    // Create a DC and a bitmap for the font
    HDC     hDC       = CreateCompatibleDC( NULL );
    HBITMAP hbmBitmap = CreateDIBSection( hDC, &bmi, DIB_RGB_COLORS,
                                          (VOID**)&pBitmapBits, NULL, 0 );
    SetMapMode( hDC, MM_TEXT );

    // Create a font.  By specifying ANTIALIASED_QUALITY, we might get an
    // antialiased font, but this is not guaranteed.
    INT nHeight    = -MulDiv(fontHeight, 
        (INT)(GetDeviceCaps(hDC, LOGPIXELSY)), 72 );
    	
    maxFontHeight=-nHeight;
	HFONT hFont    = CreateFont( nHeight, 0, 0, 0, weight, bItalic,
        FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        VARIABLE_PITCH, type.c_str());
    if( NULL==hFont )
        return FALSE;

	SelectObject( hDC, hbmBitmap );
    SelectObject( hDC, hFont );

    // Set text properties
    SetTextColor( hDC, RGB(255,255,255) );
    SetBkColor(   hDC, 0x00000000 );
    SetTextAlign( hDC, TA_TOP );

	 // Loop through all printable character and output them to the bitmap..
    // Meanwhile, keep track of the corresponding tex coords for each character.
    DWORD x = 0;
    DWORD y = 0;
    TCHAR str[2] = _T("x");
    	
	UINT maxFontWidth=0;
    
	SIZE size;
	
	for( TCHAR c=32; c<127; c++ )
    {
        str[0] = c;
        GetTextExtentPoint32( hDC, str, 1, &size );

        int xtraWidth=0;
		if (c=='j')
		{
			xtraWidth = int(size.cx * 0.25f);
			size.cx += xtraWidth;
		}
        
		if ((UINT)size.cx > maxFontWidth)
			maxFontWidth = size.cx;
		if (size.cy>maxFontHeight)
			maxFontHeight=size.cy;

		if((DWORD)(x+size.cx+1) > (DWORD)width )
        {
            x  = 0;
            y += size.cy+1;
        }

        ExtTextOut( hDC, x+xtraWidth, y+0, ETO_OPAQUE, NULL, str, 1, NULL );

        fTexCoords[c-32].left = x;
        fTexCoords[c-32].top = y;
        fTexCoords[c-32].right = x + size.cx;
        fTexCoords[c-32].bottom = y + size.cy;
		charSize[c-32].cx=size.cx;
		charSize[c-32].cy=size.cy;
        x += size.cx+1;
    }

    CTexture ftexture;
	if (image!="")
	{
		b_texture=true;
		ftexture.createSurface(maxFontWidth, maxFontHeight, sf);
		ftexture.loadImage(pak, image, 0, 0, D3DX_DEFAULT);
	}

	// Lock the surface and write the alpha values for the set pixels
    D3DLOCKED_RECT d3dlr;
    pd3dSurface->LockRect(&d3dlr, 0, 0 );
    DWORD* pDest = (DWORD*)d3dlr.pBits;
    BYTE bAlpha; // 8-bit measure of pixel intensity

    if (b_texture)
	{
		if (!ftexture.lock(0, D3DLOCK_READONLY))
		return FALSE;
	}
	for(int i=32; i<127; i++ )
    {
        for (y=fTexCoords[i-32].top;y<(DWORD)fTexCoords[i-32].bottom;y++)
		{
			for (x=fTexCoords[i-32].left;x<(DWORD)fTexCoords[i-32].right;x++)
			{
				bAlpha = (BYTE)(pBitmapBits[width*y + x] & 0xff);
				if (bAlpha > 0)
				{
	                *(pDest+y*width+x) = (bAlpha << 24);
					if (b_texture)
						*(pDest+y*width+x) += ftexture.point(x-fTexCoords[i-32].left, y-fTexCoords[i-32].top) & 0x00ffffff;
				}
				else
					*(pDest+y*width+x) = 0x00000000;
			}
		}
    }

    // Done updating texture, so clean up used objects
    if (b_texture && !ftexture.unlock())
		return FALSE;
	if (FAILED(pd3dSurface->UnlockRect()))
		return FALSE;
    //put(texture);
	//*this<<texture;
	DeleteObject( hbmBitmap );
    DeleteDC( hDC );
    DeleteObject( hFont );
	return TRUE;
}

Cpoint Cfont::getSize(const string &str)
{
	static Cpoint size;
	size.x=0;
	for (UINT i=0;i<str.size();i++)
		size.x+=charSize[str[i]-32].cx;
	size.y=maxFontHeight;
	return size;
}

Cpoint Cfont::centerText(const Cpoint &size, const string &str)
{
	Cpoint ts=getSize(str);
	static Cpoint start;
	start.x = (size.x - ts.x) / 2;
	start.y = (size.y - ts.y) / 2;
	return start;
}
	
	
