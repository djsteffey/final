// Copyright Daniel J. Steffey -- 2016

#include "GShaderRadial.hpp"
#include "utils.hpp"
#include <cmath>
#include <iostream>


GShaderRadial::GShaderRadial(float cx, float cy, float radius, const GColor colors[], int count)
{

	// save the colors
	this->m_count = count;
	this->m_colors = new GColor[this->m_count];

	for (int i = 0; i < this->m_count; ++i)
	{
		this->m_colors[i] = colors[i];
	}


	// calculate the local matrix
	this->m_local_ctm.set6(radius, 0, cx, 0, radius, cy);


	// set our global to the identity
	this->m_global_ctm.setIdentity();

	// combine them
	this->m_combined_ctm.setConcat(this->m_global_ctm, this->m_local_ctm);

	// now compute the inverse
	this->m_combined_ctm.invert(&(this->m_combined_ctm));

	// set our alpha
	this->m_alpha = 1.0f;
}

GShaderRadial::~GShaderRadial()
{
	delete [] this->m_colors;
}

bool GShaderRadial::setContext(const GMatrix& ctm, float alpha)
{
	// save the passed in global ctm
	this->m_global_ctm = ctm;

	// save the alpha
	this->m_alpha = alpha;

	// recompute our combined and invert it
	this->m_combined_ctm.setConcat(this->m_global_ctm, this->m_local_ctm);
	if (this->m_combined_ctm.invert(&(this->m_combined_ctm)) == false)
	{
		return false;
	}

	// success
	return true;
}

void GShaderRadial::shadeRow(int x, int y, int count, GPixel row[])
{

	GPoint src_point = this->m_combined_ctm.mapXY(x + 0.5f, y + 0.5f);

	for (int i = 0; i < count; ++i)
	{
		// compute distance at x, y
		float distance = std::sqrt((src_point.fX * src_point.fX) + (src_point.fY * src_point.fY));

		// now distance is in value from 0.0f -> 1.0f (or beyond just clamp)
		if (distance >= 1.0f)
		{
			// autoamatically last color
			row[i] = convert_color_to_pixel(this->m_colors[this->m_count-1]);
		}
		else
		{
			float temp = distance * (this->m_count - 1);
			int c0 = std::floor(temp);
			int c1 = std::ceil(temp);
			temp -= c0;

			// now interpolate between c0 and c1 linearly by temp percentage
			float a = ((1.0f - temp) * this->m_colors[c0].fA) + (temp * this->m_colors[c1].fA) * this->m_alpha;
			float r = ((1.0f - temp) * this->m_colors[c0].fR) + (temp * this->m_colors[c1].fR);
			float g = ((1.0f - temp) * this->m_colors[c0].fG) + (temp * this->m_colors[c1].fG);
			float b = ((1.0f - temp) * this->m_colors[c0].fB) + (temp * this->m_colors[c1].fB);
			row[i] = convert_color_to_pixel(GColor::MakeARGB(a, r, g, b));
		}

		src_point.fX += this->m_combined_ctm[0];
		src_point.fY += this->m_combined_ctm[3];
	}
}