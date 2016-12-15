// Copyright Daniel J. Steffey -- 2016

#include "utils.hpp"
#include <sstream>

GPixel convert_color_to_pixel(const GColor& color)
{
	// convert the GColor (un-premult in floats) to an CPixel (premult in bytes)
	unsigned int a = color.fA * 255 + 0.5f;
	unsigned int r = color.fR * 255 * color.fA + 0.5f;
	unsigned int g = color.fG * 255 * color.fA + 0.5f;
	unsigned int b = color.fB * 255 * color.fA + 0.5f;
	return GPixel_PackARGB(a, r, g, b);
}

#include <iostream>
std::string convert_point_to_string(const GPoint& p, const std::string name)
{
	std::stringstream ss;
	ss << "GPoint " << name << " = <" << p.fX << ", " << p.fY << ">";
	return ss.str();
}

std::string convert_matrix_to_string(const GMatrix& matrix, const std::string name)
{
	std::stringstream ss;
	ss << "==========Matrix " << name << "==========" << std::endl;
	ss << matrix[GMatrix::SX] << "\t" << matrix[GMatrix::KX] << "\t" << matrix[GMatrix::TX] << std::endl;
	ss << matrix[GMatrix::KY] << "\t" << matrix[GMatrix::SY] << "\t" << matrix[GMatrix::TY] << std::endl;
	ss << "==============================";
	ss << std::endl;
	return ss.str();
}

void bitmap_setup(GBitmap& bitmap, int w, int h)
{
	bitmap.fWidth = w;
	bitmap.fHeight = h;
	bitmap.fRowBytes = bitmap.fWidth * 4;
	bitmap.fPixels = new GPixel[bitmap.fWidth * bitmap.fHeight];
}

void bitmap_cleanup(GBitmap& bitmap)
{

	delete [] bitmap.fPixels;
}

std::string convert_bitmap_to_string(const GBitmap& bitmap)
{
	std::stringstream ss;
	for (int y = 0; y < bitmap.fHeight; ++y)
	{
		for (int x = 0; x < bitmap.fWidth; ++x)
		{
			GPixel* p = bitmap.getAddr(x, y);
			ss << /*GPixel_GetA(*p) << "," <<*/ GPixel_GetR(*p) << /*"," << GPixel_GetG(*p) << "," << GPixel_GetB(*p) << */"\t";
		}
		ss << std::endl;
	}
	return ss.str();
}

std::string convert_pixel_to_string(const GPixel& pixel)
{
	// convert the pixel to string for pleasant viewing
	std::stringstream ss;
	ss << "[";
	ss << GPixel_GetA(pixel) << ", ";
	ss << GPixel_GetR(pixel) << ", ";
	ss << GPixel_GetG(pixel) << ", ";
	ss << GPixel_GetB(pixel) << "]";
	return ss.str();
}


std::string convert_edge_list_to_string(const std::vector<PolygonEdge>& edges)
{
	// convert the list of edges to a string for pleasant viewing
	std::stringstream ss;
	for (auto& edge : edges)
	{
		ss << edge.to_string() << std::endl;
	}
	return ss.str();
}

std::string convert_edge_list_to_string(const std::list<PolygonEdge>& edges)
{
	// convert the list of edges to a string for pleasant viewing
	std::stringstream ss;
	for (auto& edge : edges)
	{
		ss << edge.to_string() << std::endl;
	}
	return ss.str();
}

std::string convert_edge_list_to_string(const std::vector<PolygonEdge*>& edges)
{
	// convert the list of edges to a string for pleasant viewing
	std::stringstream ss;
	for (auto& edge : edges)
	{
		ss << edge->to_string() << std::endl;
	}
	return ss.str();
}

std::string convert_edge_list_to_string(const std::list<PolygonEdge*>& edges)
{
	// convert the list of edges to a string for pleasant viewing
	std::stringstream ss;
	for (auto& edge : edges)
	{
		ss << edge->to_string() << std::endl;
	}
	return ss.str();
}

std::string convert_grect_to_string(const GRect& grect)
{
	std::stringstream ss;
	ss << "GRect=[" << std::to_string(grect.fLeft) << ", " << std::to_string(grect.fTop) << ", " << 
		std::to_string(grect.fRight) << ", " << std::to_string(grect.fBottom) << "]";
	return ss.str();
}

GPixel blend(const GPixel& source, const GPixel& destination)
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

void blend(const GPixel& source, GPixel* dest, int count)
{
	for (int i = 0; i < count; ++i)
	{
		dest[i] = blend(source, dest[i]);
	}
}

void blend_opaque(const GPixel& source, GPixel* dest, int count)
{
	for (int i = 0; i < count; ++i)
	{
		dest[i] = source;
	}	
}

void blend(const GPixel* source, GPixel* dest, int count)
{
	for (int i = 0; i < count; ++i)
	{
		dest[i] = blend(source[i], dest[i]);
	}
}

unsigned int divide_by_255(unsigned int p)
{
	// fast divide by 255 by approximating very very *very* close
	// and getting to use a shift instead of divide
	return (p * 65793 + (1 << 23)) >> 24;
}
