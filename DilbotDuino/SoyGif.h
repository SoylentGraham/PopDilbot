#pragma once

#include <functional>

//	arduino's string is String :)
#include <string>
typedef std::string String;


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

class TImageBlock;
class TCallbacks;

namespace Gif
{
	class THeader;
	
	void	ParseGif(TCallbacks& Callbacks,std::function<void(const TImageBlock&)> DrawPixels);
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

class TCallbacks
{
public:
	std::function<bool(uint8_t*,int)> ReadBytes;
	std::function<void(const String&)> OnError;
	std::function<void(const String&)> OnDebug;
};

//	for use on arduino, so no exceptions, error func instead
class Gif::THeader
{
public:
	THeader(TCallbacks& Callbacks);
	
	//	returns true if more data to parse
	bool		ParseNextBlock(TCallbacks& Callbacks,std::function<void()> OnGraphicControlBlock,std::function<void()> OnCommentBlock,std::function<void(const TImageBlock&)> OnImageBlock);

private:
	void		ParseImageBlock(TCallbacks& Callbacks,std::function<void(const TImageBlock&)> OnImageBlock);
	void		ParseExtensionBlock(TCallbacks& Callbacks,std::function<void()> OnGraphicControlBlock,std::function<void()> OnCommentBlock);

public:
	TRgb8		mPalette[256];	//	global/default palette
	size_t		mPaletteSize;
	size_t		mWidth;
	size_t		mHeight;
	uint8_t		mTransparentPaletteIndex;
};

