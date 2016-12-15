// Copyright Daniel J. Steffey -- 2016

#ifndef GShaderBitmapProxy_hpp
#define GShaderBitmapProxy_hpp

#include "include/GShader.h"
#include "include/GMatrix.h"

class GShaderBitmapProxy : public GShader
{
public:
    GShaderBitmapProxy(GShader* bitmap_shader, const GPoint pts[], const GPoint tex[]);
    ~GShaderBitmapProxy();

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
    GShader* m_bitmap_shader;
    GMatrix m_pts_matrix;
    GMatrix m_tex_matrix;

private:

};

#endif
