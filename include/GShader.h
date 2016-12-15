/*
 *  Copyright 2015 Mike Reed
 */

#ifndef GShader_DEFINED
#define GShader_DEFINED

#include "GPixel.h"

class GBitmap;
class GColor;
class GMatrix;
class GPoint;

/**
 *  GShaders create colors to fill whatever geometry is being drawn to a GCanvas.
 */
class GShader {
public:
    enum TileMode {
        kClamp,
        kRepeat,
        kMirror,
    };

    virtual ~GShader() {}

    /**
     *  Called with the drawing's current matrix (ctm) and paint's alpha.
     *
     *  Subsequent calls to shadeRow() must respect the CTM, and have its colors
     *  modulated by alpha.
     */
    virtual bool setContext(const GMatrix& ctm, float alpha) = 0;

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    virtual void shadeRow(int x, int y, int count, GPixel row[]) = 0;

    /**
     *  Return a subclass of GShader that draws the specified bitmap and local-matrix.
     *  Returns null if the either parameter is not valid.
     */
    static GShader* FromBitmap(const GBitmap&, const GMatrix& localMatrix, TileMode = kClamp);

    static GShader* LinearGradient(const GPoint& p0, const GPoint& p1,
                                   const GColor& c0, const GColor& c1, TileMode = kClamp);
};

#endif
