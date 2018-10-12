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
//#include "SoyGif.h"

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
	mChannels = 3;
	mPixels = stbi_load_from_memory( GifData.data(), GifData.size(), &mWidth, &mHeight, &mChannels, mChannels );

	//	test
	int v = 0;
	for  ( int x=0;	x<mWidth;	x++)
	for  ( int x=0;	x<mWidth;	x++)
	for  ( int x=0;	x<mWidth;	x++)
	v +=mPixels[ (x + (y * mWidth)) * mChannels]
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


