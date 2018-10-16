#include "SoyGif.h"


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

void Gif::THeader::ParseImageBlock(std::function<bool(uint8_t*,int)> ReadBytes,std::function<void(const char*)> OnError,std::function<void(const TImageBlock&)> OnImageBlock)
{
	uint8_t HeaderBytes[9];
	if ( !ReadBytes( HeaderBytes, sizeof(HeaderBytes) ) )
	{
		OnError("Missing bytes for image block header");
		return;
	}
	
	TImageBlock Block;
	Block.mLeft = HeaderBytes[0] | (HeaderBytes[1]<<8);
	Block.mTop = HeaderBytes[2] | (HeaderBytes[3]<<8);
	Block.mWidth = HeaderBytes[4] | (HeaderBytes[5]<<8);
	Block.mHeight = HeaderBytes[6] | (HeaderBytes[7]<<8);
	
	auto Flags = HeaderBytes[8];
	auto HasLocalPalette = (Flags >> 7) & BITCOUNT(1);
	auto Interlacted = (Flags >> 6) & BITCOUNT(1);
	auto SortedPalette = (Flags >> 5) & BITCOUNT(1);
	auto Reserved = (Flags >> 3) & BITCOUNT(2);
	auto PaletteSize = (Flags >> 0) & BITCOUNT(3);
	PaletteSize = 2 << (PaletteSize);
	
	//	read palette
	if ( HasLocalPalette && PaletteSize == 0 )
	{
		OnError("Flagged as having a global palette size, and palette size is 0");
		return;
	}
		
	TRgb8 LocalPalette[256];
	auto* LocalPalette8 = &LocalPalette[0].r;
	if ( !ReadBytes( LocalPalette8, sizeof(TRgb8) * PaletteSize ) )
	{
		OnError("Missing bytes for image block palette");
		return;
	}
	
	uint8_t LzwMinimumCodeSize;
	ReadBytes( &LzwMinimumCodeSize, 1 );
	
	//	read lzw blocks
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
	
}

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




