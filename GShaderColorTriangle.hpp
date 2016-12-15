// Copyright Daniel J. Steffey -- 2016

#ifndef GShaderColorTriangle_hpp
#define GShaderColorTriangle_hpp

#include "include/GShader.h"
#include "include/GBitmap.h"
#include "include/GMatrix.h"
#include "include/GPixel.h"

class GShaderColorTriangle : public GShader
{
public:
    GShaderColorTriangle(const GPoint pts[], const GColor colors[]);
    ~GShaderColorTriangle();

    /**
     *  Called with the drawing's current matrix (ctm) and paint's alpha.
     *
     *  Subsequent calls to shadeRow() must respect the CTM, and have its colors
     *  modulated by alpha.
     */
    bool setContext(const GMatrix& ctm, float alpha) override;

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override;

protected:


private:
    void shadeRow_clamp(int x, int y, int count, GPixel row[]);
    void shadeRow_repeat(int x, int y, int count, GPixel row[]);
    void shadeRow_mirror(int x, int y, int count, GPixel row[]);

    GColor m_c0;
    GColor m_c1;
    GColor m_c2;
    GPoint m_p0;
    GPoint m_p1;
    GPoint m_p2;

    GMatrix m_local_ctm;
    GMatrix m_global_ctm;
    GMatrix m_combined_ctm;
    float m_alpha;
};

#endif
