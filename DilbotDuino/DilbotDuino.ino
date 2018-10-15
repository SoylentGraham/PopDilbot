//	using https://codeload.github.com/ZinggJM/GxEPD/zip/master
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

void DebugPrint(const String& Text)
{
	Serial.println(Text);
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

void setup()
{
	delay(100);
	Serial.begin(115200);
	Serial.println();

	Serial.println("Initialising display");
	display.init(115200); // enable diagnostic output on Serial

	DebugPrint("Initialised display.");

	auto Ssid = "ZaegerMeister";
	auto Password = "InTheYear2525";
	DebugPrint( String("Setting up wifi to ") + Ssid + "/" + Password);

	WiFi.persistent(true);
	WiFi.mode(WIFI_STA); // switch off AP
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.disconnect();
	WiFi.begin(Ssid, Password);

	int Attempt=0;
	do
	{
		delay(500);
		DebugPrint( String("Attempt #") + Attempt + "\nStatus:" + WiFi.status());
		Attempt++;
	}
	while ( WiFi.status() != WL_CONNECTED );
}

bool drawBitmapFrom_HTTP_ToBuffer(const char* host,const char* path,int16_t x, int16_t y, bool with_color);


void loop()
{
	auto* Host = "assets.amuniversal.com";
	auto* Path = "/a2bb8780a4c401365a02005056a9545d";

	drawBitmapFrom_HTTP_ToBuffer( Host, Path, 0, 0, true );
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

bool drawBitmapFrom_HTTP_ToBuffer(const char* host,const char* path,int16_t x, int16_t y, bool with_color)
{
	WiFiClient client;
	bool valid = false; // valid format to be handled
	bool flip = true; // bitmap is stored bottom-to-top
	uint32_t startTime = millis();

  	DebugPrint(String("Fetching ") + host + path);
	
#define HTTP_PORT	80
	if ( !client.connect(host, HTTP_PORT) )
	{
		DebugPrint("Failed to connect");
		return false;
	}

	client.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: GxEPD2_Spiffs_Loader\r\n" +
               "Connection: close\r\n\r\n");
	Serial.println("request sent");

	//	got headers?
	bool connection_ok = false;

	//	read
	while (client.connected())
	{
		//	stop nodemcu timeouts
		delay(1);
		
		String line = client.readStringUntil('\n');
		if (!connection_ok)
		{
			connection_ok = line.startsWith("HTTP/1.1 200 OK");
			Serial.println(line);
		}
		if (line == "\r")
		{
			Serial.println("headers received");
			break;
		}
		Serial.println(line);
	}

	if (!connection_ok) 
	{
		DebugPrint("Failed to get headers");
		return false;
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

  return true;
}
