// Copyright Daniel J. Steffey -- 2016

#include "GShaderColorTriangle.hpp"
#include "utils.hpp"
#include <cmath>
#include <iostream>


GShaderColorTriangle::GShaderColorTriangle(const GPoint pts[], const GColor colors[])
{
	// save the points
	this->m_p0 = pts[0];
	this->m_p1 = pts[1];
	this->m_p2 = pts[2];

	// save the colors
	this->m_c0 = colors[0];
	this->m_c1 = colors[1];
	this->m_c2 = colors[2];

	// calculate the local matrix
	// p0 is the pivot point!!!!
	// p0 -> p1 is u
	// p0 -> p2 is v
	this->m_local_ctm.set6(pts[1].fX - pts[0].fX, pts[2].fX - pts[0].fX, pts[0].fX, pts[1].fY - pts[0].fY, pts[2].fY - pts[0].fY, pts[0].fY);


	// set our global to the identity
	this->m_global_ctm.setIdentity();

	// combine them
	this->m_combined_ctm.setConcat(this->m_global_ctm, this->m_local_ctm);

	// now compute the inverse
	this->m_combined_ctm.invert(&(this->m_combined_ctm));

	// set our alpha
	this->m_alpha = 1.0f;
}

GShaderColorTriangle::~GShaderColorTriangle()
{
	// nothing to destroy
}

bool GShaderColorTriangle::setContext(const GMatrix& ctm, float alpha)
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

void GShaderColorTriangle::shadeRow(int x, int y, int count, GPixel row[])
{
	GPoint src_point = this->m_combined_ctm.mapXY(x + 0.5f, y + 0.5f);
	for (int i = 0; i < count; ++i)
	{
		// compute the color at x, y
		float a = ((src_point.fX * this->m_c1.fA) + (src_point.fY * this->m_c2.fA) + ((1 - src_point.fX - src_point.fY) * this->m_c0.fA)) * this->m_alpha;
		float r = ((src_point.fX * this->m_c1.fR) + (src_point.fY * this->m_c2.fR) + ((1 - src_point.fX - src_point.fY) * this->m_c0.fR));
		float g = ((src_point.fX * this->m_c1.fG) + (src_point.fY * this->m_c2.fG) + ((1 - src_point.fX - src_point.fY) * this->m_c0.fG));
		float b = ((src_point.fX * this->m_c1.fB) + (src_point.fY * this->m_c2.fB) + ((1 - src_point.fX - src_point.fY) * this->m_c0.fB));

		row[i] = convert_color_to_pixel(GColor::MakeARGB(a, r, g, b));

		src_point.fX += this->m_combined_ctm[0];
		src_point.fY += this->m_combined_ctm[3];
	}
}