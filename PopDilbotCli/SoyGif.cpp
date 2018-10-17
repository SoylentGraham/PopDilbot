#include "SoyGif.h"


void Gif::ParseGif(TCallbacks& Callbacks,std::function<void(const TImageBlock&)> DrawPixels)
{
	auto& OnDebug = Callbacks.OnDebug;
	
	Gif::THeader Header( Callbacks );
	{
		String Debug;
		Debug += "read gif ";
		Debug += std::to_string(Header.mWidth);
		Debug += "x";
		Debug += std::to_string(Header.mHeight);
		OnDebug( Debug );
	}
	
	auto OnGraphicControlBlock = []
	{
	};
	auto OnCommentBlock = []
	{
	};
	
	while ( true )
	{
		auto MoreData = Header.ParseNextBlock( Callbacks, OnGraphicControlBlock, OnCommentBlock, DrawPixels );
		if ( !MoreData )
			return;
	}
	
}



Gif::THeader::THeader(TCallbacks& Callbacks)
{
	auto& OnError = Callbacks.OnError;
	auto& ReadBytes = Callbacks.ReadBytes;

	uint8_t HeaderBytes[13];
	if ( !ReadBytes(HeaderBytes,sizeof(HeaderBytes) ) )
	{
		OnError("Bytes missing for gif header");
		return;
	}
	//	support others, but this is fine for now
	const auto* Magic = "GIF89a";
	const auto MagicLength = strlen(Magic);
	if ( 0 != memcmp( HeaderBytes, Magic, MagicLength ) )
	{
		OnError("Gif magic incorrect");
		return;
	}
	
	mWidth = HeaderBytes[6] | (HeaderBytes[7]<<8);
	mHeight = HeaderBytes[8] | (HeaderBytes[9]<<8);
	auto Flags = HeaderBytes[10];
#define BITCOUNT(N)	( (1<<(N))-1 )
	auto HasPalette = (Flags >> 7) & BITCOUNT(1);
	auto ColourRes = (Flags >> 6) & BITCOUNT(3);
	auto SortPalette = (Flags >> 3) & BITCOUNT(1);
	mPaletteSize = (Flags >> 0) & BITCOUNT(3);
	mPaletteSize = 2 << (mPaletteSize);	//	2 ^ (1+N)
	
	mTransparentPaletteIndex = HeaderBytes[11];
	auto AspectRatio = HeaderBytes[12];
	
	//	read palette
	if ( HasPalette && mPaletteSize == 0 )
	{
		OnError("Flagged as having a global palette size, and palette size is 0");
		return;
	}
	else if ( !HasPalette && mPaletteSize != 0 )
	{
		OnError("Flagged as having no global palette, and palette size > 0");
		return;
	}
	
	//	read palette
	auto* Palette8 = &mPalette[0].r;
	if ( !ReadBytes(Palette8, static_cast<int>( sizeof(TRgb8)*mPaletteSize) ) )
	{
		OnError("Missing bytes for palette");
		return;
	}
}

bool Gif::THeader::ParseNextBlock(TCallbacks& Callbacks,std::function<void()> OnGraphicControlBlock,std::function<void()> OnCommentBlock,std::function<void(const TImageBlock&)> OnImageBlock)
{
	auto& ReadBytes = Callbacks.ReadBytes;
	auto& OnError = Callbacks.OnError;
	
	uint8_t BlockId;
	if ( !ReadBytes( &BlockId, 1 ) )
	{
		OnError("Failed to read block id");
		return false;
	}
	
	switch ( BlockId )
	{
		case 0x2c:
			ParseImageBlock( Callbacks, OnImageBlock );
			break;
			
		case 0x21:
			ParseExtensionBlock( Callbacks, OnGraphicControlBlock, OnCommentBlock );
			break;
			
		case 0x3b:
			//	end block, bail out
			return false;
		
		default:
			OnError("Unknown block type");
			return false;
	}

	uint8_t Terminator = 0xdd;
	if ( !ReadBytes( &Terminator, 1 ) )
	{
		OnError("Block terminator missing");
		return false;
	}
	if ( Terminator != 0 )
	{
		OnError("Block terminator not zero");
		return false;
	}
	
	//	more data to go
	return true;
}


class LzwDecoder
{
public:
	LzwDecoder(std::function<bool(uint8_t*,int)> ReadBlock) :
		readIntoBuffer	( ReadBlock )
	{
		
	}
	
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
	 uint8_t temp_buffer[260];
	//uint8_t * temp_buffer;
	
	// Masks for 0 .. 16 bits
	unsigned int mask[17] = {
		0x0000, 0x0001, 0x0003, 0x0007,
		0x000F, 0x001F, 0x003F, 0x007F,
		0x00FF, 0x01FF, 0x03FF, 0x07FF,
		0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF,
		0xFFFF
	};
#define lzwMaxBits	12
	#define LZW_SIZTABLE  (1 << lzwMaxBits)
	uint8_t stack  [LZW_SIZTABLE];
	uint8_t suffix [LZW_SIZTABLE];
	uint16_t prefix [LZW_SIZTABLE];
	
	void 	Init(int csize);
	int		get_code();
	int 	decode(uint8_t *buf, int len, uint8_t *bufend);
	
	std::function<bool(uint8_t*,int)>	readIntoBuffer;
};

void LzwDecoder::Init(int csize)
{
	// Initialize read buffer variables
	bbuf = 0;
	bbits = 0;
	bs = 0;
	bcnt = 0;
	
	// Initialize decoder variables
	codesize = csize;
	cursize = codesize + 1;
	curmask = mask[cursize];
	top_slot = 1 << cursize;
	clear_code = 1 << codesize;
	end_code = clear_code + 1;
	slot = newcodes = clear_code + 2;
	oc = fc = -1;
	sp = stack;
}

//  Get one code of given number of bits from stream
int LzwDecoder::get_code()
{
	while (bbits < cursize) {
		if (bcnt == bs) {
			// get number of bytes in next block
			readIntoBuffer(temp_buffer, 1);
			bs = temp_buffer[0];
			readIntoBuffer(temp_buffer, bs);
			bcnt = 0;
		}
		bbuf |= temp_buffer[bcnt] << bbits;
		bbits += 8;
		bcnt++;
	}
	int c = bbuf;
	bbuf >>= cursize;
	bbits -= cursize;
	return c & curmask;
}


// Decode given number of bytes
//   buf 8 bit output buffer
//   len number of pixels to decode
//   returns the number of bytes decoded
int LzwDecoder::decode(uint8_t *buf, int len, uint8_t *bufend)
{
	int l, c, code;
	
#if LZWDEBUG == 1
	unsigned char debugMessagePrinted = 0;
#endif
	
	if (end_code < 0) {
		return 0;
	}
	l = len;
	
	for (;;) {
		while (sp > stack) {
			// load buf with data if we're still within bounds
			if(buf < bufend) {
				*buf++ = *(--sp);
			} else {
				// out of bounds, keep incrementing the pointers, but don't use the data
#if LZWDEBUG == 1
				// only print this message once per call to lzw_decode
				if(buf == bufend)
					Serial.println("****** LZW imageData buffer overrun *******");
#endif
			}
			if ((--l) == 0) {
				return len;
			}
		}
		c = get_code();
		if (c == end_code) {
			break;
			
		}
		else if (c == clear_code) {
			cursize = codesize + 1;
			curmask = mask[cursize];
			slot = newcodes;
			top_slot = 1 << cursize;
			fc= oc= -1;
			
		}
		else    {
			
			code = c;
			if ((code == slot) && (fc >= 0)) {
				*sp++ = fc;
				code = oc;
			}
			else if (code >= slot) {
				break;
			}
			while (code >= newcodes) {
				*sp++ = suffix[code];
				code = prefix[code];
			}
			*sp++ = code;
			if ((slot < top_slot) && (oc >= 0)) {
				suffix[slot] = code;
				prefix[slot++] = oc;
			}
			fc = code;
			oc = c;
			if (slot >= top_slot) {
				if (cursize < lzwMaxBits) {
					top_slot <<= 1;
					curmask = mask[++cursize];
				} else {
#if LZWDEBUG == 1
					if(!debugMessagePrinted) {
						debugMessagePrinted = 1;
						Serial.println("****** cursize >= lzwMaxBits *******");
					}
#endif
				}
				
			}
		}
	}
	end_code = -1;
	return len - l;
}

void Gif::THeader::ParseImageBlock(TCallbacks& Callbacks,std::function<void(const TImageBlock&)> OnImageBlock)
{
	auto& ReadBytes = Callbacks.ReadBytes;
	auto& OnError = Callbacks.OnError;

	uint8_t HeaderBytes[9];
	if ( !ReadBytes( HeaderBytes, sizeof(HeaderBytes) ) )
	{
		OnError("Missing bytes for image block header");
		return;
	}
	
	auto BlockLeft = HeaderBytes[0] | (HeaderBytes[1]<<8);
	auto BlockTop = HeaderBytes[2] | (HeaderBytes[3]<<8);
	auto BlockWidth = HeaderBytes[4] | (HeaderBytes[5]<<8);
	auto BlockHeight = HeaderBytes[6] | (HeaderBytes[7]<<8);
	
	auto Flags = HeaderBytes[8];
	auto HasLocalPalette = (Flags >> 7) & BITCOUNT(1);
	auto Interlacted = (Flags >> 6) & BITCOUNT(1);
	auto SortedPalette = (Flags >> 5) & BITCOUNT(1);
	auto Reserved = (Flags >> 3) & BITCOUNT(2);
	auto PaletteSize = (Flags >> 0) & BITCOUNT(3);
	PaletteSize = 2 << (PaletteSize);
	
	if ( Interlacted )
	{
		OnError("Interlaced gif not supported");
		return;
	}
	
	//	read palette
	if ( HasLocalPalette && PaletteSize == 0 )
	{
		OnError("Flagged as having a global palette size, and palette size is 0");
		return;
	}
	
	if ( !HasLocalPalette )
		PaletteSize = 0;
	
	TRgb8 LocalPalette[256];
	auto* LocalPalette8 = &LocalPalette[0].r;
	if ( !ReadBytes( LocalPalette8, sizeof(TRgb8) * PaletteSize ) )
	{
		OnError("Missing bytes for image block palette");
		return;
	}
	
	uint8_t LzwMinimumCodeSize;
	ReadBytes( &LzwMinimumCodeSize, 1 );
	
	/*
	//	https://github.com/pixelmatix/AnimatedGIFs/blob/master/GifDecoder_Impl.h#L557
	//	read lzw blocks
	//	LZW doesn't parse through all the data, manually set position
	 //	so if we're parsing all the frames, then we need to work out how much space the blocks take up
	while ( true )
	{
		uint8_t LzwBlockSize;
		ReadBytes( &LzwBlockSize, 1 );
		if ( LzwBlockSize == 0 )
			break;
		
		//	walk over
		if ( !ReadBytes( nullptr, LzwBlockSize ) )
		{
			OnError("Error reading lzw block");
			return;
		}
	}
	*/
	
	LzwDecoder Decoder(ReadBytes);
	Decoder.Init(LzwMinimumCodeSize);
	
	auto* Palette = HasLocalPalette ? LocalPalette : this->mPalette;
	auto GetColour = [&](uint8_t ColourIndex)
	{
		bool Transparent = this->mTransparentPaletteIndex == ColourIndex;
		TRgba8 Rgba;
		Rgba.r = Palette[ColourIndex].r;
		Rgba.g = Palette[ColourIndex].g;
		Rgba.b = Palette[ColourIndex].b;
		Rgba.a = Transparent ? 0 : 255;
		return Rgba;
	};
	
	// Decode the non interlaced LZW data into the image data buffer
	for (auto y=BlockTop;	y<BlockTop+BlockHeight;	y++)
	{
		//int lzw_decode(uint8_t *buf, int len, uint8_t *bufend);
		uint8_t RowData[BlockWidth];
		
		//lzw_decode( imageData + (y * maxGifWidth) + x, Block.mWidth, imageData + sizeof(imageData) );
		auto DecodedCount = Decoder.decode( RowData, BlockWidth, RowData+sizeof(RowData) );
		
		//	output block
		TImageBlock Row;
		Row.mLeft = BlockLeft;
		Row.mTop = BlockTop;
		Row.mWidth = BlockWidth;
		Row.mHeight = 1;
		Row.mPixels = RowData;
		Row.GetColour = GetColour;
		OnImageBlock(Row);
	}
	
}


void Gif::THeader::ParseExtensionBlock(TCallbacks& Callbacks,std::function<void()> OnGraphicControlBlock,std::function<void()> OnCommentBlock)
{
	auto& OnError = Callbacks.OnError;
	auto& ReadBytes = Callbacks.ReadBytes;
	
	//	http://www.onicos.com/staff/iz/formats/gif.html
	uint8_t Type;	//	label
	ReadBytes( &Type, 1 );
	
	//	blocks defined by length
	while ( true )
	{
		uint8_t BlockSize;
		ReadBytes( &BlockSize, 1 );
		
		//	block terminator, unpop!
		if ( BlockSize == 0 )
		{
			ReadBytes(nullptr,-1);
			break;
		}
		
		if ( !ReadBytes( nullptr, BlockSize ) )
		{
			OnError("Error getting next application block size (ood)");
			return;
		}
	}
}



