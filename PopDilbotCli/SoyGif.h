#pragma once

#include <functional>

class TRgb8
{
public:
	uint8_t	r,g,b;
};

class TRgba8
{
public:
	uint8_t	r,g,b,a;
};

namespace Gif
{
	class THeader;
}


class TImageBlock
{
public:
	uint16_t	mLeft;
	uint16_t	mTop;
	uint16_t	mWidth;
	uint16_t	mHeight;
	uint8_t*	mPixels;
	std::function<TRgba8(uint8_t)>	GetColour;
};

//	for use on arduino, so no exceptions, error func instead
class Gif::THeader
{
public:
	THeader(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError);
	
	void		ParseNextBlock(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError,std::function<void()> OnGraphicControlBlock,std::function<void()> OnCommentBlock,std::function<void(const TImageBlock&)> OnImageBlock);

private:
	void		ParseImageBlock(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError,std::function<void(const TImageBlock&)> OnImageBlock);
	void		ParseExtensionBlock(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError,std::function<void()> OnGraphicControlBlock,std::function<void()> OnCommentBlock);

public:
	TRgb8		mPalette[256];	//	global/default palette
	size_t		mPaletteSize;
	size_t		mWidth;
	size_t		mHeight;
	uint8_t		mTransparentPaletteIndex;
};

