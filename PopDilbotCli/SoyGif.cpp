#include "SoyGif.h"
#include <exception>


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







