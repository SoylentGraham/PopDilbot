#include "SoyGif.h"


bool TStreamBuffer::HasSpace()
{
	auto CurrentBufferSize = GetBufferSize();
	//	gr: something wrong with the algo here I think, capacity should be 100% :)
	if ( CurrentBufferSize == BUFFERSIZE-1 )
		return false;
	return true;
}

bool TStreamBuffer::Push(uint8_t Data)
{
	if ( !HasSpace() )
		return false;

	mBuffer[mBufferTail] = Data;
	mBufferTail++;
	mBufferTail %= BUFFERSIZE;
	return true;
}

bool TStreamBuffer::Pop(uint8_t* Data,size_t DataSize)
{
	auto CurrentBufferSize = GetBufferSize();
	if ( CurrentBufferSize < DataSize )
		return false;
	
	for ( int i=0;	i<DataSize;	i++ )
	{
		auto BufferIndex = (mBufferHead + i) % BUFFERSIZE;
		if ( Data != nullptr )
			Data[i] = mBuffer[BufferIndex];
	}
	mBufferHead += DataSize;
	mBufferHead %= BUFFERSIZE;
	return true;
}

void TStreamBuffer::Unpop(size_t DataSize)
{
	//	check in case this wraps us back
	auto NewHead = static_cast<ssize_t>(mBufferHead) - static_cast<ssize_t>(DataSize);
	if ( NewHead < 0 )
		NewHead += BUFFERSIZE;
	mBufferHead = NewHead;
}

size_t TStreamBuffer::GetBufferSize()
{
	auto Tail = mBufferTail;
	if ( Tail < mBufferHead )
		Tail += BUFFERSIZE;
	return Tail - mBufferHead;
}




TDecodeResult::Type Gif::ParseGif(Gif::THeader& Header,TCallbacks& Callbacks,std::function<void(const TImageBlock&)> DrawPixels)
{
	if ( !Header.mGotHeader )
	{
		auto Result = Header.ParseHeader(Callbacks);
		if ( Result != TDecodeResult::Finished )
			return Result;
		Header.mGotHeader = true;
	}
	
	if ( !Header.mGotPalette )
	{
		auto Result = Header.ParseGlobalPalette(Callbacks);
		if ( Result != TDecodeResult::Finished )
			return Result;
		Header.mGotPalette = true;
	}
	
	//	continue a pending image block
	if ( Header.mHasPendingImageBlock )
	{
		bool ImageBlockFinished = false;
		auto Result = Header.ParseImageBlockRow( Callbacks, Header.mPendingImageBlock, ImageBlockFinished, DrawPixels );
		if ( Result != TDecodeResult::Finished )
			return Result;
		if ( ImageBlockFinished )
			Header.mHasPendingImageBlock = false;
		
		return TDecodeResult::NeedMoreData;
	}
	
	if ( Header.mPendingExtensionBlockType != 0 )
	{
		auto Result = Header.ParseExtensionBlockChunk( Callbacks.mStreamBuffer );
		if ( Result != TDecodeResult::Finished )
			return Result;
		
		Header.mPendingExtensionBlockType = 0x0;
		return TDecodeResult::NeedMoreData;
	}
	
	return Header.ParseNextBlock( Callbacks, DrawPixels );
}



TDecodeResult::Type Gif::THeader::ParseHeader(TCallbacks& Callbacks)
{
	auto& OnError = Callbacks.OnError;
	auto& OnDebug = Callbacks.OnDebug;
	auto& StreamBuffer = Callbacks.mStreamBuffer;

	uint8_t HeaderBytes[13];
	if ( !StreamBuffer.Pop(HeaderBytes,sizeof(HeaderBytes) ) )
		return TDecodeResult::NeedMoreData;

	//	support others, but this is fine for now
	const auto* Magic = "GIF89a";
	const auto MagicLength = strlen(Magic);
	if ( 0 != memcmp( HeaderBytes, Magic, MagicLength ) )
	{
		String Error = "Gif magic incorrect: ";
		Error += (char)HeaderBytes[0];
		Error += (char)HeaderBytes[1];
		Error += (char)HeaderBytes[2];
		Error += (char)HeaderBytes[3];
		Error += (char)HeaderBytes[4];
		Error += (char)HeaderBytes[5];
		OnError(Error.c_str());
		return TDecodeResult::Error;
	}
	
	mWidth = HeaderBytes[6] | (HeaderBytes[7]<<8);
	mHeight = HeaderBytes[8] | (HeaderBytes[9]<<8);
	auto Flags = HeaderBytes[10];
#define BITCOUNT(N)	( (1<<(N))-1 )
	auto HasPalette = (Flags >> 7) & BITCOUNT(1);
	//auto ColourRes = (Flags >> 6) & BITCOUNT(3);
	//auto SortPalette = (Flags >> 3) & BITCOUNT(1);
	mPalette.mSize = (Flags >> 0) & BITCOUNT(3);
	mPalette.mSize = 2 << (mPalette.mSize);	//	2 ^ (1+N)
	
	mTransparentPaletteIndex = HeaderBytes[11];
	//auto AspectRatio = HeaderBytes[12];

	//	read palette
	if ( HasPalette && mPalette.mSize == 0 )
	{
		OnError("Flagged as having a global palette size, and palette size is 0");
		return TDecodeResult::Error;
	}
	else if ( !HasPalette )
	{
		//	the shift means this will always be non-zero, so reset
		mPalette.mSize = 0;
	}

	{
		String Debug = "Gif ";
		Debug += IntToString( static_cast<int>(mWidth) );
		Debug += "x";
		Debug += IntToString( static_cast<int>(mHeight) );
		Debug += ". Global palette size: ";
		Debug += IntToString( static_cast<int>(mPalette.mSize) );
		OnDebug( Debug.c_str() );
	}
	
	return TDecodeResult::Finished;
}


TDecodeResult::Type Gif::THeader::ParseGlobalPalette(TCallbacks& Callbacks)
{
	auto& StreamBuffer = Callbacks.mStreamBuffer;
	
	//	read palette
	auto* Palette8 = &mPalette.mColours[0].r;
	if ( !StreamBuffer.Pop( Palette8, static_cast<int>( sizeof(TRgb8)*mPalette.mSize) ) )
		return TDecodeResult::NeedMoreData;
	
	return TDecodeResult::Finished;
}

TDecodeResult::Type Gif::THeader::ParseNextBlock(TCallbacks& Callbacks,std::function<void(const TImageBlock&)>& OnImageBlock)
{
	auto& StreamBuffer = Callbacks.mStreamBuffer;
	
	//	read byte wrapper so we can unpop
	size_t ReadCount = 0;
	std::function<bool(uint8_t*,size_t)> ReadBytes = [&](uint8_t* Buffer,size_t BufferSize)
	{
		if ( !StreamBuffer.Pop( Buffer, BufferSize ) )
			return false;
		ReadCount += BufferSize;
		return true;
	};
	auto Unpop = [&]()
	{
		StreamBuffer.Unpop(ReadCount);
		return TDecodeResult::NeedMoreData;
	};
	
	auto& OnError = Callbacks.OnError;
	auto& OnDebug = Callbacks.OnDebug;
	
	uint8_t BlockId;
	if ( !ReadBytes( &BlockId, 1 ) )
	{
		OnError("Failed to read block id");
		return Unpop();
	}
	
	switch ( BlockId )
	{
		case 0x2c:
		{
			OnDebug("ParseImageBlock"); 
			auto BlockResult = ParseImageBlock( ReadBytes, Callbacks, OnImageBlock );
			if ( BlockResult == TDecodeResult::Error )
				return BlockResult;
			if ( BlockResult == TDecodeResult::NeedMoreData )
				return Unpop();
			//	async process the rest of the image block
			return TDecodeResult::NeedMoreData;
		}
		break;
			
		case 0x21:
		{
			OnDebug("ParseExtensionBlock"); 
			auto BlockResult = ParseExtensionBlock( ReadBytes, Callbacks );
			if ( BlockResult == TDecodeResult::Error )
				return BlockResult;
			if ( BlockResult == TDecodeResult::NeedMoreData )
			{
				OnDebug("Out of data");
				return Unpop();
			}
			//	process next chunks async
			return TDecodeResult::NeedMoreData;
		}
		break;
			
		case 0x3b:
			OnDebug("End Block"); 
			//	end block, bail out
			return TDecodeResult::Finished;
		
		default:
			OnError("Unknown block type");
			return TDecodeResult::Error;
	}

	uint8_t Terminator = 0xdd;
	if ( !ReadBytes( &Terminator, 1 ) )
		return Unpop();
	
	if ( Terminator != 0 )
	{
		OnError("Block terminator not zero");
		return TDecodeResult::Error;
	}
	
	//	more data to go
	return TDecodeResult::NeedMoreData;
}


void Lzw::Decoder::Init(int csize)
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
bool Lzw::Decoder::get_code(std::function<bool(uint8_t*,size_t)>& ReadBytes,int& Code)
{
	//static_assert( sizeof(temp_buffer) >= 256, "Temp buffer needs to be 1-byte-max length");
	while (bbits < cursize)
	{
		if (bcnt == bs)
		{
			// get number of bytes in next block
			uint8_t BlockSize;
			if ( !ReadBytes(&BlockSize, 1) )
				return false;
			bs = BlockSize;
			if ( !ReadBytes(temp_buffer, bs) )
				return false;
			bcnt = 0;
		}
		bbuf |= temp_buffer[bcnt] << bbits;
		bbits += 8;
		bcnt++;
	}
	int c = bbuf;
	bbuf >>= cursize;
	bbits -= cursize;
	Code = c & curmask;
	return true;
}


// Decode given number of bytes
//   buf 8 bit output buffer
//   len number of pixels to decode
//   returns the number of bytes decoded
TDecodeResult::Type Lzw::Decoder::decode(uint8_t *buf, int len, uint8_t *bufend,std::function<bool(uint8_t*,size_t)>& ReadBytes,std::function<void(const char*)>& Debug,int& DecodedCount)
{
	//	https://arduino-esp8266.readthedocs.io/en/latest/faq/a02-my-esp-crashes.html#what-is-the-cause-of-restart
	int l, c, code;
	
	if (end_code < 0)
	{
		DecodedCount = 0;
		return TDecodeResult::Finished;
	}
	l = len;

	int LoopCount = 0;
	for (;;) 
	{
		//Debug( String("lzw LoopCount=") + IntToString(LoopCount) );
		LoopCount++;
		while (sp > stack) 
		{
			// load buf with data if we're still within bounds
			if(buf < bufend) {
				*buf++ = *(--sp);
			} else {
				// out of bounds, keep incrementing the pointers, but don't use the data

				// only print this message once per call to lzw_decode
				if(buf == bufend)
					Debug("LZW imageData buffer overrun");
				DecodedCount = 0;
				return TDecodeResult::Error;
			}
			if ((--l) == 0) 
			{
				DecodedCount = len;
				return TDecodeResult::Finished;
			}
		}
		if ( !get_code( ReadBytes, c ) )
			return TDecodeResult::NeedMoreData;
		//Debug("Get_code()");
		if (c == end_code) 
		{
			break;			
		}
		else if (c == clear_code) 
		{
			cursize = codesize + 1;
			curmask = mask[cursize];
			slot = newcodes;
			top_slot = 1 << cursize;
			fc= oc= -1;
			
		}
		else   
		{
			
			code = c;
			if ((code == slot) && (fc >= 0)) {
				*sp++ = fc;
				code = oc;
			}
			else if (code >= slot) {
				break;
			}
			while (code >= newcodes)
			{
				//Debug( String("while (code[") + IntToString(code) +"/" + IntToString(LZW_SIZTABLE) + " >= newcodes) {");
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
			if (slot >= top_slot)
			{
				if (cursize < lzwMaxBits)
				{
					top_slot <<= 1;
					curmask = mask[++cursize];
				}
				else
				{
					Debug("lzw cursize >= lzwMaxBits");
					DecodedCount = 0;
					return TDecodeResult::Error;
				}
			}
		}
	}
	end_code = -1;
	DecodedCount = len - l;
	return TDecodeResult::Finished;
}


TDecodeResult::Type Gif::THeader::ParseImageBlockRow(TCallbacks& Callbacks,TPendingImageBlock& Block,bool& FinishedBlock,std::function<void(const TImageBlock&)>& OnImageBlock)
{
	auto& StreamBuffer = Callbacks.mStreamBuffer;
	auto& OnError = Callbacks.OnError;
	
	//	read byte wrapper so we can unpop
	size_t ReadCount = 0;
	std::function<bool(uint8_t*,size_t)> ReadBytes = [&](uint8_t* Buffer,size_t BufferSize)
	{
		if ( !StreamBuffer.Pop( Buffer, BufferSize ) )
			return false;
		ReadCount += BufferSize;
		return true;
	};
	auto Unpop = [&]()
	{
		StreamBuffer.Unpop(ReadCount);
		return TDecodeResult::NeedMoreData;
	};
	
	
	auto& Decoder = Block.mLzwDecoder;
	//auto* Palette = Block.mPaletteSize ? Block.mPalette.mColours : this->mPalette.mColours;
	auto* Palette = this->mPalette.mColours;
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
	
	uint8_t RowData[Block.mWidth];
	int DecodedCount;
	auto Result = Decoder.decode( RowData, Block.mWidth, RowData+sizeof(RowData), ReadBytes, Callbacks.OnDebug, DecodedCount );
	if ( Result == TDecodeResult::NeedMoreData )
		return Unpop();
	if ( Result == TDecodeResult::Error )
		return Result;
	
	//	output block
	TImageBlock Row;
	Row.mLeft = Block.mLeft;
	Row.mTop = Block.mTop + Block.mCurrentRow;
	Row.mWidth = DecodedCount;
	Row.mHeight = 1;
	Row.mPixels = RowData;
	Row.GetColour = GetColour;

	//	in the rare event that we need one more byte after processing the image, we grab it before rendering
	if ( Block.mCurrentRow >= Block.mHeight )
	{
		//	read terminator
		uint8_t Terminator = 0xcc;
		if ( !ReadBytes( &Terminator, 1 ) )
			return Unpop();
		if ( Terminator != 0 )
		{
			OnError("Image block terminator not zero");
			return TDecodeResult::Error;
		}
		FinishedBlock = true;
	}
	
	Block.mCurrentRow++;
	OnImageBlock(Row);
	

	return TDecodeResult::Finished;
}


TDecodeResult::Type Gif::THeader::ParseImageBlock(std::function<bool(uint8_t*,size_t)>& ReadBytes,TCallbacks& Callbacks,std::function<void(const TImageBlock&)>& OnImageBlock)
{
	/*
	auto& StreamBuffer = Callbacks.mStreamBuffer;
	
	//	read byte wrapper so we can unpop
	size_t ReadCount = 0;
	std::function<bool(uint8_t*,size_t)> ReadBytes = [&](uint8_t* Buffer,size_t BufferSize)
	{
		if ( !StreamBuffer.Pop( Buffer, BufferSize ) )
			return false;
		ReadCount += BufferSize;
		return true;
	};
	auto Unpop = [&]()
	{
		StreamBuffer.Unpop(ReadCount);
		return TDecodeResult::NeedMoreData;
	};
	*/
	auto& OnDebug = Callbacks.OnDebug;

	OnDebug("Reading image block header...");
	uint8_t HeaderBytes[9];
	if ( !ReadBytes( HeaderBytes, sizeof(HeaderBytes) ) )
	{
		OnDebug("Missing bytes for image block header");
		return TDecodeResult::NeedMoreData;
	}
	
	mPendingImageBlock.mLeft = HeaderBytes[0] | (HeaderBytes[1]<<8);
	mPendingImageBlock.mTop = HeaderBytes[2] | (HeaderBytes[3]<<8);
	mPendingImageBlock.mWidth = HeaderBytes[4] | (HeaderBytes[5]<<8);
	mPendingImageBlock.mHeight = HeaderBytes[6] | (HeaderBytes[7]<<8);
	
	auto Flags = HeaderBytes[8];
	auto HasLocalPalette = (Flags >> 7) & BITCOUNT(1);
	auto Interlacted = (Flags >> 6) & BITCOUNT(1);
	//auto SortedPalette = (Flags >> 5) & BITCOUNT(1);
	//auto Reserved = (Flags >> 3) & BITCOUNT(2);
	auto PaletteSize = (Flags >> 0) & BITCOUNT(3);
	PaletteSize = 2 << (PaletteSize);
	
	if ( Interlacted )
	{
		OnDebug("Interlaced gif not supported");
		return TDecodeResult::Error;
	}
	
	//	read palette
	if ( HasLocalPalette && PaletteSize == 0 )
	{
		OnDebug("Flagged as having a global palette size, and palette size is 0");
		return TDecodeResult::Error;
	}
	
	if ( !HasLocalPalette )
		PaletteSize = 0;

	OnDebug( (String("Reading local palette x") + IntToString(PaletteSize)).c_str() );

	//auto& LocalPalette = mPendingImageBlock.mPalette.mColours;
	//auto* LocalPalette8 = &LocalPalette[0].r;
	uint8_t* LocalPalette8 = nullptr;
	if ( !ReadBytes( LocalPalette8, sizeof(TRgb8) * PaletteSize ) )
	{
		OnDebug("Missing bytes for image block palette");
		return TDecodeResult::NeedMoreData;
	}
	
	uint8_t LzwMinimumCodeSize;
	if ( !ReadBytes( &LzwMinimumCodeSize, 1 ) )
		return TDecodeResult::NeedMoreData;
	
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

	mPendingImageBlock.mLzwDecoder.Init(LzwMinimumCodeSize);
	mHasPendingImageBlock = true;
	mPendingImageBlock.mCurrentRow = 0;
	return TDecodeResult::Finished;
}


TDecodeResult::Type Gif::THeader::ParseExtensionBlock(std::function<bool(uint8_t*,size_t)>& ReadBytes,TCallbacks& Callbacks)
{
	auto& OnError = Callbacks.OnError;
	
	//	http://www.onicos.com/staff/iz/formats/gif.html
	uint8_t Type;	//	label
	if ( !ReadBytes( &Type, 1 ) )
	{
		OnError("Error getting next application type (ood)");
		return TDecodeResult::NeedMoreData;
	}
	
	mPendingExtensionBlockType = Type;
	return TDecodeResult::Finished;
}

TDecodeResult::Type Gif::THeader::ParseExtensionBlockChunk(TStreamBuffer& StreamBuffer)
{
	//	blocks defined by length
	while ( true )
	{
		uint8_t BlockSize;
		if ( !StreamBuffer.Pop( &BlockSize, 1 ) )
			return TDecodeResult::NeedMoreData;
		
		//	block terminator
		if ( BlockSize == 0 )
			break;
		
		if ( !StreamBuffer.Pop( nullptr, BlockSize ) )
		{
			//	unpop the block size
			StreamBuffer.Unpop(1);
			return TDecodeResult::NeedMoreData;
		}
	}
	
	return TDecodeResult::Finished;
}
