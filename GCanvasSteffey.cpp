// Copyright Daniel J. Steffey -- 2016

//#define _VERBOSE

#include "GCanvasSteffey.hpp"
#include <cstring>
#include "GShaderBitmapSteffey.hpp"
#include "GShaderColorTriangle.hpp"
#include "GShaderBitmapProxy.hpp"
#include "GShaderCompose.hpp"
#include <iostream>
#include "utils.hpp"

GCanvas* GCanvas::Create(const GBitmap& bitmap)
{
	// sanity checks for valid GBitmap
	if (bitmap.width() < 0)
	{
		return NULL;
	}
	if (bitmap.height() < 0)
	{
		return NULL;
	}
	if (bitmap.pixels() == NULL)
	{
		return NULL;
	}
	if (bitmap.rowBytes() < 4 * bitmap.width())
	{
		return NULL;
	}

	// seems the bitmap is valid
	return new GCanvasSteffey(bitmap);
}

GCanvasSteffey::GCanvasSteffey(const GBitmap& bitmap)
{
	// store the bitmap
	this->m_bitmap = &bitmap;

	// no ctm for now
	this->m_global_ctm_current.setIdentity();
}

GCanvasSteffey::~GCanvasSteffey()
{
	// nothing to destroy
}

void GCanvasSteffey::clear(const GColor& color)
{
	// clear the entire bitmap to the given color
	
	// first convert that nasty GColor into an GPixel
	GPixel new_pixel = convert_color_to_pixel(color.pinToUnit());
	
	// get pointer to beginning of pixels of the bitmap
	GPixel* pixels = this->m_bitmap->pixels();
	
	// get pointer to beginning of first row
	GPixel* row = pixels;
	
	// set all pixels in the first row to the color
	for (int x = 0; x < this->m_bitmap->width(); ++x)
	{
		row[x] = new_pixel;
	}
	
	// now memcpy the first row to all subsequent rows..the entire row at a time
	// this is faster than continuing to set each pixel at a time
	for (int y = 1; y < this->m_bitmap->height(); ++y)
	{
		// advance the pointer to the next row by the number of pixels wide
		// the actual bitmap memory takes up, which is number bytes / 4
		row += (this->m_bitmap->rowBytes() >> 2);
		
		// copy the first row to the current row
		std::memcpy(row, pixels, this->m_bitmap->rowBytes());
	}
}

void GCanvasSteffey::fillBitmapRect(const GBitmap& src, const GRect& dst)
{
	// compute the local matrix for the shader that will translate from the source bitmap
	// to the dst rect
	GMatrix translate_matrix;
	translate_matrix.setTranslate(dst.left(), dst.top());
	GMatrix scale_matrix;
	scale_matrix.setScale(dst.width() / (float)src.width(), dst.height() / (float)src.height());
	GMatrix local_ctm;
	local_ctm.setConcat(translate_matrix, scale_matrix);

	// now create the bitmap shader
	GShaderBitmapSteffey bitmap_shader(src, local_ctm, GShader::kClamp);

	// create our paint object
	GPaint paint(&bitmap_shader);

	// now draw it as a rect
	this->drawRect(dst, paint);
}

void GCanvasSteffey::save()
{
	// push the current matrix on the stack
	this->m_global_ctm_stack.push(this->m_global_ctm_current);
}

void GCanvasSteffey::restore()
{
	// make the current matrix the top element of the stack
	this->m_global_ctm_current = this->m_global_ctm_stack.top();
	// then pop it off the stack
	this->m_global_ctm_stack.pop();
}

void GCanvasSteffey::concat(const GMatrix& matrix)
{
	// set up the matrix so that CTM = CTM * matrix
	this->m_global_ctm_current = this->m_global_ctm_current.preConcat(matrix);
}

void GCanvasSteffey::drawRect(const GRect& rect, const GPaint& paint)
{
	// make points out of the 4 corners of the rect
	GPoint points[] = { GPoint::Make(rect.fLeft, rect.fTop), GPoint::Make(rect.fRight, rect.fTop),
						GPoint::Make(rect.fRight, rect.fBottom), GPoint::Make(rect.fLeft, rect.fBottom) };

	// call our draw polygon function
	this->drawConvexPolygon(points, 4, paint);
}

void GCanvasSteffey::drawConvexPolygon(const GPoint points[], int count, const GPaint& paint)
{
	// not enough points
	if (count < 3)
	{
		// quick reject
		return;
	}

	// create the clip rect the size of our canvas/bitmap
	GRect clip_rect = GRect::MakeWH(this->m_bitmap->width(), this->m_bitmap->height());


	// vector to hold our edges
	std::vector<PolygonEdge> edges;

	// foreach pair of points, send to the create_and_clip_polygon_edges 
	for (int i = 0; i < count - 1; ++i)
	{
		GPoint p1 = this->m_global_ctm_current.mapPt(points[i]);
		GPoint p2 = this->m_global_ctm_current.mapPt(points[i + 1]);
		GCanvasSteffey::create_and_clip_polygon_edges(p1, p2, clip_rect, edges);
	}
	// from last point to first point
	GCanvasSteffey::create_and_clip_polygon_edges(this->m_global_ctm_current.mapPt(points[count - 1]), this->m_global_ctm_current.mapPt(points[0]), clip_rect, edges);


	// check to see if we got any edges
	if (edges.size() < 2)
	{
		// nothing to draw
		return;
	}

	// now sort our edges
	GCanvasSteffey::sort_polygon_edges(edges);

	// draw
	// get left edge
	PolygonEdge* left_edge = &(edges[0]);

	// get right edge
	PolygonEdge* right_edge = &(edges[1]);
	
	// store index to next edge
	int next_edge_index = 2;

	// init our first scanline
	int current_scanline = left_edge->y_min;

	// convert that silly color into a pixel if needed (i.e. not using a shader)
	GPixel pixel;
	int pixel_alpha = 0;
	if (paint.getShader() == nullptr)
	{
		pixel = convert_color_to_pixel(paint.getColor().pinToUnit());
		pixel_alpha = GPixel_GetA(pixel);
	}
	else
	{
		// we have a shader so set its context
		paint.getShader()->setContext(this->m_global_ctm_current, paint.getAlpha());
	}

	// keep LOOPING forever and ever and ever...but return from the function when we are out of edges
	while (true)
	{
		// get the starting and ending x coords for this scanline
		int start_x = (int)(left_edge->x_current + 0.5f);
		int end_x = (int)(right_edge->x_current + 0.5f);

		// get the destination pixels
		GPixel* dest_pixels = this->m_bitmap->pixels() + ((this->m_bitmap->rowBytes() >> 2) * current_scanline);
		dest_pixels += start_x;

		// will it blend ?!?
		if (paint.getShader() != nullptr)
		{
			// do it with the shader
			GPixel buffer[256];
			int count = end_x - start_x;
			while (count > 0)
			{
				int n = std::min(count, 256);
				paint.getShader()->shadeRow(start_x, current_scanline, n, buffer);
				GCanvasSteffey::blend(buffer, dest_pixels, n);
				count -= n;
				start_x += n;
				dest_pixels += n;
			}
		}
		else
		{
			// do it with the color
			if (pixel_alpha == 255)
			{
				GCanvasSteffey::blend_opaque(pixel, dest_pixels, end_x - start_x + 0);
			}
			else
			{
				GCanvasSteffey::blend(pixel, dest_pixels, end_x - start_x + 0);
			}
		}
		// advance the scanline
		++current_scanline;

		// determine if we ran out of scanlines on the left edge
		if (current_scanline == left_edge->y_max)
		{
			if (next_edge_index == edges.size())
			{
				// out of edges
				return;
			}
			left_edge = &(edges[next_edge_index]);
			++next_edge_index;
		}
		else
		{
			left_edge->x_current += left_edge->m_slope;
		}

		// determine if we ran out of scanlines on the right edge
		if (current_scanline == right_edge->y_max)
		{
			if (next_edge_index == edges.size())
			{
				// out of edges
				return;
			}
			right_edge = &(edges[next_edge_index]);
			++next_edge_index;
		}
		else
		{
			right_edge->x_current += right_edge->m_slope;
		}
	}
}

void GCanvasSteffey::drawContours(const GContour ctrs[], int count, const GPaint& paint)
{
	#ifdef _VERBOSE
		std::cout << "drawContours" << std::endl;
		for (int i = 0; i < count; ++i)
		{
			std::cout << "\tctrs[" << i << "]\n";
			for (int j = 0; j < ctrs[i].fCount; ++j)
			{
				std::cout << "\t\t" << ctrs[i].fPts[j].fX << "," << ctrs[i].fPts[j].fY << "\n";
			}
		}
		std::cout << std::endl;
	#endif

	// create the clip rect the size of our canvas/bitmap
	GRect clip_rect = GRect::MakeWH(this->m_bitmap->width(), this->m_bitmap->height());

	#ifdef _VERBOSE
		std::cout << "clip_rect: " << convert_grect_to_string(clip_rect) << std::endl;
	#endif
	
	// vector to hold our edges
	std::vector<PolygonEdge> edges;

	// check if stroking
	if (paint.isStroke() == true)
	{
		this->draw_stroked_contours(ctrs, count, paint);
		return;

	}

	// put all the contours into an edge list
	for (int i = 0; i < count; ++i)
	{
		// need at least 3 points for a contour
		if (ctrs[i].fCount >= 3)
		{
			// foreach pair of points, send to the create_and_clip_polygon_edges 
			for (int j = 0; j < ctrs[i].fCount - 1; ++j)
			{
				GPoint p1 = this->m_global_ctm_current.mapPt(ctrs[i].fPts[j]);
				GPoint p2 = this->m_global_ctm_current.mapPt(ctrs[i].fPts[j + 1]);

				GCanvasSteffey::create_and_clip_polygon_edges(p1, p2, clip_rect, edges);
			}
			// from last point to first point
			GCanvasSteffey::create_and_clip_polygon_edges(this->m_global_ctm_current.mapPt(ctrs[i].fPts[ctrs[i].fCount - 1]),
				this->m_global_ctm_current.mapPt(ctrs[i].fPts[0]), clip_rect, edges);
		}
	}

	// check to see if we got any edges
	if (edges.size() < 2)
	{
		// nothing to draw
		return;
	}

	// now sort all our edges in y and then x
	GCanvasSteffey::sort_polygon_edges(edges);

	#ifdef _VERBOSE
		std::cout << "polygon edges sorted: \n";
		std::cout << convert_edge_list_to_string(edges) << "\n";
	#endif

	// convert that silly color into a pixel if needed (i.e. not using a shader)
	GPixel pixel;
	int pixel_alpha = 0;
	if (paint.getShader() == nullptr)
	{
		pixel = convert_color_to_pixel(paint.getColor().pinToUnit());
		pixel_alpha = GPixel_GetA(pixel);
	}
	else
	{
		// we have a shader so set its context
		paint.getShader()->setContext(this->m_global_ctm_current, paint.getAlpha());
	}

	// maintain a list of current drawing edges
	std::vector<PolygonEdge*> drawing_edges;

	// store index to next edge to check in the edge vector
	int next_edge_index = 0;

	// init our first scanline
	int current_scanline = 0;

	// keep track of checking if we need to sort
	bool need_to_sort = true;

	// loop forever and ever and ever....or at least until we run out of edges and return
	while (true)
	{
		// determine if any edges need to be removed
		for (int i = 0; i < drawing_edges.size(); ++i)
		{
			if (drawing_edges[i]->y_max <= current_scanline)
			{
				drawing_edges.erase(drawing_edges.begin() + i);
				--i;
				need_to_sort = true;
			}
		}

		// determine if any other edges need to be added
		if (drawing_edges.size() == 0)
		{
			// make sure we still have enough edges for drawing
			if (next_edge_index >= edges.size())
			{
				// out of edges, means we are done drawing
				return;
			}
			// just add in the first edge
			drawing_edges.push_back(&(edges[next_edge_index]));
			next_edge_index += 1;
			current_scanline = drawing_edges[0]->y_min;
			need_to_sort = true;
		}
		for (int i = next_edge_index; i < edges.size(); ++i)
		{
			if (edges[i].y_min == current_scanline)
			{
				drawing_edges.push_back(&(edges[i]));
				next_edge_index += 1;
				need_to_sort = true;
			}
			else
			{
				// break out of the for loop
				break;
			}
		}

		// ensure edges are sorted for drawing
		if (need_to_sort == true)
		{
			GCanvasSteffey::sort_polygon_drawing_edges(drawing_edges);
		}
		need_to_sort = false;

		#ifdef _VERBOSE
			std::cout << "\nscanline sorted polygon drawing edges\n";
			std::cout << convert_edge_list_to_string(drawing_edges);
		#endif

		// start our accumulator at 0
		int edge_orientation_accumulator = 0;

		// go through the edges looking for start and stop pixel runs to send to the renderer
		for (int i = 0; i < drawing_edges.size(); ++i)
		{
			// reset the orientation accumulator
			edge_orientation_accumulator = 0;

			// calculate the start x
			int start_x = (int)(drawing_edges[i]->x_current + 0.5f);

			// update the orientation accumulator
			edge_orientation_accumulator += drawing_edges[i]->m_orientation;

			// look for the end
			for (int j = i + 1; j < drawing_edges.size(); ++j)
			{
				edge_orientation_accumulator += drawing_edges[j]->m_orientation;

				// if the accumulator is 0 then we stop drawing here
				if (edge_orientation_accumulator == 0)
				{
					// this edge marks an end to a run
					int end_x = (int)(drawing_edges[j]->x_current + 0.5f);

					#ifdef _VERBOSE
						std::cout << "scanline=" << current_scanline << "\tstart=" << start_x << "\tend=" << (end_x - 0) << "\n";
					#endif

					// draw the run from start to end
					// get the destination pixels
					GPixel* dest_pixels = this->m_bitmap->pixels() + ((this->m_bitmap->rowBytes() >> 2) * current_scanline);
					dest_pixels += start_x;

					// will it blend ?!?
					if (paint.getShader() != nullptr)
					{
						// do it with the shader
						GPixel buffer[256];
						int count = end_x - start_x;
						while (count > 0)
						{
							int n = std::min(count, 256);
							paint.getShader()->shadeRow(start_x, current_scanline, n, buffer);
							GCanvasSteffey::blend(buffer, dest_pixels, n);
							count -= n;
							start_x += n;
							dest_pixels += n;
						}
					}
					else
					{
						// do it with the color
						if (pixel_alpha == 255)
						{
							GCanvasSteffey::blend_opaque(pixel, dest_pixels, end_x - start_x + 0);
						}
						else
						{
							GCanvasSteffey::blend(pixel, dest_pixels, end_x - start_x + 0);
						}
					}

					// update i to be the next edge after j
					i = j;

					// break out of the j loop
					break;
				}
			}
		}

		// before going to the next scanline we update all the x_currents
		drawing_edges[0]->x_current += drawing_edges[0]->m_slope;
		for (int i = 1; i < drawing_edges.size(); ++i)
		{
			drawing_edges[i]->x_current += drawing_edges[i]->m_slope;
			if (drawing_edges[i]->x_current < drawing_edges[i-1]->x_current)
			{
				// out of order now so need to resort
				need_to_sort = true;
			}
		}

		// scanline complete
		current_scanline += 1;
	}
}

inline GPixel GCanvasSteffey::blend(const GPixel& source, const GPixel& destination)
{
	// precalc once what we can
	unsigned int source_pixel_a = GPixel_GetA(source);
	int sa_compute = 255 - source_pixel_a;


	if (sa_compute == 0)
	{
		// full alpha so only the source
		return source;
	}
	else if (sa_compute == 255)
	{
		// no alpha so only the dest
		return destination;
	}

	// calculate each result color component
	unsigned int Ra = source_pixel_a + divide_by_255(sa_compute * GPixel_GetA(destination));
	unsigned int Rr = GPixel_GetR(source) + divide_by_255(sa_compute * GPixel_GetR(destination));
	unsigned int Rg = GPixel_GetG(source) + divide_by_255(sa_compute * GPixel_GetG(destination));
	unsigned int Rb = GPixel_GetB(source) + divide_by_255(sa_compute * GPixel_GetB(destination));

	// pack it into a pixel
	return GPixel_PackARGB(Ra, Rr, Rg, Rb);
}

inline void GCanvasSteffey::blend(const GPixel& source, GPixel* dest, int count)
{
	for (int i = 0; i < count; ++i)
	{
		dest[i] = GCanvasSteffey::blend(source, dest[i]);
	}
}

inline void GCanvasSteffey::blend_opaque(const GPixel& source, GPixel* dest, int count)
{
	for (int i = 0; i < count; ++i)
	{
		dest[i] = source;
	}	
}

inline void GCanvasSteffey::blend(const GPixel* source, GPixel* dest, int count)
{
	for (int i = 0; i < count; ++i)
	{
		dest[i] = GCanvasSteffey::blend(source[i], dest[i]);
	}
}

inline unsigned int GCanvasSteffey::divide_by_255(unsigned int p)
{
	// fast divide by 255 by approximating very very *very* close
	// and getting to use a shift instead of divide
	return (p * 65793 + (1 << 23)) >> 24;
}

inline void GCanvasSteffey::sort_polygon_edges(std::vector<PolygonEdge>& edges)
{
	std::sort(edges.begin(), edges.end(), [] (const PolygonEdge& a, const PolygonEdge& b)
		{
			if (a.y_min < b.y_min)
			{
				// a is higher than b
				return true;
			}
			else if (a.y_min > b.y_min)
			{
				// b is higher than a
				return false;
			}
			else
			{
				// same height
				if (a.x_current < b.x_current)
				{
					// a starting x is to the left of b starting x
					return true;
				}
				else if (a.x_current > b.x_current)
				{
					// b starting x is to the left of a starting x
					return false;
				}
				else
				{
					// same exact point...compare the slopes
					if (a.m_slope < b.m_slope)
					{
						// a is to the left
						return true;
					}
					return false;
				}
			}
			return false;
		}
	);
}

inline void GCanvasSteffey::sort_polygon_drawing_edges(std::vector<PolygonEdge*>& edges)
{
	// only sort on x_current
	std::sort(edges.begin(), edges.end(), [] (const PolygonEdge* a, const PolygonEdge* b)
		{
			if (a->x_current <= b->x_current)
			{
				// a x is to the left of (or equal to) b x
				return true;
			}
			return false;
		}
	);
}

void GCanvasSteffey::create_and_clip_polygon_edges(const GPoint& p0, const GPoint& p1, const GRect& clip_rect, std::vector<PolygonEdge>& edges)
{
	int orientation = 1;

	// our working points
	GPoint working_p0;
	GPoint working_p1;

	// sort in y
	if (p0.fY <= p1.fY)
	{
		working_p0 = p0;
		working_p1 = p1;
	}
	else
	{
		working_p0 = p1;
		working_p1 = p0;
		orientation = -orientation;
	}

	// quick reject top
	if (working_p1.fY < clip_rect.fTop)
	{
		// entire line segment is above the clip rect
		return;
	}

	// quick reject bottom
	if (working_p0.fY > clip_rect.fBottom)
	{
		// entire line segment is below the clip rect
		return;
	}

	// chop y against top
	if (working_p0.fY < clip_rect.fTop)
	{
		// recompute top point
		working_p0.fX = working_p0.fX + (working_p1.fX - working_p0.fX) * (clip_rect.fTop - working_p0.fY) / (working_p1.fY - working_p0.fY);
		working_p0.fY = clip_rect.fTop;
	}

	// chop y against bottom
	if (working_p1.fY > clip_rect.fBottom)
	{
		// recompute bottom point
		working_p1.fX = working_p1.fX + (working_p0.fX - working_p1.fX) * (clip_rect.fBottom - working_p1.fY) / (working_p0.fY - working_p1.fY);
		working_p1.fY = clip_rect.fBottom;	
	}

	// check if horizontal line and reject
	if ((int)(working_p0.fY + 0.5f) == (int)(working_p1.fY + 0.5f))
	{
		return;
	}

	// sort in x
	if (working_p0.fX > working_p1.fX)
	{
		GPoint temp = working_p0;
		working_p0 = working_p1;
		working_p1 = temp;
		orientation = -orientation;
	}

	// check which of 6 cases of location in/out the clip rect our points are
	if (working_p1.fX <= clip_rect.fLeft)
	{
		// both points to left of clip_rect (or on the boundry)
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(working_p0.fY + 0.5f);
			int y_max = (int)(working_p1.fY + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fLeft, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			orientation = -orientation;
			int y_min = (int)(working_p1.fY + 0.5f);
			int y_max = (int)(working_p0.fY + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fLeft, orientation);
				edges.push_back(edge);
			}
		}
	}
	else if (working_p0.fX >= clip_rect.fRight)
	{
		// both points to the right of clip_rect (or on the boundry)
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(working_p0.fY + 0.5f);
			int y_max = (int)(working_p1.fY + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fRight, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			orientation = -orientation;
			int y_min = (int)(working_p1.fY + 0.5f);
			int y_max = (int)(working_p0.fY + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fRight, orientation);
				edges.push_back(edge);
			}
		}
	}
	else if ((working_p0.fX < clip_rect.fLeft) && (working_p1.fX <= clip_rect.fRight))
	{
		// p0 to the left and p1 inside
		float m_slope = (working_p1.fY - working_p0.fY) / (working_p1.fX - working_p0.fX);
		float inv_slope = (working_p1.fX - working_p0.fX) / (working_p1.fY - working_p0.fY);
		float y_intercept = m_slope * (clip_rect.fLeft - working_p0.fX) + working_p0.fY;

		// edge bordering clip left
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(working_p0.fY + 0.5f);
			int y_max = (int)(y_intercept + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fLeft, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			orientation = -orientation;
			int y_min = (int)(y_intercept + 0.5f);
			int y_max = (int)(working_p0.fY + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fLeft, orientation);
				edges.push_back(edge);
			}
		}

		// edge from clip-left, y_intercept to p1
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(y_intercept + 0.5f);
			int y_max = (int)(working_p1.fY + 0.5f);
			if (y_min != y_max)
			{
				float x_current = ((y_min + 0.5f) - y_intercept) * inv_slope + clip_rect.fLeft;
				PolygonEdge edge(y_min, y_max, inv_slope, x_current, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			int y_min = (int)(working_p1.fY + 0.5f);
			int y_max = (int)(y_intercept + 0.5f);
			if (y_min != y_max)
			{
				float x_current = ((y_min + 0.5f) - y_intercept) * inv_slope + clip_rect.fLeft;
				PolygonEdge edge(y_min, y_max, inv_slope, x_current, orientation);
				edges.push_back(edge);
			}
		}
	}
	else if ((working_p0.fX >= clip_rect.fLeft) && (working_p1.fX > clip_rect.fRight))
	{
		// p0 inside and p1 to the right
		float m_slope = (working_p1.fY - working_p0.fY) / (working_p1.fX - working_p0.fX);
		float inv_slope = (working_p1.fX - working_p0.fX) / (working_p1.fY - working_p0.fY);
		float y_intercept = m_slope * (clip_rect.fRight - working_p0.fX) + working_p0.fY;

		// edge bordering clip right
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(y_intercept + 0.5f);
			int y_max = (int)(working_p1.fY + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fRight, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			orientation = -orientation;
			int y_min = (int)(working_p1.fY + 0.5f);
			int y_max = (int)(y_intercept + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fRight, orientation);
				edges.push_back(edge);
			}
		}

		// edge from p0 to clip-right, y_intercept
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(working_p0.fY + 0.5f);
			int y_max = (int)(y_intercept + 0.5f);
			if (y_min != y_max)
			{
				float x_current = ((y_min + 0.5f) - working_p0.fY) * inv_slope + working_p0.fX;
				PolygonEdge edge(y_min, y_max, inv_slope, x_current, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			int y_min = (int)(y_intercept + 0.5f);
			int y_max = (int)(working_p0.fY + 0.5f);
			if (y_min != y_max)
			{
				float x_current = ((y_min + 0.5f) - working_p0.fY) * inv_slope + working_p0.fX;
				PolygonEdge edge(y_min, y_max, inv_slope, x_current, orientation);
				edges.push_back(edge);
			}
		}
	}
	else if ((working_p0.fX < clip_rect.fLeft) && (working_p1.fX > clip_rect.fRight))
	{
		// p0 to the left and p1 to the right
		float m_slope = (working_p1.fY - working_p0.fY) / (working_p1.fX - working_p0.fX);
		float inv_slope = (working_p1.fX - working_p0.fX) / (working_p1.fY - working_p0.fY);

		// clip-left edge
		float y_intercept_left = m_slope * (clip_rect.fLeft - working_p0.fX) + working_p0.fY;
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(working_p0.fY + 0.5f);
			int y_max = (int)(y_intercept_left + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fLeft, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			orientation = -orientation;
			int y_min = (int)(y_intercept_left + 0.5f);
			int y_max = (int)(working_p0.fY + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fLeft, orientation);
				edges.push_back(edge);
			}
		}

		// clip-right edge
		float y_intercept_right = m_slope * (clip_rect.fRight - working_p0.fX) + working_p0.fY;
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(y_intercept_right + 0.5f);
			int y_max = (int)(working_p1.fY + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fRight, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			int y_min = (int)(working_p1.fY + 0.5f);
			int y_max = (int)(y_intercept_right + 0.5f);
			if (y_min != y_max)
			{
				PolygonEdge edge(y_min, y_max, 0.0f, clip_rect.fRight, orientation);
				edges.push_back(edge);
			}
		}

		// now line segment inside of the clip area from clip-left, y-intercept-left to clip-right, y-intercept-right
		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(y_intercept_left + 0.5f);
			int y_max = (int)(y_intercept_right + 0.5f);
			if (y_min != y_max)
			{
				float x_current = ((y_min + 0.5f) - y_intercept_left) * inv_slope + clip_rect.fLeft;
				PolygonEdge edge(y_min, y_max, inv_slope, x_current, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			int y_min = (int)(y_intercept_right + 0.5f);
			int y_max = (int)(y_intercept_left + 0.5f);
			if (y_min != y_max)
			{
				float x_current = ((y_min + 0.5f) - y_intercept_left) * inv_slope + clip_rect.fLeft;
				PolygonEdge edge(y_min, y_max, inv_slope, x_current, orientation);
				edges.push_back(edge);
			}
		}
	}
	else
	{
		// p0 and p1 inside
		float inv_slope = (working_p1.fX - working_p0.fX) / (working_p1.fY - working_p0.fY);

		// ensure we are ordered in y from min to max
		if (working_p0.fY < working_p1.fY)
		{
			int y_min = (int)(working_p0.fY + 0.5f);
			int y_max = (int)(working_p1.fY + 0.5f);
			if (y_min != y_max)
			{
				float x_current = ((y_min + 0.5f) - working_p0.fY) * inv_slope + working_p0.fX;
				PolygonEdge edge(y_min, y_max, inv_slope, x_current, orientation);
				edges.push_back(edge);
			}
		}
		else
		{
			orientation = -orientation;
			int y_min = (int)(working_p1.fY + 0.5f);
			int y_max = (int)(working_p0.fY + 0.5f);
			if (y_min != y_max)
			{
				float x_current = ((y_min + 0.5f) - working_p0.fY) * inv_slope + working_p0.fX;
				PolygonEdge edge(y_min, y_max, inv_slope, x_current, orientation);
				edges.push_back(edge);
			}
		}
	}
}

void GCanvasSteffey::draw_stroked_contours(const GContour contours[], int count, const GPaint& paint)
{
	// TODO come back and figure out the joints if I have time
	// I have tried 3-4 different approaches and could not get it to 
	// work properly....i made the images worse
	
	// this function will take our array of contours
	// for EACH contour that represents a stroked line, we will generate alot of contours
	// for that single stroked contour and add it to a list
	// once all have been processed then we will have a list of contours and need to be filled
	// and we can call our normal drawContour method
	// create a container of new generated contours
	std::vector<GContour> contour_vector;

	// loop through each input contour
	for (int i = 0; i < count; ++i)
	{
		// get a pointer to this contour for easy handling
		const GContour* contour = &(contours[i]);

		if (contour->fCount < 2)
		{
			// not enough points
			// continue this i loop
			continue;
		}
		else if (contour->fCount == 2)
		{
			// simple with no joints
			// put a new contour in the vector
			GContour empty;
			contour_vector.push_back(empty);
		
			// get a pointer to it for manipulation
			GContour* new_contour = &(contour_vector[contour_vector.size() - 1]);

			// call our special for 2 points
			this->create_stroked_contour_2_points_cap_AB(contour->fPts[0], contour->fPts[1], new_contour, paint.getStrokeWidth());

			// continue with the next i
			continue;
		}
		else
		{
			// 3 or more points....harder
			// loop through all the points in the contour
			for (int j = 0; j < contour->fCount; ++j)
			{
				#ifdef _VERBOSE
					std::cout << "counter: " << i << " point: " << j << "\n";
				#endif
				// check if we need to go from last point to first point
				if ((j == (contour->fCount - 1)) && (contour->fClosed == false))
				{
					// we are on the last point
					// if closed is false then nothing else to do and break out of this j loop
					// this will cause it to go back to the next i
					break;
				}
				else if ((j == (contour->fCount - 2)) && (contour->fClosed == false))
				{
					// next to last point AND NOT closed
					// so just need to build a simple quad from A to B and cap B
					// no other joint needed
					// put a new contour in the vector
					GContour empty;
					contour_vector.push_back(empty);
		
					// get a pointer to it for manipulation
					GContour* new_contour = &(contour_vector[contour_vector.size() - 1]);
					this->create_stroked_contour_2_points_cap_B(contour->fPts[contour->fCount - 2], contour->fPts[contour->fCount - 1], new_contour, paint.getStrokeWidth());

					// finsihed with this contour
					// break out of j
					break;
				}
				else
				{
					// we will have valid A, B, C points to build the quad and joint
					// gather our known points
					GPoint A, B, C, Q, P, R;
					if (j == contour->fCount - 2)
					{
						A = contour->fPts[j];
						B = contour->fPts[j+1];
						C = contour->fPts[0];
					}
					else if (j == contour->fCount - 1)
					{
						A = contour->fPts[j];
						B = contour->fPts[0];
						C = contour->fPts[1];
					}
					else
					{
						A = contour->fPts[j];
						B = contour->fPts[j+1];
						C = contour->fPts[j+2];
					}

					// compute width over 2
					float w2 = paint.getStrokeWidth() / 2.0f;

					// create all of our vectors
					float ABx = B.fX - A.fX;
					float ABy = B.fY - A.fY;
					float length = sqrt((ABx * ABx) + (ABy * ABy));
					if (length == 0.0f)
					{
						// A and B are same point
						// skip this whole thing
						// continue to the next j
						continue;
					}
					ABx /= length;
					ABy /= length;

					float BCx = C.fX - B.fX;
					float BCy = C.fY - B.fY;
					length = sqrt((BCx * BCx) + (BCy * BCy));
					if (length == 0.0f)
					{
						// B and C are same point
						// just make the AB quad and forget C
						GContour empty;
						contour_vector.push_back(empty);
			
						// get a pointer to it for manipulation
						GContour* new_contour = &(contour_vector[contour_vector.size() - 1]);
						this->create_stroked_contour_2_points_no_cap(A, B, new_contour, paint.getStrokeWidth());

						// continue to next j
						continue;
					}

					BCx /= length;
					BCy /= length;

					float BQx = ABy;
					float BQy = -ABx;

					float BRx = BCy;
					float BRy = -BCx;

					Q.fX = B.fX + (BQx * w2);
					Q.fY = B.fY + (BQy * w2);

					R.fX = B.fX + (BRx * w2);
					R.fY = B.fY + (BRy * w2);

					float BPx = BQx + BRx;
					float BPy = BQy + BRy;
					length = sqrt((BPx * BPx) + (BPy * BPy));
					BPx /= length;
					BPy /= length;


					float ABdotBC = (-ABx * BCx) + (-ABy * BCy);
					float ABxBC = (-ABx * BCy) - (-ABy * BCx);

					float BPlength = w2 * sqrt(2 / (1 - ABdotBC));



					// should have everything we need now
					// first the quad 
					if (j == 0 && contour->fClosed == false)
					{
						// create the contour with A capped and B not
						GContour empty;
						contour_vector.push_back(empty);
				
						// get a pointer to it for manipulation
						GContour* new_contour = &(contour_vector[contour_vector.size() - 1]);
						this->create_stroked_contour_2_points_cap_A(A, B, new_contour, paint.getStrokeWidth());
					}
					else
					{
						// create the contour with neither capped
						GContour empty;
						contour_vector.push_back(empty);
				
						// get a pointer to it for manipulation
						GContour* new_contour = &(contour_vector[contour_vector.size() - 1]);
						this->create_stroked_contour_2_points_no_cap(A, B, new_contour, paint.getStrokeWidth());
					}
					// lets try and build that joint now
					if (BPlength > (paint.getMiterLimit() * w2))
					{
						// just blunt it between BQR
						if (ABxBC == 0)
						{
							// straight, no need for joint
						}
						else
						{
							// create the BQR triangle
							GContour empty;
							contour_vector.push_back(empty);
					
							// get a pointer to it for manipulation
							GContour* new_contour = &(contour_vector[contour_vector.size() - 1]);

							// setup the new contour
							new_contour->fCount = 3;
							GPoint* points = new GPoint[3];
							new_contour->fPts = &(points[0]);
							new_contour->fClosed = true;
							if (ABxBC > 0)
							{
								Q.fX = B.fX + (BQx * -w2);
								Q.fY = B.fY + (BQy * -w2);
								R.fX = B.fX + (BRx * -w2);
								R.fY = B.fY + (BRy * -w2);
								points[0] = B;
								points[1] = Q;
								points[2] = R;
							}
							else if (ABxBC < 0)
							{
								points[0] = B;
								points[1] = Q;
								points[2] = R;
							}
						}
					}
					else
					{
						if (ABxBC == 0)
						{
							// straight ? no need to do a joint
						}
						else
						{
							// create the BQPR quad
							// create a new quad
							GContour empty;
							contour_vector.push_back(empty);
					
							// get a pointer to it for manipulation
							GContour* new_contour = &(contour_vector[contour_vector.size() - 1]);

							// setup the new contour
							new_contour->fCount = 4;
							GPoint* points = new GPoint[4];
							new_contour->fPts = &(points[0]);
							new_contour->fClosed = true;

							if (ABxBC > 0)
							{
								// bend one way
								// create the 4 corner points
								P.fX = B.fX + BPx * -BPlength;
								P.fY = B.fY + BPy * -BPlength;
								Q.fX = B.fX + (BQx * -w2);
								Q.fY = B.fY + (BQy * -w2);
								R.fX = B.fX + (BRx * -w2);
								R.fY = B.fY + (BRy * -w2);

								points[0] = B;
								points[1] = Q;
								points[2] = P;
								points[3] = R;
							}
							else if (ABxBC < 0)
							{
								// bend the other
								// create the 4 corner points
								P.fX = B.fX + BPx * BPlength;
								P.fY = B.fY + BPy * BPlength;
								points[0] = B;
								points[1] = R;
								points[2] = P;
								points[3] = Q;
							}
						}
					}
					#ifdef _VERBOSE
						std::cout << convert_point_to_string(A, "A") << "\n";
						std::cout << convert_point_to_string(B, "B") << "\n";
						std::cout << convert_point_to_string(C, "C") << "\n";
						std::cout << convert_point_to_string(Q, "Q") << "\n";
						std::cout << convert_point_to_string(R, "R") << "\n";
						std::cout << convert_point_to_string(P, "P") << "\n";
						std::cout << "AB: " << ABx << " " << ABy << "\n";
						std::cout << "BQ: " << BQx << " " << BQy << "\n";
						std::cout << "BC: " << BCx << " " << BCy << "\n";
						std::cout << "BR: " << BRx << " " << BRy << "\n";
						std::cout << "BP: " << BPx << " " << BPy << "\n";
						std::cout << "BPlength: " << BPlength << "\n";
						std::cout << "ABdotBC: " << ABdotBC << "\n";
						std::cout << "ABxBC: " << ABxBC << "\n";
					#endif
				}
			}
		}

	}

	// now we have a container full of contours, lets call drawContours on it
	// adjust the paint width since we have made the strokes
	GPaint new_paint;
	new_paint.setColor(paint.getColor());
	new_paint.setShader(paint.getShader());
	new_paint.setFill();
	// miter limit doesnt matter

	this->drawContours(&(contour_vector[0]), contour_vector.size(), new_paint);
	// free up our memory
	for (int i = 0; i < contour_vector.size(); ++i)
	{
		delete contour_vector[i].fPts;
	}
}

void GCanvasSteffey::create_stroked_contour_2_points_cap_AB(const GPoint& p0, const GPoint& p1, GContour* fill_contour, float width)
{
	// get points A and B
	GPoint A = p0;
	GPoint B = p1;

	// compute the vector between them
	float ABx = p1.fX - p0.fX;
	float ABy = p1.fY - p0.fY;

	// normalize it
	float length = sqrt((ABx * ABx) + (ABy * ABy));
	ABx = ABx / length;
	ABy = ABy / length;

	// extend out A and B by width / 2
	// this is for capping
	float w2 = width / 2.0f;
	A.fX = A.fX - (ABx * w2);
	A.fY = A.fY - (ABy * w2);
	B.fX = B.fX + (ABx * w2);
	B.fY = B.fY + (ABy * w2);

	// now compute the transpose of AB
	float ABTx = -ABy;
	float ABTy = ABx;

	// setup the new contour
	fill_contour->fCount = 4;
	GPoint* points = new GPoint[4];
	fill_contour->fPts = &(points[0]);
	fill_contour->fClosed = true;

	// create the 4 corner points
	points[0] = GPoint::Make(A.fX + ABTx * w2, A.fY + ABTy * w2);
	points[1] = GPoint::Make(B.fX + ABTx * w2, B.fY + ABTy * w2);
	points[2] = GPoint::Make(B.fX + -ABTx * w2, B.fY + -ABTy * w2);
	points[3] = GPoint::Make(A.fX + -ABTx * w2, A.fY + -ABTy * w2);
}

void GCanvasSteffey::create_stroked_contour_2_points_cap_B(const GPoint& p0, const GPoint& p1, GContour* fill_contour, float width)
{
	// get points A and B
	GPoint A = p0;
	GPoint B = p1;

	// compute the vector between them
	float ABx = p1.fX - p0.fX;
	float ABy = p1.fY - p0.fY;

	// normalize it
	float length = sqrt((ABx * ABx) + (ABy * ABy));
	ABx = ABx / length;
	ABy = ABy / length;

	// extend out B by width / 2
	// this is for capping
	float w2 = width / 2.0f;
	B.fX = B.fX + (ABx * w2);
	B.fY = B.fY + (ABy * w2);

	// now compute the transpose of AB
	float ABTx = -ABy;
	float ABTy = ABx;

	// setup the new contour
	fill_contour->fCount = 4;
	GPoint* points = new GPoint[4];
	fill_contour->fPts = &(points[0]);
	fill_contour->fClosed = true;

	// create the 4 corner points
	points[0] = GPoint::Make(A.fX + ABTx * w2, A.fY + ABTy * w2);
	points[1] = GPoint::Make(B.fX + ABTx * w2, B.fY + ABTy * w2);
	points[2] = GPoint::Make(B.fX + -ABTx * w2, B.fY + -ABTy * w2);
	points[3] = GPoint::Make(A.fX + -ABTx * w2, A.fY + -ABTy * w2);
}

void GCanvasSteffey::create_stroked_contour_2_points_cap_A(const GPoint& p0, const GPoint& p1, GContour* fill_contour, float width)
{
	// get points A and B
	GPoint A = p0;
	GPoint B = p1;

	// compute the vector between them
	float ABx = p1.fX - p0.fX;
	float ABy = p1.fY - p0.fY;

	// normalize it
	float length = sqrt((ABx * ABx) + (ABy * ABy));
	ABx = ABx / length;
	ABy = ABy / length;

	// extend out A by width / 2
	// this is for capping
	float w2 = width / 2.0f;
	A.fX = A.fX - (ABx * w2);
	A.fY = A.fY - (ABy * w2);

	// now compute the transpose of AB
	float ABTx = -ABy;
	float ABTy = ABx;

	// setup the new contour
	fill_contour->fCount = 4;
	GPoint* points = new GPoint[4];
	fill_contour->fPts = &(points[0]);
	fill_contour->fClosed = true;

	// create the 4 corner points
	points[0] = GPoint::Make(A.fX + ABTx * w2, A.fY + ABTy * w2);
	points[1] = GPoint::Make(B.fX + ABTx * w2, B.fY + ABTy * w2);
	points[2] = GPoint::Make(B.fX + -ABTx * w2, B.fY + -ABTy * w2);
	points[3] = GPoint::Make(A.fX + -ABTx * w2, A.fY + -ABTy * w2);
}

void GCanvasSteffey::create_stroked_contour_2_points_no_cap(const GPoint& p0, const GPoint& p1, GContour* fill_contour, float width)
{
	// get points A and B
	GPoint A = p0;
	GPoint B = p1;

	// compute the vector between them
	float ABx = p1.fX - p0.fX;
	float ABy = p1.fY - p0.fY;

	// normalize it
	float length = sqrt((ABx * ABx) + (ABy * ABy));
	ABx = ABx / length;
	ABy = ABy / length;

	// now compute the transpose of AB
	float ABTx = -ABy;
	float ABTy = ABx;

	float w2 = width / 2.0f;

	// setup the new contour
	fill_contour->fCount = 4;
	GPoint* points = new GPoint[4];
	fill_contour->fPts = &(points[0]);
	fill_contour->fClosed = true;

	// create the 4 corner points
	points[0] = GPoint::Make(A.fX + ABTx * w2, A.fY + ABTy * w2);
	points[1] = GPoint::Make(B.fX + ABTx * w2, B.fY + ABTy * w2);
	points[2] = GPoint::Make(B.fX + -ABTx * w2, B.fY + -ABTy * w2);
	points[3] = GPoint::Make(A.fX + -ABTx * w2, A.fY + -ABTy * w2);
}

void GCanvasSteffey::drawMesh(int triCount, const GPoint pts[], const int indices[], const GColor colors[], const GPoint tex[], const GPaint& paint)
{

	int index = 0;
	for (int i = 0; i < triCount; ++i)
	{
		GPoint p[3];
		GColor c[3];
		GPoint t[3];
		GContour contour;
		contour.fCount = 3;
		contour.fClosed = true;
		contour.fPts = p;
		if (indices == nullptr)
		{
			p[0] = pts[index + 0];
			p[1] = pts[index + 1];
			p[2] = pts[index + 2];

			if (colors != nullptr)
			{
				c[0] = colors[index + 0];
				c[1] = colors[index + 1];
				c[2] = colors[index + 2];
			}

			if (tex != nullptr)
			{
				t[0] = tex[index + 0];
				t[1] = tex[index + 1];
				t[2] = tex[index + 2];
			}
		}
		else
		{
			int i0 = indices[index + 0];
			int i1 = indices[index + 1];
			int i2 = indices[index + 2];

			p[0] = pts[i0];
			p[1] = pts[i1];
			p[2] = pts[i2];

			if (colors != nullptr)
			{
				c[0] = colors[i0];
				c[1] = colors[i1];
				c[2] = colors[i2];
			}

			if (tex != nullptr)
			{
				t[0] = tex[i0];
				t[1] = tex[i1];
				t[2] = tex[i2];
			}
		}

		// now construct a paint with a shader based on if we have colors and/or tex
		GPaint new_paint;
		// set the alpha
		new_paint.setAlpha(paint.getAlpha());

		if (colors != nullptr && tex != nullptr)
		{
			// do the color shader
			GShaderColorTriangle tri_shader(p, c);

			// do the bitmap proxy shader
			GShaderBitmapProxy tex_shader(paint.getShader(), p, t);

			// do the composite shader
			GShaderCompose comp_shader(&tex_shader, &tri_shader);
			new_paint.setShader(&comp_shader);
			this->drawContours(&contour, 1, new_paint);

		}
		else if (colors != nullptr && tex == nullptr)
		{
			// do the color shader
			GShaderColorTriangle tri_shader(p, c);
			new_paint.setShader(&tri_shader);
			this->drawContours(&contour, 1, new_paint);
		}
		else if (colors == nullptr && tex != nullptr)
		{
			// do the bitmap proxy shader
			GShaderBitmapProxy tex_shader(paint.getShader(), p, t);
			new_paint.setShader(&tex_shader);
			this->drawContours(&contour, 1, new_paint);
		}
		else
		{
			// give up
			continue;
		}

		index += 3;
	}
}

GShader* GCanvasSteffey::makeRadialGradient(float cx, float cy, float radius, const GColor colors[], int count)
{
	return new GShaderRadial(cx, cy, radius, colors, count);
}