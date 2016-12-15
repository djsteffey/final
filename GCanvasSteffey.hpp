// Copyright Daniel J. Steffey -- 2016

#ifndef GCanvasSteffey_hpp
#define GCanvasSteffey_hpp

#include "include/GCanvas.h"
#include "include/GBitmap.h"
#include "include/GColor.h"
#include "include/GRect.h"
#include "include/GMatrix.h"
#include "include/GPaint.h"
#include "include/GPoint.h"
#include "include/GPixel.h"
#include <vector>
#include <list>
#include "PolygonEdge.hpp"
#include <stack>
#include "include/GContour.h"
#include "GShaderRadial.hpp"


class GCanvasSteffey : public GCanvas
{
public:
	// constructor
	GCanvasSteffey(const GBitmap& bitmap);
	
	// destructor
	~GCanvasSteffey();

	// clear the whole canvas to the given color
	void clear(const GColor& color) override;

	// fill the given src bitmap into the given dst rect
	void fillBitmapRect(const GBitmap& src, const GRect& dst) override;

	// set the matrix that will affect all subsequent drawing functions
	void save() override;
	void restore() override;
	void concat(const GMatrix& matrix) override;

	// draw a rectangle
	void drawRect(const GRect& rect, const GPaint& paint) override;

	// draw a polygon
	void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) override;

	// draw some contours
	void drawContours(const GContour ctrs[], int count, const GPaint& paint) override;

	// draw a mesh
	void drawMesh(int triCount, const GPoint pts[], const int indices[], const GColor colors[], const GPoint tex[], const GPaint& paint) override;
	
	GShader* makeRadialGradient(float cx, float cy, float radius, const GColor colors[], int count) override;


protected:
	
private:


	// blend the source and destination pixels
	static inline GPixel blend(const GPixel& source, const GPixel& destination);

	// blend a single color to an array destination
	static inline void blend(const GPixel& source, GPixel* dest, int count);
	static inline void blend_opaque(const GPixel& source, GPixel* dest, int count);

	// blend the arrays of source and destination pixels
	static inline void blend(const GPixel* source, GPixel* dest, int count);	

	// execute a very fast divide by 255
	static inline unsigned int divide_by_255(unsigned int p);

	// sort edges
	static inline void sort_polygon_edges(std::vector<PolygonEdge>& edges);
	static inline void sort_polygon_drawing_edges(std::vector<PolygonEdge*>& edges);

	// clip edges
	static void create_and_clip_polygon_edges(const GPoint& p0, const GPoint& p1, const GRect& clip_rect, std::vector<PolygonEdge>& edges);

	// create the strokes
	void draw_stroked_contours(const GContour contours[], int count, const GPaint& paint);
	void create_stroked_contour_2_points_cap_AB(const GPoint& p0, const GPoint& p1, GContour* fill_contour, float width);
	void create_stroked_contour_2_points_cap_A(const GPoint& p0, const GPoint& p1, GContour* fill_contour, float width);
	void create_stroked_contour_2_points_cap_B(const GPoint& p0, const GPoint& p1, GContour* fill_contour, float width);
	void create_stroked_contour_2_points_no_cap(const GPoint& p0, const GPoint& p1, GContour* fill_contour, float width);


	// the bitmap our canvas draws onto and our matrix
	const GBitmap* m_bitmap;
	std::stack<GMatrix> m_global_ctm_stack;
	GMatrix m_global_ctm_current;
};

#endif