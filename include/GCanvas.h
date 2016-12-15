/*
 *  Copyright 2015 Mike Reed
 */

#ifndef GCanvas_DEFINED
#define GCanvas_DEFINED

#include "GContour.h"
#include "GPaint.h"
#include "GShader.h"

class GBitmap;
class GColor;
class GMatrix;
class GPoint;
class GRect;

class GCanvas {
public:
    static GCanvas* Create(const GBitmap&);

    virtual ~GCanvas() {}

    /**
     *  Saves a copy of the CTM, allowing subsequent modifications (by calling concat()) to be
     *  undone when restore() is called.
     *
     *  e.g.
     *  // CTM is in state A
     *  canvas->save();
     *  canvas->conact(...);
     *  canvas->conact(...);
     *  // Now the CTM has been modified by the calls to concat()
     *  canvas->restore();
     *  // Now the CTM is again in state A
     */
    virtual void save() = 0;
    
    /**
     *  Balances calls to save(), returning the CTM to the state it was in when the corresponding
     *  call to save() was made. These calls can be nested.
     *
     *  e.g.
     *  save()
     *      concat()
     *      concat()
     *      save()
     *          concat()
     *          ...
     *      restore()
     *      ...
     *  restore()
     */
    virtual void restore() = 0;
    
    /**
     *  Modifies the CTM (current transformation matrix) by pre-concatenating it with the specfied
     *  matrix.
     *
     *  CTM' = CTM * matrix
     *
     *  The result is that any drawing that uses the new CTM' will be affected AS-IF it were
     *  first transformed by matrix, and then transformed by the previous CTM.
     */
    virtual void concat(const GMatrix&) = 0;
    
    /**
     *  Pretranslates the CTM by the specified tx, ty
     */
    void translate(float tx, float ty);
    
    /**
     *  Prescales the CTM by the specified sx, sy
     */
    void scale(float sx, float sy);
    
    /**
     *  Prerotates the CTM by the specified angle in radians.
     */
    void rotate(float radians);

    //////////

    /**
     *  Fill the entire canvas with the specified color.
     *
     *  This completely overwrites any previous colors, it does not blend.
     */
    virtual void clear(const GColor&) = 0;
    
    /**
     *  Scale and translate the bitmap such that it fills the specific rectangle.
     *
     *  Any area in the rectangle that is outside of the bounds of the canvas is ignored.
     *
     *  Draws using SRCOVER blend mode.
     */
    virtual void fillBitmapRect(const GBitmap& src, const GRect& dst) = 0;

    /**
     *  Fill the rectangle with the color.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     *
     *  Any area in the rectangle that is outside of the bounds of the canvas is ignored.
     *
     *  Draws using SRCOVER blend mode.
     */
    virtual void drawRect(const GRect&, const GPaint&) = 0;
    
    /**
     *  Fill the convex polygon with the color, following the same "containment" rule as
     *  rectangles.
     *
     *  Any area in the polygon that is outside of the bounds of the canvas is ignored.
     *
     *  Draws using SRCOVER blend mode.
     */
    virtual void drawConvexPolygon(const GPoint[], int count, const GPaint&) = 0;

    void fillRect(const GRect& r, const GColor& c) {
        this->drawRect(r, GPaint(c));
    }

    void fillConvexPolygon(const GPoint pts[], int count, const GColor& c) {
        this->drawConvexPolygon(pts, count, GPaint(c));
    }

    /**
     *  Each contour need not be convex.
     */
    virtual void drawContours(const GContour ctrs[], int count, const GPaint&) = 0;

    /**
     *  Draw a mesh of triangles, each with optional colors and/or text-coordinates at each
     *  vertex.
     *
     *  The triangles are specified in one of two ways:
     *  - If indices is null, then each triangle is 3 consecutive points from pts[].
     *      { pts[0], pts[1], pts[2] }
     *  - Else each triangle is formed by references points from 3 consecutive indices...
     *      { pts[indices[0]], pts[indices[1]], pts[indices[2]] }
     *
     *  If colors is not null, then each vertex has an associated color, to be interpolated
     *  across the triangle. The colors are referenced in the same way as the pts, either
     *  sequentially or via indices[].
     *
     *  If tex is not null, then each vertex has an associated texture coordinate, to be used
     *  to specify a coordinate in the paint's shader's space. If there is no shader on the
     *  paint, then tex[] should be ignored. It is referenced in the same way as pts and colors,
     *  either sequentially or via indices[].
     *
     *  If both colors and tex[] are specified, then at each pixel their values are multiplied
     *  together, component by component.
     */
    virtual void drawMesh(int triCount, const GPoint pts[], const int indices[],
                          const GColor colors[], const GPoint tex[], const GPaint&) = 0;

    // FINAL

    /**
     *  Implement the bilinear patch we have been demoing in class. Based on the parameters,
     *  your code will create a mesh of triangles with appropriate points and color values,
     *  and then call your drawMesh() function.
     *
     *  The pts[4] parameter specifies the 4 corners of the quad, in this order:
     *      [0]: left,  top                [0] ------- [1]
     *      [1]: right, top                 |           |
     *      [2]: right, bottom              |           |
     *      [3]: left,  bottom             [3] ------- [2]
     *
     *  colors[4] are the colors at the corresponding corners of the quad. These should be
     *  interpolated across the quad using bilinear interpolation.
     *
     *  stripCount is the number of strips (horizontally and vertically) to be used when creating
     *  the final mesh that will be drawn (with your drawMesh() method implemented for pa7).
     *  e.g.
     *      stripCount == 1 :                                        -----
     *          - produces no intermediate points                    |\  |
     *          - results in only the original "quad"                | \ |
     *          - results in 2 triangles for the call to drawMesh()  |  \|
     *                                                               -----
     *
     *      stripCount == 2 :                                        -------
     *          - produces 2 "strips" horizontally and vertically    |\ |\ |
     *          - results in 4 intermediate "quads"                  | \| \|
     *          - results in 8 triangles for the call to drawMesh()  -------
     *                                                               |\ |\ |
     *                                                               | \| \|
     *                                                               -------
     *
     *  When you have to triangulate each quad (after bilinear interpolation of the original quad)
     *  make the two triangles share the diagonal
     *      - (u,v) .. (u+du, v+dv)
     *      - this is conceptually the diagonal from left,top to right,bottom
     *  This is not more accurate, but it will match the expected images for the final.
     */
    virtual void drawQuadPatch(const GPoint pts[4], const GColor colors[4], int stripCount) {}

    /**
     *  Return a radial-gradient shader.
     *
     *  This is a shader defined by a circle with center point (cx, cy) and a radius.
     *  It supports an array colors (count >= 2) where
     *      color[0]       is the color at the center
     *      color[count-1] is the color at the outer edge of the circle
     *      the other colors (if any) are even distributed along the radius
     *
     *  e.g. If there are 4 colors and a radius of 90 ...
     *
     *      color[0] is at the center
     *      color[1] is at a distance of 30 from the center
     *      color[2] is at a distance of 60 from the center
     *      color[3] is at a distance of 90 from the center
     *
     *  Positions outside of the radius are clamped to color[count - 1].
     */
    virtual GShader* makeRadialGradient(float cx, float cy, float radius,
                                        const GColor colors[], int count) {
        return nullptr;
    }
};

#endif
