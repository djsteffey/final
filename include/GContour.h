/*
 *  Copyright 2016 Mike Reed
 */

#ifndef GContour_DEFINED
#define GContour_DEFINED

#include "GPoint.h"

struct GContour {
    int             fCount;
    const GPoint*   fPts;
    bool            fClosed;
};

#endif
