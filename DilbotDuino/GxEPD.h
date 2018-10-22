// class GxEPD : Base Class for Display Classes for e-Paper Displays from Dalian Good Display Co., Ltd.: www.good-display.com
//
// based on Demo Examples from Good Display, available here: http://www.good-display.com/download_list/downloadcategoryid=34&isMode=false.html
//
// Author : J-M Zingg
//
// Version : see library.properties
//
// License: GNU GENERAL PUBLIC LICENSE V3, see LICENSE
//
// Library: https://github.com/ZinggJM/GxEPD

#ifndef _GxEPD_H_
#define _GxEPD_H_

#include <Arduino.h>
#include <SPI.h>
#include "GxIO.h"

// the only colors supported by any of these displays; mapping of other colors is class specific
#define GxEPD_BLACK     0x0000
#define GxEPD_DARKGREY  0x7BEF      /* 128, 128, 128 */
#define GxEPD_LIGHTGREY 0xC618      /* 192, 192, 192 */
#define GxEPD_WHITE     0xFFFF
#define GxEPD_RED       0xF800      /* 255,   0,   0 */


class GxEPD 
{
  public:
    // bitmap presentation modes may be partially implemented by subclasses
    enum bm_mode //BM_ModeSet
    {
      bm_normal = 0,
      bm_default = 1, // for use for BitmapExamples
      // these potentially can be combined
      bm_invert = (1 << 1),
      bm_flip_x = (1 << 2),
      bm_flip_y = (1 << 3),
      bm_r90 = (1 << 4),
      bm_r180 = (1 << 5),
      bm_r270 = bm_r90 | bm_r180,
      bm_partial_update = (1 << 6),
      bm_invert_red = (1 << 7),
      bm_transparent = (1 << 8)
    };
  public:
    GxEPD(uint16_t w, uint16_t h) :
    	mWidth	( w ),
    	mHeight	( h )
    	{
       	}
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void init(uint32_t serial_diag_bitrate = 0) = 0; // = 0 : disabled
    virtual void fillScreen(uint16_t color) = 0; // to buffer
    virtual void update(void) = 0;
    virtual void eraseDisplay(bool using_partial_update = false) {};
    // partial update of rectangle from buffer to screen, does not power off
    virtual void updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation = true) {};
    static inline uint16_t gx_uint16_min(uint16_t a, uint16_t b) {return (a < b ? a : b);};
    static inline uint16_t gx_uint16_max(uint16_t a, uint16_t b) {return (a > b ? a : b);};

	//	fixed at 90
	void		setRotation(int r)	{}
	int			getRotation() const	{	return 1;	}
	uint16_t	width() const	{	return mWidth;	}
	uint16_t	height() const	{	return mHeight;	}
	uint16_t	mWidth;
	uint16_t	mHeight;
};

#endif
