// Copyright Daniel J. Steffey -- 2016

#include "GShaderBitmapProxy.hpp"
#include "include/GPoint.h"
#include <iostream>

GShaderBitmapProxy::GShaderBitmapProxy(GShader* bitmap_shader, const GPoint pts[], const GPoint tex[])
{

	// calculate the pts matrix
	// p0 is the pivot point!!!!
	// p0 -> p1 is u
	// p0 -> p2 is v
	this->m_pts_matrix.set6(pts[1].fX - pts[0].fX, pts[2].fX - pts[0].fX, pts[0].fX, pts[1].fY - pts[0].fY, pts[2].fY - pts[0].fY, pts[0].fY);

	// calculate tex matrix
	this->m_tex_matrix.set6(tex[1].fX - tex[0].fX, tex[2].fX - tex[0].fX, tex[0].fX, tex[1].fY - tex[0].fY, tex[2].fY - tex[0].fY, tex[0].fY);
	// invert it
	this->m_tex_matrix.invert(&(this->m_tex_matrix));

	// save the shader
	this->m_bitmap_shader = bitmap_shader;
}

GShaderBitmapProxy::~GShaderBitmapProxy()
{
	// nothing to destroy
}

bool GShaderBitmapProxy::setContext(const GMatrix& ctm, float alpha)
{
	GMatrix combined;

	combined.setConcat(ctm, this->m_pts_matrix);
	combined.setConcat(combined, this->m_tex_matrix);

	this->m_bitmap_shader->setContext(combined, alpha);

	// success
	return true;
}

void GShaderBitmapProxy::shadeRow(int x, int y, int count, GPixel row[])
{
	this->m_bitmap_shader->shadeRow(x, y, count, row);
}