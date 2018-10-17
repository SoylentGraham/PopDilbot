//	using https://github.com/ZinggJM/GxEPD/
//	commit b59ae551a18b8e2dbeeed1bb62f8197e15554be9
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

// for SPI pin definitions see e.g.:
// C:\Users\xxx\AppData\Local\Arduino15\packages\arduino\hardware\avr\1.6.21\variants\standard\pins_arduino.h

//#define BOARD_NANO
#define BOARD_NODEMCU
#include <Fonts/FreeMonoBold9pt7b.h>

#if defined(BOARD_NANO)
GxIO_Class io(SPI, /*CS=*/ SS, /*DC=*/ 9, /*RST=*/ 8); // arbitrary selection of 8, 9 selected for default of GxEPD_Class
GxEPD_Class display(io,8,7 /*RST=9*/ /*BUSY=7*/); // default selection of (9), 7
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
GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
GxEPD_Class display(io /*RST=D4*/ /*BUSY=D2*/); // default selection of D4(=2), D2(=4)
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
#else
#error define a board!
#endif

String ExtractAndRenderGif(WiFiClient& Client,std::function<void(const String&)> OnError,std::function<void(const String&)> OnDebug);
#include "SoyGif.h"


std::function<void()> OnDrawStuffFunc = nullptr;


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
	delay(100);
	Serial.begin(115200);
	Serial.println();

	Serial.println("Initialising display");
	display.init(115200); // enable diagnostic output on Serial
	display.setRotation(ROTATION_90);

	DebugPrint("Initialised display.");

}


void ConnectToWifi(const String& Ssid,const String& Password,std::function<void(const String&)> Debug)
{
	Debug( String("Setting up wifi to ") + Ssid + "/" + Password);

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
}


void loop()
{
	if ( WiFi.status() != WL_CONNECTED )
	{
		ConnectToWifi( "ZaegerMeister", "InTheYear2525", DebugPrint );
	}
	
	auto* Host = "assets.amuniversal.com";
	auto* Path = "/a2bb8780a4c401365a02005056a9545d";

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
	Client.print(String("GET ") + Path + " HTTP/1.1\r\n" +
               "Host: " + Host + "\r\n" +
               "User-Agent: Dilbot\r\n" +
               "Connection: close\r\n\r\n");

	auto* OkayResponsePrefix = "HTTP/1.1 200";

	//	read response
	{
		auto ResponseLine = Client.readStringUntil('\n');
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

	//if ( ImageBlock.mTop % 4 != 0 )
	//	return;

	bool UsingRotation = true;
	auto CurrentX = ImageBlock.mLeft;
	auto CurrentY = ImageBlock.mTop;

	//	draw colours
	auto DrawColor = [&](TRgba8 Colour)
	{
		auto Luma = std::max( Colour.r, std::max( Colour.g, Colour.b ) );
		if ( Luma > 200 )
			display.drawPixel( CurrentX, CurrentY, GxEPD_WHITE );
		else if ( Luma > 120 )
			display.drawPixel( CurrentX, CurrentY, GxEPD_RED );
		else
			display.drawPixel( CurrentX, CurrentY, GxEPD_BLACK );
		
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
	auto SubSample = 2;
	auto Width = std::min<int>( ImageBlock.mWidth/SubSample, display.width() );
	for ( int x=0;	x<Width-SubSample;	x+=SubSample )
	{
		//	try and avoid WDT timeout (wifi chip)
		delay(1);
		auto x0 = ImageBlock.mPixels[x+0];
		auto x1 = ImageBlock.mPixels[x+1];
		auto x2 = ImageBlock.mPixels[x+2];
		auto x3 = ImageBlock.mPixels[x+3];
			
		if ( SubSample == 1 )
			DrawColor( ImageBlock.GetColour(x0) );
		else if ( SubSample == 2 )
			DrawColor2( x0, x1 );
		else if ( SubSample == 3 )
			DrawColor3( x0, x1, x2 );
		else if ( SubSample == 4 )
			DrawColor4( x0, x1, x2, x3 );
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

	auto DrawBlock = [&](const TImageBlock& ImageBlock)
	{
		DrawImageBlock( ImageBlock, OnDebug );
	};
	
	Gif::ParseGif( Callbacks, DrawBlock );

	if ( !AnyError )
	{
		OnDebug("Display update");
		display.update();
	}
	return String();
}

	/*
  // Parse BMP header
  if (read16(client) == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(client);
    uint32_t creatorBytes = read32(client);
    uint32_t imageOffset = read32(client); // Start of image data
    uint32_t headerSize = read32(client);
    uint32_t width  = read32(client);
    uint32_t height = read32(client);
    uint16_t planes = read16(client);
    uint16_t depth = read16(client); // bits per pixel
    uint32_t format = read32(client);
    uint32_t bytes_read = 7 * 4 + 3 * 2; // read so far
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Bit Depth: "); Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())  w = display.width()  - x;
      if ((y + h - 1) >= display.height()) h = display.height() - y;
      valid = true;
      uint8_t bitmask = 0xFF;
      uint8_t bitshift = 8 - depth;
      uint16_t red, green, blue;
      bool whitish, colored;
      if (depth == 1) with_color = false;
      if (depth <= 8)
      {
        if (depth < 8) bitmask >>= depth;
        bytes_read += skip(client, 54 - bytes_read); //palette is always @ 54
        for (uint16_t pn = 0; pn < (1 << depth); pn++)
        {
          blue  = client.read();
          green = client.read();
          red   = client.read();
          client.read();
          bytes_read += 4;
          whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
          colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
          if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
          mono_palette_buffer[pn / 8] |= whitish << pn % 8;
          if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
          color_palette_buffer[pn / 8] |= colored << pn % 8;
          //Serial.print("0x00"); Serial.print(red, HEX); Serial.print(green, HEX); Serial.print(blue, HEX);
          //Serial.print(" : "); Serial.print(whitish); Serial.print(", "); Serial.println(colored);
        }
      }
      display.fillScreen(GxEPD_WHITE);
      uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
      //Serial.print("skip "); Serial.println(rowPosition - bytes_read);
      bytes_read += skip(client, rowPosition - bytes_read);
      for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
      {
        if (!connection_ok || !client.connected()) break;
        delay(1); // yield() to avoid WDT
        uint32_t in_remain = rowSize;
        uint32_t in_idx = 0;
        uint32_t in_bytes = 0;
        uint8_t in_byte = 0; // for depth <= 8
        uint8_t in_bits = 0; // for depth <= 8
        uint16_t color = GxEPD_WHITE;
        for (uint16_t col = 0; col < w; col++) // for each pixel
        {
          yield();
          if (!connection_ok || !client.connected()) break;
          // Time to read more pixel data?
          if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
          {
            uint32_t get = in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain;
            //if (get > client.available()) delay(200); // does improve? yes, if >= 200
            // there seems an issue with long downloads on ESP8266
            tryToWaitForAvailable(client, get);
            uint32_t got = client.read(input_buffer, get);
            while ((got < get) && connection_ok)
            {
              //Serial.print("got "); Serial.print(got); Serial.print(" < "); Serial.print(get); Serial.print(" @ "); Serial.println(bytes_read);
              //if ((get - got) > client.available()) delay(200); // does improve? yes, if >= 200
              // there seems an issue with long downloads on ESP8266
              tryToWaitForAvailable(client, get - got);
              uint32_t gotmore = client.read(input_buffer + got, get - got);
              got += gotmore;
              connection_ok = gotmore > 0;
            }
            in_bytes = got;
            in_remain -= got;
            bytes_read += got;
          }
          if (!connection_ok)
          {
            Serial.print("Error: got no more after "); Serial.print(bytes_read); Serial.println(" bytes read!");
            break;
          }
          switch (depth)
          {
            case 24:
              blue = input_buffer[in_idx++];
              green = input_buffer[in_idx++];
              red = input_buffer[in_idx++];
              whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
              colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
              break;
            case 16:
              {
                uint8_t lsb = input_buffer[in_idx++];
                uint8_t msb = input_buffer[in_idx++];
                if (format == 0) // 555
                {
                  blue  = (lsb & 0x1F) << 3;
                  green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                  red   = (msb & 0x7C) << 1;
                }
                else // 565
                {
                  blue  = (lsb & 0x1F) << 3;
                  green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                  red   = (msb & 0xF8);
                }
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
              }
              break;
            case 1:
            case 4:
            case 8:
              {
                if (0 == in_bits)
                {
                  in_byte = input_buffer[in_idx++];
                  in_bits = 8;
                }
                uint16_t pn = (in_byte >> bitshift) & bitmask;
                whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                in_byte <<= depth;
                in_bits -= depth;
              }
              break;
          }
          if (whitish)
          {
            color = GxEPD_WHITE;
          }
          else if (colored && with_color)
          {
            color = GxEPD_RED;
          }
          else
          {
            color = GxEPD_BLACK;
          }
          uint16_t yrow = y + (flip ? h - row - 1 : row);
          display.drawPixel(x + col, yrow, color);
        } // end pixel
      } // end line
    }
    Serial.print("bytes read "); Serial.println(bytes_read);
  }
  Serial.print("loaded in "); Serial.print(millis() - startTime); Serial.println(" ms");
  if (!valid)
  {
    Serial.println("bitmap format not handled.");
  }
  */
