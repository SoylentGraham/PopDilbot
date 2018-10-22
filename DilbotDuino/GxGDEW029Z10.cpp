// class GxGDEW029Z10 : Display class for GDEW029Z10 e-Paper from Dalian Good Display Co., Ltd.: www.good-display.com
//
// based on Demo Example from Good Display, available here: http://www.good-display.com/download_detail/downloadsId=515.html
// Controller: IL0373 : http://www.good-display.com/download_detail/downloadsId=535.html
//
// Author : J-M Zingg
//
// Version : see library.properties
//
// License: GNU GENERAL PUBLIC LICENSE V3, see LICENSE
//
// Library: https://github.com/ZinggJM/GxEPD

#include "GxGDEW029Z10.h"

//#define DISABLE_DIAGNOSTIC_OUTPUT

#if defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif

// Partial Update Delay, may have an influence on degradation
#define GxGDEW029Z10_PU_DELAY 500

GxGDEW029Z10::GxGDEW029Z10(GxIO& io, int8_t rst, int8_t busy)
  : 
  GxEPD(GxGDEW029Z10_WIDTH, GxGDEW029Z10_HEIGHT),
  IO(io), 
   _diag_enabled(false),
  _rst(rst),
  _busy(busy)
{
}
/*
void GxGDEW029Z10::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;

  // check rotation, move pixel around if necessary
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      x = GxGDEW029Z10_WIDTH - x - 1;
      break;
    case 2:
      x = GxGDEW029Z10_WIDTH - x - 1;
      y = GxGDEW029Z10_HEIGHT - y - 1;
      break;
    case 3:
      swap(x, y);
      y = GxGDEW029Z10_HEIGHT - y - 1;
      break;
  }
  uint16_t i = x / 8 + y * GxGDEW029Z10_WIDTH / 8;
  if (_current_page < 1)
  {
    if (i >= sizeof(_black_buffer)) return;
  }
  else
  {
  	Serial.println( String("Current page is ") + String(_current_page) );
    y -= _current_page * GxGDEW029Z10_PAGE_HEIGHT;
    if ((y < 0) || (y >= GxGDEW029Z10_PAGE_HEIGHT)) return;
    i = x / 8 + y * GxGDEW029Z10_WIDTH / 8;
  }

  _black_buffer[i] = (_black_buffer[i] & (0xFF ^ (1 << (7 - x % 8)))); // white
  #if defined(ENABLE_RED)
  _red_buffer[i] = (_red_buffer[i] & (0xFF ^ (1 << (7 - x % 8)))); // white
  #endif
  
  if (color == GxEPD_WHITE) 
  return;
  else if (color == GxEPD_BLACK) 
  _black_buffer[i] = (_black_buffer[i] | (1 << (7 - x % 8)));
  else if (color == GxEPD_RED) 
  {
  	#if defined(ENABLE_RED)
  	_red_buffer[i] = (_red_buffer[i] | (1 << (7 - x % 8)));
  	#endif
  }
  else
  {
  	Serial.println("Unknown Color");
    //if ((color & 0xF100) > (0xF100 / 2)) _red_buffer[i] = (_red_buffer[i] | (1 << (7 - x % 8)));
    //else 
    if ((((color & 0xF100) >> 11) + ((color & 0x07E0) >> 5) + (color & 0x001F)) < 3 * 255 / 2)
    {
      _black_buffer[i] = (_black_buffer[i] | (1 << (7 - x % 8)));
    }
  }
}
*/

void GxGDEW029Z10::init(uint32_t serial_diag_bitrate)
{
  if (serial_diag_bitrate > 0)
  {
    Serial.begin(serial_diag_bitrate);
    _diag_enabled = true;
  }
  IO.init();
  IO.setFrequency(4000000); // 4MHz
  if (_rst >= 0)
  {
    digitalWrite(_rst, HIGH);
    pinMode(_rst, OUTPUT);
  }
  pinMode(_busy, INPUT);

}


void GxGDEW029Z10::DrawRow8(int Row,std::function<bool(int,int)> GetColour,uint8_t ColourCommand)
{
	int Row8 = Row / 8;
	int y = 8 * Row8;
  	int x = 8 * 0;

	if ( y >= GxGDEW029Z10_WIDTH )
		return;
Serial.println( String("DrawRow8(") + String(Row) );
	//	enter partial window mode
	IO.writeCommandTransaction(0x91); 

 	//int Rows = 8 * 20;
	int Rows = GxGDEW029Z10_HEIGHT;
	int Columns = 8;

	auto YTop = y;
	auto YBottom = y+Columns;

	bool Flip = true;
	if ( Flip )
	{
		YTop = GxGDEW029Z10_WIDTH - YTop;
		YBottom = GxGDEW029Z10_WIDTH - YBottom;
		if ( YBottom < YTop )
			swap( YTop, YBottom );
	}
	
	_setPartialRamArea( YTop, x, YBottom, x+Rows );
	//y = YTop;
	_writeCommand(ColourCommand);
	//for ( int i=0; i < GxGDEW029Z10_BUFFER_SIZE; i++)
	for ( int i=0; i <Rows*1; i++)
	{
		//	accumulate black, then write opposite so we have a nicer api
		uint8_t Column = 0x0;

		bool FlipBlock = true;
		if ( FlipBlock  )
		{
			Column |= GetColour(x,y+0) << 0;
			Column |= GetColour(x,y+1) << 1;
			Column |= GetColour(x,y+2) << 2;
			Column |= GetColour(x,y+3) << 3;
			Column |= GetColour(x,y+4) << 4;
			Column |= GetColour(x,y+5) << 5;
			Column |= GetColour(x,y+6) << 6;
			Column |= GetColour(x,y+7) << 7;
		}
		else
		{
			Column |= GetColour(x,y+0) << 7;
			Column |= GetColour(x,y+1) << 6;
			Column |= GetColour(x,y+2) << 5;
			Column |= GetColour(x,y+3) << 4;
			Column |= GetColour(x,y+4) << 3;
			Column |= GetColour(x,y+5) << 2;
			Column |= GetColour(x,y+6) << 1;
			Column |= GetColour(x,y+7) << 0;
		}
		_writeData(~Column);
		x++;
	}
	//	end partial window mode
	IO.writeCommandTransaction(0x92); 
}
  
void GxGDEW029Z10::DrawRow8(int Row,std::function<bool(int,int)> GetBlack,std::function<bool(int,int)> GetRed,bool Finished)
{
	_wakeUp();
#define CMD_BLACK_PIXELS	0x10
#define CMD_RED_PIXELS		0x13
#define CMD_REFRESH_DISPLAY	0x12
	DrawRow8( Row, GetBlack, CMD_BLACK_PIXELS );
	DrawRow8( Row, GetRed, CMD_RED_PIXELS );

	/*
	 * 	for ( int y=0;	y<GxGDEW029Z10_WIDTH/8;	y++ )
	  		DrawRow8( y );
  	
	 */
	if ( Finished )
	{
		FinishRowDrawing();
	}
}

void GxGDEW029Z10::FinishRowDrawing()
{
	//	display refresh
	_writeCommand(CMD_REFRESH_DISPLAY); 
	_waitWhileBusy("update");
}

uint16_t GxGDEW029Z10::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye)
{
  x &= 0xFFF8; // byte boundary
  xe = (xe - 1) | 0x0007; // byte boundary - 1
  IO.writeCommandTransaction(0x90); // partial window
  //IO.writeDataTransaction(x / 256);
  IO.writeDataTransaction(x % 256);
  //IO.writeDataTransaction(xe / 256);
  IO.writeDataTransaction(xe % 256);
  IO.writeDataTransaction(y / 256);
  IO.writeDataTransaction(y % 256);
  IO.writeDataTransaction(ye / 256);
  IO.writeDataTransaction(ye % 256);
  IO.writeDataTransaction(0x01); // don't see any difference
  //IO.writeDataTransaction(0x00); // don't see any difference
  return (7 + xe - x) / 8; // number of bytes to transfer per line
}

void GxGDEW029Z10::_writeCommand(uint8_t command)
{
  IO.writeCommandTransaction(command);
}

void GxGDEW029Z10::_writeData(uint8_t data)
{
  IO.writeDataTransaction(data);
}

void GxGDEW029Z10::_waitWhileBusy(const char* comment)
{
  unsigned long start = micros();
  while (1)
  {
    if (digitalRead(_busy) == 1) break;
    delay(1);
    if (micros() - start > 20000000) // >14.9s !
    {
      if (_diag_enabled) Serial.println("Busy Timeout!");
      break;
    }
  }
  if (comment)
  {
#if !defined(DISABLE_DIAGNOSTIC_OUTPUT)
    if (_diag_enabled)
    {
      unsigned long elapsed = micros() - start;
      Serial.print(comment);
      Serial.print(" : ");
      Serial.println(elapsed);
    }
#endif
  }
  (void) start;
}

void GxGDEW029Z10::_wakeUp()
{
  // reset required for wakeup
  if (_rst >= 0)
  {
    digitalWrite(_rst, 0);
    delay(10);
    digitalWrite(_rst, 1);
    delay(10);
  }

  _writeCommand(0x06);
  _writeData (0x17);
  _writeData (0x17);
  _writeData (0x17);
  _writeCommand(0x04);
  _waitWhileBusy("_wakeUp Power On");
  _writeCommand(0X00);
  _writeData(0x8f);
  _writeCommand(0X50);
  _writeData(0x77);
  _writeCommand(0x61);
  _writeData (0x80);
  _writeData (0x01);
  _writeData (0x28);
}

void GxGDEW029Z10::_sleep(void)
{
  _writeCommand(0x02);      //power off
  _waitWhileBusy("_sleep Power Off");
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData (0xa5);
  }
}


void GxGDEW029Z10::_rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h)
{
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      swap(w, h);
      x = GxGDEW029Z10_WIDTH - x - w - 1;
      break;
    case 2:
      x = GxGDEW029Z10_WIDTH - x - w - 1;
      y = GxGDEW029Z10_HEIGHT - y - h - 1;
      break;
    case 3:
      swap(x, y);
      swap(w, h);
      y = GxGDEW029Z10_HEIGHT - y - h - 1;
      break;
  }
}
