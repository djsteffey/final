// Copyright Daniel J. Steffey -- 2016

#ifndef GShaderLinearGradientSteffey_hpp
#define GShaderLinearGradientSteffey_hpp

#include "include/GShader.h"
#include "include/GBitmap.h"
#include "include/GMatrix.h"
#include "include/GPixel.h"

class GShaderLinearGradientSteffey : public GShader
{
public:
    GShaderLinearGradientSteffey(const GPoint& p0, const GPoint& p1, const GColor& c0, const GColor& c1, GShader::TileMode tilemode);
    ~GShaderLinearGradientSteffey();

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
    GMatrix m_local_ctm;
    GMatrix m_global_ctm;
    GMatrix m_combined_ctm;
    float m_alpha;
    GShader::TileMode m_tilemode;
    GPixel m_lookup_table[256];
};

#endif
