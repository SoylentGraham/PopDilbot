//	using https://github.com/ZinggJM/GxEPD/
//	commit b59ae551a18b8e2dbeeed1bb62f8197e15554be9
#include "SoyGif.h"

//#define DISPLAY_ENABLED

#if defined(DISPLAY_ENABLED)
#include <GxEPD.h>

// select the display class to use, only one
//#include <GxGDEP015OC1/GxGDEP015OC1.h>    // 1.54" b/w
//#include <GxGDEW0154Z04/GxGDEW0154Z04.h>  // 1.54" b/w/r 200x200
//#include <GxGDEW0154Z17/GxGDEW0154Z17.h>  // 1.54" b/w/r 152x152
//#include <GxGDE0213B1/GxGDE0213B1.h>      // 2.13" b/w
//#include <GxGDEW0213Z16/GxGDEW0213Z16.h>  // 2.13" b/w/r
//#include <GxGDEH029A1/GxGDEH029A1.h>      // 2.9" b/w
#include <GxGDEW029Z10/GxGDEW029Z10.h>    // 2.9" b/w/r
//#include <GxGDEW027C44/GxGDEW027C44.h>    // 2.7" b/w/r
//#include <GxGDEW027W3/GxGDEW027W3.h>      // 2.7" b/w
//#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w
//#include <GxGDEW042Z15/GxGDEW042Z15.h>    // 4.2" b/w/r
//#include <GxGDEW0583T7/GxGDEW0583T7.h>    // 5.83" b/w
//#include <GxGDEW075T8/GxGDEW075T8.h>      // 7.5" b/w
//#include <GxGDEW075Z09/GxGDEW075Z09.h>    // 7.5" b/w/r

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#endif

// for SPI pin definitions see e.g.:
// C:\Users\xxx\AppData\Local\Arduino15\packages\arduino\hardware\avr\1.6.21\variants\standard\pins_arduino.h

//#define BOARD_NANO
//#define BOARD_NODEMCU
#define BOARD_MKR1010
#if defined(DISPLAY_ENABLED)
#include <Fonts/FreeMonoBold9pt7b.h>
#endif

#if defined(BOARD_NANO)
#if defined(DISPLAY_ENABLED)
GxIO_Class io(SPI, /*CS=*/ SS, /*DC=*/ 9, /*RST=*/ 8); // arbitrary selection of 8, 9 selected for default of GxEPD_Class
GxEPD_Class display(io,8,7 /*RST=9*/ /*BUSY=7*/); // default selection of (9), 7
#endif
/*
[ 2]
[ 3] 
[ 4] 
[ 5] 
[ 6] 
[ 7]purple/busy
[ 8]white/RST
[ 9]green/DC
[10]SS/orange/CS
[11]MOSI/blue/din 	[3v] red
[12] 				[13]SCK/yellow/CLK
 */

#elif defined(BOARD_NODEMCU)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#if defined(DISPLAY_ENABLED)
GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io /*RST=D4*/ /*BUSY=D2*/); // default selection of D4(=2), D2(=4)
#endif
/*
[ 0]
[ 1] 
[ 2] purple/busy
[ 3] green/DC
[ 4] white/RST
[3v] red
[gd] ground
[ 5] SCK/yellow/CLK
[ 6]
[ 7] MOSI/blue/din
[ 8] SS/orange/CS
 */
#elif defined(BOARD_MKR1010)
//#include <WiFi101.h>
//#include <WiFiClient.h>
#include <WiFiNINA.h>
#if defined(DISPLAY_ENABLED)
GxIO_Class io(SPI, 7, 2, 1); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io,1,10); // default selection of D4(=2), D2(=4)
#endif

#else
#error define a board!
#endif


#if !defined(DISPLAY_ENABLED)
#define GxEPD_WHITE	' '
#define GxEPD_BLACK	'#'
#define GxEPD_RED	';'
class TDisplayStub
{
public:
	void	init(int)			{}
	void	fillScreen(int)		{}
	void	setRotation(int)	{}
	void	update()			{}
	void	drawPixel(int,int,char p)	{	Serial.print(p);	}
};
TDisplayStub display;
#endif

//	https://github.com/ZinggJM/GxEPD/issues/54 :|
#define ROTATION_0		0
#define ROTATION_90		1
#define ROTATION_180	2
#define ROTATION_270	3


template<size_t SIZE>
class LittleString
{
public:
	LittleString<SIZE>&	Add(const char* Text);
	LittleString<SIZE>&	Add(const String& Text)
	{
		return Add( Text.c_str() );
	}
	template<size_t OTHERSIZE>
	LittleString<SIZE>&	Add(const LittleString<OTHERSIZE>& Text)
	{
		return Add( Text.c_str() );
	}

	template<typename STRING>
	LittleString<SIZE>&	operator=(const STRING& Text)
	{
		Clear();
		return Add(Text);
	}

	int					length() const	{	return mUsed;	}
	LittleString<SIZE>&	Clear()			{	mUsed = 0;	mBuffer[mUsed] = 0;	return *this;	}
	const char*			c_str() const	{	return &mBuffer[0];	}

	uint16_t			mUsed = 0;
	char				mBuffer[SIZE] = {0};
};

typedef void(*TDebugFunc)(const char*);

class TUrl
{
public:
	const char*	mHost = "mHost";
	const char*	mPath = "mPath";
	int		mPort = 80;
};

//	should be using SoyProtocols
class THttpFetch
{
public:
	bool	IsHeaderComplete()	{	return mGotHeaders;	}
	bool	HasError()			{	return mError.length() > 0;	}

	void	Push(char Char,TDebugFunc& Debug);

	void	SetChunkedEncoding()
	{
		mChunkedEncoding = true;
		mChunkLength = 0;
		mChunkFirst = true;
	}
	void	OnReadContentByte()	{	mChunkLength--;	}
	
public:
	LittleString<60>	mError;
	bool	mGotInitialResponse = false;
	String	mCurrentHeaderLine = String();
	bool	mGotHeaders = false;
	//String	mMimeType = String();
	bool		mChunkedEncoding = false;
	uint32_t	mChunkLength;
	bool		mChunkFirst;	
};


template<size_t SIZE>
LittleString<SIZE>&	LittleString<SIZE>::Add(const char* Text)
{
	//	walk back over last terminator
	if ( mUsed > 0 )
		mUsed--;
	while ( true )
	{
		mBuffer[mUsed] = *Text;
		mUsed = std::min<uint16_t>( SIZE-1, mUsed+1 );
		if ( *Text == 0 )
			break;
		Text++;
	}
	//	ensure terminator exists
	mBuffer[mUsed] = 0;
	return *this;
}
	

//	returns false if not ending with \r\n
bool TrimLineFeed(String& Line)
{
	auto Length = Line.length();
	if ( Length < 2 )
		return false;
	if ( Line[Length-1] != '\n' )
		return false;
	if ( Line[Length-2] != '\r' )
		return false;
	Line.remove( Length-2, 2 );
	return true;
}

namespace TState
{
	enum Type
	{
		ConnectToWifi=0,
		GetGifUrl,
		ConnectToGifUrl,
		GetGifHttpHeaders,
		ParseGif,
		DisplayGif,
		DisplayError,

		COUNT
	};
}

typedef TState::Type(*TUpdateFunc)(bool);


class TApp
{
public:
	TApp()	{}
	
	TState::Type	Update_ConnectWifi(bool FirstCall);
	TState::Type	Update_GetGifUrl(bool FirstCall);
	TState::Type	Update_ConnectToGifUrl(bool FirstCall);
	TState::Type	Update_GetGifHttpHeaders(bool FirstCall);
	TState::Type	Update_ParseGif(bool FirstCall);
	TState::Type	Update_DisplayGif(bool FirstCall);
	
	template<typename STRING>
	TState::Type	OnError(const STRING& Error)
	{
		mError = Error;
		Debug( mError.c_str() );
		return TState::DisplayError;
	}
	
	TDecodeResult::Type	EatChunkedHeader(THttpFetch& Fetch);
	TDecodeResult::Type	ReadMoreHttpContentBytes(THttpFetch& Fetch);

public:
	LittleString<40>	mError;
	const char*	mWifiSsid = nullptr;
	const char*	mWifiPassword = nullptr;

	WiFiClient	mWebClient;
	TUrl		mGifUrl;
	THttpFetch	mGifFetch;
	
	TDebugFunc	Debug = nullptr;

	Gif::THeader	mGifHeader;
	TStreamBuffer	mStreamBuffer;
};



TState::Type TApp::Update_ConnectWifi(bool FirstCall)
{
	if ( !FirstCall )
	{
		Debug("Not first connect wifi attempt, pausing for 1000ms");
		delay(1000);
	}

	Debug("Connecting to wifi...");
	Debug( LittleString<40>().Add("Connecting to wifi: ").Add(mWifiSsid).c_str() );
	auto Status = WiFi.begin( mWifiSsid, mWifiPassword );
	if ( Status == WL_CONNECTED )
	{
		Debug("Wifi connected");
		return TState::GetGifUrl;
	}

	Debug( (String("Wifi status is ") + IntToString(Status)).c_str() );

	return TState::ConnectToWifi;
}


TState::Type TApp::Update_GetGifUrl(bool FirstCall)
{
	mGifUrl.mHost = "assets.amuniversal.com";
	mGifUrl.mPath = "/a2bb8780a4c401365a02005056a9545d";

	//mGifUrl.mHost = "i.giphy.com";
	//mGifUrl.mPath = "/media/UaoxTrl8z1wre/giphy.gif";
	
	mGifUrl.mHost = "assets.amuniversal.com";
	mGifUrl.mPath = "/ac10d37065d50135e432005056a9545d";
	
	return TState::ConnectToGifUrl;
}

TState::Type TApp::Update_ConnectToGifUrl(bool FirstCall)
{
	if ( FirstCall )
	{
		//	reset old connection
		mWebClient.stop();
	}

	{
		String str;
		str += "Connecting to ";
		str += mGifUrl.mHost;
		str += ":";
		str += mGifUrl.mPort;
		Debug(str.c_str());
	}

	if ( !mWebClient.connect( mGifUrl.mHost, mGifUrl.mPort ) )
		return OnError( String("Failed to connect to ") + mGifUrl.mHost + ":" + IntToString(mGifUrl.mPort) );

	Debug( LittleString<60>().Add("Connected to ").Add(mGifUrl.mHost).c_str() );
	return TState::GetGifHttpHeaders;
}

TState::Type TApp::Update_GetGifHttpHeaders(bool FirstCall)
{
	if ( FirstCall )
	{
		Debug("Update_GetGifHttpHeaders");
		mGifFetch = THttpFetch();

		//	send request
		Debug("Sending GET request...");
		String Req;
		
		Req = String("GET ") + String(mGifUrl.mPath) + " HTTP/1.1";
		Debug(Req.c_str());
		mWebClient.println(Req);
		
		Req = String("Host: ") + mGifUrl.mHost;
		Debug(Req.c_str());
		mWebClient.println(Req);
		
		mWebClient.println("Connection: close");
		mWebClient.println();
		
		/*
    	mWebClient.println(String("GET ") + mGifUrl.mPath + " HTTP/1.1");
    	mWebClient.println(String("Host: ") + mGifUrl.mHost );
    	mWebClient.println("Connection: close");
    	mWebClient.println();
    	*/
    	Debug("Sent GET request");
	}
	
	//	read bytes as availible, when they're not, let the loop return
	if ( !mWebClient.connected() )
		return OnError("WebClient no longer connected");

	//	read more bytes
	while ( mWebClient.available() )
	{
		char NextChar;
		auto ReadCount = mWebClient.readBytes(&NextChar,1);
		if ( ReadCount == 0 )
			break;
		mGifFetch.Push( NextChar, Debug );
		
		if ( mGifFetch.IsHeaderComplete() )
		{
			Debug("read headers");
			//if ( mGifFetch.mMimeType != "image/gif" )
				//return OnError( String("Mime is not image/gif, is ") + mGifFetch.mMimeType );
			return TState::ParseGif;
		}

		if ( mGifFetch.HasError() )
		{
			Debug("header error");
			return OnError( mGifFetch.mError );
		}
	}
	
	Debug("Waiting for more data...");
	delay(100);
	return TState::GetGifHttpHeaders;
}


TDecodeResult::Type TApp::EatChunkedHeader(THttpFetch& Fetch)
{
	if ( !Fetch.mChunkedEncoding )
		return TDecodeResult::Finished;

	//	more of last chunk still to go
	if ( Fetch.mChunkLength > 0 )
		return TDecodeResult::Finished;
	
	//	if not the first header, we need to read the tailing \r\n
	if ( !Fetch.mChunkFirst )
	{
		if ( mWebClient.available() < 2 )
			return TDecodeResult::NeedMoreData;

		auto TerminatorLineFeed = mWebClient.readStringUntil('\n');
		TerminatorLineFeed+='\n';
		if ( !TrimLineFeed(TerminatorLineFeed) )
		{
			Debug("Chunk tail didn't end with \\r\\n (maybe timeout?)");
			return TDecodeResult::Error;
		}
		
		//	this should now be zero length
		if ( TerminatorLineFeed.length() != 0 )
		{
			Debug("Chunk tail length not zero");
			return TDecodeResult::Error;
		}
	}
	
	//	blocking-read next chunk size
	//	<lengthinhex>\r\n
	auto ChunkHeader = mWebClient.readStringUntil('\n');
	ChunkHeader+='\n';
	if ( !TrimLineFeed(ChunkHeader) )
	{
		Debug("Chunk header didn't end with \\r\\n (maybe timeout?)");
		return TDecodeResult::Error;
	}
	Fetch.mChunkFirst = false;

	//	read new size
	Fetch.mChunkLength = 0;
	for ( int i=0;	i<ChunkHeader.length();	i++ )
	{
		auto NextByte = ChunkHeader[i];
		//	not hex!
		if ( NextByte >= 'A' && NextByte <= 'F' )
			NextByte = (NextByte - 'A') + 0xA;
		else if ( NextByte >= 'a' && NextByte <= 'f' )
			NextByte = (NextByte - 'a') + 0xA;
		else if ( NextByte >= '0' && NextByte <= '9' )
			NextByte -= '0';
		else
		{
			if ( NextByte == ';' )
			{
				Debug("Http chunked extensions not supported");
				return TDecodeResult::Error;
			}
			Debug("GetChunkLength not hex");
			return TDecodeResult::Error;
		}

		//	shift up last hex
		Fetch.mChunkLength <<= 4;
		Fetch.mChunkLength |= NextByte;
	}

	{
		String DebugString;
		DebugString += "Read next chunk length: ";
		DebugString += IntToString( Fetch.mChunkLength );
		Debug(DebugString.c_str());
	}
	return TDecodeResult::Finished;
}


TDecodeResult::Type TApp::ReadMoreHttpContentBytes(THttpFetch& Fetch)
{
	while ( true )
	{
		if ( !mWebClient.available() )
			break;
		if ( !mStreamBuffer.HasSpace() )
			break;

		auto ChunkResult = EatChunkedHeader(Fetch);
		if ( ChunkResult != TDecodeResult::Finished )
			return ChunkResult;
	
		char NextChar;
		auto ReadCount = mWebClient.readBytes(&NextChar,1);
		if ( ReadCount == 0 )
		{
			Debug("Unexpectedly read zero bytes from webclient");
			return TDecodeResult::Error;
		}
		Fetch.OnReadContentByte();

		if ( !mStreamBuffer.Push( NextChar ) )
		{
			Debug("Unexpectedly failed to push byte to streambuffer");
			return TDecodeResult::Error;
		}
	}

	return TDecodeResult::Finished;
}

TState::Type TApp::Update_ParseGif(bool FirstCall)
{
	if ( FirstCall )
	{
		Debug("Initialising gif setup");
		mStreamBuffer = TStreamBuffer();
		//mGifHeader = Gif::THeader();
		display.fillScreen(GxEPD_WHITE);
	}

	//	read more bytes into the stream buffer
	auto ReadResult = ReadMoreHttpContentBytes( mGifFetch );
	if ( ReadResult == TDecodeResult::Error )
		return OnError("Error reading more http content");


	auto DrawImageBlock = [](const TImageBlock& ImageBlock)
	{
		auto DrawColor = [&](int x,int y,TRgba8 Colour)
		{
			auto Luma = std::max( Colour.r, std::max( Colour.g, Colour.b ) );
			if ( Luma > 200 )
				display.drawPixel( x, y, GxEPD_WHITE );
			else if ( Luma > 120 )
				display.drawPixel( x, y, GxEPD_RED );
			else
				display.drawPixel( x, y, GxEPD_BLACK );
		};

		for ( int y=0;	y<ImageBlock.mHeight;	y++ )
		for ( int x=0;	x<ImageBlock.mWidth;	x++ )
		{
			auto p0 = ImageBlock.mPixels[x];
			auto Colour0 = ImageBlock.GetColour(p0);
			DrawColor( x, y, Colour0 );
		}
		Serial.println();
	};	

	TCallbacks Callbacks( mStreamBuffer );
	Callbacks.OnDebug = [&](const char* t)	{	Debug(t);	};
	Callbacks.OnError = Callbacks.OnDebug;

	auto Result = Gif::ParseGif( mGifHeader, Callbacks, DrawImageBlock );
	if ( Result == TDecodeResult::Error )
		return OnError("ParseGif error");
		
	if ( Result == TDecodeResult::Finished )
		return TState::DisplayGif;
		
	//	need more data in the buffer
	//	gr: or need to re-process, in lzw case, we don't even need more bytes sometimes
	if ( Result == TDecodeResult::NeedMoreData )
	{
		/*
		if ( !mWebClient.available() && !mWebClient.connected() )
		{
			Debug( (String("data left in buffer x") + IntToString(mStreamBuffer.GetBufferSize() )).c_str() );
			return OnError("WebClient no longer connected");
		}	
		*/
		//Debug( (String("need more, left in buffer x") + IntToString(mStreamBuffer.GetBufferSize() )).c_str() );
		return TState::ParseGif;
	}
	
	return OnError("ParseGif unexpected result");
}

TState::Type TApp::Update_DisplayGif(bool FirstCall)
{
	if ( FirstCall )
	{
		Debug("Refreshing display");
		display.update();
		auto SleepAfterDisplaySecs = 30;
		Debug("Now sleeping for X secs");
		delay( 1000 * SleepAfterDisplaySecs );
	}

	return TState::ConnectToWifi;
}


template<typename STATETYPE>
class TStateMachine
{
public:
	void		Update(TDebugFunc& Debug);

	STATETYPE	mCurrentState;
	bool		mCurrentStateFirstCall = true;

	//	gr: this seems to solve dodgy execution... not sure if it eats up a lot of ram?
	TUpdateFunc	mUpdates[STATETYPE::COUNT];
	//std::function<STATETYPE(bool)>	mUpdates[STATETYPE::COUNT];
};

template<typename STATETYPE>
void TStateMachine<STATETYPE>::Update(TDebugFunc& Debug)
{
	/*
	for ( int i=0;	i<1000;	i++ )
	{
		Serial.println( String("Hello ") + IntToString(i) );
	}
	*/
	//Debug( (String("current state = ") + IntToString(mCurrentState)).c_str() );
	//delay(1000*1);
	
	auto& StateUpdate = mUpdates[mCurrentState];
	if ( StateUpdate == nullptr )
	{
		Debug( (String("Null state #") + IntToString(mCurrentState)).c_str() );
		delay(1000*10);
		return;
	}
	auto NewState = StateUpdate( mCurrentStateFirstCall );
	mCurrentStateFirstCall = false;
	if ( NewState != mCurrentState )
	{
		mCurrentState = NewState;
		mCurrentStateFirstCall = true;
	}
}

	
void THttpFetch::Push(char Char,TDebugFunc& Debug)
{
	mCurrentHeaderLine += Char;

	//	waiting for EOL's
	if ( Char != '\n' )
		return;

	//	process current buffer
	const char* ContentTypeKey = "Content-Type: ";
	const char* ChunkedEncodingKey = "Transfer-Encoding: chunked";
	const char* Response11_200Key = "HTTP/1.1 200";
	const char* Response10_200Key = "HTTP/1.0 200";

	//	waiting for initial response
	if ( !mGotInitialResponse )
	{
		if ( mCurrentHeaderLine.startsWith(Response11_200Key) )
		{
			
		}
		else if ( mCurrentHeaderLine.startsWith(Response10_200Key) )
		{
			
		}
		else
		{
			mError = "Unxpected http response: ";
			mError.Add(mCurrentHeaderLine);
			return;
		}
		mGotInitialResponse = true;
		mCurrentHeaderLine = String();
		return;		
	}

	if ( !TrimLineFeed(mCurrentHeaderLine) )
	{
		mError = "Header line didn't end with proper line feed";
		return;
	}
		
	//	process header line
	if ( mCurrentHeaderLine.startsWith(ChunkedEncodingKey) )
	{
		SetChunkedEncoding();
	}
	else if ( mCurrentHeaderLine.startsWith(ContentTypeKey) )
	{		
		//mMimeType = mCurrentHeaderLine.substring( strlen(ContentTypeKey) );
	}
	else if ( mCurrentHeaderLine.length() == 0 )
	{
		mGotHeaders = true;
	}
	else
	{
		//Debug("Skipped header: [" + mCurrentHeaderLine  + "]x" + IntToString(mCurrentHeaderLine.length()));
		Debug("Skipped header: ");
		Debug( mCurrentHeaderLine.c_str() );
	}

	//	done with this header line
	mCurrentHeaderLine = String();
}





















TApp App;
TStateMachine<TState::Type> AppState;


void setup()
{
	#define SERIAL_BAUD	57600
	delay(1000);
	Serial.begin(SERIAL_BAUD);
	Serial.println();
	
	Serial.println("Initialising display...");
	display.init(SERIAL_BAUD);	// enable diagnostic output on Serial
	display.setRotation(ROTATION_90);


	App.mWifiSsid = "ZaegerMeister";
	App.mWifiPassword = "InTheYear2525";
	App.Debug = [&](const char* Text)
	{
		Serial.println(Text);
	};

	AppState.mUpdates[TState::ConnectToWifi] = [&](bool FirstCall)		{	return App.Update_ConnectWifi(FirstCall);	};
	AppState.mUpdates[TState::GetGifUrl] = [&](bool FirstCall)			{	return App.Update_GetGifUrl(FirstCall);	};
	AppState.mUpdates[TState::ConnectToGifUrl] = [&](bool FirstCall)	{	return App.Update_ConnectToGifUrl(FirstCall);	};
	AppState.mUpdates[TState::GetGifHttpHeaders] = [&](bool FirstCall)	{	return App.Update_GetGifHttpHeaders(FirstCall);	};
	AppState.mUpdates[TState::ParseGif] = [&](bool FirstCall)			{	return App.Update_ParseGif(FirstCall);	};
	AppState.mUpdates[TState::DisplayGif] = [&](bool FirstCall)			{	return App.Update_DisplayGif(FirstCall);	};
	//AppState.mUpdates[TState::DisplayError] = [&](bool FirstCall)		{	return App.Update_DisplayError(FirstCall);	};
	AppState.mCurrentState = TState::ConnectToWifi;
}

void loop()
{
	//delay(500);
	//App.Debug("Loop");
	AppState.Update( App.Debug );
}


#if defined(BOARD_MKR1010)
namespace std
{
	#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
	void __throw_bad_function_call()
	{
		
		//throw bad_function_call("invalid function called");
		App.Debug("Bad function called");
	}
	#pragma GCC diagnostic pop
}
#endif
