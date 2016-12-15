// Copyright Daniel J. Steffey -- 2016

#ifndef PolygonEdge_hpp
#define PolygonEdge_hpp

#include <string>

struct PolygonEdge
{
	int y_min;
	int y_max;
	float m_slope;
	float x_current;
	int m_orientation;
	
	PolygonEdge(int y_min, int y_max, float m_slope, float x_current, int orientation)
	{
		this->y_min = y_min;
		this->y_max = y_max;
		this->m_slope = m_slope;
		this->x_current = x_current;
		this->m_orientation = orientation;
	}

	std::string to_string() const
	{
		std::string s = "PolygonEdge{ " + std::to_string(this->y_min) + ", " + std::to_string(this->y_max) + ", " + 
			std::to_string(this->m_slope) + ", " + std::to_string(this->x_current) + ", " +
			std::to_string(this->m_orientation) + " }";

		return s;
	}
};

#endif