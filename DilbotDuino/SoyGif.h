#pragma once

//	mkr board has some min/max defines 
#undef min
#undef max
#include <functional>

#if defined(ARDUINO)
#define TARGET_ARDUINO
#endif

#if defined(TARGET_ARDUINO)
	#include <Arduino.h>	//	for String
	inline String IntToString(int Value)	{	return String(Value);	}
#else
	//	arduino's string is String :)
	#include <string>
	typedef std::string String;
	inline String IntToString(int Value)	{	return std::to_string(Value);	}
#endif

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

namespace TDecodeResult
{
	enum Type
	{
		NeedMoreData,
		Finished,
		Error,
	};
}


namespace Lzw
{
	class Decoder;
}



class TStreamBuffer
{
	static const size_t BUFFERSIZE = 1000;
public:
	bool	Push(uint8_t Data);
	bool	Pop(uint8_t* Data,size_t DataSize);
	void	Unpop(size_t DataSize);
	size_t	GetBufferSize();
	bool	HasSpace();
	
public:
	//	ring buffer
	size_t	mBufferHead = 0;
	size_t	mBufferTail = 0;
	uint8_t	mBuffer[BUFFERSIZE];
};


class Lzw::Decoder
{
public:
	// LZW variables
	int bbits = -999;
	int bbuf = -999;
	int cursize = -999;                // The current code size
	int curmask = -999;
	int codesize = -999;
	int clear_code = -999;
	int end_code = -999;
	int newcodes = -999;               // First available code
	int top_slot = -999;               // Highest code for current size
	//int extra_slot = -999;
	int slot = -999;                   // Last read code
	int fc = -999;
	int oc = -999;
	int bs = -999;                     // Current buffer size for GIF
	int bcnt = -999;
	uint8_t *sp = nullptr;
	uint8_t temp_buffer[256];
#define lzwMaxBits	12
#define LZW_SIZTABLE  (1 << lzwMaxBits)
	uint8_t stack  [LZW_SIZTABLE];
	uint8_t suffix [LZW_SIZTABLE];
	uint16_t prefix [LZW_SIZTABLE];
	
	// Masks for 0 .. 16 bits
	unsigned int mask[17] = {
		0x0000, 0x0001, 0x0003, 0x0007,
		0x000F, 0x001F, 0x003F, 0x007F,
		0x00FF, 0x01FF, 0x03FF, 0x07FF,
		0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF,
		0xFFFF
	};
	
	
	void 					Init(int csize);
	bool					get_code(std::function<bool(uint8_t*,size_t)>& ReadBytes,int& Code);
	TDecodeResult::Type 	decode(uint8_t *buf, int len, uint8_t *bufend,std::function<bool(uint8_t*,size_t)>& ReadBytes,std::function<void(const String&)>& Debug,int& DecodedCount);
	/*
	std::function<bool(uint8_t*,size_t)>&	readIntoBuffer;
	std::function<void(const String&)>& Debug;
	 */
};

class TImageBlock;
class TCallbacks;

namespace Gif
{
	class THeader;
	
	TDecodeResult::Type	ParseGif(Gif::THeader& Header,TCallbacks& Callbacks,std::function<void(const TImageBlock&)> DrawPixels);
}

class TPendingImageBlock
{
public:
	uint8_t			mHeader[9];
	size_t			mPaletteSize;
	TRgb8			mPalette[256];
	size_t			mCurrentRow;
	Lzw::Decoder	mLzwDecoder;
	
	uint16_t	mLeft;
	uint16_t	mTop;
	uint16_t	mWidth;
	uint16_t	mHeight;

};

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
	TCallbacks(TStreamBuffer& StreamBuffer) :
		mStreamBuffer	( StreamBuffer )
	{
	}
	TStreamBuffer&						mStreamBuffer;
	std::function<void(const String&)>	OnError;
	std::function<void(const String&)>	OnDebug;
};

//	for use on arduino, so no exceptions, error func instead
class Gif::THeader
{
public:
	TDecodeResult::Type		ParseHeader(TCallbacks& Callbacks);
	TDecodeResult::Type		ParseGlobalPalette(TCallbacks& Callbacks);

	TDecodeResult::Type		ParseNextBlock(TCallbacks& Callbacks,std::function<void(const TImageBlock&)>& OnImageBlock);

	TDecodeResult::Type		ParseImageBlockRow(TCallbacks& Callbacks,TPendingImageBlock& Block,bool& FinishedBlock,std::function<void(const TImageBlock&)>& OnImageBlock);
	TDecodeResult::Type		ParseImageBlock(std::function<bool(uint8_t*,size_t)>& ReadBytes,TCallbacks& Callbacks,std::function<void(const TImageBlock&)>& OnImageBlock);
	TDecodeResult::Type		ParseExtensionBlock(std::function<bool(uint8_t*,size_t)>& ReadBytes,TCallbacks& Callbacks);
	TDecodeResult::Type		ParseExtensionBlockChunk(TStreamBuffer& StreamBuffer);

public:
	bool		mGotHeader = false;
	size_t		mPaletteSize;
	size_t		mWidth;
	size_t		mHeight;
	uint8_t		mTransparentPaletteIndex;
	bool		mGotPalette = false;
	TRgb8		mPalette[256];	//	global/default palette
	bool		mHasPendingImageBlock = false;
	TPendingImageBlock	mPendingImageBlock;
	uint8_t		mPendingExtensionBlockType = 0x0;	//	when non zero, we're eating chunks from the block
};
