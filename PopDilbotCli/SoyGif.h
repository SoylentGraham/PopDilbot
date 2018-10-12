#pragma once

#include <string>
#include <vector>

class TRgba8
{
public:
	uint8_t	r,g,b,a;
};

namespace Gif
{
	class THeader;
}


class Gif::THeader
{
public:
	THeader(const uint8_t* Data,size_t DataSize);
	
public:
	TRgba8		mPalette[256];	//	global/default palette
	size_t		mWidth;
	size_t		mHeight;
	uint8_t		mTransparentPaletteIndex;
};

