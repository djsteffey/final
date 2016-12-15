// Copyright Daniel J. Steffey -- 2016

#ifndef GShaderCompose_hpp
#define GShaderCompose_hpp

#include "include/GShader.h"
#include "include/GMatrix.h"

class GShaderCompose : public GShader
{
public:
    GShaderCompose(GShader* shader_1, GShader* shader_2);
    ~GShaderCompose();

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
    GShader* m_shader_1;
    GShader* m_shader_2;

private:

};

#endif
