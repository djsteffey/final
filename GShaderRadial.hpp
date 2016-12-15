// Copyright Daniel J. Steffey -- 2016

#ifndef GShaderRadial_hpp
#define GShaderRadial_hpp

#include "include/GShader.h"
#include "include/GBitmap.h"
#include "include/GMatrix.h"
#include "include/GPixel.h"
#include "include/GColor.h"

class GShaderRadial : public GShader
{
public:
    GShaderRadial(float cx, float cy, float radius, const GColor colors[], int count);
    ~GShaderRadial();

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

    GColor* m_colors;
    int m_count;

    GMatrix m_local_ctm;
    GMatrix m_global_ctm;
    GMatrix m_combined_ctm;
    float m_alpha;
};

#endif
