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
#include <sstream>
#include "SoyGif.h"

void WDT_YEILD()
{
	
}

void OutputGif(const std::vector<uint8_t>& GifData_)
{
	std::vector<uint8_t> GifData = GifData_;
	
	TStreamBuffer StreamBuffer;
	auto OnDebug = [](const String& Text)
	{
		std::cout << Text << std::endl;
	};
	
	auto OnError = [](const String& Text)
	{
		throw std::runtime_error(Text);
	};
	
	{
		std::stringstream Debug;
		Debug << "Downloaded " << GifData.size() << " bytes";
		OnDebug( Debug.str() );
	}
	
	auto DrawImageBlock = [](const TImageBlock& ImageBlock)
	{
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
		auto SubSample = 2;
		if ( ImageBlock.mTop % SubSample != 0 )
			return;
		auto Width = std::min<int>( ImageBlock.mWidth, 400*SubSample );
		//auto Width = std::min<int>( std::min<int>( ImageBlock.mWidth, 240*SubSample ), 200 );
		
		for ( int x=0;	x<Width;	x+=SubSample )
		{
			auto x0 = std::min( Width-1, x+0 );
			auto x1 = std::min( Width-1, x+1 );
			auto x2 = std::min( Width-1, x+2 );
			auto x3 = std::min( Width-1, x+3 );
			auto p0 = ImageBlock.mPixels[x0];
			auto p1 = ImageBlock.mPixels[x1];
			auto p2 = ImageBlock.mPixels[x2];
			auto p3 = ImageBlock.mPixels[x3];
			
			if ( SubSample == 1 )
				DrawColor( ImageBlock.GetColour(p0) );
			else if ( SubSample == 2 )
				DrawColor2( p0, p1 );
			else if ( SubSample == 3 )
				DrawColor3( p0, p1, p2 );
			else if ( SubSample == 4 )
				DrawColor4( p0, p1, p2, p3 );
		}
		std::cout << std::endl;
	};
	
	Gif::THeader GifHeader;
	TCallbacks Callbacks( StreamBuffer );
	Callbacks.OnError = OnError;
	Callbacks.OnDebug = OnDebug;
	
	while ( true )
	{
		auto Result = Gif::ParseGif( GifHeader, Callbacks, DrawImageBlock );
		
		if ( Result == TDecodeResult::Error )
			throw std::runtime_error("ParseGif error");
		
		if ( Result == TDecodeResult::Finished )
			break;
		
		//	need more data in the buffer
		if ( Result == TDecodeResult::NeedMoreData )
		{
			for ( int i=0;	i<100 && GifData.size()>0;	i++ )
			{
				auto Byte = GifData[0];
				if ( !StreamBuffer.Push(Byte) )
				{
					//throw std::runtime_error("Streambuffer overflowed");
					break;
				}
				GifData.erase( GifData.begin() );
			}
		}
		
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
		//
		//GetUrl("https://assets.amuniversal.com/985bb260a4c401365a02005056a9545d",OnDownloaded);
		//GetUrl("http://i.giphy.com/media/UaoxTrl8z1wre/giphy.gif",OnDownloaded);
		GetUrl("http://assets.amuniversal.com/ac10d37065d50135e432005056a9545d",OnDownloaded);

		
		
		while ( !Downloaded )
		{
			NSLog(@"Waiting for download...");
			std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
		}
		
		OutputGif( GifData );
	}
	return 0;
}


