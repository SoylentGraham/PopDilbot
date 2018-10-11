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
	
	auto GetPx0 = [&](int x,int y)
	{
		auto PixelIndex = x + (y * w);
		auto* Pixel = &Bytes[PixelIndex * Channels];
		return Pixel[0];
	};
	auto x = w * u;
	auto y = h * v;
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


void RenderCli(TImageSampler& Sampler,int Width,int Height)
{
	auto GetPaletteChar = [](uint8_t Colour)
	{
		switch (Colour)
		{
			case 0:		return ' ';
			case 1:		return '#';
			default:	return 'H';
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
		GetUrl("https://assets.amuniversal.com/9cde8ef0a4c401365a02005056a9545d",OnDownloaded);
		
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
		TImageBufferTest Test( 4, 4, Square4x4 );
		RenderCli( Test, 80, 20 );
	}
	return 0;
}


