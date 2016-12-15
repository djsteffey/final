/*
 *  Copyright 2016 Mike Reed
 */

#ifndef GPaint_DEFINED
#define GPaint_DEFINED

#include "GColor.h"

class GShader;

class GPaint {
public:
    GPaint() : fColor(GColor::MakeARGB(1, 0, 0, 0)), fShader(nullptr) {}
    GPaint(const GColor& c) : fColor(c), fShader(nullptr) {}
    GPaint(GShader* s) : fColor(GColor::MakeARGB(1, 0, 0, 0)), fShader(s) {}

    const GColor& getColor() const { return fColor; }
    void setColor(GColor c) { fColor = c; }
    void setARGB(float a, float r, float g, float b) {
        fColor = GColor::MakeARGB(a, r, g, b);
    }
    float getAlpha() const { return fColor.fA; }
    void setAlpha(float alpha) {
        fColor.fA = alpha;
    }

    /**
     *  If there is a shader, then its output colors must be modulated by the paint.s alpha.
     */
    GShader* getShader() const { return fShader; }
    void setShader(GShader* s) { fShader = s; }

    bool isFill() const { return fWidth < 0; }
    bool isStroke() const { return !this->isFill(); }
    void setFill() { fWidth = -1; }

    /**
     *  Return the paint's stroke width. If this is < 0, then "fill" the geometry rather
     *  than stroke it.
     *
     *  When stroking, use a miter-join (subject to MiterLimit) and square-caps.
     */
    float getStrokeWidth() const { return fWidth; }
    void setStrokeWidth(float w) { fWidth = w; }

    /**
     *  If the projected miter point exceeds this limit * stroke radius, then draw a blunt
     *  joint instead of a miter join.
     */
    float getMiterLimit() const { return fMiterLimit; }
    void setMiterLimit(float limit) { fMiterLimit = limit; }

private:
    GColor      fColor;
    GShader*    fShader;
    float       fWidth = -1;
    float       fMiterLimit = 4;
};

#endif
