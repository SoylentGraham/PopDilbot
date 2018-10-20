
#if 0


String ExtractAndRenderGif(WiFiClient& Client,std::function<void(const String&)> OnError,std::function<void(const String&)> OnDebug);



std::function<void()> OnDrawStuffFunc = nullptr;



void WDT_YEILD()
{
	#if defined(BOARD_NODEMCU)
	ESP.wdtFeed();
	#endif
	//yield();
}

void OnDrawStuffCallback()
{
	OnDrawStuffFunc();

}

void DrawStuff(std::function<void()> OnDraw)
{
	OnDrawStuffFunc = OnDraw;
	display.drawPaged( OnDrawStuffCallback );
}


void DrawText(const String& Text)
{
	auto PrintFunc = [&]()
	{
		const GFXfont* f = &FreeMonoBold9pt7b;
		display.fillScreen(GxEPD_BLACK);
		display.setTextColor(GxEPD_WHITE);
		display.setFont(f);
		display.setCursor(0, 0);
		display.println();
		display.println(Text);
	};
	DrawStuff( PrintFunc );
}

void DebugPrint(const String& Text)
{
	Serial.println(Text);
	/*
	auto PrintFunc = [&]()
	{
		const GFXfont* f = &FreeMonoBold9pt7b;
		display.fillScreen(GxEPD_BLACK);
		display.setTextColor(GxEPD_WHITE);
		display.setFont(f);
		display.setCursor(0, 0);
		display.println();
		display.println(Text);
	};
	DrawStuff( PrintFunc );
	*/
}


void FontTest()
{
	  const char* name = "FreeMonoBold9pt7b";
  const GFXfont* f = &FreeMonoBold9pt7b;
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
  display.println(name);
  display.println(" !\"#$%&'()*+,-./");
  display.println("0123456789:;<=>?");
  display.println("@ABCDEFGHIJKLMNO");
  display.println("PQRSTUVWXYZ[\\]^_");
#if defined(_GxGDEW0154Z04_H_) || defined(_GxGDEW0213Z16_H_) || defined(_GxGDEW029Z10_H_) || defined(_GxGDEW027C44_H_) || defined(_GxGDEW042Z15_H_) || defined(_GxGDEW075Z09_H_)
  display.setTextColor(GxEPD_RED);
#endif
  display.println("`abcdefghijklmno");
  display.println("pqrstuvwxyz{|}~ ");
}

//	https://github.com/ZinggJM/GxEPD/issues/54 :|
#define ROTATION_0		0
#define ROTATION_90		1
#define ROTATION_180	2
#define ROTATION_270	3

void setup()
{
	delay(1000);
	Serial.begin(115200);
	Serial.println();

	Serial.println("Initialising display...");
	display.init(115200); // enable diagnostic output on Serial
	display.setRotation(ROTATION_90);

	DebugPrint("Initialised display.");

	//	disable software watchdog
	#if defined(BOARD_NODEMCU)
	ESP.wdtDisable();
	ESP.wdtEnable(WDTO_8S);
	#endif
}


void ConnectToWifi(const String& Ssid,const String& Password,std::function<void(const String&)> Debug)
{
	Debug( String("Setting up wifi to ") + Ssid + "/" + Password);

#if defined(BOARD_NODEMCU)
	WiFi.persistent(true);
	WiFi.mode(WIFI_STA); // switch off AP
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.disconnect();
	WiFi.begin(Ssid.c_str(), Password.c_str());

	int Attempt=0;
	do
	{
		delay(500);
		if ( (Attempt % 10) == 0 )
			Debug( String("Attempt #") + Attempt + "\nStatus:" + WiFi.status());
		Attempt++;
	}
	while ( WiFi.status() != WL_CONNECTED );

#elif defined(BOARD_MKR1010)

	int Attempt=0;
	auto Status = WiFi.status();
	do
	{
		Status = WiFi.begin(Ssid.c_str(), Password.c_str());

		delay(1000);
		Debug( String("Attempt #") + Attempt + "\nStatus:" + WiFi.status());
		Attempt++;
	}
	while ( Status != WL_CONNECTED );
#endif
}



namespace TState
{
	enum Type
	{
		Start = 0,
		ConnectWifi,
		ConnectToHost,
		GetHeaders,
		ParseGif,
		DisplayImage,
		Error,

		COUNT,
	};
}


TState::Type Update_Start(bool FirstCall,std::function<void(const String&)>& Debug)
{
	Debug("Hello!");
	return TState::ConnectWifi;
}



void OldLoop()
{
		
	if ( WiFi.status() != WL_CONNECTED )
	{
		ConnectToWifi( "ZaegerMeister", "InTheYear2525", DebugPrint );
		//DrawText("Connected to wifi!");
	}
	
	//auto* Path = "/985bb260a4c401365a02005056a9545d";

	auto OnError = [&](const String& Error)
	{
		DebugPrint( Error );
		DrawText(Error);	
	};
	auto OnConnected = [&](WiFiClient& Client,bool IsContentChunked)
	{
		return ExtractAndRenderGif( Client, IsContentChunked, OnError, DebugPrint );
	};
	FetchHttp( Host, Path, "image/gif", OnConnected, OnError, DebugPrint );
	delay(100000);
}
/*

void showFontCallback()
{
  const char* name = "FreeMonoBold9pt7b";
  const GFXfont* f = &FreeMonoBold9pt7b;
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
  display.println(name);
  display.println(" !\"#$%&'()*+,-./");
  display.println("0123456789:;<=>?");
  display.println("@ABCDEFGHIJKLMNO");
  display.println("PQRSTUVWXYZ[\\]^_");
#if defined(_GxGDEW0154Z04_H_) || defined(_GxGDEW0213Z16_H_) || defined(_GxGDEW029Z10_H_) || defined(_GxGDEW027C44_H_) || defined(_GxGDEW042Z15_H_) || defined(_GxGDEW075Z09_H_)
  display.setTextColor(GxEPD_RED);
#endif
  display.println("`abcdefghijklmno");
  display.println("pqrstuvwxyz{|}~ ");
}
*/

void FetchHttp(const char* Host,const char* Path,const char* ExpectedMimeType,std::function<String(WiFiClient&,bool)> OnConnected,std::function<void(const String&)> OnError,std::function<void(const String&)> Debug)
{
	WiFiClient Client;

  	Debug(String("Fetching ") + Host + Path);
	
#define HTTP_PORT	80
	if ( !Client.connect(Host, HTTP_PORT) )
	{
		OnError( String("Failed to connect to ") + Host + HTTP_PORT );
		return;
	}

	//	send get request
	/*
	Client.print(String("GET ") + Path + " HTTP/1.1\r\n" +
               "Host: " + Host + "\r\n" +
               "User-Agent: grahamatgrahamreevesdotcom\r\n" +
               "Connection: close\r\n\r\n");
               */
    Client.println(String("GET ") + Path + " HTTP/1.1");
    Client.println(String("Host: ") + Host);
    Client.println("Connection: close");
    Client.println();

while(true)
{
	delay(10);
	yield();
	Serial.println("while (client.available()) ");
    while (Client.available()) 
    {
    	char c = Client.read();
		Serial.write(c);
  	}
}
	auto* OkayResponsePrefix = "HTTP/1.1 200";

	//	read response
	{
		Debug("Waiting for first line of response...");
		String ResponseLine;
		while ( true )
		{
			if ( !Client.connected() )
			{
				OnError("Lost connection getting response");
				return;
			}
			char Char;
			if ( Client.readBytes(&Char,1) == 0 )
			{
				Debug("Read zero bytes...");
				delay(100);
				continue;	
			}
			if ( Char == '\n' )
				break;
			ResponseLine += Char;
			Debug(ResponseLine);
		}
		//auto ResponseLine = Client.readStringUntil('\n');
		Debug( ResponseLine );
		if ( !ResponseLine.startsWith(OkayResponsePrefix) )
		{
			OnError( String("First header response not ") + OkayResponsePrefix + ": " + ResponseLine );
			return;
		}
	}

	//	read headers
	String ResponseValue;
	const char* ContentTypeKey = "Content-Type: ";
	const char* ChunkedEncodingKey = "Transfer-Encoding: chunked";
	String ContentTypeValue;
	bool IsChunked = false;

	while ( true )
	{
		if ( !Client.connected() )
		{
			OnError("Client lost connection");
			return;
		}

		//	read next header
		auto ResponseLine = Client.readStringUntil('\n');
		auto DebugLine = ResponseLine;
		DebugLine.replace("\r","\\r");
		DebugLine.replace("\n","\\n");
		Debug( DebugLine );

		ResponseLine.replace('\r',' ');
		ResponseLine.replace('\n',' ');
		ResponseLine.trim();
		if ( ResponseLine.length() == 0 )
			break;

		if ( ResponseLine.startsWith(ChunkedEncodingKey) )
			IsChunked = true;

		if ( ResponseLine.startsWith(ContentTypeKey) )
		{
			ContentTypeValue = ResponseLine.substring( strlen(ContentTypeKey) );
			
			if ( ExpectedMimeType != nullptr )
			{
				if ( ContentTypeValue != ExpectedMimeType )
				{
					OnError( String("Content type is ") + ContentTypeValue + " not " + ExpectedMimeType );
					return;
				}
			}
		}			
	}

	//	process content
	auto Error = OnConnected( Client, IsChunked );
	if ( Error.length() > 0 )
	{
		OnError( Error );
	}
}

void DrawImageBlockDebug(const TImageBlock& ImageBlock,std::function<void(const String&)> OnDebug)
{
	{
		String Debug;
		Debug += "Got image block: [";
		Debug += IntToString( ImageBlock.mLeft );
		Debug += ",";
		Debug += IntToString( ImageBlock.mTop );
		Debug += ",";
		Debug += IntToString( ImageBlock.mWidth );
		Debug += ",";
		Debug += IntToString( ImageBlock.mHeight );
		Debug += "]";
		OnDebug( Debug );
	}
	auto w = ImageBlock.mWidth;
	auto h = ImageBlock.mHeight;
	auto l = ImageBlock.mLeft;
	auto r = (l+w)-1;
	auto t = ImageBlock.mTop;
	auto b = (t+h)-1;
	t = std::min<int>( t, display.height()-1 );
	b = std::min<int>( b, display.height()-1 );
	l = std::min<int>( l, display.width()-1 );
	r = std::min<int>( r, display.width()-1 );
	for ( int y=t;	y<=b;	y++ )
	{
		for ( int x=l;	x<=r;	x++ )
		{
			display.drawPixel( x, y, GxEPD_BLACK );
		}
	}
}

void DrawImageBlock(const TImageBlock& ImageBlock,std::function<void(const String&)> OnDebug)
{
	{
		String Debug;
		Debug += "Got image block: [";
		Debug += IntToString( ImageBlock.mLeft );
		Debug += ",";
		Debug += IntToString( ImageBlock.mTop );
		Debug += ",";
		Debug += IntToString( ImageBlock.mWidth );
		Debug += ",";
		Debug += IntToString( ImageBlock.mHeight );
		Debug += "]";
		OnDebug( Debug );
	}

	auto SubSample = 2;
	//if ( ImageBlock.mTop % SubSample != 0 )
	//	return;

	bool UsingRotation = true;
	auto CurrentX = ImageBlock.mLeft;
	auto CurrentY = ImageBlock.mTop;

	//	draw colours
	auto DrawColor = [&](TRgba8 Colour)
	{
		/*
		//yield();
		auto Luma = std::max( Colour.r, std::max( Colour.g, Colour.b ) );
		if ( Luma > 200 )
			display.drawPixel( CurrentX, CurrentY, GxEPD_WHITE );
		else if ( Luma > 120 )
			display.drawPixel( CurrentX, CurrentY, GxEPD_RED );
		else
			display.drawPixel( CurrentX, CurrentY, GxEPD_BLACK );
		*/
		//OnDebug( String("DrawPixel(") + IntToString(CurrentX) + "," + IntToString(CurrentY) );
		CurrentX++;
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
	#define QUICK_MIN(a,b)		( ((a) < (b)) ? (a) : (b) )
	//auto Width = std::min<int>( ImageBlock.mWidth/SubSample, display.width() );
	auto Width = QUICK_MIN( ImageBlock.mWidth/SubSample, display.width() );
	OnDebug( String("Drawing row width: ") + IntToString(Width) );
	for ( int x=0;	x<Width;	x+=SubSample )
	{
		//	try and avoid WDT timeout (wifi chip)
		WDT_YEILD();
		auto xmax = ImageBlock.mWidth-1;
		/*
		auto x0 = ImageBlock.mPixels[std::min(x+0,xmax)];
		auto x1 = ImageBlock.mPixels[std::min(x+1,xmax)];
		auto x2 = ImageBlock.mPixels[std::min(x+2,xmax)];
		auto x3 = ImageBlock.mPixels[std::min(x+3,xmax)];
			
		if ( SubSample == 1 )
			DrawColor( ImageBlock.GetColour(x0) );
		else if ( SubSample == 2 )
			DrawColor2( x0, x1 );
		else if ( SubSample == 3 )
			DrawColor3( x0, x1, x2 );
		else if ( SubSample == 4 )
			DrawColor4( x0, x1, x2, x3 );
			*/
	}
	//display.updateWindow( ImageBlock.mLeft, ImageBlock.mTop, ImageBlock.mWidth, ImageBlock.mHeight, UsingRotation );
}

String ExtractAndRenderGif(WiFiClient& Client,bool IsContentChunked,std::function<void(const String&)> OnError,std::function<void(const String&)> OnDebug)
{
	bool FirstChunkHeader = true;
	int ChunkLength = 0;
	int BytesRead = 0;

	auto EatChunkHeader = [&]()
	{
		if ( !IsContentChunked )
			return true;

		//	more of last chunk still to go
		if ( ChunkLength > 0 )
			return true;

		//	if not the first header, we need to read the tailing \r\n
		if ( !FirstChunkHeader )
		{
			uint8_t Tailrn[2];
			if ( Client.readBytes( Tailrn, sizeof(Tailrn) ) != 2 )
			{
				OnDebug("Failed to read chunk tail \\r\\n");
				return false;
			}
			if ( Tailrn[0] != '\r' || Tailrn[1] != '\n' )
			{
				OnDebug("Chunk tail not \\r\\n");
				return false;
			}
		}
		FirstChunkHeader = false;

		//	read next chunk size
		//	<lengthinhex>\r\n
		ChunkLength = 0;

		while ( true )
		{
			uint8_t NextByte;
			if ( Client.readBytes( &NextByte, 1 ) == 0 )
			{
				OnDebug("GetChunkLength nextbyte failed to read");
				return false;
			}
			
			//	eat trailing \r\n
			if ( NextByte == '\r' )
			{
				OnDebug("GetChunkLength found \\r");
				Client.readBytes( &NextByte, 1 );
				if ( NextByte != '\n' )
				{
					OnDebug("GetChunkLength tail not \n");
					return false;
				}
				OnDebug("GetChunkLength found \\n");
				break;
			}

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
					OnDebug("Http chunked extensions not supported");
					return false;	
				}
				OnDebug( String("GetChunkLength not hex (decimal: ") + IntToString(NextByte) + ")" );
				return false;
			}

			//	shift up last hex
			ChunkLength <<= 4;
			ChunkLength |= NextByte;
		}
		
		String Debug;
		Debug += "Read next chunk length: ";
		Debug += IntToString( ChunkLength );
		OnDebug(Debug);
			
		return true;
	};
	
	auto ReadBytes = [&](uint8_t* Buffer,size_t BufferSize)
	{
		for ( size_t i=0;	i<BufferSize;	i++ )
		{
			//	read next byte
			if ( !EatChunkHeader() )
				return false;

			//	readBytes does a timeout for us :)
			uint8_t NextByte;
			auto ReadCount = Client.readBytes( &NextByte, 1 );
			if ( ReadCount == 0 )
			{
				OnDebug("Didn't read next byte");
				if ( !Client.connected() )
					OnDebug("Client not connected");
				if ( !Client.available() )
					OnDebug("Data not availible");
				return false;
			}

			ChunkLength--;
			
			//	walking over data if null
			if ( Buffer != nullptr )
				Buffer[i] = NextByte;	
		}
		BytesRead += BufferSize;
		return true;
	};

	bool AnyError = false;
	auto ErrorWrapper = [&](const String& Error)
	{
		OnError(Error);
		AnyError = true;
	};	
	
	TCallbacks Callbacks;
	Callbacks.ReadBytes = ReadBytes;
	Callbacks.OnError = ErrorWrapper;
	Callbacks.OnDebug = OnDebug;

	display.fillScreen(GxEPD_WHITE);
	auto DrawBlock = [&](const TImageBlock& ImageBlock)
	{
		DrawImageBlockDebug( ImageBlock, OnDebug );
	};
	
	Gif::ParseGif( Callbacks, DrawBlock );

	if ( !AnyError )
	{
		OnDebug("Display update");
		display.update();
	}
	return String();
}



std::function<TState::Type(bool,std::function<void(const String&)>)> StateMachineUpdates[TState::COUNT] =
{
	[TState::Start] = Update_Start,
	[TState::ConnectWifi] = Update_ConnectWifi,
	[TState::ConnectToHost] = Update_ConnectToHost,
	[TState::GetHeaders] = Update_GetHeaders,
	//[TState::ParseGif] = Update_ParseGif,
	//[TState::DisplayGif] = Update_DisplayGif,
	[TState::Error] = Update_Error,
	//[TState::Start] = Update_Start,
};
auto CurrentState = TState::Start;
bool CurrentStateFirstCall = true;


void loop()
{
	auto& StateUpdate = StateMachineUpdates[CurrentState];
	auto NewState = StateMachineUpdates( CurrentStateFirstCall, DebugPrint );
	CurrentStateFirstCall = false;
	if ( NewState != CurrentState )
	{
		CurrentState = NewState;
		CurrentStateFirstCall = true;
	}
}

  


#endif
