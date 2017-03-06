// Copyright Daniel J. Steffey -- 2016

#ifndef utils_hpp
#define utils_hpp

#include <string>
#include "include/GPoint.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"
#include "include/GPixel.h"
#include <vector>
#include <list>
#include "include/GRect.h"
#include "PolygonEdge.hpp"

GPixel convert_color_to_pixel(const GColor& color);

std::string convert_point_to_string(const GPoint& p, const std::string name);

std::string convert_matrix_to_string(const GMatrix& matrix, const std::string name);

void bitmap_setup(GBitmap& bitmap, int w, int h);

void bitmap_cleanup(GBitmap& bitmap);

std::string convert_bitmap_to_string(const GBitmap& bitmap);

std::string convert_pixel_to_string(const GPixel& pixel);

std::string convert_edge_list_to_string(const std::vector<PolygonEdge>& edges);

std::string convert_edge_list_to_string(const std::list<PolygonEdge>& edges);

std::string convert_edge_list_to_string(const std::vector<PolygonEdge*>& edges);

std::string convert_edge_list_to_string(const std::list<PolygonEdge*>& edges);

std::string convert_grect_to_string(const GRect& grect);

GPixel blend(const GPixel& source, const GPixel& destination);
void blend(const GPixel& source, GPixel* dest, int count);
void blend_opaque(const GPixel& source, GPixel* dest, int count);
void blend(const GPixel* source, GPixel* dest, int count);	
unsigned int divide_by_255(unsigned int p);

#endifit worked!!!