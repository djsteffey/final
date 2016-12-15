// Copyright Daniel J. Steffey -- 2016

#include "GShaderLinearGradientSteffey.hpp"
#include "utils.hpp"
#include <cmath>


GShader* GShader::LinearGradient(const GPoint& p0, const GPoint& p1, const GColor& c0, const GColor& c1, GShader::TileMode tilemode)
{
	// sanity checks
	// cant think of any
	return new GShaderLinearGradientSteffey(p0, p1, c0, c1, tilemode);
}



GShaderLinearGradientSteffey::GShaderLinearGradientSteffey(const GPoint& p0, const GPoint& p1, const GColor& c0, const GColor& c1, GShader::TileMode tilemode)
{
	// calculate the local matrix
	float dx = p1.fX - p0.fX;
	float dy = p1.fY - p0.fY;
	this->m_local_ctm.set6(dx, -dy, p0.fX, dy, dx, p0.fY);

	// set our global to the identity
	this->m_global_ctm.setIdentity();

	// combine them
	this->m_combined_ctm.setConcat(this->m_local_ctm, this->m_global_ctm);

	// now compute the inverse
	this->m_combined_ctm.invert(&(this->m_combined_ctm));

	// set our alpha
	this->m_alpha = 1.0f;

	// save the colors
	this->m_c0 = c0;
	this->m_c1 = c1;

	// save our tilemode
	this->m_tilemode = tilemode;
}

GShaderLinearGradientSteffey::~GShaderLinearGradientSteffey()
{
	// nothing to destroy
}

bool GShaderLinearGradientSteffey::setContext(const GMatrix& ctm, float alpha)
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

	// generate our lookup table
	// the change in the color components each loop
	float delta_ca = (1.0f/255.0f) * ((this->m_c1.fA * this->m_alpha) - (this->m_c0.fA * this->m_alpha));
	float delta_cr = (1.0f/255.0f) * ((this->m_c1.fR) - (this->m_c0.fR));
	float delta_cg = (1.0f/255.0f) * ((this->m_c1.fG) - (this->m_c0.fG));
	float delta_cb = (1.0f/255.0f) * ((this->m_c1.fB) - (this->m_c0.fB));

	// each color component starts off at c0 values
	float a = (this->m_c0.fA * this->m_alpha);
	float r = (this->m_c0.fR);
	float g = (this->m_c0.fG);
	float b = (this->m_c0.fB);

	// fill up the table
	for (int i = 0; i < 256; ++i)
	{
		this->m_lookup_table[i] = convert_color_to_pixel(GColor::MakeARGB(a, r, g, b));
		a += delta_ca;
		r += delta_cr;
		g += delta_cg;
		b += delta_cb;
	}

	// success
	return true;
}

void GShaderLinearGradientSteffey::shadeRow(int x, int y, int count, GPixel row[])
{
	switch (this->m_tilemode)
	{
		case GShader::kClamp: { this->shadeRow_clamp(x, y, count, row); } break;
		case GShader::kRepeat: { this->shadeRow_repeat(x, y, count, row); } break;
		case GShader::kMirror: { this->shadeRow_mirror(x, y, count, row); } break;
	}
}

void GShaderLinearGradientSteffey::shadeRow_clamp(int x, int y, int count, GPixel row[])
{
	// calculate the first source point
	GPoint src_point = this->m_combined_ctm.mapXY(x + 0.5f, y + 0.5f);

	// loop through the count number of pixels
	for (int i = 0; i < count; ++i)
	{
		// determine the xvalue based on the tilemode
		float x_value = std::max(0.0f, std::min(src_point.fX, 1.0f));

		// look up the pixel and store it in the row
		row[i] = this->m_lookup_table[(int)(x_value * 255 + 0.5f)];

		// increment the source point based on values in the matrix
		src_point.fX += this->m_combined_ctm[GMatrix::SX];
	}
}

void GShaderLinearGradientSteffey::shadeRow_repeat(int x, int y, int count, GPixel row[])
{
	// calculate the first source point
	GPoint src_point = this->m_combined_ctm.mapXY(x + 0.5f, y + 0.5f);

	// loop through the count number of pixels
	for (int i = 0; i < count; ++i)
	{
		// determine the xvalue based on the tilemode
		float x_value = src_point.fX - std::floor(src_point.fX);

		// look up the pixel and store it in the row
		row[i] = this->m_lookup_table[(int)(x_value * 255 + 0.5f)];

		// increment the source point based on values in the matrix
		src_point.fX += this->m_combined_ctm[GMatrix::SX];
	}
}

void GShaderLinearGradientSteffey::shadeRow_mirror(int x, int y, int count, GPixel row[])
{
	// calculate the first source point
	GPoint src_point = this->m_combined_ctm.mapXY(x + 0.5f, y + 0.5f);

	// loop through the count number of pixels
	for (int i = 0; i < count; ++i)
	{
		float x_value = src_point.fX * 0.5f;
		x_value -= std::floor(x_value);
		x_value *= 2;
		x_value = 1.0f - std::abs(1.0f - x_value);

		// look up the pixel and store it in the row
		row[i] = this->m_lookup_table[(int)(x_value * 255 + 0.5f)];

		// increment the source point based on values in the matrix
		src_point.fX += this->m_combined_ctm[GMatrix::SX];
	}
}