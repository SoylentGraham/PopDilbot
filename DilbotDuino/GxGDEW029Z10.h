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

#ifndef _GxGDEW029Z10_H_
#define _GxGDEW029Z10_H_

#include "GxEPD.h"

#if defined(__AVR)
#error gr: cut out AVR stuff
#endif
#undef min
#undef max
#include <functional>

#define GxGDEW029Z10_WIDTH 128
#define GxGDEW029Z10_HEIGHT 296

//#define ENABLE_RED

#define GxGDEW029Z10_BUFFER_SIZE (uint32_t(GxGDEW029Z10_WIDTH) * uint32_t(GxGDEW029Z10_HEIGHT) / 8)

// divisor for AVR, should be factor of GxGDEW029Z10_HEIGHT
#define GxGDEW029Z10_PAGES 8

#define GxGDEW029Z10_PAGE_HEIGHT (GxGDEW029Z10_HEIGHT / GxGDEW029Z10_PAGES)
#define GxGDEW029Z10_PAGE_SIZE (GxGDEW029Z10_BUFFER_SIZE / GxGDEW029Z10_PAGES)

class GxGDEW029Z10 : public GxEPD
{
  public:
#if defined(ESP8266)
    //GxGDEW029Z10(GxIO& io, int8_t rst = D4, int8_t busy = D2);
    // use pin numbers, other ESP8266 than Wemos may not use Dx names
    GxGDEW029Z10(GxIO& io, int8_t rst = 2, int8_t busy = 4);
#else
    GxGDEW029Z10(GxIO& io, int8_t rst = 9, int8_t busy = 7);
#endif
    void init(uint32_t serial_diag_bitrate = 0); // = 0 : disabled
    void powerDown();
    // paged drawing, for limited RAM, drawCallback() is called GxGDEW029Z10_PAGES times
    // each call of drawCallback() should draw the same
  	void	DrawRow8(int Row,std::function<bool(int,int)> GetBlack,std::function<bool(int,int)> GetRed,bool Finished);
	void	FinishRowDrawing();
	
    void Sleep()	{	_sleep();	}
    
  private:
    template <typename T> static inline void
    swap(T& a, T& b)
    {
      T t = a;
      a = b;
      b = t;
    }
    void _writeToWindow(uint16_t xs, uint16_t ys, uint16_t xd, uint16_t yd, uint16_t w, uint16_t h, bool using_rotation = true);
    uint16_t _setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye);
    void _writeData(uint8_t data);
    void _writeCommand(uint8_t command);
    void _wakeUp();
    void _sleep();
    void _waitWhileBusy(const char* comment = 0);
    void _rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h);
	void	DrawRow8(int Row,std::function<bool(int,int)> GetColour,uint8_t ColourCommand);
  private:


    GxIO& IO;
     bool _diag_enabled;
    int8_t _rst;
    int8_t _busy;
};

#ifndef GxEPD_Class
#define GxEPD_Class GxGDEW029Z10
#define GxEPD_WIDTH GxGDEW029Z10_WIDTH
#define GxEPD_HEIGHT GxGDEW029Z10_HEIGHT
#define GxEPD_BitmapExamples <GxGDEW029Z10/BitmapExamples.h>
#define GxEPD_BitmapExamplesQ "GxGDEW029Z10/BitmapExamples.h"
#endif

#endif
