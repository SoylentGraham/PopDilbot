#include "SoyGif.h"
#include <iostream>

Gif::THeader::THeader(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError)
{
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
	if ( !ReadBytes(Palette8,sizeof(TRgb8)*mPaletteSize ) )
	{
		OnError("Missing bytes for palette");
		return;
	}
}

void Gif::THeader::ParseNextBlock(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError,std::function<void()> OnGraphicControlBlock,std::function<void()> OnCommentBlock,std::function<void(const TImageBlock&)> OnImageBlock)
{
	uint8_t BlockId;
	ReadBytes( &BlockId, 1 );
	
	switch ( BlockId )
	{
		case 0x2c:
			ParseImageBlock( ReadBytes, OnError, OnImageBlock );
			break;
			
		case 0x21:
			ParseExtensionBlock( ReadBytes, OnError, OnGraphicControlBlock, OnCommentBlock );
			break;
			
		case 0x3b:
			//	end block, read up all the rest of the bytes
			while ( true )
			{
				if ( !ReadBytes(nullptr,1) )
					break;
			}
			return;
		
		default:
			OnError("Unknown block type");
			return;
	}

	uint8_t Terminator = 0xdd;
	if ( !ReadBytes( &Terminator, 1 ) )
	{
		OnError("Block terminator missing");
		return;
	}
	if ( Terminator != 0 )
	{
		OnError("Block terminator not zero");
		return;
	}
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

void Gif::THeader::ParseImageBlock(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError,std::function<void(const TImageBlock&)> OnImageBlock)
{
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

/*
static void stbi__out_gif_code(stbi__gif *g, stbi__uint16 code)
{
	stbi_uc *p, *c;
	int idx;
	
	// recurse to decode the prefixes, since the linked-list is backwards,
	// and working backwards through an interleaved image would be nasty
	if (g->codes[code].prefix >= 0)
		stbi__out_gif_code(g, g->codes[code].prefix);
	
	if (g->cur_y >= g->max_y) return;
	
	idx = g->cur_x + g->cur_y;
	p = &g->out[idx];
	g->history[idx / 4] = 1;
	
	c = &g->color_table[g->codes[code].suffix * 4];
	if (c[3] > 128) { // don't render transparent pixels;
		p[0] = c[2];
		p[1] = c[1];
		p[2] = c[0];
		p[3] = c[3];
	}
	g->cur_x += 4;
	
	if (g->cur_x >= g->max_x) {
		g->cur_x = g->start_x;
		g->cur_y += g->step;
		
		while (g->cur_y >= g->max_y && g->parse > 0) {
			g->step = (1 << g->parse) * g->line_size;
			g->cur_y = g->start_y + (g->step >> 1);
			--g->parse;
		}
	}
}

void parselzw()
{
	stbi_uc lzw_cs;
	stbi__int32 len, init_code;
	stbi__uint32 first;
	stbi__int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
	stbi__gif_lzw *p;
	
	lzw_cs = stbi__get8(s);
	if (lzw_cs > 12) return NULL;
	clear = 1 << lzw_cs;
	first = 1;
	codesize = lzw_cs + 1;
	codemask = (1 << codesize) - 1;
	bits = 0;
	valid_bits = 0;
	for (init_code = 0; init_code < clear; init_code++) {
		g->codes[init_code].prefix = -1;
		g->codes[init_code].first = (stbi_uc) init_code;
		g->codes[init_code].suffix = (stbi_uc) init_code;
	}
	
	// support no starting clear code
	avail = clear+2;
	oldcode = -1;
	
	len = 0;
	for(;;) {
		if (valid_bits < codesize) {
			if (len == 0) {
				len = stbi__get8(s); // start new block
				if (len == 0)
					return g->out;
			}
			--len;
			bits |= (stbi__int32) stbi__get8(s) << valid_bits;
			valid_bits += 8;
		} else {
			stbi__int32 code = bits & codemask;
			bits >>= codesize;
			valid_bits -= codesize;
			// @OPTIMIZE: is there some way we can accelerate the non-clear path?
			if (code == clear) {  // clear code
				codesize = lzw_cs + 1;
				codemask = (1 << codesize) - 1;
				avail = clear + 2;
				oldcode = -1;
				first = 0;
			} else if (code == clear + 1) { // end of stream code
				stbi__skip(s, len);
				while ((len = stbi__get8(s)) > 0)
					stbi__skip(s,len);
					return g->out;
			} else if (code <= avail) {
				if (first) {
					return stbi__errpuc("no clear code", "Corrupt GIF");
				}
				
				if (oldcode >= 0) {
					p = &g->codes[avail++];
					if (avail > 8192) {
						return stbi__errpuc("too many codes", "Corrupt GIF");
					}
					
					p->prefix = (stbi__int16) oldcode;
					p->first = g->codes[oldcode].first;
					p->suffix = (code == avail) ? p->first : g->codes[code].first;
				} else if (code == avail)
					return stbi__errpuc("illegal code in raster", "Corrupt GIF");
					
					stbi__out_gif_code(g, (stbi__uint16) code);
					
					if ((avail & codemask) == 0 && avail <= 0x0FFF) {
						codesize++;
						codemask = (1 << codesize) - 1;
					}
				
				oldcode = code;
			} else {
				return stbi__errpuc("illegal code in raster", "Corrupt GIF");
			}
		}
	}
}
*/
#include <iostream>

void Gif::THeader::ParseExtensionBlock(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError,std::function<void()> OnGraphicControlBlock,std::function<void()> OnCommentBlock)
{
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
/*
	//	application has a bit of extra data
	if ( Type == 0xff )
	{
		uint8_t ApplicationIdentifier[9];
		ReadBytes( ApplicationIdentifier, 8 );
		ApplicationIdentifier[8]=0;
		uint8_t ApplicationAuthCode[4];
		ReadBytes( ApplicationAuthCode, 3 );
		ApplicationAuthCode[3]=0;
		std::cout << "App extension: " << ApplicationIdentifier << " code: " <<ApplicationAuthCode << std::endl;
		
		//	read application data until we find terminator.
		//	app specific length? nothing explains it; http://www.vurdalakov.net/misc/gif/application-extension
		auto AppBytesRead = 0;
		while ( true )
		{
			uint8_t AppData;
			if ( !ReadBytes( &AppData, 1 ) )
			{
				OnError("Out of data reading app data");
				return;
			}
			AppBytesRead++;
			if ( AppBytesRead > 2000 )
			{
				OnError("Breaking out of app data loop after 2000 bytes");
				return;
			}
			
			//	found terminator
			if ( AppData == 0 )
			{
				std::cout << "read " << AppBytesRead << " application data bytes" << std::endl;
				ReadBytes(nullptr, -1 );
				break;
			}
		}
	}
	else
	{
		//	skip over data for now
		ReadBytes( nullptr, BlockSize );
	}
 */
}


/*
Gif::THeader::THeader(const uint8_t* Data,size_t DataSize)
{
	auto MagicSize = 6;
	auto DescriptionSize = 7;
	auto PaletteSize = 256*3;
	auto TotalSize = MagicSize+DescriptionSize+PaletteSize;
	if ( DataSize < TotalSize )
		throw std::underflow_error("Missing data for gif header");
	auto* MagicData = &Data[0];
	auto* DescriptionData = &Data[0+MagicSize];
	auto* PaletteData = &Data[0+MagicSize+DescriptionSize];
	
	//	GIFXXX
	if ( memcmp( "GIF", MagicData, 3 ) != 0 )
		throw std::underflow_error("gif Magic header mis match");
	
	auto Width = Description[0] | (Description[1]<<8);
	auto Height = Description[2] | (Description[3]<<8);
	
	//	decode this byte
	//	f0 == 2
	auto Flags = Description[4];
	auto BackgroundPalIndex = Description[5];
	//auto Ratio = Description[6];

	auto HasPalette = bool_cast( Flags & bits(7) );
	//auto ColourResolution = Flags & bits(6,5,4);
	//auto Sorted = bool_cast( Flags & bits(3) );
	auto PaletteSize = GetPaletteSize( HasPalette, Flags, bits(0,1,2) );
	
	//	read palette
	BufferArray<Soy::TRgba8,256> Palette;
	if ( !ReadPalette( GetArrayBridge(Palette), GetArrayBridge(PaletteData), HasPalette ? PaletteSize : 0, BackgroundPalIndex, Buffer ) )
		return Unpop( TProtocolState::Waiting );
	
	mHeader = THeader( Width, Height, GetArrayBridge( Palette ), BackgroundPalIndex );
	
	//	success, but didn't generate any packets
	return TProtocolState::Ignore;
}

*/




