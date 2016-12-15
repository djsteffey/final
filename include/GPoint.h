/**
 *  Copyright 2015 Mike Reed
 */

#ifndef GPoint_DEFINED
#define GPoint_DEFINED

#include "GMath.h"

class GPoint {
public:
    float fX;
    float fY;

    float x() const { return fX; }
    float y() const { return fY; }

    void set(float x, float y) {
        fX = x;
        fY = y;
    }

    static GPoint Make(float x, float y) {
        GPoint pt = { x, y };
        return pt;
    }
};

class GISize {
public:
    int fWidth, fHeight;

    int width() { return fWidth; }
    int height() { return fHeight; }
};

#endif
