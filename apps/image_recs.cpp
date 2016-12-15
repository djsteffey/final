/**
 *  Copyright 2016 Mike Reed
 */

#include "image.h"
#include "GCanvas.h"
#include "GBitmap.h"
#include "GColor.h"
#include "GPoint.h"
#include "GRect.h"
#include "GShader.h"
#include <memory>
#include <string>

static void draw_patch0(GCanvas* canvas) {
    const GPoint pts[4] = {
        { 1, 1 }, { 10, 1 }, { 10, 10 }, { 1, 10 }
    };
    const GColor colors[4] = {
        { 1, 1, 1, 0 },
        { 1, 0, 1, 1 },
        { 1, 1, 0, 1 },
        { 1, 0, 0, 0 },
    };
    
    canvas->scale(25, 25);
    int strips = 1;
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            canvas->save();
            canvas->translate(x * 10.f, y * 10.f);
            canvas->drawQuadPatch(pts, colors, strips);
            canvas->restore();
            strips <<= 1;
        }
    }
}

static void draw_patch1(GCanvas* canvas) {
    const GPoint pts[4] = {
        { 2, 0 },
        { 0, 3 },
        { 3, 3.5 },
        { 4, 2 }
    };
    const GColor colors[4] = {
        { 1, 0, 0, 0 },
        { 1, 1, 0, 0 },
        { 1, 0, 1, 0 },
        { 1, 0, 0, 1 },
    };

    canvas->scale(128, 128);
    canvas->drawQuadPatch(pts, colors, 24);
}

static void draw_patch2(GCanvas* canvas) {
    const GPoint pts[4] = {
        { 0, 0 },
        { 500, 375 },
        { 250, 0 },
        { 125, 500 }
    };
    const GColor colors[4] = {
        { 1, 1, 1, 0 },
        { 1, 0, 1, 1 },
        { 1, 1, 0, 1 },
        { 1, 0, 0, 0 },
    };
    
    canvas->translate(6, 6);
    canvas->drawQuadPatch(pts, colors, 16);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static void draw_radial0(GCanvas* canvas) {
    const GColor c1[] = {
        { 1, 1, 1, 1 },
        { 1, 0, 0, 0 },
    };
    std::unique_ptr<GShader> shader(canvas->makeRadialGradient(256, 256, 256,
                                                               c1, GARRAY_COUNT(c1)));
    if (!shader) {
        return;
    }
    const GPoint pts[] = {
        { 256, 0 }, { 512, 256 }, { 256, 512 }, { 0, 256 },
    };
    canvas->drawConvexPolygon(pts, 4, GPaint(shader.get()));
}

static void draw_radial1(GCanvas* canvas) {
    const GColor c1[] = {
        { 1, 1, 0, 0 },
        { 1, 0, 1, 0 },
        { 1, 0, 0, 1 },
    };
    std::unique_ptr<GShader> shader(canvas->makeRadialGradient(0, 0, 5, c1, GARRAY_COUNT(c1)));
    if (!shader) {
        return;
    }

    const GPoint pts[] {
        { 0, 0.1f }, { -1, 5 }, { 0, 6 }, { 1, 5 },
    };
    
    canvas->translate(256, 256);
    canvas->scale(40, 40);
    
    float steps = 12;
    float r = 0;
    float b = 1;
    float step = 1 / (steps - 1);
    
    GPaint paint;
    for (float angle = 0; angle < 2*M_PI - 0.001f; angle += 2*M_PI/steps) {
        canvas->save();
        canvas->rotate(angle);
        paint.setColor({ 1, r, 0, b });
        paint.setShader(shader.get());
        canvas->drawConvexPolygon(pts, 4, paint);
        canvas->restore();
        r += step;
        b -= step;
    }
}

static void draw_radial2(GCanvas* canvas) {
    const GColor c1[] = {
        { 1, 1, 0, 0 },
        { 1, 0, 1, 0 },
        { 1, 0, 0, 1 },
        { 1, 1, 1, 0 },
        { 1, 1, 0, 1 },
        { 1, 0, 1, 1 },
    };
    std::unique_ptr<GShader> shader(canvas->makeRadialGradient(250, 250, 280,
                                                               c1, GARRAY_COUNT(c1)));
    if (!shader) {
        return;
    }
    canvas->drawRect(GRect::MakeWH(512, 512), GPaint(shader.get()));

    const GColor c2[] = {
        { 0.5, 0, 0, 1 },
        { 0.5, 0, 1, 0 },
        { 0.5, 1, 0, 0 },
    };
    shader.reset(canvas->makeRadialGradient(30, 30, 550, c2, GARRAY_COUNT(c2)));
    canvas->drawRect(GRect::MakeWH(512, 512), GPaint(shader.get()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

const GDrawRec gDrawRecs[] = {
    { draw_patch0,  512, 512, "final_patch0",  8 },
    { draw_patch1,  512, 512, "final_patch1",  8 },
    { draw_patch2,  512, 512, "final_patch2",  8 },

    { draw_radial0, 512, 512, "final_radial0", 8 },
    { draw_radial1, 512, 512, "final_radial1", 8 },
    { draw_radial2, 512, 512, "final_radial2", 8 },

    { nullptr, 0, 0, nullptr, 0 },
};
