/*
 *  Copyright 2016 Mike Reed
 */

#ifndef GPath_DEFINED
#define GPath_DEFINED

#include "GPoint.h"
#include <vector>

struct GContour;

class GPath {
    enum class Verb {
        kMove,
        kLine,
    };

public:
    GPath() {}

    GPath& moveTo(const GPoint&);
    GPath& lineTo(const GPoint&);

    GPath& moveTo(float x, float y) { return this->moveTo({x, y}); }
    GPath& lineTo(float x, float y) { return this->lineTo({x, y}); }

    class Iter {
    public:
        Iter(const GPath&);
        bool next(GContour*);

    private:
        const GPoint* fPts;
        const GPath::Verb*  fVerbs;
        const GPath::Verb*  fStopVerbs;
    };

    void dump() const;

private:
    std::vector<GPoint> fPts;
    std::vector<Verb>   fVerbs;
};

#endif

