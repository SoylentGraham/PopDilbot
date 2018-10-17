//
//  main.m
//  PopDilbotCli
//
//  Created by graham on 11/10/2018.
//  Copyright © 2018 NewChromantics. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <thread>
#include <vector>
#include <iostream>
#include "SoyGif.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


class TImageSampler
{
public:
	virtual uint8_t	GetPixel(float u,float v)=0;
};

class TImageBuffer : public TImageSampler
{
public:
	virtual uint8_t			GetPixel(float u,float v) override;
	virtual int				GetWidth()=0;
	virtual int				GetHeight()=0;
	virtual int				GetChannelCount()=0;
	virtual const uint8_t*	GetBytes()=0;
};


uint8_t TImageBuffer::GetPixel(float u,float v)
{
	auto w = GetWidth();
	auto h = GetHeight();
	auto* Bytes = GetBytes();
	auto Channels = GetChannelCount();
	
	auto GetPx0 = [=](int x,int y)
	{
		auto PixelIndex = x + (y * w);
		auto* Pixel = &Bytes[PixelIndex * Channels];
		
		//	get luma
		float Luma = 0;
		for ( int c=0;	c<std::min(3,Channels);	c++)
			Luma += Pixel[c] / 255.0f;
		Luma /= std::min(3,Channels);
		
		if ( Luma < 0.1f )
		return 0;
		if ( Luma < 0.7f )
		return 2;
		return 1;
	};
	auto x = w * u;
	auto y = h * v;
	x = std::min<int>( x, w-1 );
	y = std::min<int>( y, h-1 );
	return GetPx0(x,y);
}


class TImageBufferTest : public TImageBuffer
{
public:
	TImageBufferTest(int Width,int Height,const uint8_t* Pixels) :
		mPixels	( Pixels ),
		mWidth	( Width ),
		mHeight	( Height )
	{
	}
	
	virtual int				GetWidth() override	 {	return mWidth;	}
	virtual int				GetHeight() override	 {	return mHeight;	}
	virtual int				GetChannelCount() override	{	return 1;	}
	virtual const uint8_t*	GetBytes() override	{	return mPixels;	}
	
	const uint8_t*		mPixels;
	int 				mWidth;
	int					mHeight;
};


class TImageBufferGif : public TImageBuffer
{
public:
	TImageBufferGif(const std::vector<uint8_t>& GifData);
	~TImageBufferGif();
	
	virtual int				GetWidth() override	 {	return mWidth;	}
	virtual int				GetHeight() override	 {	return mHeight;	}
	virtual int				GetChannelCount() override	{	return mChannels;	}
	virtual const uint8_t*	GetBytes() override	{	return mPixels;	}
	
	int			mWidth;
	int			mHeight;
	int			mChannels;
	uint8_t*	mPixels;
};


TImageBufferGif::TImageBufferGif(const std::vector<uint8_t>& GifData)
{
	//	desired channels
	//mChannels = 3;
	//mPixels = stbi_load_from_memory( GifData.data(), GifData.size(), &mWidth, &mHeight, &mChannels, mChannels );


	size_t BytesRead = 0;
	auto ReadBytes = [&](uint8_t* Buffer,int BufferSize)
	{
		//	unpop
		if ( BufferSize < 0 )
		{
			BytesRead += BufferSize;
			return true;
		}
		if ( BytesRead + BufferSize > GifData.size() )
			return false;
		
		//	walking over data if null
		if ( Buffer != nullptr )
		{
			for ( int i=0;	i<BufferSize;	i++ )
			{
				Buffer[i] = GifData[BytesRead+i];
			}
		}
		BytesRead += BufferSize;
		return true;
	};
	
	auto OnError = [&](const char* Error)
	{
		std::cout << Error << std::endl;
		throw std::runtime_error(Error);
	};
	
	Gif::THeader Header( ReadBytes, OnError );
	std::cout << "read gif " << Header.mWidth << "x" << Header.mHeight << std::endl;

	auto OnGraphicControlBlock = []
	{
	};
	auto OnCommentBlock = []
	{
	};
	auto OnImageBlock = [](const TImageBlock& ImageBlock)
	{
		if ( ImageBlock.mTop % 4 != 0 )
			return;
		//	draw colours
		auto DrawColor = [&](TRgba8 Colour)
		{
			auto Luma = std::max( Colour.r, std::max( Colour.g, Colour.b ) );
			if ( Luma > 200 )
				std::cout << " ";
			else if ( Luma > 120 )
				std::cout << ";";
			else
				std::cout << "#";
		};
		//	subsampler
		auto DrawColor4 = [&](uint8_t a,uint8_t b,uint8_t c,uint8_t d)
		{
			auto Coloura = ImageBlock.GetColour(a);
			auto Colourb = ImageBlock.GetColour(b);
			auto Colourc = ImageBlock.GetColour(c);
			auto Colourd = ImageBlock.GetColour(d);
			//	get an average
			TRgba8 rgba;
			rgba.r = (Coloura.r + Colourb.r + Colourc.r + Colourd.r)/4.0f;
			rgba.g = (Coloura.g + Colourb.g + Colourc.g + Colourd.g)/4.0f;
			rgba.b = (Coloura.b + Colourb.b + Colourc.b + Colourd.b)/4.0f;
			rgba.a = 1;
			DrawColor( rgba );
		};
		auto DrawColor3 = [&](uint8_t a,uint8_t b,uint8_t c)
		{
			auto Coloura = ImageBlock.GetColour(a);
			auto Colourb = ImageBlock.GetColour(b);
			auto Colourc = ImageBlock.GetColour(c);
			//	get an average
			TRgba8 rgba;
			rgba.r = (Coloura.r + Colourb.r + Colourc.r)/3.0f;
			rgba.g = (Coloura.g + Colourb.g + Colourc.g)/3.0f;
			rgba.b = (Coloura.b + Colourb.b + Colourc.b)/3.0f;
			rgba.a = 1;
			DrawColor( rgba );
		};
		auto DrawColor2 = [&](uint8_t a,uint8_t b)
		{
			auto Coloura = ImageBlock.GetColour(a);
			auto Colourb = ImageBlock.GetColour(b);
			//	get an average
			TRgba8 rgba;
			rgba.r = (Coloura.r + Colourb.r)/2.0f;
			rgba.g = (Coloura.g + Colourb.g)/2.0f;
			rgba.b = (Coloura.b + Colourb.b)/2.0f;
			rgba.a = 1;
			DrawColor( rgba );
		};
		auto Width = std::min<int>( ImageBlock.mWidth, 300 );
		auto SubSample = 3;
		for ( int x=0;	x<Width;	x+=SubSample )
		{
			auto x0 = ImageBlock.mPixels[x+0];
			auto x1 = ImageBlock.mPixels[x+1];
			auto x2 = ImageBlock.mPixels[x+2];
			auto x3 = ImageBlock.mPixels[x+3];
			//DrawColor4( x0, x1, x2, x3 );
			DrawColor3( x0, x1, x2 );
			//DrawColor2( x0, x1 );
		}
		std::cout << std::endl;
	};
	
	while ( BytesRead < GifData.size() )
	{
		Header.ParseNextBlock( ReadBytes, OnError, OnGraphicControlBlock, OnCommentBlock, OnImageBlock );
	}
	
	/*
/*
	//	test
	int v = 0;
	for  ( int x=0;	x<mWidth;	x++)
	for  ( int x=0;	x<mWidth;	x++)
	for  ( int x=0;	x<mWidth;	x++)
	v +=mPixels[ (x + (y * mWidth)) * mChannels]
 */
}

TImageBufferGif::~TImageBufferGif()
{
	stbi_image_free( mPixels );
}

void RenderCli(TImageSampler& Sampler,int Width,int Height)
{
	auto GetPaletteChar = [](uint8_t Colour)
	{
		switch (Colour)
		{
			case 0:		return '#';
			case 1:		return ' ';
			default:	return ':';
		}
	};
	
	for ( int y=0;	y<Height;	y++ )
	{
		for ( int x=0;	x<Width;	x++ )
		{
			float u = x / (float)Width;
			float v = y / (float)Height;
			auto Pixel = Sampler.GetPixel(u,v);
			auto PixelChar = GetPaletteChar(Pixel);
			std::cout << PixelChar;
		}
		std::cout << std::endl;
	}
}


void GetUrl(const char* Url,std::function<void(NSData*)> OnFinished)
{
	auto* UrlString = [NSString stringWithUTF8String:Url];
	NSLog(@"Waiting for download...");

	// making a GET request to /init
	//NSString *targetUrl = [NSString stringWithFormat:@"%@/init", baseUrl];
	NSMutableURLRequest *request = [[NSMutableURLRequest alloc] init];
	[request setHTTPMethod:@"GET"];
	[request setURL:[NSURL URLWithString:UrlString]];
	
	[[[NSURLSession sharedSession] dataTaskWithRequest:request completionHandler:
	  ^(NSData * _Nullable data,
		NSURLResponse * _Nullable response,
		NSError * _Nullable error) {
		  
		  OnFinished( data );
	  }] resume];
}



void GetUrl(const char* Url,std::function<void(std::vector<uint8_t>)> OnFinished)
{
	auto OnDownloaded = [&](NSData* Data)
	{
		std::vector<uint8_t> Bytes;
		auto DataSize = [Data length];
		auto* DataBytes = reinterpret_cast<const uint8_t*>([Data bytes]);
		for ( auto i=0;	i<DataSize;	i++ )
		{
			Bytes.push_back( DataBytes[i] );
		}
		OnFinished( Bytes );
	};
	GetUrl( Url, OnDownloaded );
}

int main(int argc, const char * argv[])
{
	@autoreleasepool
	{
		bool Downloaded = false;
		std::vector<uint8_t> GifData;
		auto OnDownloaded = [&](std::vector<uint8_t> Data)
		{
			GifData = Data;
			Downloaded = true;
		};
		GetUrl("https://assets.amuniversal.com/985bb260a4c401365a02005056a9545d",OnDownloaded);
		
		while ( !Downloaded )
		{
			NSLog(@"Waiting for download...");
			std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
		}
		std::cout << "Downloaded " << GifData.size() << " bytes" << std::endl;

		uint8_t Square4x4[] = {
			1,1,1,1,
			1,0,0,1,
			1,0,0,1,
			1,1,1,1
		};
		//TImageBufferTest Test( 4, 4, Square4x4 );
		//RenderCli( Test, 80, 20 );
		
		TImageBufferGif Gif( GifData );
		RenderCli( Gif, 180, 150*0.30f );
	}
	return 0;
}


